#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>
#include <memory>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

namespace blink {

constexpr size_t kBlinkPageSizeLog2 = 17;
constexpr size_t kBlinkPageSize = 1 << kBlinkPageSizeLog2;
constexpr size_t kHeaderWrapperMarkBitMask = 1u << kBlinkPageSizeLog2;
constexpr size_t kHeaderGCInfoIndexShift = kBlinkPageSizeLog2 + 1;
constexpr size_t kHeaderGCInfoIndexMask = (static_cast<size_t>((1 << 14) - 1))
                                          << kHeaderGCInfoIndexShift;
constexpr size_t kHeaderSizeMask = (static_cast<size_t>((1 << 14) - 1)) << 3;
constexpr size_t kHeaderMarkBitMask = 1;
constexpr size_t kHeaderFreedBitMask = 2;

class BasePage : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

protected:
  std::string type_;
  std::unique_ptr<BasePage> next_;

public:
  static BasePage *create(COREADDR addr);

  BasePage() {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::BasePage";

      ULONG offset = 0;
      const char *field_name = nullptr;

      LOAD_FIELD_OFFSET("next_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    type_ = ResolveType(addr_);
    next_ = 0;

    if (addr) {
      src = addr + offsets_["next_"];
      LOAD_MEMBER_POINTER(addr);
      next_.reset(create(addr));
    }
  }

  virtual size_t size() { return 0; }
  const std::string &type() const { return type_; }
  BasePage* Next() const { return next_.get(); }
};

class NormalPage : public BasePage {
public:
  size_t size() override { return kBlinkPageSize; }
};

BasePage *BasePage::create(COREADDR addr) {
  static std::map<std::string, BasePage*(*)()> ctors;
  if (ctors.size() == 0) {
    ADD_CTOR(BasePage, blink::NormalPage,
                       blink::NormalPage);
  }

  BasePage *p = nullptr;
  if (!addr) {
    p = new BasePage();
  }
  else {
    CHAR buf[20];
    const auto &type = ResolveType(addr);
    if (ctors.find(type) != ctors.end()) {
      p = ctors[type]();
    }
    else {
      dprintf("> Fallback to BasePage::ctor() for %s (= %s)\n",
              type.c_str(),
              ptos(addr, buf, sizeof(buf)));
      p = new BasePage();
    }
    p->load(addr);
  }
  return p;
}

class ThreadState : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  COREADDR heap_;

public:
  ThreadState() : heap_(0) {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::ThreadState";

      ULONG offset = 0;
      const char *field_name = nullptr;

      LOAD_FIELD_OFFSET("heap_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr) {
      src = addr_ + offsets_["heap_"];
      LOAD_MEMBER_POINTER(heap_);
    }
    else {
      heap_ = 0;
    }
  }

  COREADDR Heap() const { return heap_; }
};

class BaseArena : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

protected:
  std::unique_ptr<BasePage> first_page_;
  ThreadState thread_state_;

public:
  static BaseArena *create(COREADDR addr);

  BaseArena() {}

  virtual bool is_current_page(COREADDR lower, COREADDR upper) const {
    return false;
  }

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::BaseArena";

      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("first_page_");
      LOAD_FIELD_OFFSET("thread_state_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["first_page_"];
      LOAD_MEMBER_POINTER(addr);
      first_page_.reset(BasePage::create(addr));

      src = addr_ + offsets_["thread_state_"];
      LOAD_MEMBER_POINTER(addr);
      thread_state_.load(addr);
    }
    else {
      first_page_.reset(BasePage::create(0));
      thread_state_.load(0);
    }
  }

  virtual void dump(std::ostream &s) const {
    char buf1[20];
    char buf2[20];
    s << ResolveType(addr_)
      << ' ' << ptos(addr_, buf1, sizeof(buf1)) << std::endl
      << "blink::ThreadHeap " << ptos(thread_state_.Heap(), buf2, sizeof(buf2))
      << "\npage chain:\n";

    for (BasePage *page = first_page_.get();
         ;
         page = page->Next()) {
      auto addr = page->addr();
      if (!addr) break;
      auto limit = addr + page->size();
      s << page->type()
        << ' ' << ptos(addr, buf1, sizeof(buf1))
        << " - " << ptos(limit, buf2, sizeof(buf2))
        << (is_current_page(addr, limit) ? " #" : "")
        << std::endl;
    }
  }
};

