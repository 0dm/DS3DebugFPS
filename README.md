# DS3DebugFPS
Unlock the frame rate in Dark Souls III. 

# Installation
Move D3DCompiler_43.dll and FPSconfig.ini into your game directory.

The executable version will only work when the game is open.

Edit FPSconfig.ini to your liking.
Preview:
```ini
; Config file for ds3DebugFps

[Settings]
fps = 144.0	              ; Fps Cap

UseCustomScreenDimensions = 0 ; Set to 1 to change how the game is stretched, by default it will stretch the game to fit screen (optimal)

; UseCustomScreenDimensions must be enabled for these
ScreenWidth = 1920           
ScreenHeight = 1080

EnableCursorClip = 0       ; Set to 1 to enable the cursor clip hotkey, used to keep the mouse locked in the window
CursorClipHotkey = 0x7A    ; default: f11 (0x7A). You can still alt tab out of the window. This value is in hex.
; Hotkey list can be found here: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

```
**If using the executable, you must place FPSconfig.ini in the same directory as ds3DebugFps.exe.**

# Is it safe to use online?
Yes, there's a very low chance that this bans.

2020/8/1: Users have reported that the mod is safe to use online without penalization, but please use it at your own discretion!

# Borderless
This will not work in full screen mode, you will need to be in windowed mode (manually set it in game). This DLL will attempt to fullscreen the game in borderless window mode - it will fit screen too!

# Cursor Clip - DLL ONLY
You can now set a hotkey to clip the cursor to the window, this *should* work for those with multiple monitors. 

# Compatibility
You can use the new executable with PyreProtecc however you will be missing the cursor clip feature.
This mod should work would PvP Watchdog.

# Other Info
This mod will sometimes get small updates on Github, but full releases will only occur when functionality changes drastically.

Support the mod on [Nexus Mods](https://www.nexusmods.com/darksouls3/mods/614?tab=description)


| Credits  |                                                                       | 
|----------|-----------------------------------------------------------------------|
| inuNorii    | Debug Menu Pointers in the public Cheat Table                      | 
| Pyre        | DLL wrapper, could not have done without PyreProtecc's source code.|
| bladecoding | Remove intro screens memory region                                 |
