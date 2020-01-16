#include <unordered_map>
#include <windows.h>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

class vtable_manager {
  static const std::string strip_symbol_name(const std::string &symbol) {
    auto bang = symbol.find('!');
    auto vtmark = symbol.rfind("::`vftable'");
    return (bang != std::string::npos && vtmark != std::string::npos)
           ? symbol.substr(bang + 1, vtmark - bang - 1)
           : "";
  }

  std::unordered_map<address_t, std::string> vtable_map_;

  void push(address_t vt) {
    static char symbol_name[1024];
    address_t displacement = 0;
    GetSymbol(vt, symbol_name, &displacement);
    if (displacement == 0) {
      vtable_map_[vt] = strip_symbol_name(symbol_name);
    }
  }

public:
  const std::string &resolve_type(address_t addr) {
    address_t vt = load_pointer(addr);
    if (vtable_map_[vt].length() == 0) {
      push(vt);
    }
    return vtable_map_[vt];
  }

  void dump_all() const {
    Log(L"vtable_map_\n");
    for (const auto &pair : vtable_map_) {
      address_string s(pair.first);
      Log(L"%s: %s\n", pair.second.c_str(), s);
    }
  }
};

namespace {
  vtable_manager vmanager;
}

const std::string &resolve_type(address_t addr) {
  return vmanager.resolve_type(addr);
}

void dump_vtable_manager() {
  vmanager.dump_all();
}