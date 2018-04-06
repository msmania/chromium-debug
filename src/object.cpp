#include <windows.h>
#include <string>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

TARGETINFO::TARGETINFO()
  : is64bit_(false), version_(0), buildnum_(0)
{}

const char *TARGETINFO::engine() const {
  return engine_.c_str();
}

TARGETINFO Object::targetinfo_;

COREADDR Object::get_expression(const char *symbol) {
  std::string t(target().engine());
  t += symbol;
  return GetExpression(t.c_str());
}

void Object::InitializeTargetInfo() {
  if (targetinfo_.engine_.size() > 0) return;

  std::string engine_to_test = "chrome_child";

  ULONG64 chrome_core = GetExpression(engine_to_test.c_str());
  if (!chrome_core) {
    dprintf("Core module is not loaded.\n");
    return;
  }

  CPEImage pe(chrome_core);
  if (pe.IsInitialized() && pe.LoadVersion()) {
    DWORD FileMS = 0;
    DWORD FileLS = 0;
    DWORD ProdMS = 0;
    DWORD ProdLS = 0;
    pe.GetVersion(&FileMS, &FileLS, &ProdMS, &ProdLS);

    targetinfo_.is64bit_ = pe.Is64bit();
    targetinfo_.version_ = HIWORD(ProdMS);
    targetinfo_.buildnum_ = HIWORD(FileLS);

    dprintf("Target Engine = %d.%d\n\n",
            targetinfo_.version_,
            targetinfo_.buildnum_);

    std::swap(targetinfo_.engine_, engine_to_test);
  }
}

const TARGETINFO &Object::target() {
  return targetinfo_;
}

bool Object::ReadPointerEx(ULONG64 address, ULONG64 &pointer) {
  pointer = 0;
  ULONG cb = 0;
  if (target().is64bit_) {
    if (ReadMemory(address, &pointer, 8, &cb)) {
      return true;
    }
  }
  else {
    ULONG pointer32 = 0;
    if (ReadMemory(address, &pointer32, 4, &cb)) {
      pointer = pointer32;
      return true;
    }
  }
  return false;
}

COREADDR Object::deref(ULONG64 address) {
  COREADDR value;
  ReadPointerEx(address, value);
  return value;
}

Object::Object()
  : addr_(0) {
  InitializeTargetInfo();
}