class HeapObjectHeader : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  uint32_t encoded_;

public:
  HeapObjectHeader() : encoded_(0) {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::HeapObjectHeader";
      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("encoded_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["encoded_"];
      LOAD_MEMBER_VALUE(encoded_);
    }
    else {
      encoded_ = 0;
    }
  }

  virtual void dump(std::ostream &s) const {
    s << "Size:         " << static_cast<uint32_t>(size()) << std::endl
      << "GCinfo index: " << static_cast<uint32_t>(GcInfoIndex()) << std::endl
      << "Free:         " << (IsFree() ? 'Y' : 'N') << std::endl
      << "Mark:         " << (IsMarked() ? 'Y' : 'N') << std::endl
      << "DOM mark:     " << (IsWrapperHeaderMarked() ? 'Y' : 'N') << std::endl;
  }

  size_t GcInfoIndex() const {
    return (encoded_ & kHeaderGCInfoIndexMask) >> kHeaderGCInfoIndexShift;
  }

  bool IsWrapperHeaderMarked() const {
    return encoded_ & kHeaderWrapperMarkBitMask;
  }

  size_t size() const {
    return encoded_ & kHeaderSizeMask;
  }

  bool IsFree() const {
    return encoded_ & kHeaderFreedBitMask;
  }

  bool IsMarked() const {
    return encoded_ & kHeaderMarkBitMask;
  }
};

class FreeListEntry final : public HeapObjectHeader {
private:
  static std::map<std::string, ULONG> offsets_;

  COREADDR next_;

public:
  FreeListEntry() : next_(0) {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::FreeListEntry";
      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("next_");
    }

    char buf1[20];
    COREADDR src = 0;

    HeapObjectHeader::load(addr);
    if (addr_) {
      src = addr_ + offsets_["next_"];
      LOAD_MEMBER_POINTER(next_);
    }
    else {
      next_ = 0;
    }
  }

  COREADDR Next() const { return next_; }
};

class FreeList : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  COREADDR free_lists_[kBlinkPageSizeLog2];

public:
  FreeList() : free_lists_{} {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::FreeList";
      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("free_lists_");
    }

    const int pointer_size = target().is64bit_ ? 8 : 4;
    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["free_lists_"];
      for (int i = 0; i < kBlinkPageSizeLog2; ++i) {
        LOAD_MEMBER_POINTER(free_lists_[i]);
        src += pointer_size;
      }
    }
    else {
      memset(free_lists_, 0, sizeof(free_lists_));
    }
  }

  virtual void dump(std::ostream &s) const {
    char buf1[20];
    for (int i = 0; i < kBlinkPageSizeLog2; ++i) {
      if (free_lists_[i]) {
        s << std::setw(3) << i;
        int count = 0;
        FreeListEntry entry;
        for (auto p = free_lists_[i];
             p;
             p = entry.Next(), ++count) {
          if (count == 0)
            s << ' ';
          else if ((count & 3) == 0)
            s << "    ";
          else
            s << ' ';
          entry.load(p);
          s << ptos(p, buf1, sizeof(buf1)) << ' ' << entry.size();
          if ((count & 3) == 3 && entry.Next())
            s << std::endl;
        }
        s << std::endl;
      }
    }
  }
};

class NormalPageArena : public BaseArena {
private:
  static std::map<std::string, ULONG> offsets_;

  FreeList free_list_;
  COREADDR current_allocation_point_;

public:
  NormalPageArena() : current_allocation_point_(0) {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::NormalPageArena";
      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("free_list_");
      LOAD_FIELD_OFFSET("current_allocation_point_");
    }

    char buf1[20];
    COREADDR src = 0;

    BaseArena::load(addr);
    if (addr_) {
      src = addr_ + offsets_["current_allocation_point_"];
      LOAD_MEMBER_POINTER(current_allocation_point_);
      free_list_.load(addr_ + offsets_["free_list_"]);
    }
    else {
      free_list_.load(0);
      current_allocation_point_ = 0;
    }
  }

  virtual void dump(std::ostream &s) const {
    BaseArena::dump(s);
    s << "free chunks:\n";
    free_list_.dump(s);
  }

  bool is_current_page(COREADDR lower, COREADDR upper) const {
    return current_allocation_point_ >= lower
           && current_allocation_point_ < upper;
  }
};

