#include <windows.h>
#include <string>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

CPEImage::CPEImage(ULONG64 ImageBase)
  : imagebase_(0),
    platform_(platform_),
    resource_dir_{0},
    import_dir_{0},
    version_{0} {
  Initialize(ImageBase);
}

CPEImage::~CPEImage() {
}

bool CPEImage::IsInitialized() const {
  return !!imagebase_;
}

bool CPEImage::Is64bit() const {
  return platform_==IMAGE_FILE_MACHINE_AMD64
         || platform_==IMAGE_FILE_MACHINE_IA64;
}

WORD CPEImage::GetPlatform() const {
  return platform_;
}

void CPEImage::GetVersion(PDWORD FileVersionMS,
                          PDWORD FileVersionLS,
                          PDWORD ProductVersionMS,
                          PDWORD ProductVersionLS) const {
  if (FileVersionMS)
    *FileVersionMS = version_.Value.dwFileVersionMS;
  if (FileVersionLS)
    *FileVersionLS = version_.Value.dwFileVersionLS;
  if (ProductVersionMS)
    *ProductVersionMS = version_.Value.dwProductVersionMS;
  if (ProductVersionLS)
    *ProductVersionLS = version_.Value.dwProductVersionLS;
}

bool CPEImage::LoadVersion() {
  bool Ret = false;
  ULONG Status = 0;
  ULONG64 ll = 0;
  ULONG BytesRead = 0;

  CHAR buf1[20];

  ULONG i;
  IMAGE_RESOURCE_DIRECTORY ResDirectory;
  IMAGE_RESOURCE_DIRECTORY_ENTRY ResDirEntry;
  IMAGE_RESOURCE_DATA_ENTRY DataEntry;
  VS_VERSIONINFO VersionInfo;

  if ( !IsInitialized() ) return false;

  ll = imagebase_ + resource_dir_.VirtualAddress;
  Status = ReadMemory(ll, &ResDirectory, sizeof(ResDirectory), &BytesRead);
  if (!Status || BytesRead!=sizeof(ResDirectory)) {
    dprintf("Failed to read IMAGE_RESOURCE_DIRECTORY at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }

  // Searching for VS_FILE_INFO directory entry
  ZeroMemory(&ResDirEntry, sizeof(ResDirEntry));
  ll += (sizeof(ResDirectory)
         + ResDirectory.NumberOfNamedEntries * sizeof(ResDirEntry));
  for ( i=0 ; i<ResDirectory.NumberOfIdEntries ; ++i ) {
    Status = ReadMemory(ll, &ResDirEntry, sizeof(ResDirEntry), &BytesRead);
    if (!Status || BytesRead!=sizeof(ResDirEntry)) {
      dprintf("Failed to read IMAGE_RESOURCE_DIRECTORY_ENTRY at 0x%s\n",
              ptos(ll, buf1, sizeof(buf1)));
      goto exit;
    }

    if (!ResDirEntry.NameIsString
        && MAKEINTRESOURCE(ResDirEntry.Id)==VS_FILE_INFO) {
      break;
    }

    ll += sizeof(ResDirEntry);
  }

  if (i >= (ULONG)ResDirectory.NumberOfIdEntries
      || !ResDirEntry.DataIsDirectory) {
    dprintf("VS_FILE_INFO resource not found. Failed to determine version.\n");
    goto exit;
  }

  // Getting directory from entry
  ll = imagebase_
       + resource_dir_.VirtualAddress
       + ResDirEntry.OffsetToDirectory;
  Status = ReadMemory(ll, &ResDirectory, sizeof(ResDirectory), &BytesRead);
  if (!Status || BytesRead!=sizeof(ResDirectory)) {
      dprintf("Failed to read IMAGE_RESOURCE_DIRECTORY at 0x%s\n",
              ptos(ll, buf1, sizeof(buf1)));
      goto exit;
  }

  // Searching for VS_VERSION_INFO directory entry
  ll += (sizeof(ResDirectory)
         + ResDirectory.NumberOfNamedEntries * sizeof(ResDirEntry));
  for (i = 0; i < ResDirectory.NumberOfIdEntries; ++i) {
    Status = ReadMemory(ll, &ResDirEntry, sizeof(ResDirEntry), &BytesRead);
    if (!Status || BytesRead!=sizeof(ResDirEntry)) {
      dprintf("Failed to read IMAGE_RESOURCE_DIRECTORY_ENTRY at 0x%s\n",
              ptos(ll, buf1, sizeof(buf1)));
      goto exit;
    }

    if ( !ResDirEntry.NameIsString && ResDirEntry.Id==VS_VERSION_INFO ) {
      break;
    }

    ll += sizeof(ResDirEntry);
  }

  if (i >= (ULONG)ResDirectory.NumberOfIdEntries
      || !ResDirEntry.DataIsDirectory) {
      dprintf("VS_VERSION_INFO resource not found. "
              "Failed to determine version.\n");
      goto exit;
  }

  // Getting directory from entry
  ll = imagebase_
       + resource_dir_.VirtualAddress
       + ResDirEntry.OffsetToDirectory;
  Status = ReadMemory(ll, &ResDirectory, sizeof(ResDirectory), &BytesRead);
  if ( !Status || BytesRead!=sizeof(ResDirectory) ) {
    dprintf("Failed to read IMAGE_RESOURCE_DIRECTORY at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }

  // Make sure directory has an entry
  if ( ResDirectory.NumberOfIdEntries<1 ) {
      dprintf("VS_VERSION_INFO has not entry.\n");
      goto exit;
  }

  // Getting the first entry from directory
  ll += sizeof(ResDirectory);
  Status = ReadMemory(ll, &ResDirEntry, sizeof(ResDirEntry), &BytesRead);
  if (!Status || BytesRead!=sizeof(ResDirEntry)) {
    dprintf("Failed to read IMAGE_RESOURCE_DIRECTORY_ENTRY at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }

  // Make sure VS_VERSION_INFO is not a directory
  if ( ResDirEntry.DataIsDirectory ) {
    dprintf("VS_VERSION_INFO is not a data entry.\n");
    goto exit;
  }

  // Getting data entry from entry
  ll = imagebase_
       + resource_dir_.VirtualAddress
       + ResDirEntry.OffsetToDirectory;
  Status = ReadMemory(ll, &DataEntry, sizeof(DataEntry), &BytesRead);
  if ( !Status || BytesRead!=sizeof(DataEntry) ) {
    dprintf("Failed to read IMAGE_RESOURCE_DATA_ENTRY at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }

  if ( DataEntry.Size<sizeof(VS_VERSIONINFO) ) {
    dprintf("VS_VERSION_INFO buffer is short.\n");
    goto exit;
  }

  ll = imagebase_ + DataEntry.OffsetToData;
  Status = ReadMemory(ll, &VersionInfo, sizeof(VersionInfo), &BytesRead);
  if ( !Status || BytesRead!=sizeof(VersionInfo) ) {
    dprintf("Failed to read VS_VERSION_INFO at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }

  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms646997(v=vs.85).aspx
  if (VersionInfo.Value.dwSignature != 0xFEEF04BD) {
    dprintf("VS_VERSION_INFO signature does not match.\n");
    goto exit;
  }

  CopyMemory(&version_, &VersionInfo, sizeof(version_));
  Ret = true;

exit:
  return Ret;
}

// https://msdn.microsoft.com/en-us/magazine/cc301808.aspx
bool CPEImage::Initialize(ULONG64 ImageBase) {
  CONST DWORD OFFSET_PEHEADER = 0x3c; // _IMAGE_DOS_HEADER::e_lfanew
  CONST DWORD PE_SIGNATURE = 0x4550;

  bool Ret = false;
  ULONG Status = 0;
  ULONG64 ll = 0;
  ULONG BytesRead = 0;

  DWORD Rva_PEHeader = 0;
  DWORD PESignature = 0;

  CHAR buf1[20];

  WORD Platform = 0;
  IMAGE_DATA_DIRECTORY DataDirectory;

  ll = ImageBase + OFFSET_PEHEADER;
  Status = ReadMemory(ll, &Rva_PEHeader, sizeof(Rva_PEHeader), &BytesRead);
  if (!Status || BytesRead!=sizeof(Rva_PEHeader)) {
    dprintf("Failed to access DOS header at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }

  ll = ImageBase + Rva_PEHeader;
  Status = ReadMemory(ll, &PESignature, sizeof(PESignature), &BytesRead);
  if (!Status
      || BytesRead != sizeof(PESignature)
      || PESignature != PE_SIGNATURE) {
    dprintf("PE header not found at 0x%s\n", ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }

  // ntdll!_IMAGE_NT_HEADERS::FileHeader::Machine
  ll = ImageBase + Rva_PEHeader + 4;
  Status = ReadMemory(ll, &Platform, sizeof(Platform), &BytesRead);
  if (!Status || BytesRead!=sizeof(Platform)) {
    dprintf("Failed to access PE header at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }

  if (Platform!=IMAGE_FILE_MACHINE_AMD64
      && Platform!=IMAGE_FILE_MACHINE_IA64
      && Platform!=IMAGE_FILE_MACHINE_I386) {
    dprintf("Unsupported platform - 0x%04x. Initialization failed.\n",
            Platform);
    goto exit;
  }

  imagebase_ = ImageBase;
  platform_ = Platform;

  // Getting RVA to each directory
  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms680305(v=vs.85).aspx
  ll = ImageBase
       + Rva_PEHeader
       + 0x18
       + (Is64bit() ? 0x70 : 0x60); // pointing to the first entry

  // Import table: 2nd entry
  ll += sizeof(DataDirectory);
  Status = ReadMemory(ll, &DataDirectory, sizeof(DataDirectory), &BytesRead);
  if (!Status || BytesRead!=sizeof(DataDirectory)) {
    dprintf("Failed to read IMAGE_DATA_DIRECTORY at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }
  CopyMemory(&import_dir_, &DataDirectory, sizeof(DataDirectory));

  // Resource table: 3rd entry
  ll += sizeof(DataDirectory);
  Status = ReadMemory(ll, &DataDirectory, sizeof(DataDirectory), &BytesRead);
  if (!Status || BytesRead!=sizeof(DataDirectory)) {
    dprintf("Failed to read IMAGE_DATA_DIRECTORY at 0x%s\n",
            ptos(ll, buf1, sizeof(buf1)));
    goto exit;
  }
  CopyMemory(&resource_dir_, &DataDirectory, sizeof(DataDirectory));

  Ret = true;

exit:
  return Ret;
}

ULONG CPEImage::ReadPointerEx(ULONG64 Address, PULONG64 Pointer) const {
  ULONG cb = 0;
  if (Is64bit()) {
    return (ReadMemory(Address, (PVOID)Pointer, sizeof(*Pointer), &cb)
            && cb == sizeof(*Pointer));
  }
  else {
    ULONG Pointer32;
    ULONG Status;
    Status = ReadMemory(Address,
                        (PVOID)&Pointer32,
                        sizeof(Pointer32),
                        &cb);
    if (Status && cb == sizeof(Pointer32)) {
        *Pointer = (ULONG64)(LONG64)(LONG)Pointer32;
        return 1;
    }
    return 0;
  }
}
