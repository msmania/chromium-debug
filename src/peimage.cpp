#include <functional>
#include <iomanip>
#include <sstream>
#define KDEXT_64BIT
#include <windows.h>
#include <wdbgexts.h>

#include "common.h"
#include "peimage.h"

// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format
// https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_data_directory
enum ImageDataDirectoryName {
  ExportTable,
  ImportTable,
  ResourceTable,
  ExceptionTable,
  CertificateTable,
  BaseRelocationTable,
  DebuggingInformation,
  ArchitectureSpecificData,
  GlobalPointerRegister,
  ThreadLocalStorageTable,
  LoadConfiguration,
  BoundImportTable,
  ImportAddressTable,
  DelayImportDescriptor,
  CLRHeader,
  Reserved,
};

PEImage::PEImage(address_t base) {
  Load(base);
}

PEImage::operator bool() const {
  return !!base_;
}

bool PEImage::IsInitialized() const {
  return !!base_;
}

bool PEImage::Is64bit() const {
  return is64bit_;
}

bool PEImage::Load(address_t base) {
  constexpr uint16_t MZ = 0x5a4d;
  constexpr uint32_t PE = 0x4550;
  constexpr uint16_t PE32 = 0x10b;
  constexpr uint16_t PE32PLUS = 0x20b;

  const auto dos = load_data<IMAGE_DOS_HEADER>(base);
  if (dos.e_magic != MZ) {
    dprintf("Invalid DOS header\n");
    return false;
  }

  if (load_data<uint32_t>(base + dos.e_lfanew) != PE) {
    dprintf("Invalid PE header\n");
    return false;
  }

  const auto fileHeader = load_data<IMAGE_FILE_HEADER>(
      base + dos.e_lfanew + sizeof(PE));
  const auto rvaOptHeader = base
    + dos.e_lfanew
    + sizeof(PE)
    + sizeof(IMAGE_FILE_HEADER);
  address_t sectionHeader = rvaOptHeader;
  switch (fileHeader.Machine) {
    default:
      dprintf("Unsupported platform - %04x.\n", fileHeader.Machine);
      return false;
    case IMAGE_FILE_MACHINE_AMD64: {
      const auto optHeader = load_data<IMAGE_OPTIONAL_HEADER64>(rvaOptHeader);
      sectionHeader += sizeof(IMAGE_OPTIONAL_HEADER64);
      if (optHeader.Magic != PE32PLUS) {
        dprintf("Invalid optional header\n");
        return false;
      }
      is64bit_ = true;
      for (int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i) {
        directories_[i] = optHeader.DataDirectory[i];
      }
      break;
    }
    case IMAGE_FILE_MACHINE_I386: {
      const auto optHeader = load_data<IMAGE_OPTIONAL_HEADER32>(rvaOptHeader);
      sectionHeader += sizeof(IMAGE_OPTIONAL_HEADER32);
      if (optHeader.Magic != PE32) {
        dprintf("Invalid optional header\n");
        return false;
      }
      is64bit_ = false;
      for (int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i) {
        directories_[i] = optHeader.DataDirectory[i];
      }
      break;
    }
  }

  for (;;) {
    const auto sig = load_data<uint64_t>(sectionHeader);
    if (!sig) break;
    const auto section = load_data<IMAGE_SECTION_HEADER>(sectionHeader);
    sections_.push_back(section);
    sectionHeader += sizeof(IMAGE_SECTION_HEADER);
  }

  base_ = base;
  return true;
}

class ResourceId {
  enum class Type {Number, String, Any};
  Type type_;
  uint16_t number_;
  std::wstring string_;

  ResourceId() : type_(Type::Any) {}

public:
  static const ResourceId &Any() {
    static ResourceId any;
    return any;
  }
  ResourceId(uint16_t id)
    : type_(Type::Number), number_(id)
  {}
  ResourceId(const wchar_t *name) {
    if (IS_INTRESOURCE(name)) {
      type_ = Type::Number;
      number_ = static_cast<uint16_t>(
        reinterpret_cast<uintptr_t>(name) & 0xffff);
    }
    else {
      type_ = Type::String;
      string_ = name;
    }
  }

