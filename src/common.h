#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

typedef uint64_t address_t;

class address_string {
  char buffer_[20];

public:
  address_string(address_t addr);
  operator const char *() const;
};

struct target_info {
  uint32_t actualProcessorType{};
  uint32_t effectiveProcessorType{};
  static const target_info &get();
  void init();
};

std::vector<std::string> get_args(const char *args);
uint32_t get_field_offset(const char *type, const char *field);
FIELD_INFO get_field_info(const char *type, const char *field);
uint32_t get_field_info_with_module(const char *type, const char *field);
address_t load_pointer(address_t addr);
void DumpAddressAndSymbol(std::ostream &s, address_t addr);
const char *ptos(uint64_t p, char *s, uint32_t len);
void Log(const wchar_t* format, ...);

class debug_object {
protected:
  address_t base_{};

public:
  virtual void load(address_t addr) = 0;
  virtual void dump(std::ostream &s) const;
  address_t addr() const { return base_; }
};

template <typename T>
void dump_object(address_t addr) {
  T t;
  t.load(addr);
  std::stringstream ss;
  t.dump(ss);
  dprintf("%s\n", ss.str().c_str());
}

template<typename T>
T load_data(address_t addr) {
  T data{};
  ULONG cb = 0;
  if (ReadMemory(addr, &data, sizeof(T), &cb) == 0
      || cb != sizeof(T)) {
    address_string s(addr);
    Log(L"Failed to load a pointer from %hs\n", s);
  }
  return data;
}
