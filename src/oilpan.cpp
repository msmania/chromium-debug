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

const size_t kBlinkPageSizeLog2 = 17;
const size_t kBlinkPageSize = 1 << kBlinkPageSizeLog2;

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

class BaseArena : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

protected:
  std::unique_ptr<BasePage> first_page_;

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
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;

    src = addr + offsets_["first_page_"];
    LOAD_MEMBER_POINTER(addr);
    first_page_.reset(BasePage::create(addr));
  }

  virtual void dump() const {
    char buf1[20];
    char buf2[20];
    std::stringstream ss;
    ss << ResolveType(addr_)
       << " " << ptos(addr_, buf1, sizeof(buf1)) << std::endl;
    for (BasePage *page = first_page_.get();
         ;
         page = page->Next()) {
      auto addr = page->addr();
      if (!addr) break;
      auto limit = addr + page->size();
      ss << "  <- " << page->type()
         << " " << ptos(addr, buf1, sizeof(buf1))
         << " - " << ptos(limit, buf2, sizeof(buf2))
         << (is_current_page(addr, limit) ? " #" : "")
         << std::endl;
    }
    dprintf(ss.str().c_str());
  }
};

class NormalPageArena : public BaseArena {
private:
  static std::map<std::string, ULONG> offsets_;

  COREADDR current_allocation_point_;

public:
  NormalPageArena() : current_allocation_point_(0) {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!blink::NormalPageArena";
      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("current_allocation_point_");
    }

    char buf1[20];
    COREADDR src = 0;

    BaseArena::load(addr);

    src = addr + offsets_["current_allocation_point_"];
    LOAD_MEMBER_POINTER(current_allocation_point_);
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

class ThreadHeap : public Object {
private:
  static std::map<std::string, ULONG> offsets_;
  static int arena_count_;

  std::vector<COREADDR> arenas_;

public:
  ThreadHeap() {}

  void load(COREADDR addr) {
    const int pointer_size = target().is64bit_ ? 8 : 4;
    if (offsets_.size() == 0) {
      std::string ts(target().engine());
      ts += "!blink::ThreadHeap";
      auto field = GetFieldInfo(ts.c_str(), "arenas_");
      if (field.size > 0) {
        offsets_["arenas_"] = field.FieldOffset;
        arena_count_ = field.size / pointer_size;
        arenas_.resize(arena_count_);
      }
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;

    src = addr_ + offsets_["arenas_"];
    for (auto &arena : arenas_) {
      LOAD_MEMBER_POINTER(addr);
      arena = addr;
      src += pointer_size;
    }
  }

  void dump() const {
    char buf1[20];
    std::stringstream ss;
    for (size_t i = 0; i < arenas_.size(); ++i) {
      ss << "arena[" << i << "] "
         << ResolveType(arenas_[i])
         << " " << ptos(COREADDR(arenas_[i]), buf1, sizeof(buf1))
         << std::endl;
    }
    dprintf(ss.str().c_str());
  }
};

std::map<std::string, ULONG> BasePage::offsets_;
std::map<std::string, ULONG> BaseArena::offsets_;
std::map<std::string, ULONG> NormalPageArena::offsets_;
std::map<std::string, ULONG> ThreadHeap::offsets_;
int ThreadHeap::arena_count_ = 0;

}

DECLARE_API(heap) {
  const char delim[] = " ";
  char args_copy[1024];
  if (args && strcpy_s(args_copy, sizeof(args_copy), args) == 0) {
    char *next_token = nullptr;
    if (auto token = strtok_s(args_copy, delim, &next_token)) {
      blink::ThreadHeap th;
      th.load(GetExpression(token));
      th.dump();
    }
  }
}

DECLARE_API(arena) {
  const char delim[] = " ";
  char args_copy[1024];
  if (args && strcpy_s(args_copy, sizeof(args_copy), args) == 0) {
    char *next_token = nullptr;
    if (auto token = strtok_s(args_copy, delim, &next_token)) {
      std::unique_ptr<blink::BaseArena> arena(blink::BaseArena::create(GetExpression(token)));
      arena->dump();
    }
  }
}