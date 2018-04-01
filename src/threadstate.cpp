#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <memory>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

class thread_state : public Object {
private:
  static std::map<std::string, ULONG> offsets_;
  static COREADDR main_thread_state_storage_;

  COREADDR heap_;

  static void LoadSymbols(LPCSTR module) {
    std::string type = module;
    type += "!blink::ThreadState";

    ULONG offset = 0;
    const char *field_name = nullptr;

    LOAD_FIELD_OFFSET("heap_");
  }

public:
  thread_state()
    : heap_(0)
  {}

  bool load() {
    if (offsets_.size() == 0) {
      LoadSymbols(target().engine());

      std::string ts(target().engine());
      ts += "!blink::ThreadState::main_thread_state_storage_";
      main_thread_state_storage_ = GetExpression(ts.c_str());
    }

    addr_ = main_thread_state_storage_;
    if (addr_ == 0) return false;

    char buf1[20];
    COREADDR src = 0;

    src = addr_ + offsets_["heap_"];
    LOAD_MEMBER_POINTER(heap_);

    return true;
  }

  void dump() const {
    char buf1[20];
    char buf2[20];
    std::stringstream ss;
    ss << "MainThreadState() = "
       << target().engine() << "!blink::ThreadState::main_thread_state_storage_\n"
       << target().engine() << "!blink::ThreadState "
       << ptos(addr_, buf1, sizeof(buf1)) << std::endl
       << target().engine() << "!blink::ThreadHeap "
       << ptos(heap_, buf2, sizeof(buf2)) << std::endl
       << std::endl;
    dprintf(ss.str().c_str());
  }
};

std::map<std::string, ULONG> thread_state::offsets_;
COREADDR thread_state::main_thread_state_storage_ = 0;

DECLARE_API(ts) {
  thread_state ts;
  if (ts.load()) {
    ts.dump();
  }
}
