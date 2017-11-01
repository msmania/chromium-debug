typedef ULONG64 COREADDR;

LPCSTR ptos(ULONG64 p, LPSTR s, ULONG len);

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

  void Reset(COREADDR addr);

public:
  static void InitializeTargetInfo();
  static const TARGETINFO &target();
  static bool ReadPointerEx(ULONG64 address, ULONG64 &pointer);

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
  operator COREADDR() const;
};
