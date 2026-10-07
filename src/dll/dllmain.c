#include "dllmain.h"
#include "../../external/inih/ini.c"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <psapi.h>

typedef struct {
  float fps;
  int UseCustomScreenDimensions;
  int ScreenWidth;
  int ScreenHeight;
  int EnableCursorClip;
  int CursorClipHotkey;
  int EnableLogging;
  int EnableBorderless;
  int FixSprintSlowdown;
} config;
config configFile;

void containCursor(void *args) {
  while (TRUE) {
    if (GetAsyncKeyState(configFile.CursorClipHotkey)) {
      if (GetForegroundWindow() == args)
        SetCapture(args);
      ClipCursor(&final);
    }
    Sleep(10);
  }
}

static int handler(void *Settings, const char *section, const char *name, const char *value) {
  config *modconfig = (config *)Settings;
  if (MATCH("Settings", "fps")) {
    modconfig->fps = atof(value);
  } else if (MATCH("Settings", "UseCustomScreenDimensions")) {
    modconfig->UseCustomScreenDimensions = atoi(value);
  } else if (MATCH("Settings", "ScreenWidth")) {
    modconfig->ScreenWidth = atoi(value);
  } else if (MATCH("Settings", "ScreenHeight")) {
    modconfig->ScreenHeight = atoi(value);
  } else if (MATCH("Settings", "EnableCursorClip")) {
    modconfig->EnableCursorClip = atoi(value);
  } else if (MATCH("Settings", "CursorClipHotkey")) {
    modconfig->CursorClipHotkey = strtol(value, NULL, 16);
  } else if (MATCH("Settings", "EnableLogging")) {
    modconfig->EnableLogging = atoi(value);
  } else if (MATCH("Settings", "EnableBorderless")) {
    modconfig->EnableBorderless = atoi(value);
  } else if (MATCH("Settings", "FixSprintSlowdown")) {
    modconfig->FixSprintSlowdown = atoi(value);
  } else
    return 0;
  return 1;
}

float readFile() {
  // Initialize config with default values
  configFile.EnableLogging = 0;     // Default logging off
  configFile.fps = 1000;            // Default FPS limit
  configFile.EnableBorderless = 1;  // Default borderless enabled
  configFile.FixSprintSlowdown = 1; // Default sprint fix enabled
  
  // Parse INI file, which will override defaults if present
  if (ini_parse("FPSconfig.ini", handler, &configFile) < 0) {
    return configFile.fps; // Return default if file is missing
  }
  return configFile.fps;
}

FILE* logFile = NULL;

