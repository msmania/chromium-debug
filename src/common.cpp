// GetFieldOffset is supported only in 64bit pointer mode.
#define KDEXT_64BIT

#include <windows.h>
#include <atlbase.h>
#include <dbgeng.h>
#include <wdbgexts.h>
#include <map>

#include "common.h"

#define LODWORD(ll) ((uint32_t)((ll)&0xffffffff))
#define HIDWORD(ll) ((uint32_t)(((ll)>>32)&0xffffffff))

address_t load_pointer(address_t addr) {
  address_t loaded{};
  if (!ReadPointer(addr, &loaded)) {
    address_string s(addr);
    Log(L"Failed to load a pointer from %hs\n", s);
  }
  return loaded;
}

address_string::address_string(address_t addr) {
  if (HIDWORD(addr) == 0)
    sprintf(buffer_, "%08x", LODWORD(addr));
  else
    sprintf(buffer_, "%08x`%08x",
            HIDWORD(addr),
            LODWORD(addr));
}

address_string::operator const char *() const {
  return buffer_;
}

void DumpAddressAndSymbol(std::ostream &s, address_t addr) {
  char symbol[1024];
  ULONG64 displacement;
  GetSymbol(addr, symbol, &displacement);
  s << address_string(addr)
    << ' ' << symbol;
  if (displacement) s << "+0x" << std::hex << displacement;
}

void Log(const wchar_t* format, ...) {
  wchar_t linebuf[1024];
  va_list v;
  va_start(v, format);
  wvsprintf(linebuf, format, v);
  va_end(v);
  OutputDebugString(linebuf);
}

std::vector<std::string> get_args(const char *args) {
  std::vector<std::string> string_array;
  const char *prev, *p;
  prev = p = args;
  while (*p) {
    if (*p == ' ') {
      if (p > prev)
        string_array.emplace_back(args, prev - args, p - prev);
      prev = p + 1;
    }
    ++p;
  }
  if (p > prev)
    string_array.emplace_back(args, prev - args, p - prev);
  return string_array;
}

void debug_object::dump(std::ostream &s) const {
  address_string s1(base_);
  s << s1 << std::endl;
}

namespace {
  target_info target_info_instance;
}

void target_info::init() {
  CComPtr<IDebugClient7> client;
  if (SUCCEEDED(DebugCreate(IID_PPV_ARGS(&client)))) {
    if (CComQIPtr<IDebugControl3> control = client) {
      ULONG type;
      if (SUCCEEDED(control->GetActualProcessorType(&type))) {
        actualProcessorType = type;
      }
      if (SUCCEEDED(control->GetEffectiveProcessorType(&type))) {
        effectiveProcessorType = type;
      }
    }
  }
}

const target_info &target_info::get() {
  return target_info_instance;
}

void init_target_info() {
  target_info_instance.init();
}

void dump_symbol_manager();
void dump_vtable_manager();

DECLARE_API(runtests) {
  dump_symbol_manager();
  dump_vtable_manager();
}
