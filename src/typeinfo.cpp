#include <windows.h>
#include <string>
#include <map>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

class VTableMap {
private:
  static const std::string StripSymbolName(const std::string &symbol) {
    auto bang = symbol.find('!');
    auto vtmark = symbol.rfind("::`vftable'");
    return (bang != std::string::npos && vtmark != std::string::npos)
           ? symbol.substr(bang + 1, vtmark - bang - 1)
           : "";
  }

  std::map<COREADDR, std::string> vt_to_class_;

  void Set(COREADDR vtAddr) {
    static char symbol_name[1024];
    COREADDR displacement = 0;
    GetSymbol(vtAddr, symbol_name, &displacement);
    if (displacement == 0) {
      vt_to_class_[vtAddr] = StripSymbolName(symbol_name);
    }
  }

public:
  const std::string &Get(COREADDR addr) {
    static std::string empty;
    COREADDR vtAddr;
    if (Object::ReadPointerEx(addr, vtAddr)) {
      if (vt_to_class_[vtAddr].length() == 0) {
        Set(vtAddr);
      }
      return vt_to_class_[vtAddr];
    }
    return empty;
  }
};

static VTableMap vtable_to_typename;

const std::string &ResolveType(COREADDR addr) {
  return vtable_to_typename.Get(addr);
}
