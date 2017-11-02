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

class LayoutObjectChildList : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  static void LoadSymbols(LPCSTR module) {
    std::string type = module;
    type += "!blink::LayoutObjectChildList";

    ULONG offset = 0;
    const char *field_name = nullptr;

    LOAD_FIELD_OFFSET("first_child_");
    LOAD_FIELD_OFFSET("last_child_");
  }

protected:
  COREADDR first_child_;
  COREADDR last_child_;

public:
  LayoutObjectChildList()
    : first_child_(0),
      last_child_(0)
  {}

  COREADDR FirstChild() const { return first_child_; }
  COREADDR LastChild() const { return last_child_; }

  virtual void Load(COREADDR addr) {
    if (offsets_.size() == 0) {
      LoadSymbols(target().engine());
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    first_child_ = last_child_ = 0;

    src = addr + offsets_["first_child_"];
    LOAD_MEMBER_POINTER(first_child_);
    src = addr + offsets_["last_child_"];
    LOAD_MEMBER_POINTER(last_child_);
  }
};

class LayoutObject : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  static void LoadSymbols(LPCSTR module) {
    std::string type = module;
    type += "!blink::LayoutObject";

    ULONG offset = 0;
    const char *field_name = nullptr;

    LOAD_FIELD_OFFSET("node_");
    LOAD_FIELD_OFFSET("parent_");
    LOAD_FIELD_OFFSET("previous_");
    LOAD_FIELD_OFFSET("next_");
  }

protected:
  std::string type_;
  COREADDR node_;
  COREADDR parent_;
  COREADDR previous_;
  COREADDR next_;

public:
  static std::unique_ptr<LayoutObject> CreateNode(COREADDR addr);

  LayoutObject()
    : parent_(0),
      previous_(0),
      next_(0)
  {}

  COREADDR Parent() const { return parent_; }
  COREADDR NextSibling() const { return next_; }

  LayoutObject GetRoot() const {
    LayoutObject node = *this;
    while (node.Parent()) {
      node.Load(node.Parent());
    }
    return node;
  }

  virtual void Load(COREADDR addr) {
    if (offsets_.size() == 0) {
      LoadSymbols(target().engine());
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    node_ = parent_ = previous_ = next_ = 0;

    src = addr + offsets_["node_"];
    LOAD_MEMBER_POINTER(node_);
    src = addr + offsets_["parent_"];
    LOAD_MEMBER_POINTER(parent_);
    src = addr + offsets_["previous_"];
    LOAD_MEMBER_POINTER(previous_);
    src = addr + offsets_["next_"];
    LOAD_MEMBER_POINTER(next_);
  }

  virtual void Dump(int &node_count, int depth) {
    char buf1[20];
    char buf2[20];
    std::stringstream ss;
    ss << std::setw(5) << node_count
       << std::string(depth + 1, ' ')
       << ptos(addr_, buf1, sizeof(buf1))
       << " " << type_;

    if (node_) {
      ss << " " << ptos(node_, buf2, sizeof(buf2))
         << " " << ResolveType(node_);
    }

    ss << std::endl;
    dprintf(ss.str().c_str());
    ++node_count;
  }
};

class LayoutText : public LayoutObject {};
class LayoutWordBreak : public LayoutText {};
class LayoutBoxModelObject : public LayoutObject {};
class LayoutBox : public LayoutBoxModelObject {};
class LayoutListMarker : public LayoutBox {};
class LayoutReplaced : public LayoutBox {};
class LayoutImage : public LayoutReplaced {};

class LayoutInline : public LayoutBoxModelObject {
private:
  static std::map<std::string, ULONG> offsets_;

  static void LoadSymbols(LPCSTR module) {
    std::string type = module;
    type += "!blink::LayoutInline";

    ULONG offset = 0;
    const char *field_name = nullptr;

    LOAD_FIELD_OFFSET("children_");
  }

protected:
  LayoutObjectChildList children_;

public:
  LayoutInline()
  {}

  virtual void Load(COREADDR addr) {
    if (offsets_.size() == 0) {
      LoadSymbols(target().engine());
    }

    LayoutBoxModelObject::Load(addr);
    children_.Load(addr + offsets_["children_"]);
  }

  virtual void Dump(int &node_count, int depth) {
    LayoutBoxModelObject::Dump(node_count, depth);

    COREADDR child_addr = children_.FirstChild();
    while (child_addr) {
      auto child = CreateNode(child_addr);
      child->Dump(node_count, depth + 1);
      child_addr = child->NextSibling();
    }
  }
};

class LayoutBlock : public LayoutBox {
private:
  static std::map<std::string, ULONG> offsets_;

  static void LoadSymbols(LPCSTR module) {
    std::string type = module;
    type += "!blink::LayoutBlock";

    ULONG offset = 0;
    const char *field_name = nullptr;

    LOAD_FIELD_OFFSET("children_");
  }

protected:
  LayoutObjectChildList children_;

public:
  LayoutBlock()
  {}

  virtual void Load(COREADDR addr) {
    if (offsets_.size() == 0) {
      LoadSymbols(target().engine());
    }

    LayoutBox::Load(addr);
    children_.Load(addr + offsets_["children_"]);
  }

  virtual void Dump(int &node_count, int depth) {
    LayoutBox::Dump(node_count, depth);

    COREADDR child_addr = children_.FirstChild();
    while (child_addr) {
      auto child = CreateNode(child_addr);
      child->Dump(node_count, depth + 1);
      child_addr = child->NextSibling();
    }
  }
};

