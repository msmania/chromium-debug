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

#define ADD_CTOR(BASE, KEY, CLASS) \
  ctors[#KEY] = []() -> BASE* {return new CLASS();}

typedef ULONG64 COREADDR;

LPCSTR ptos(ULONG64 p, LPSTR s, ULONG len);
FIELD_INFO GetFieldInfo(IN LPCSTR Type, IN LPCSTR Field);
const std::string &ResolveType(COREADDR addr);

class CPEImage {
private:
  ULONG64 imagebase_;
  WORD platform_;
  IMAGE_DATA_DIRECTORY resource_dir_;
  IMAGE_DATA_DIRECTORY import_dir_;

  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms647001(v=vs.85).aspx
  struct VS_VERSIONINFO {
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[16]; // L"VS_VERSION_INFO"
    VS_FIXEDFILEINFO Value;
  } version_;

  bool Initialize(ULONG64 ImageBase);
  ULONG ReadPointerEx(ULONG64 Address, PULONG64 Pointer) const;

public:
  CPEImage(ULONG64 ImageBase);
  virtual ~CPEImage();

  bool IsInitialized() const;
  bool Is64bit() const;
  bool LoadVersion();
  WORD GetPlatform() const;
  void GetVersion(PDWORD FileVersionMS,
                  PDWORD FileVersionLS,
                  PDWORD ProductVersionMS,
                  PDWORD ProductVersionLS) const;
  void DumpImportTable(LPCSTR DllName) const;
};

struct TARGETINFO {
  std::string engine_;
  bool is64bit_;
  WORD version_;
  WORD buildnum_;

  TARGETINFO();
  const char *engine() const;
};

class Object {
private:
  static TARGETINFO targetinfo_;

protected:
  COREADDR addr_;

public:
  static COREADDR get_expression(const char *symbol);
  static void InitializeTargetInfo();
  static const TARGETINFO &target();
  static bool ReadPointerEx(ULONG64 address, ULONG64 &pointer);
  static COREADDR deref(ULONG64 address);

  template<typename T>
  static bool ReadValue(ULONG64 address, T &pointer) {
    pointer = static_cast<T>(0);
    ULONG cb = 0;
    if (ReadMemory(address, &pointer, sizeof(T), &cb)) {
      return true;
    }
    return false;
  }

  Object();
  COREADDR addr() const { return addr_; }
};

template <typename T>
static void dump_arg(PCSTR args) {
  const char delim[] = " ";
  char args_copy[1024];
  if (args && strcpy_s(args_copy, sizeof(args_copy), args) == 0) {
    char *next_token = nullptr;
    if (auto token = strtok_s(args_copy, delim, &next_token)) {
      T t;
      t.load(GetExpression(token));
      std::stringstream s;
      t.dump(s);
      dprintf(s.str().c_str());
    }
  }
}

template <bool newline, typename T>
static void dump_arg_factory(PCSTR args, T*(*factory)(COREADDR)) {
  const char delim[] = " ";
  char args_copy[1024];
  if (args && strcpy_s(args_copy, sizeof(args_copy), args) == 0) {
    char *next_token = nullptr;
    if (auto token = strtok_s(args_copy, delim, &next_token)) {
      Object global_initializer;
      std::unique_ptr<T> t(factory(GetExpression(token)));
      std::stringstream s;
      t->dump(s);
      if (newline) s << std::endl;
      dprintf(s.str().c_str());
    }
  }
}
