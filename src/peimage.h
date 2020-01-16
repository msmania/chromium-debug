#pragma once

class PEImage final {
  address_t base_{};
  bool is64bit_{};
  IMAGE_DATA_DIRECTORY directories_[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
  std::vector<IMAGE_SECTION_HEADER> sections_;

  bool Load(ULONG64 ImageBase);

public:
  PEImage(address_t base);

  operator bool() const;

  bool IsInitialized() const;
  bool Is64bit() const;

  VS_FIXEDFILEINFO GetVersion() const;
};