  ResourceId(ResourceId &&other) = default;
  ResourceId &ResourceId::operator=(ResourceId &&other) = default;

  bool operator==(const ResourceId &other) const {
    if (type_ == Type::Any || other.type_ == Type::Any)
      return true;

    if (type_ != other.type_)
      return false;

    switch (type_) {
    case Type::Number:
      return number_ == other.number_;
    case Type::String:
      return string_ == other.string_;
    default:
      return false;
    }
  }

  bool operator!=(const ResourceId &other) const {
    return !(*this == other);
  }
};

using ResourceIterator = std::function<void (const IMAGE_RESOURCE_DATA_ENTRY &)>;

class ResourceDirectory {
  class DirectoryEntry {
    address_t base_;
    IMAGE_RESOURCE_DIRECTORY_ENTRY data_;

    static std::wstring ResDirString(address_t addr) {
      const uint16_t len = load_data<uint16_t>(addr);
      std::wstring ret;
      if (auto buf = new wchar_t[len + 1]) {
        ULONG cb = 0;
        ReadMemory(addr + 2, buf, len * sizeof(wchar_t), &cb);
        buf[len] = 0;
        ret = buf;
        delete [] buf;
      }
      return ret;
    }

  public:
    DirectoryEntry(address_t base)
      : base_(base)
    {}

    void Iterate(address_t addr,
                 const ResourceId &filter,
                 ResourceIterator func) {
      const auto entry = load_data<IMAGE_RESOURCE_DIRECTORY_ENTRY>(addr);
      if (!entry.Name) return;

      const ResourceId id = entry.NameIsString
        ? ResourceId(ResDirString(base_ + entry.NameOffset).c_str())
        : ResourceId(entry.Id);

      if (id != filter) return;

      if (entry.DataIsDirectory) {
        // dprintf("Dir -> %p\n", base_ + entry.OffsetToDirectory);
        ResourceDirectory dir(base_);
        dir.Iterate(base_ + entry.OffsetToDirectory, ResourceId::Any(), func);
      }
      else {
        // dprintf("Data %p\n", base_ + entry.OffsetToData);
        const auto data_entry =
          load_data<IMAGE_RESOURCE_DATA_ENTRY>(base_ + entry.OffsetToData);
        func(data_entry);
      }
    }
  };

  address_t base_;

public:
  ResourceDirectory(address_t base)
    : base_(base)
  {}

  void Iterate(address_t addr,
               const ResourceId &filter,
               ResourceIterator func) {
    const auto fastLookup = load_data<IMAGE_RESOURCE_DIRECTORY>(addr);
    const int num_entries =
      fastLookup.NumberOfNamedEntries + fastLookup.NumberOfIdEntries;
    const address_t first_entry = addr + sizeof(IMAGE_RESOURCE_DIRECTORY);
    for (int i = 0; i < num_entries; ++i) {
      DirectoryEntry entry(base_);
      entry.Iterate(first_entry + i * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY),
                    filter,
                    func);
    }
  }
};

VS_FIXEDFILEINFO PEImage::GetVersion() const {
  VS_FIXEDFILEINFO version{};

  if (!IsInitialized())
    return version;

  const address_t
    dir_start = base_ + directories_[ResourceTable].VirtualAddress,
    dir_end = dir_start + directories_[ResourceTable].Size;

  ResourceDirectory dir(dir_start);
  dir.Iterate(dir_start,
              ResourceId(RT_VERSION),
              [this, &version](const IMAGE_RESOURCE_DATA_ENTRY &data) {
                struct VS_VERSIONINFO {
                    WORD wLength;
                    WORD wValueLength;
                    WORD wType;
                    WCHAR szKey[16]; // L"VS_VERSION_INFO"
                    VS_FIXEDFILEINFO Value;
                };

                if (data.Size < sizeof(VS_VERSIONINFO)) return;

                const auto ver =
                  load_data<VS_VERSIONINFO>(base_ + data.OffsetToData);
                if (ver.wValueLength != sizeof(VS_FIXEDFILEINFO)
                    || ver.Value.dwSignature != 0xFEEF04BD)
                  return;

                version = ver.Value;
              });

  return version;
}
