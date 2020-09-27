#ifndef _dllmain_h   
#define _dllmain_h
#define importD3D(...) __VA_ARGS__;
#define IMPORT(a) a##_=GetProcAddress(BaseAddressGenuine,#a)
#define MATCH(s,n) strcmp(section, s) == 0 && strcmp(name, n) == 0
#include <windows.h>

char useDebug = TRUE;
DWORD64 SprjFlipper = 0x144888440;
RECT final;

void setFps(float rFPS);
float readFile();
DWORD WINAPI containCursor(void* args);
#endif