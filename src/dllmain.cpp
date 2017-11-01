#include <windows.h>
#include <stdio.h>
#include <string>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

#define LODWORD(ll) ((DWORD)((ll)&0xffffffff))
#define HIDWORD(ll) ((DWORD)(((ll)>>32)&0xffffffff))

LPCSTR ptos(ULONG64 p, LPSTR s, ULONG len) {
  LPCSTR Ret = NULL;
  if (HIDWORD(p) == 0 && len >= 9) {
    sprintf_s(s, len, "%08x", LODWORD(p));
    Ret = s;
  }
  else if (HIDWORD(p) != 0 && len >= 18) {
    sprintf_s(s, len, "%08x`%08x", HIDWORD(p), LODWORD(p));
    Ret = s;
  }
  else if (len > 0) {
    s[0] = 0;
    Ret = s;
  }
  return Ret;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
  return TRUE;
}

// https://msdn.microsoft.com/en-us/library/windows/hardware/ff543968(v=vs.85).aspx
static EXT_API_VERSION ApiVersion = {
  0, 0,                       // These values are not checked by debugger. Any value is ok.
  EXT_API_VERSION_NUMBER64,   // Must be 64bit to get `args` pointer
  0                           // Reserved
};

WINDBG_EXTENSION_APIS ExtensionApis;

LPEXT_API_VERSION ExtensionApiVersion(void) {
  return &ApiVersion;
}

VOID WinDbgExtensionDllInit(PWINDBG_EXTENSION_APIS lpExtensionApis,
                            USHORT MajorVersion,
                            USHORT MinorVersion) {
  ExtensionApis = *lpExtensionApis;
  return;
}

DECLARE_API(help) {
  dprintf("!dom <Node*>                    - Dump DOM tree\n"
          "!lay <LayoutObject*>            - Dump Layout tree\n"
          "\n"
          "# Friendly info :)\n\n"
          "Symbol server:\n"
          "https://chromium-browser-symsrv.commondatastorage.googleapis.com\n\n"
          "Optional flags:\n"
          "--single-process\n"
          "--renderer-startup-dialog --no-sandbox\n"
          "--renderer-process-limit=1\n\n"
          "See also:\n"
          "https://www.chromium.org/developers/how-tos/debugging-on-windows\n\n"
          "Good luck!\n\n"
          );
}
