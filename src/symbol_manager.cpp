#include <unordered_map>
#include <windows.h>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

class symbol_manager {
  const std::string module_;
  std::unordered_map<std::string, uint32_t> offset_map_;
  std::unordered_map<std::string, FIELD_INFO> field_info_map_;

  std::string get_type_name(const char *type) const {
    if (module_.size() > 0)
      return module_ + '!' + type;
    else
      return type;
  }

public:
  symbol_manager() {}

  symbol_manager(const char *module_name)
    : module_(module_name)
  {}

  uint32_t get_field_offset(const char *type, const char *field) {
    const auto full_type_name = get_type_name(type);
    const auto full_symbol_name = get_type_name(type) + '.' + field;
    auto found = offset_map_.find(full_symbol_name);
    if (found != offset_map_.end()) {
      return found->second;
    }

    ULONG offset = 0xffffffff;
    uint32_t status = GetFieldOffset(full_type_name.c_str(), field, &offset);
    // Regardless of the result, we update the map to block subsequent attempts
    offset_map_[full_symbol_name] = offset;
    if (status != 0) {
      Log(L"GetFieldOffset(%hs) failed with %08x\n",
          full_symbol_name.c_str(),
          status);
    }
    return offset;
  }

  FIELD_INFO get_field_info(const char *type, const char *field) {
    const auto full_type_name = get_type_name(type);
    const auto full_symbol_name = full_type_name + '.' + field;
    auto found = field_info_map_.find(full_symbol_name);
    if (found != field_info_map_.end()) {
      return found->second;
    }

    FIELD_INFO flds = {
      (PUCHAR)field,
      (PUCHAR)"",
      0,
      DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_RETURN_ADDRESS,
      0,
      nullptr
    };
    SYM_DUMP_PARAM sym = {
      sizeof (SYM_DUMP_PARAM),
      (PUCHAR)full_type_name.c_str(),
      DBG_DUMP_NO_PRINT,
      0,
      nullptr,
      nullptr,
      nullptr,
      1,
      &flds
    };
    sym.nFields = 1;
    auto status = Ioctl(IG_DUMP_SYMBOL_INFO, &sym, sym.size);
    // Regardless of the result, we update the map to block subsequent attempts
    field_info_map_[full_symbol_name] = flds;
    if (status != 0) {
      Log(L"GetFieldInfo(%hs) failed with %08x\n",
          full_symbol_name.c_str(),
          status);
    }
    return flds;
  }

  void dump_all() const {
    Log(L"offset_map_\n");
    for (const auto &pair : offset_map_) {
      Log(L"%hs: %08x\n", pair.first.c_str(), pair.second);
    }
    Log(L"field_info_map_\n");
    for (const auto &pair : field_info_map_) {
      Log(L"%hs: +%08x %d\n",
          pair.first.c_str(),
          pair.second.FieldOffset,
          pair.second.size);
    }
  }
};

symbol_manager smanager;

uint32_t get_field_offset(const char *type, const char *field) {
  return smanager.get_field_offset(type, field);
}

FIELD_INFO get_field_info(const char *type, const char *field) {
  return smanager.get_field_info(type, field);
}

void dump_symbol_manager() {
  smanager.dump_all();
}