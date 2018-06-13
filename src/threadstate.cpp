#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <memory>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

namespace blink {

struct PersistentNode : public Object {
  static std::map<std::string, ULONG> offsets_;
  COREADDR self_;
  COREADDR trace_;

  PersistentNode() : self_(0), trace_(0) {}

  void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type(target().engine());
      type += "!blink::PersistentNode";

      ULONG offset = 0;
      const char *field_name = nullptr;

      LOAD_FIELD_OFFSET("self_");
      LOAD_FIELD_OFFSET("trace_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["self_"];
      LOAD_MEMBER_POINTER(self_);
      src = addr_ + offsets_["trace_"];
      LOAD_MEMBER_POINTER(trace_);
    }
    else {
      self_ = trace_ = 0;
    }
  }

  bool IsUnused() const { return !trace_; }
};

struct PersistentNodeSlots : public Object {
  static std::map<std::string, ULONG> offsets_;
  static const int kSlotCount = 256;
  static int node_size_;
  COREADDR next_;
  PersistentNode slot_[kSlotCount];

  PersistentNodeSlots() : next_(0) {}

  void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type(target().engine());
      type += "!blink::PersistentNode";
      node_size_ = GetTypeSize(type.c_str());

      type += "Slots";
      ULONG offset = 0;
      const char *field_name = nullptr;

      LOAD_FIELD_OFFSET("next_");
      LOAD_FIELD_OFFSET("slot_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["next_"];
      LOAD_MEMBER_POINTER(next_);
      src = addr_ + offsets_["slot_"];
      for (int i = 0; i < kSlotCount; ++i) {
        slot_[i].load(src);
        src += node_size_;
      }
    }
    else {
      next_ = 0;
      memset(slot_, 0, sizeof(slot_));
    }
  }
};

class PersistentRegion : public Object {
  static std::map<std::string, ULONG> offsets_;
  COREADDR free_list_head_;
  COREADDR slots_;

public:
  PersistentRegion()
    : free_list_head_(0), slots_(0)
  {}

  void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type(target().engine());
      type += "!blink::PersistentRegion";

      ULONG offset = 0;
      const char *field_name = nullptr;

      LOAD_FIELD_OFFSET("free_list_head_");
      LOAD_FIELD_OFFSET("slots_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["free_list_head_"];
      LOAD_MEMBER_POINTER(free_list_head_);
      src = addr_ + offsets_["slots_"];
      LOAD_MEMBER_POINTER(slots_);
    }
    else {
      free_list_head_ = slots_ = 0;
    }
  }

  void dump(std::ostream &s) const {
    const int pointer_size = target().is64bit_ ? 8 : 4;
    char buf1[20];
    char buf2[20];
    PersistentNodeSlots slots;
    int slot_index = 0;
    for (COREADDR p = slots_; p; ) {
      s << "slot#" << (slot_index++)
        << ' ' << ptos(p, buf1, sizeof(buf1)) << std::endl;
      slots.load(p);
      for (int i = 0; i < PersistentNodeSlots::kSlotCount; ++i) {
        const PersistentNode &node = slots.slot_[i];
        if (!node.IsUnused()) {
          s << std::setw(4) << (i)
            << ' ' << ptos(node.self_, buf1, sizeof(buf1))
            << ' ' << ptos(node.trace_, buf2, sizeof(buf2));
          COREADDR raw, node_addr;
          if (ReadPointerEx(node.self_, raw)
              && raw
              && ReadPointerEx(node.self_ + pointer_size, node_addr)) {
            s << ' ' << ptos(raw, buf1, sizeof(buf1))
              << ' ' << ptos(node_addr, buf2, sizeof(buf2));
          }
          s << std::endl;
        }
      }
      p = slots.next_;
    }
  }
};

class thread_state : public Object {
private:
  static std::map<std::string, ULONG> offsets_;
  static COREADDR main_thread_state_storage_;

  COREADDR heap_;
  COREADDR persistent_region_;

public:
  thread_state()
    : heap_(0), persistent_region_(0)
  {}

  void load(COREADDR addr) {
    if (offsets_.size() == 0 || main_thread_state_storage_ == 0) {
      std::string type(target().engine());
      type += "!blink::ThreadState";

      if (offsets_.size() == 0) {
        ULONG offset = 0;
        const char *field_name = nullptr;

        LOAD_FIELD_OFFSET("heap_");
        LOAD_FIELD_OFFSET("persistent_region_");
      }

      if (addr == 0 && main_thread_state_storage_ == 0) {
        type += "::main_thread_state_storage_";
        main_thread_state_storage_ = GetExpression(type.c_str());
      }
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr ? addr : main_thread_state_storage_;
    if (addr_) {
      src = addr_ + offsets_["heap_"];
      LOAD_MEMBER_POINTER(heap_);
      src = addr_ + offsets_["persistent_region_"];
      LOAD_MEMBER_POINTER(persistent_region_);
    }
    else {
      heap_ = persistent_region_ = 0;
    }
  }

  void dump(std::ostream &s) const {
    char buf1[20];
    char buf2[20];
    char buf3[20];
    s << "MainThreadState() = "
      << target().engine() << "!blink::ThreadState::main_thread_state_storage_\n"
      << target().engine() << "!blink::ThreadState "
      << ptos(addr_, buf1, sizeof(buf1)) << std::endl
      << target().engine() << "!blink::ThreadHeap "
      << ptos(heap_, buf2, sizeof(buf2)) << std::endl
      << target().engine() << "!blink::PersistentRegion "
      << ptos(persistent_region_, buf3, sizeof(buf3)) << std::endl;
  }
};

std::map<std::string, ULONG> PersistentNode::offsets_;
std::map<std::string, ULONG> PersistentNodeSlots::offsets_;
std::map<std::string, ULONG> PersistentRegion::offsets_;
std::map<std::string, ULONG> thread_state::offsets_;
int PersistentNodeSlots::node_size_ = 0;
COREADDR thread_state::main_thread_state_storage_ = 0;

}

DECLARE_API(ts) {
  dump_arg<blink::thread_state>(args);
}

DECLARE_API(persistent) {
  dump_arg<blink::PersistentRegion>(args);
}