class LargeObjectArena : public BaseArena {};

BaseArena *BaseArena::create(COREADDR addr) {
  static std::map<std::string, BaseArena*(*)()> ctors;
  if (ctors.size() == 0) {
    ADD_CTOR(BaseArena, blink::NormalPageArena,
                        blink::NormalPageArena);
    ADD_CTOR(BaseArena, blink::LargeObjectArena,
                        blink::LargeObjectArena);
  }

  BaseArena *p = nullptr;
  if (!addr) {
    p = new BaseArena();
  }
  else {
    CHAR buf[20];
    const auto &type = ResolveType(addr);
    if (ctors.find(type) != ctors.end()) {
      p = ctors[type]();
    }
    else {
      dprintf("> Fallback to BaseArena::ctor() for %s (= %s)\n",
              type.c_str(),
              ptos(addr, buf, sizeof(buf)));
      p = new BaseArena();
    }
    p->load(addr);
  }
  return p;
}

class MemoryRegion : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  COREADDR base_;
  uint64_t size_;

public:
  MemoryRegion() : base_(0), size_(0) {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::MemoryRegion";
      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("base_");
      LOAD_FIELD_OFFSET("size_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["base_"];
      LOAD_MEMBER_POINTER(base_);
      src = addr_ + offsets_["size_"];
      LOAD_MEMBER_VALUE(size_);
    }
    else {
      base_ = size_ = 0;
    }
  }

  COREADDR Base() const { return base_; }
  uint64_t size() const { return size_; }
};

class PageMemoryRegion : public MemoryRegion {};

class PageMemory : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  MemoryRegion reserved_;
  MemoryRegion writable_;

public:
  PageMemory() {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::PageMemory";
      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("reserved_");
      LOAD_FIELD_OFFSET("writable_");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["reserved_"];
      LOAD_MEMBER_POINTER(addr);
      reserved_.load(addr);
      writable_.load(addr_ + offsets_["writable_"]);
    }
    else {
      reserved_.load(0);
      writable_.load(0);
    }
  }

  void dump(std::ostream &s) const {
    char buf1[20];
    char buf2[20];
    s << ptos(reserved_.Base(), buf1, sizeof(buf1)) << ' '
      << static_cast<uint32_t>(reserved_.size()) << ' '
      << ptos(writable_.Base(), buf2, sizeof(buf2)) << ' '
      << static_cast<uint32_t>(writable_.size())
      << std::endl;
  }
};

class PagePool : public Object {
private:
  std::vector<COREADDR> pool_;

public:
  struct PoolEntry : public Object {
    static std::map<std::string, ULONG> offsets_;

    PageMemory data;
    COREADDR next;

    PoolEntry() : next(0) {}

    void load(COREADDR addr) {
      const int pointer_size = target().is64bit_ ? 8 : 4;
      if (offsets_.size() == 0) {
        std::string type(target().engine());
        type += "!blink::PagePool::PoolEntry";

        ULONG offset = 0;
        const char *field_name = nullptr;
        LOAD_FIELD_OFFSET("data");
        LOAD_FIELD_OFFSET("next");
      }

      char buf1[20];
      COREADDR src = 0;

      addr_ = addr;
      if (addr_) {
        src = addr_ + offsets_["data"];
        LOAD_MEMBER_POINTER(addr);
        data.load(addr);
        src = addr_ + offsets_["next"];
        LOAD_MEMBER_POINTER(next);
      }
      else {
        data.load(0);
        next = 0;
      }
    }

    void dump(std::ostream &s) const {
      s << "blink::PagePool::PoolEntry chain:" << std::endl;
      char buf1[20];
      int count = 0;
      PoolEntry entry;
      for (COREADDR p = addr_; p; p = entry.next, ++count) {
        entry.load(p);
        s << ptos(p, buf1, sizeof(buf1)) << ' ';
        entry.data.dump(s);
      }
    }
  };

  PagePool() {}

