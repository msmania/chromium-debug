#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <memory>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

#define LOAD_FIELD_OFFSET(FIELD) \
  field_name = FIELD; \
  if (GetFieldOffset(type.c_str(), field_name, &offset) == 0) { \
    offsets_[field_name] = offset; \
  } \
  else { \
    dprintf("ERR> Symbol not found: %s::%s\n", type.c_str(), field_name); \
  }

#define LOAD_MEMBER_POINTER(MEMBER) \
  if (!ReadPointerEx(src, MEMBER)) { \
    dprintf("ERR> Failed to load a pointer at %s\n", \
            ptos(src, buf1, sizeof(buf1))); \
  }

#define LOAD_MEMBER_VALUE(MEMBER) \
    if (!ReadValue(src, MEMBER)) { \
      dprintf("ERR> Failed to load a value at %s\n", \
              ptos(src, buf1, sizeof(buf1))); \
    }

class VTableMap {
private:
  char symbol_name_[1024];
  std::map<COREADDR, std::string> vt_to_class_;

  static const std::string StripSymbolName(const std::string &symbol) {
    auto bang = symbol.find('!');
    auto vtmark = symbol.rfind("::`vftable'");
    return (bang != std::string::npos && vtmark != std::string::npos)
           ? symbol.substr(bang + 1, vtmark - bang - 1)
           : "";
  }

  void Set(COREADDR vtAddr) {
    COREADDR displacement = 0;
    GetSymbol(vtAddr, symbol_name_, &displacement);
    if (displacement == 0) {
      vt_to_class_[vtAddr] = StripSymbolName(symbol_name_);
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

namespace blink {

class Node : public Object {
private:
  static std::map<std::string, ULONG> offsets_;
  static VTableMap vtable_map_;

  static void LoadSymbols(LPCSTR module) {
    std::string type = module;
    type += "!blink::Node";

    ULONG offset = 0;
    const char *field_name = nullptr;

    LOAD_FIELD_OFFSET("node_flags_");
    LOAD_FIELD_OFFSET("parent_or_shadow_host_node_");
    LOAD_FIELD_OFFSET("previous_");
    LOAD_FIELD_OFFSET("next_");
  }

  enum NodeFlags {
    kZero = 0,
    kHasRareDataFlag = 1,

    // Node type flags. These never change once created.
    kIsTextFlag = 1 << 1,
    kIsContainerFlag = 1 << 2,
    kIsElementFlag = 1 << 3,
    kIsHTMLFlag = 1 << 4,
    kIsSVGFlag = 1 << 5,
    kIsDocumentFragmentFlag = 1 << 6,
    kIsV0InsertionPointFlag = 1 << 7,
  };

protected:
  NodeFlags node_flags_;
  COREADDR parent_or_shadow_host_node_;
  COREADDR previous_;
  COREADDR next_;

  virtual void Reset(COREADDR addr) {
    Object::Reset(addr);
    node_flags_ = kZero;
    parent_or_shadow_host_node_ = previous_ = next_ = 0;
  }

public:
  static std::unique_ptr<Node> CreateNode(COREADDR addr);

  Node()
    : node_flags_(kZero),
      parent_or_shadow_host_node_(0),
      previous_(0),
      next_(0)
  {}

  bool GetFlag(NodeFlags mask) const { return node_flags_ & mask; }
  bool IsElementNode() const { return GetFlag(kIsElementFlag); }
  bool IsContainerNode() const { return GetFlag(kIsContainerFlag); }
  bool IsTextNode() const { return GetFlag(kIsTextFlag); }
  bool IsHTMLElement() const { return GetFlag(kIsHTMLFlag); }
  bool IsSVGElement() const { return GetFlag(kIsSVGFlag); }
  COREADDR nextSibling() const { return next_; }

  COREADDR Parent() const {
    return parent_or_shadow_host_node_;
  }

  Node GetRoot() const {
    Node node = *this;
    while (node.Parent()) {
      node.Load(node.Parent());
    }
    return node;
  }

  virtual void Load(COREADDR addr) {
    char buf1[20];
    COREADDR src = 0;

    if (offsets_.size() == 0) {
      LoadSymbols(target().engine());
    }

    Reset(addr);

    src = addr + offsets_["node_flags_"];
    LOAD_MEMBER_VALUE(node_flags_);
    src = addr + offsets_["parent_or_shadow_host_node_"];
    LOAD_MEMBER_POINTER(parent_or_shadow_host_node_);
    src = addr + offsets_["previous_"];
    LOAD_MEMBER_POINTER(previous_);
    src = addr + offsets_["next_"];
    LOAD_MEMBER_POINTER(next_);
  }

  virtual void Dump(int &node_count, int depth) {
    char buf1[20];

    const auto &type = vtable_map_.Get(addr_);

    std::stringstream ss;
    ss << std::setw(5) << node_count
       << std::string(depth + 1, ' ')
       << ptos(addr_, buf1, sizeof(buf1))
       << " " << type
       << " " << std::hex << std::setfill('0') << std::setw(8)
       << node_flags_
       << std::endl;
    dprintf(ss.str().c_str());
    ++node_count;
  }
};

class ContainerNode : public Node {
private:
  static std::map<std::string, ULONG> offsets_;

  static void LoadSymbols(LPCSTR module) {
    std::string type = module;
    type += "!blink::ContainerNode";

    ULONG offset = 0;
    const char *field_name = nullptr;

    LOAD_FIELD_OFFSET("first_child_");
    LOAD_FIELD_OFFSET("last_child_");
  }

  virtual void Reset(COREADDR addr) {
    Node::Reset(addr);
    first_child_ = last_child_ = 0;
  }

protected:
  COREADDR first_child_;
  COREADDR last_child_;

public:
  ContainerNode()
    : first_child_(0),
      last_child_(0)
  {}

  virtual void Load(COREADDR addr) {
    char buf1[20];
    COREADDR src = 0;

    if (offsets_.size() == 0) {
      LoadSymbols(target().engine());
    }

    Node::Load(addr);

    src = addr + offsets_["first_child_"];
    LOAD_MEMBER_POINTER(first_child_);
    src = addr + offsets_["last_child_"];
    LOAD_MEMBER_POINTER(last_child_);
  }

  virtual void Dump(int &node_count, int depth) {
    Node::Dump(node_count, depth);

    COREADDR child_addr = first_child_;
    while (child_addr) {
      auto child = CreateNode(child_addr);
      child->Dump(node_count, depth + 1);
      child_addr = child->nextSibling();
    }
  }
};

std::map<std::string, ULONG> Node::offsets_;
VTableMap Node::vtable_map_;
std::map<std::string, ULONG> ContainerNode::offsets_;

std::unique_ptr<Node> Node::CreateNode(COREADDR addr) {
  Node node;
  node.Load(addr);
  std::unique_ptr<Node> p = std::unique_ptr<Node>(node.IsContainerNode()
                                                  ? new ContainerNode()
                                                  : new Node());
  p->Load(addr);
  return p;
}

}

DECLARE_API(dom) {
  const char delim[] = " ";
  char args_copy[1024];
  if ( args && strcpy_s(args_copy, sizeof(args_copy), args)==0 ) {
    char *next_token = nullptr;
    if (auto token = strtok_s(args_copy, delim, &next_token)) {
      blink::Node node;
      node.Load(GetExpression(token));

      int node_count = 0;
      auto root = blink::Node::CreateNode(node.GetRoot());
      root->Dump(node_count, 0);
    }
  }
}
