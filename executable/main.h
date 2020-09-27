#ifndef _main_h   
#define _main_h
#define MATCH(s,n) strcmp(section, s) == 0 && strcmp(name, n) == 0
#include <stdio.h>
#include <windows.h>

char useDebug = TRUE;
DWORD64 SprjFlipper = 0x144888440;
RECT final;

void setFps(float rFPS);
float readFile();
#endif