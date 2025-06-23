#include "dllmain.h"
#include "../../external/inih/ini.c"
#include <stdio.h>
#include <stdarg.h>
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
} config;
config configFile;

void containCursor(void *args) {
  while (TRUE) {
    if (GetAsyncKeyState(configFile.CursorClipHotkey)) {
      if (GetForegroundWindow() == args)
        SetCapture(args);
      ClipCursor(&final);
    }
  }
}

static int handler(void *Settings, const char *section, const char *name, const char *value) {
  config *modconfig = (config *)Settings;
  if (MATCH("Settings", "fps")) {
    modconfig->fps = atof(value);
  } 
  if (MATCH("Settings", "UseCustomScreenDimensions")) {
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
  } else
    return 0;
  return 1;
}

float readFile() {
  // Initialize config with default values
  configFile.EnableLogging = 0;     // Default logging off
  configFile.fps = 1000;            // Default FPS limit
  configFile.EnableBorderless = 1;  // Default borderless enabled
  
  // Parse INI file, which will override defaults if present
  if (ini_parse("FPSconfig.ini", handler, &configFile) < 0) {
    return configFile.fps; // Return default if file is missing
  }
  return configFile.fps;
}

FILE* logFile = NULL;

void log_init() {
    if (!configFile.EnableLogging) return;
    logFile = fopen("DS3DebugFPS_log.txt", "a");
    if (logFile) {
        fprintf(logFile, "[INFO] Log started\n");
        fflush(logFile);
    }
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

void setFps(float rFPS) {
  log_print("[INFO] setFps called with rFPS = %f", rFPS);
  // Find Process
  DWORD PID;
  HWND hWnd = FindWindowA(NULL, "DARK SOULS III");
  log_print("[INFO] FindWindowA returned HWND = %p", hWnd);
  GetWindowThreadProcessId(hWnd, &PID);
  log_print("[INFO] GetWindowThreadProcessId returned PID = %lu", PID);
  HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
  log_print("[INFO] OpenProcess returned HANDLE = %p", pHandle);

  // Skip Intros 
  unsigned char SkipIntro[20] = {0x48, 0x31, 0xC0, 0x48, 0x89, 0x02, 0x49, 0x89, 0x04, 0x24, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
  NtWriteVirtualMemory(pHandle, (LPVOID)0x140BF66AE, SkipIntro, 20, 0);

  // Borderless window mode (if enabled)
  if (configFile.EnableBorderless) {
    log_print("[INFO] Applying borderless window mode");
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
    MoveWindow(hWnd, final.left, final.top, final.right - final.left, final.bottom - final.top, TRUE);
    log_print("[INFO] Borderless window mode applied successfully");
  }

  // --- ORIGINAL CODE WITH HARDCODED OFFSET ---
  // HMODULE hModule = GetModuleHandleA("darksoulsiii.exe");
  // log_print("[ORIG] Module base address: 0x%p", hModule);
  // DWORD64 origSprjFlipper = (DWORD64)hModule + 0x489DD10;
  // log_print("[ORIG] SprjFlipper (base + offset): 0x%llx", origSprjFlipper);

  // DWORD64 SprjFlipper = origSprjFlipper;
  // NtReadVirtualMemory(pHandle, (LPVOID)SprjFlipper, &SprjFlipper, sizeof(SprjFlipper), NULL);
  // log_print("[ORIG] NtReadVirtualMemory for SprjFlipper done.");
  // NtWriteVirtualMemory(pHandle, (LPVOID)(SprjFlipper + 0x354), &rFPS, sizeof(DWORD), NULL);
  // log_print("[ORIG] NtWriteVirtualMemory for rFPS.");
  // NtWriteVirtualMemory(pHandle, (LPVOID)(SprjFlipper + 0x358), &useDebug, sizeof(char), NULL);
  // log_print("[ORIG] NtWriteVirtualMemory for useDebug.");

  // --- NEW PATTERN SCAN APPROACH (single scan for the two known matching patterns) ---
  HMODULE hModule = GetModuleHandleA("darksoulsiii.exe");
  log_print("[SCAN] Module base address: 0x%p", hModule);

  BYTE sprjFlipperPattern1[] = { 0x50, 0x60, 0x3E, 0x08, 0xF4, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  BYTE sprjFlipperPattern2[] = { 0x50, 0x60, 0x3E, 0x08, 0xF3, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  const char* sprjFlipperMask = "xx??xxxxxxxxxxxx";

  HMODULE scanModule = GetModuleHandleA("darksoulsiii.exe");
  MODULEINFO moduleInfo;
  if (GetModuleInformation(pHandle, scanModule, &moduleInfo, sizeof(moduleInfo))) {
    BYTE* moduleBase = (BYTE*)moduleInfo.lpBaseOfDll;
    DWORD moduleSize = moduleInfo.SizeOfImage;
    size_t patternLen = strlen(sprjFlipperMask);
    int foundCount = 0;
    for (DWORD i = 0; i <= moduleSize - patternLen; ++i) {
      BOOL found1 = TRUE, found2 = TRUE;
      for (size_t j = 0; j < patternLen; ++j) {
        if (sprjFlipperMask[j] == 'x') {
          if (sprjFlipperPattern1[j] != moduleBase[i + j]) found1 = FALSE;
          if (sprjFlipperPattern2[j] != moduleBase[i + j]) found2 = FALSE;
        }
      }
      if (found1 || found2) {
        foundCount++;
        DWORD64 matchAddr = (DWORD64)(moduleBase + i);
        log_print("[SCAN] Pattern %s match at: 0x%llx", found1 ? "F4" : "F3", matchAddr);
        DWORD64 realSprjFlipper = 0;
        NtReadVirtualMemory(pHandle, (LPVOID)matchAddr, &realSprjFlipper, sizeof(realSprjFlipper), NULL);
        log_print("[PATCH] Read pointer at match: 0x%llx", realSprjFlipper);
        // Write the FPS value at the pointer + 0x354
        NtWriteVirtualMemory(pHandle, (LPVOID)(realSprjFlipper + 0x354), &rFPS, sizeof(DWORD), NULL);
        log_print("[PATCH] Patched rFPS at: 0x%llx", realSprjFlipper + 0x354);
        // Write the useDebug value at the pointer + 0x358
        NtWriteVirtualMemory(pHandle, (LPVOID)(realSprjFlipper + 0x358), &useDebug, sizeof(char), NULL);
        log_print("[PATCH] Patched useDebug at: 0x%llx", realSprjFlipper + 0x358);
        break;
      }
    }
    if (foundCount == 0) {
      log_print("[SCAN] No matching patterns found in module.");
    } else {
      log_print("[SCAN] Successfully patched FPS and useDebug values.");
    }
  } else {
    log_print("[SCAN] Failed to get module information for scan.");
  }

  if (configFile.EnableCursorClip != 0) {
    HANDLE thread = CreateThread(NULL, 0, (void *)containCursor, &hWnd, 0, NULL);
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