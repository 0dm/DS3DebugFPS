#include "main.h"
#include "../../external/inih/ini.c"
#include <psapi.h>
#include <math.h>

typedef struct {
  float fps;
  int UseCustomScreenDimensions;
  int ScreenWidth;
  int ScreenHeight;
  int FixSprintSlowdown;
} config;
config configFile;

HANDLE pHandle;
DWORD64 speedFactors;
float fpsCap;

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
  } else if (MATCH("Settings", "FixSprintSlowdown")) {
    modconfig->FixSprintSlowdown = atoi(value);
  } else
    return 0;
  return 1;
}

// will set framerate limit to 1000 if the ini file is missing
float readFile() {
  configFile.FixSprintSlowdown = 1;
  return ini_parse("FPSconfig.ini", handler, &configFile) < 0 ? 1000 : configFile.fps;
}

DWORD64 getModuleBase(HANDLE process, const char *name, DWORD *size) {
  HMODULE modules[1024];
  DWORD needed = 0;
  if (!EnumProcessModules(process, modules, sizeof(modules), &needed))
    return 0;
  for (DWORD i = 0; i < needed / sizeof(HMODULE); ++i) {
    char modName[MAX_PATH];
    MODULEINFO info;
    if (!GetModuleBaseNameA(process, modules[i], modName, sizeof(modName)) || _stricmp(modName, name))
      continue;
    if (size && GetModuleInformation(process, modules[i], &info, sizeof(info)))
      *size = info.SizeOfImage;
    return (DWORD64)modules[i];
  }
  return 0;
}

DWORD64 remoteScan(HANDLE process, DWORD64 start, DWORD64 end, const BYTE *pattern, const char *mask) {
  size_t patternLen = strlen(mask);
  BYTE buf[0x10000 + 64];
  MEMORY_BASIC_INFORMATION mbi;
  for (DWORD64 region = start; region < end && VirtualQueryEx(process, (LPCVOID)region, &mbi, sizeof(mbi));) {
    DWORD64 regionEnd = (DWORD64)mbi.BaseAddress + mbi.RegionSize;
    if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
      DWORD64 from = region > (DWORD64)mbi.BaseAddress ? region : (DWORD64)mbi.BaseAddress;
      DWORD64 to = regionEnd < end ? regionEnd : end;
      for (DWORD64 addr = from; addr + patternLen <= to;) {
        size_t chunk = sizeof(buf);
        if (addr + chunk > to)
          chunk = (size_t)(to - addr);
        if (NtReadVirtualMemory(process, (LPVOID)addr, buf, chunk, NULL) == 0) {
          for (size_t i = 0; i + patternLen <= chunk; ++i) {
            size_t j = 0;
            while (j < patternLen && (mask[j] != 'x' || buf[i + j] == pattern[j]))
              ++j;
            if (j == patternLen)
              return addr + i;
          }
        }
        if (chunk <= patternLen)
          break;
        addr += chunk - patternLen + 1;
      }
    }
    if (regionEnd <= region)
      break;
    region = regionEnd;
  }
  return 0;
}

void remoteWrite(HANDLE process, DWORD64 address, const void *bytes, size_t size) {
  DWORD oldProtect;
  VirtualProtectEx(process, (LPVOID)address, (SIZE_T)size, PAGE_EXECUTE_READWRITE, &oldProtect);
  NtWriteVirtualMemory(process, (LPVOID)address, (PVOID)bytes, size, NULL);
  VirtualProtectEx(process, (LPVOID)address, (SIZE_T)size, oldProtect, &oldProtect);
}

void remoteRedirect(HANDLE process, DWORD64 disp, DWORD64 target) {
  int rel = (int)(target - (disp + 4));
  remoteWrite(process, disp, &rel, sizeof(rel));
}

void writeSpeedFactors(float frameTime) {
  float factors[3];
  float slack = fpsCap >= 90.0f ? 1.2f : 1.0f;
  float recovery = fpsCap >= 90.0f ? 1.5f : 1.2f;
  if (frameTime < 1.0f / 480.0f)
    frameTime = 1.0f / 480.0f;
  if (frameTime > 0.05f)
    frameTime = 0.05f;
  factors[0] = 0.5f / frameTime * slack;
  factors[1] = expf(logf(0.8f) * frameTime * 60.0f);
  factors[2] = expf(logf(recovery) * frameTime * 60.0f);
  NtWriteVirtualMemory(pHandle, (LPVOID)speedFactors, factors, sizeof(factors), NULL);
}

