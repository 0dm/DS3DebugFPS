#include "main.h"
#include "lib/inih/ini.h"

typedef struct {
    float fps;
    int UseCustomScreenDimensions;
    int ScreenWidth;
    int ScreenHeight;
} config;
config configFile;


static int handler(void* Settings, const char* section, const char* name, const char* value) {
    config* modconfig = (config*)Settings;
    if (MATCH("Settings", "fps")) {
        modconfig->fps = atof(value);
    }
    if (MATCH("Settings", "UseCustomScreenDimensions")){
        modconfig->UseCustomScreenDimensions = atoi(value);
    }
    else if (MATCH("Settings", "ScreenWidth")){
        modconfig->ScreenWidth = atoi(value);
    }
    else if (MATCH("Settings", "ScreenHeight")){
        modconfig->ScreenHeight = atoi(value);
    }
     else 
        return 0; // error
    return 1;

}


float readFile() {   
    return ini_parse("FPSconfig.ini", handler, &configFile) < 0 ? 1000: configFile.fps; // will set framerate limit to 1000 if the ini file is missing
}

void setFps(float rFPS) {
   
    // Find Process
    DWORD PID;
    HWND hWnd = FindWindowA(NULL, "DARK SOULS III");
    GetWindowThreadProcessId(hWnd, &PID);
    HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);

    // Borderless
    if (configFile.UseCustomScreenDimensions != 1) {
        final.right = GetSystemMetrics(SM_CXSCREEN);
        final.bottom = GetSystemMetrics(SM_CYSCREEN);
    } 
    else {
        final.right = configFile.ScreenWidth;
        final.bottom = configFile.ScreenHeight;
    }
    final.left = 0;
    final.top = 0;
    SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    AdjustWindowRect(&final, GetWindowLong(hWnd, GWL_STYLE), FALSE);
    SetWindowLong(hWnd, GWL_EXSTYLE, (GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOPMOST));
    MoveWindow(hWnd, final.left, final.top, final.right - final.left, final.bottom - final.top, TRUE);

    // GRAPHICS -> GFX
    ReadProcessMemory(pHandle, (LPVOID)SprjFlipper, &SprjFlipper, sizeof(SprjFlipper), NULL);
    WriteProcessMemory(pHandle, (LPVOID)(SprjFlipper + 0x354), &rFPS, sizeof(DWORD), NULL); // Debug FPS | デバッグFPS
    WriteProcessMemory(pHandle, (LPVOID)(SprjFlipper + 0x358), &useDebug, sizeof(char), NULL); // Use Debug FPS | デバッグFPSを利用するか
}

int main() {
    setFps(readFile());
}