void log_init() {
    if (!configFile.EnableLogging) return;
    logFile = fopen("DS3DebugFPS_log.txt", "w");
    if (logFile) {
        fprintf(logFile, "[INFO] Log started\n");
        fflush(logFile);
    }
    fclose(logFile);
    logFile = fopen("DS3DebugFPS_log.txt", "a");
}
void log_close() {
    if (!configFile.EnableLogging) return;
    if (logFile) fclose(logFile);
}
void log_print(const char* fmt, ...) {
    if (!configFile.EnableLogging || !logFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(logFile, fmt, args);
    fprintf(logFile, "\n");
    fflush(logFile);
    va_end(args);
}

// Committed pages only, Proton leaves unreadable gaps in the module
BYTE *scan(BYTE *start, BYTE *end, const BYTE *pattern, const char *mask) {
  size_t patternLen = strlen(mask);
  MEMORY_BASIC_INFORMATION mbi;
  for (BYTE *region = start; region < end && VirtualQuery(region, &mbi, sizeof(mbi)); region = (BYTE *)mbi.BaseAddress + mbi.RegionSize) {
    if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
      continue;
    BYTE *regionEnd = (BYTE *)mbi.BaseAddress + mbi.RegionSize;
    if (regionEnd > end)
      regionEnd = end;
    for (BYTE *p = region; p + patternLen <= regionEnd; ++p) {
      size_t j = 0;
      while (j < patternLen && (mask[j] != 'x' || p[j] == pattern[j]))
        ++j;
      if (j == patternLen)
        return p;
    }
  }
  return NULL;
}

void writeCode(void *address, const void *bytes, size_t size) {
  DWORD oldProtect;
  VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
  memcpy(address, bytes, size);
  VirtualProtect(address, size, oldProtect, &oldProtect);
  FlushInstructionCache(GetCurrentProcess(), address, size);
}

void redirect(BYTE *disp, void *target) {
  int rel = (int)((BYTE *)target - (disp + 4));
  writeCode(disp, &rel, sizeof(rel));
}

BOOL isSprjFlipper(DWORD64 object, BYTE *moduleBase, DWORD moduleSize) {
  DWORD64 vtable = 0;
  if (!object || (object & 7))
    return FALSE;
  if (NtReadVirtualMemory(GetCurrentProcess(), (LPVOID)object, &vtable, sizeof(vtable), NULL) != 0)
    return FALSE;
  return vtable >= (DWORD64)moduleBase && vtable < (DWORD64)moduleBase + moduleSize;
}

DWORD64 findSprjFlipper(BYTE *moduleBase, DWORD moduleSize) {
  BYTE *moduleEnd = moduleBase + moduleSize;

  // mov ecx, 0x368; call; ... mov [rip+slot], rax
  BYTE ctorPattern[] = { 0xB9, 0x68, 0x03, 0x00, 0x00, 0xE8 };
  for (BYTE *hit = moduleBase; (hit = scan(hit, moduleEnd, ctorPattern, "xxxxxx")); ++hit) {
    for (int i = 0; i < 64 - 7; ++i) {
      if (hit[i] == 0x48 && hit[i + 1] == 0x89 && hit[i + 2] == 0x05) {
        DWORD64 slot = (DWORD64)(hit + i + 7) + *(int *)(hit + i + 3);
        DWORD64 object = 0;
        NtReadVirtualMemory(GetCurrentProcess(), (LPVOID)slot, &object, sizeof(object), NULL);
        if (isSprjFlipper(object, moduleBase, moduleSize)) {
          log_print("[SCAN] Constructor pattern match at: 0x%llx, slot 0x%llx", (DWORD64)hit, slot);
          return object;
        }
        break;
      }
    }
  }

  // Fallback for Archthrones
  BYTE sprjFlipperPattern[] = { 0x50, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  for (BYTE *hit = moduleBase; (hit = scan(hit, moduleEnd, sprjFlipperPattern, "xx????xxxxxxxxxx")); ++hit) {
    if (isSprjFlipper(*(DWORD64 *)hit, moduleBase, moduleSize)) {
      log_print("[SCAN] Pointer pattern match at: 0x%llx", (DWORD64)hit);
      return *(DWORD64 *)hit;
    }
  }
  return 0;
}

float fpsCap;

DWORD WINAPI patchFps(void *args) {
  MODULEINFO moduleInfo;
  if (!GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(NULL), &moduleInfo, sizeof(moduleInfo))) {
    log_print("[SCAN] Failed to get module information for scan.");
    return 0;
  }
  log_print("[SCAN] Module base address: 0x%p", moduleInfo.lpBaseOfDll);

  for (int i = 0; i < 40 && !SprjFlipper; ++i) {
    SprjFlipper = findSprjFlipper(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
    if (!SprjFlipper)
      Sleep(250);
  }
  if (!SprjFlipper) {
    log_print("[SCAN] No matching patterns found in module.");
    return 0;
  }

  NtWriteVirtualMemory(GetCurrentProcess(), (LPVOID)(SprjFlipper + 0x354), &fpsCap, sizeof(DWORD), NULL);
  log_print("[PATCH] Patched rFPS at: 0x%llx", SprjFlipper + 0x354);
  NtWriteVirtualMemory(GetCurrentProcess(), (LPVOID)(SprjFlipper + 0x358), &useDebug, sizeof(char), NULL);
  log_print("[PATCH] Patched useDebug at: 0x%llx", SprjFlipper + 0x358);
  return 0;
}

float *speedFactors;
void *SpeedHookReturn;
void SpeedHook();

void UpdateSpeedFactors() {
  float frameTime = 1.0f / fpsCap;
  if (SprjFlipper) {
    float measured = *(float *)(SprjFlipper + 0x26C);
    if (measured >= 1.0f / 480.0f && measured <= 0.05f)
      frameTime = measured;
  }
  if (frameTime < 1.0f / 480.0f)
    frameTime = 1.0f / 480.0f;
  if (frameTime > 0.05f)
    frameTime = 0.05f;

  float slack = fpsCap >= 90.0f ? 1.2f : 1.0f;
  float recovery = fpsCap >= 90.0f ? 1.5f : 1.2f;
  speedFactors[0] = 0.5f / frameTime * slack;
  speedFactors[1] = expf(logf(0.8f) * frameTime * 60.0f);
  speedFactors[2] = expf(logf(recovery) * frameTime * 60.0f);
}

void fixSprintSlowdown(BYTE *moduleBase, DWORD moduleSize) {
  // Stuck check: distance * 30.0 < 1, tuned for 60 FPS
  BYTE speedPattern[] = { 0xF3, 0x0F, 0x58, 0x00, 0x0F, 0xC6, 0x00, 0x00, 0x0F, 0x51, 0x00, 0xF3, 0x0F, 0x59, 0x05, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x2F };
  BYTE *hit = scan(moduleBase, moduleBase + moduleSize, speedPattern, "xxx?xx?xxx?xxxx????xx");
  if (!hit) {
    log_print("[SCAN] Running-speed check not found, sprint slowdown is unchanged.");
    return;
  }
  log_print("[SCAN] Running-speed check at: 0x%llx", (DWORD64)hit);

  // Must be within 2GB of the game code
  BYTE *page = NULL;
  for (BYTE *hint = (BYTE *)(((DWORD64)moduleBase + moduleSize + 0xFFFF) & ~0xFFFFULL); !page && hint < moduleBase + 0x70000000; hint += 0x10000)
    page = VirtualAlloc(hint, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if (!page) {
    log_print("[PATCH] Could not allocate memory near the game code.");
    return;
  }
  speedFactors = (float *)page;
  UpdateSpeedFactors();

  redirect(hit + 15, &speedFactors[0]);
  if (memcmp(hit + 0x29, "\xF3\x0F\x59\x05", 4) == 0 && memcmp(hit + 0x54, "\xF3\x0F\x59\x05", 4) == 0) {
    redirect(hit + 0x2D, &speedFactors[1]);
    redirect(hit + 0x58, &speedFactors[2]);
  } else {
    log_print("[PATCH] Slowdown and recovery rates not found, left alone.");
  }

  BYTE prologue[] = { 0x4C, 0x8B, 0xDC, 0x57, 0x48, 0x83, 0xEC, 0x70 };
  BYTE *function = NULL;
  for (int i = 8; i <= 0x180 && !function; ++i)
    if (memcmp(hit - i, prologue, sizeof(prologue)) == 0)
      function = hit - i;
  if (!function) {
    log_print("[PATCH] Movement function not found, sprint fix uses the FPS cap only.");
    return;
  }

  BYTE *trampoline = page + 0x40;
  trampoline[0] = 0xFF;
  trampoline[1] = 0x25;
  *(DWORD *)(trampoline + 2) = 0;
  *(void **)(trampoline + 6) = (void *)SpeedHook;
  SpeedHookReturn = function + sizeof(prologue);

  BYTE jump[8] = { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90 };
  *(int *)(jump + 1) = (int)(trampoline - (function + 5));
  writeCode(function, jump, sizeof(jump));
  log_print("[PATCH] Sprint slowdown fixed, movement function at: 0x%llx", (DWORD64)function);
}

void applyBorderless(HWND hWnd) {
  static BOOL applying = FALSE;
  if (!hWnd || applying)
    return;
  applying = TRUE;
  if (configFile.UseCustomScreenDimensions != 1) {
    final.right = GetSystemMetrics(SM_CXSCREEN);
    final.bottom = GetSystemMetrics(SM_CYSCREEN);
  } else {
    final.right = configFile.ScreenWidth;
    final.bottom = configFile.ScreenHeight;
  }
  final.left = 0;
  final.top = 0;
  SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
  AdjustWindowRect(&final, GetWindowLong(hWnd, GWL_STYLE), FALSE);
  SetWindowLong(hWnd, GWL_EXSTYLE, (GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOPMOST));
  SetWindowPos(hWnd, NULL, final.left, final.top, final.right - final.left, final.bottom - final.top, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
  applying = FALSE;
}

// Archthrones resets the window style after borderless is applied
LONG (WINAPI *SetWindowLongW_)(HWND hWnd, int nIndex, LONG dwNewLong);
LONG WINAPI SetWindowLongHook(HWND hWnd, int nIndex, LONG dwNewLong) {
  BOOL gameWindow = hWnd == FindWindowA(NULL, "DARK SOULS III");
  if (gameWindow && nIndex == GWL_STYLE)
    dwNewLong = (dwNewLong & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU)) | WS_POPUP | WS_VISIBLE;
  LONG result = SetWindowLongW_(hWnd, nIndex, dwNewLong);
  if (gameWindow && (nIndex == GWL_STYLE || nIndex == GWL_EXSTYLE))
    applyBorderless(hWnd);
  return result;
}

void hookImport(const char *dll, const char *function, void *hook, void **original) {
  BYTE *base = (BYTE *)GetModuleHandleA(NULL);
  IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
  IMAGE_IMPORT_DESCRIPTOR *import = (IMAGE_IMPORT_DESCRIPTOR *)(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
  for (; import->Name; ++import) {
    if (_stricmp((char *)(base + import->Name), dll) != 0 || !import->OriginalFirstThunk)
      continue;
    IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(base + import->OriginalFirstThunk);
    IMAGE_THUNK_DATA *iat = (IMAGE_THUNK_DATA *)(base + import->FirstThunk);
    for (; names->u1.AddressOfData; ++names, ++iat) {
      if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal) || strcmp((char *)((IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData))->Name, function) != 0)
        continue;
      DWORD oldProtect;
      VirtualProtect(&iat->u1.Function, sizeof(iat->u1.Function), PAGE_READWRITE, &oldProtect);
      *original = (void *)iat->u1.Function;
      iat->u1.Function = (ULONG_PTR)hook;
      VirtualProtect(&iat->u1.Function, sizeof(iat->u1.Function), oldProtect, &oldProtect);
      log_print("[PATCH] Hooked %s!%s", dll, function);
      return;
    }
  }
  log_print("[PATCH] Import %s!%s not found.", dll, function);
}

void setFps(float rFPS) {
  log_print("[INFO] setFps called with rFPS = %f", rFPS);
  fpsCap = rFPS;
  HWND hWnd = FindWindowA(NULL, "DARK SOULS III");
  if (!hWnd) {
    hWnd = FindWindowA(NULL, "Dark Souls: Archthrones");
  }
  log_print("[INFO] FindWindowA returned HWND = %p", hWnd);

  // Borderless window mode (if enabled)
  if (configFile.EnableBorderless) {
    log_print("[INFO] Applying borderless window mode");
    applyBorderless(hWnd);
    log_print("[INFO] Borderless window mode applied successfully");
  }

  MODULEINFO moduleInfo = {0};
  if (configFile.FixSprintSlowdown && rFPS > 0 &&
      GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(NULL), &moduleInfo, sizeof(moduleInfo)))
    fixSprintSlowdown(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);

  CloseHandle(CreateThread(NULL, 0, patchFps, NULL, 0, NULL));

  if (configFile.EnableCursorClip != 0) {
    HANDLE thread = CreateThread(NULL, 0, (void *)containCursor, hWnd, 0, NULL);
    log_print("[INFO] Cursor clip thread created.");
  }
}

HINSTANCE BaseAddress, BaseAddressGenuine;
wchar_t *BaseFileName, FullFilePath[512];
BOOL WINAPI DllMain(HINSTANCE baseaddr, DWORD reason, BOOL isstatic) {
  switch (reason) {
  case DLL_PROCESS_ATTACH:
    BaseFileName = FullFilePath + GetModuleFileNameW(baseaddr, FullFilePath, _countof(FullFilePath));
    while (BaseFileName-- > FullFilePath)
      if (*BaseFileName == L'\\')
        break;
    readFile();
    log_init();
    if (configFile.EnableBorderless)
      hookImport("USER32.dll", "SetWindowLongW", (void *)SetWindowLongHook, (void **)&SetWindowLongW_);
  case DLL_THREAD_ATTACH:
    break;
  }
  return 1;
}

importD3D(FARPROC D3DAssemble_, DebugSetMute_, D3DCompile_, D3DCompressShaders_, D3DCreateBlob_, D3DDecompressShaders_, D3DDisassemble_, D3DDisassemble10Effect_, D3DGetBlobPart_, D3DGetDebugInfo_, D3DGetInputAndOutputSignatureBlob_, D3DGetInputSignatureBlob_,
          D3DGetOutputSignatureBlob_, D3DPreprocess_, D3DReflect_, D3DReturnFailure1_, D3DStripShader_) void LoadGenuineDll() {
  if (!BaseAddressGenuine) {
    static wchar_t filename[512];
    GetSystemDirectoryW(filename, _countof(filename));
    BaseAddressGenuine = LoadLibraryW(wcscat(filename, BaseFileName));
    IMPORT(D3DAssemble);
    IMPORT(DebugSetMute);
    IMPORT(D3DCompile);
    IMPORT(D3DCompressShaders);
    IMPORT(D3DCreateBlob);
    IMPORT(D3DDecompressShaders);
    IMPORT(D3DDisassemble);
    IMPORT(D3DDisassemble10Effect);
    IMPORT(D3DGetBlobPart);
    IMPORT(D3DGetDebugInfo);
    IMPORT(D3DGetInputAndOutputSignatureBlob);
    IMPORT(D3DGetInputSignatureBlob);
    IMPORT(D3DGetOutputSignatureBlob);
    IMPORT(D3DPreprocess);
    IMPORT(D3DReflect);
    IMPORT(D3DReturnFailure1);
    IMPORT(D3DStripShader);
    setFps(readFile());
  }
}

// Dummy atexit implementation for -nostartfiles builds
int atexit(void (*func)(void)) { return 0; }