void updateSpeedFactors() {
  float frameTime = 1.0f / fpsCap;
  if (SprjFlipper) {
    float measured = 0;
    NtReadVirtualMemory(pHandle, (LPVOID)(SprjFlipper + 0x26C), &measured, sizeof(measured), NULL);
    if (measured >= 1.0f / 480.0f && measured <= 0.05f)
      frameTime = measured;
  }
  writeSpeedFactors(frameTime);
}

void fixSprintSlowdown(DWORD64 moduleBase, DWORD moduleSize) {
  BYTE speedPattern[] = { 0xF3, 0x0F, 0x58, 0x00, 0x0F, 0xC6, 0x00, 0x00, 0x0F, 0x51, 0x00, 0xF3, 0x0F, 0x59, 0x05, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x2F };
  DWORD64 hit = remoteScan(pHandle, moduleBase, moduleBase + moduleSize, speedPattern, "xxx?xx?xxx?xxxx????xx");
  if (!hit)
    return;

  DWORD64 page = 0;
  for (DWORD64 hint = (moduleBase + moduleSize + 0xFFFF) & ~0xFFFFULL; !page && hint < moduleBase + 0x70000000; hint += 0x10000)
    page = (DWORD64)VirtualAllocEx(pHandle, (LPVOID)hint, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!page)
    return;

  speedFactors = page;
  writeSpeedFactors(1.0f / fpsCap);
  remoteRedirect(pHandle, hit + 15, speedFactors);

  BYTE check[8];
  if (NtReadVirtualMemory(pHandle, (LPVOID)(hit + 0x29), check, 4, NULL) == 0 && !memcmp(check, "\xF3\x0F\x59\x05", 4) &&
      NtReadVirtualMemory(pHandle, (LPVOID)(hit + 0x54), check, 4, NULL) == 0 && !memcmp(check, "\xF3\x0F\x59\x05", 4)) {
    remoteRedirect(pHandle, hit + 0x2D, speedFactors + 4);
    remoteRedirect(pHandle, hit + 0x58, speedFactors + 8);
  }
}

void setFps(float rFPS) {
  fpsCap = rFPS;

  // Find Process
  DWORD PID;
  HWND hWnd = FindWindowA(NULL, "DARK SOULS III");
  if (!hWnd) {
    hWnd = FindWindowA(NULL, "Dark Souls: Archthrones");
  }
  GetWindowThreadProcessId(hWnd, &PID);
  pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);

  // Borderless
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

  DWORD moduleSize = 0;
  DWORD64 moduleBase = getModuleBase(pHandle, "DarkSoulsIII.exe", &moduleSize);
  if (!moduleBase)
    return;

  for (int i = 0; i < 40 && !SprjFlipper; ++i) {
    DWORD64 slot = moduleBase + 0x489DD10;
    // GRAPHICS -> GFX
    NtReadVirtualMemory(pHandle, (LPVOID)slot, &SprjFlipper, sizeof(SprjFlipper), NULL);
    if (!SprjFlipper)
      Sleep(250);
  }
  if (!SprjFlipper)
    return;

  // Debug FPS | デバッグFPS
  NtWriteVirtualMemory(pHandle, (LPVOID)(SprjFlipper + 0x354), &rFPS, sizeof(DWORD), NULL);

  // Use Debug FPS | デバッグFPSを利用するか
  NtWriteVirtualMemory(pHandle, (LPVOID)(SprjFlipper + 0x358), &useDebug, sizeof(char), NULL);

  if (configFile.FixSprintSlowdown && rFPS > 0)
    fixSprintSlowdown(moduleBase, moduleSize);
}

int main() {
  setFps(readFile());
  if (!speedFactors)
    return 0;
  while (FindWindowA(NULL, "DARK SOULS III") || FindWindowA(NULL, "Dark Souls: Archthrones")) {
    updateSpeedFactors();
    Sleep(16);
  }
  return 0;
}