  void load(COREADDR addr, int arena_count) {
    // Assuming pool_ is the only member in PagePool for simplicity

    const int pointer_size = target().is64bit_ ? 8 : 4;
    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      pool_.resize(arena_count);
      src = addr_;
      for (auto &it : pool_) {
        LOAD_MEMBER_POINTER(addr);
        it = addr;
        src += pointer_size;
      }
    }
    else {
      pool_.clear();
    }
  }

  void dump(std::ostream &s) const {
    char buf1[20];
    for (size_t i = 0; i < pool_.size(); ++i) {
      if (pool_[i]) {
        s << std::setw(3) << i
          << " blink::PagePool::PoolEntry "
          << ptos(pool_[i], buf1, sizeof(buf1));

        PoolEntry entry;
        int count = 0;
        for (COREADDR p = pool_[i]; p; p = entry.next, ++count)
          entry.load(p);
        s << ' ' << count << std::endl;
      }
    }
  }
};

class ThreadHeap : public Object {
private:
  static std::map<std::string, ULONG> offsets_;
  static int arena_count_;

  PagePool free_page_pool_;
  std::vector<COREADDR> arenas_;

public:
  ThreadHeap() {}

  void load(COREADDR addr) {
    const int pointer_size = target().is64bit_ ? 8 : 4;
    if (offsets_.size() == 0) {
      std::string type(target().engine());
      type += "!blink::ThreadHeap";

      ULONG offset = 0;
      const char *field_name = nullptr;

      LOAD_FIELD_OFFSET("free_page_pool_");

      auto field = GetFieldInfo(type.c_str(), "arenas_");
      if (field.size > 0) {
        offsets_["arenas_"] = field.FieldOffset;
        arena_count_ = field.size / pointer_size;
      }
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      arenas_.resize(arena_count_);
      src = addr_ + offsets_["arenas_"];
      for (auto &arena : arenas_) {
        LOAD_MEMBER_POINTER(addr);
        arena = addr;
        src += pointer_size;
      }
      src = addr_ + offsets_["free_page_pool_"];
      LOAD_MEMBER_POINTER(addr);
      free_page_pool_.load(addr, arena_count_);
    }
    else {
      free_page_pool_.load(0, 0);
      arenas_.clear();
    }
  }

  void dump(std::ostream &s) const {
    char buf1[20];
    s << "arenas:" << std::endl;
    for (size_t i = 0; i < arenas_.size(); ++i) {
      s << std::setw(3) << i << ' '
        << ResolveType(arenas_[i])
        << ' ' << ptos(arenas_[i], buf1, sizeof(buf1))
        << std::endl;
    }
    s << "free page pool entries:" << std::endl;
    free_page_pool_.dump(s);
  }
};

std::map<std::string, ULONG> HeapObjectHeader::offsets_;
std::map<std::string, ULONG> FreeListEntry::offsets_;
std::map<std::string, ULONG> FreeList::offsets_;
std::map<std::string, ULONG> BasePage::offsets_;
std::map<std::string, ULONG> ThreadState::offsets_;
std::map<std::string, ULONG> BaseArena::offsets_;
std::map<std::string, ULONG> NormalPageArena::offsets_;
std::map<std::string, ULONG> PageMemory::offsets_;
std::map<std::string, ULONG> MemoryRegion::offsets_;
std::map<std::string, ULONG> PagePool::PoolEntry::offsets_;
std::map<std::string, ULONG> ThreadHeap::offsets_;
int ThreadHeap::arena_count_ = 0;

}

DECLARE_API(pool) {
  dump_arg<blink::PagePool::PoolEntry>(args);
}

DECLARE_API(heap) {
  dump_arg<blink::ThreadHeap>(args);
}

DECLARE_API(chunk) {
  dump_arg<blink::HeapObjectHeader>(args);
}

DECLARE_API(arena) {
  const char delim[] = " ";
  char args_copy[1024];
  if (args && strcpy_s(args_copy, sizeof(args_copy), args) == 0) {
    char *next_token = nullptr;
    if (auto token = strtok_s(args_copy, delim, &next_token)) {
      Object global_initializer;
      std::unique_ptr<blink::BaseArena> arena(
        blink::BaseArena::create(GetExpression(token)));
      std::stringstream s;
      arena->dump(s);
      dprintf(s.str().c_str());
    }
  }
}
