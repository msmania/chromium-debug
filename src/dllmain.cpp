#define KDEXT_64BIT
#include <windows.h>
#include <wdbgexts.h>

// https://msdn.microsoft.com/en-us/library/windows/hardware/ff543968(v=vs.85).aspx
static EXT_API_VERSION ApiVersion = {
  0, 0,                       // These values are not checked by debugger. Any value is ok.
  EXT_API_VERSION_NUMBER64,   // Must be 64bit to get `args` pointer
  0                           // Reserved
};

WINDBG_EXTENSION_APIS ExtensionApis;

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
  return TRUE;
}

LPEXT_API_VERSION ExtensionApiVersion(void) {
  return &ApiVersion;
}

void init_target_info();

VOID WinDbgExtensionDllInit(PWINDBG_EXTENSION_APIS lpExtensionApis,
                            USHORT MajorVersion,
                            USHORT MinorVersion) {
  ExtensionApis = *lpExtensionApis;
  init_target_info();
}

DECLARE_API(help) {
  dprintf(
    "\n# DOM\n"
    "!dom <Node*>         - Dump DOM tree\n"

    "\n# Layout\n"
    "!lay <LayoutObject*> - Dump Layout tree\n"
    "\n# PartitionAlloc\n"
    "!fast_malloc_allocator [-buckets] [address]\n"
    "!array_buffer_allocator [-buckets] [address]\n"
    "!buffer_allocator [-buckets] [address]\n"
    "!layout_allocator [-buckets] [address]\n"
    "!bucket <address>\n"
    "!ppage <address>\n"
    "!slot <address>\n"
    "!spage <address>\n"

    "\n# Oilpan\n"
    "!ts [address]\n"
    "!persistent <address>\n"
    "!heap <address>\n"
    "!pool <address>\n"
    "!arena <address>\n"
    "!hpage <address>\n"
    "!scan [-bitmap] <address>\n"
    "!chunk <address>\n"

    "\n# Friendly info :)\n\n"
    "Symbol server:\n"
    "https://chromium-browser-symsrv.commondatastorage.googleapis.com\n\n"
    "Useful flags:\n"
    "--enable-heap-profiling\n"
    "--single-process\n"
    "--renderer-startup-dialog --no-sandbox\n"
    "--renderer-process-limit=1\n\n"
    "See also:\n"
    "https://www.chromium.org/developers/how-tos/debugging-on-windows\n\n"
    "Good luck!\n\n"
    );
}