class LayoutBlockFlow : public LayoutBlock {};
class LayoutView : public LayoutBlockFlow {};
class LayoutListItem : public LayoutBlockFlow {};
class LayoutTable : public LayoutBlock {};
class LayoutTableCell : public LayoutBlockFlow {};
class LayoutTextControlInnerEditor : public LayoutBlockFlow {};
class LayoutTextControl : public LayoutBlockFlow {};
class LayoutTextControlMultiLine : public LayoutTextControl {};
class LayoutTextControlSingleLine : public LayoutTextControl {};

class LayoutTableBoxComponent : public LayoutBox {
private:
  static std::map<std::string, ULONG> offsets_;

  static void LoadSymbols(LPCSTR module) {
    std::string type = module;
    type += "!blink::LayoutTableBoxComponent";

    ULONG offset = 0;
    const char *field_name = nullptr;

    LOAD_FIELD_OFFSET("children_");
  }

protected:
  LayoutObjectChildList children_;

public:
  LayoutTableBoxComponent()
  {}

  virtual void Load(COREADDR addr) {
    if (offsets_.size() == 0) {
      LoadSymbols(target().engine());
    }

    LayoutBox::Load(addr);
    children_.Load(addr + offsets_["children_"]);
  }

  virtual void Dump(int &node_count, int depth) {
    LayoutBox::Dump(node_count, depth);

    COREADDR child_addr = children_.FirstChild();
    while (child_addr) {
      auto child = CreateNode(child_addr);
      child->Dump(node_count, depth + 1);
      child_addr = child->NextSibling();
    }
  }
};

class LayoutTableCol : public LayoutTableBoxComponent {};
class LayoutTableRow : public LayoutTableBoxComponent {};
class LayoutTableSection : public LayoutTableBoxComponent {};

std::map<std::string, ULONG> LayoutBlock::offsets_;
std::map<std::string, ULONG> LayoutInline::offsets_;
std::map<std::string, ULONG> LayoutObject::offsets_;
std::map<std::string, ULONG> LayoutObjectChildList::offsets_;
std::map<std::string, ULONG> LayoutTableBoxComponent::offsets_;

#define ADD_CTOR(BASE, KEY, CLASS) \
  ctors[#KEY] = []() -> BASE* {return new CLASS();}

std::unique_ptr<LayoutObject> LayoutObject::CreateNode(COREADDR addr) {
  static std::map<std::string, LayoutObject*(*)()> ctors;
  if (ctors.size() == 0) {
    ADD_CTOR(LayoutObject, blink::LayoutBlock,
                           blink::LayoutBlock);
    ADD_CTOR(LayoutObject, blink::LayoutBlockFlow,
                           blink::LayoutBlockFlow);
    ADD_CTOR(LayoutObject, blink::LayoutImage,
                           blink::LayoutImage);
    ADD_CTOR(LayoutObject, blink::LayoutInline,
                           blink::LayoutInline);
    ADD_CTOR(LayoutObject, blink::LayoutListItem,
                           blink::LayoutListItem);
    ADD_CTOR(LayoutObject, blink::LayoutListMarker,
                           blink::LayoutListMarker);
    ADD_CTOR(LayoutObject, blink::LayoutTable,
                           blink::LayoutTable);
    ADD_CTOR(LayoutObject, blink::LayoutTableCell,
                           blink::LayoutTableCell);
    ADD_CTOR(LayoutObject, blink::LayoutTableCol,
                           blink::LayoutTableCol);
    ADD_CTOR(LayoutObject, blink::LayoutTableRow,
                           blink::LayoutTableRow);
    ADD_CTOR(LayoutObject, blink::LayoutTableSection,
                           blink::LayoutTableSection);
    ADD_CTOR(LayoutObject, blink::LayoutText,
                           blink::LayoutText);
    ADD_CTOR(LayoutObject, blink::LayoutTextControlInnerEditor,
                           blink::LayoutTextControlInnerEditor);
    ADD_CTOR(LayoutObject, blink::LayoutTextControlMultiLine,
                           blink::LayoutTextControlMultiLine);
    ADD_CTOR(LayoutObject, blink::LayoutTextControlSingleLine,
                           blink::LayoutTextControlSingleLine);
    ADD_CTOR(LayoutObject, blink::LayoutView,
                           blink::LayoutView);
    ADD_CTOR(LayoutObject, blink::LayoutWordBreak,
                           blink::LayoutWordBreak);
  }

  LayoutObject *p = nullptr;
  if (!addr) {
    p = new LayoutObject();
  }
  else {
    CHAR buf[20];
    const auto &type = ResolveType(addr);
    if (ctors.find(type) != ctors.end()) {
      p = ctors[type]();
    }
    else {
      dprintf("> Fallback to LayoutObject::ctor() for %s (= %s)\n",
              type.c_str(),
              ptos(addr, buf, sizeof(buf)));
      p = new LayoutObject();
    }
    p->type_ = type;
    p->Load(addr);
  }
  return std::unique_ptr<LayoutObject>(p);
}

}

DECLARE_API(lay) {
  const char delim[] = " ";
  char args_copy[1024];
  if ( args && strcpy_s(args_copy, sizeof(args_copy), args)==0 ) {
    char *next_token = nullptr;
    if (auto token = strtok_s(args_copy, delim, &next_token)) {
      blink::LayoutObject node;
      node.Load(GetExpression(token));

      int node_count = 0;
      auto root = blink::LayoutObject::CreateNode(node.GetRoot());
      root->Dump(node_count, 0);
    }
  }
}
