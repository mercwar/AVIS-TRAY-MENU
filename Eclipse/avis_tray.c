// ======================================================
// ECLIPSE TRAY MENU (Formerly AVIS Tray)
// File Name: avis_tray.c
// Pure JSON-Driven Tray Launcher with Live Menu Reload & Graceful Engine Teardown
// ======================================================

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"
#include "resource.h"
#include "avis_tray.h"
#include "system_loader.h"

#define AVIS_WM_TRAYICON (WM_USER + 100)
#define AVIS_RELOAD_ID   (AVIS_FIRST_ID + 999)

typedef void (*pfnRunHeadlessShutdown)(void);

AVIS_MENU_ITEM g_items[AVIS_MAX_ITEMS];
int g_item_count = 0;

static HMENU g_menu = NULL;
static NOTIFYICONDATAA g_nid;
static HMODULE g_hHeadlessDll = NULL;

static void avis_parse_items(cJSON* items, HMENU parentMenu);

static char* avis_load_file(const char* filename) {
    FILE* fp = NULL;
    fopen_s(&fp, filename, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char* buffer = (char*)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    fread(buffer, 1, (size_t)size, fp);
    buffer[size] = 0;
    fclose(fp);
    return buffer;
}

int avis_load_json_menu(const char* filename) {
    char* json_text = avis_load_file(filename);
    if (!json_text) return 0;

    cJSON* root = cJSON_Parse(json_text);
    free(json_text);
    if (!root) return 0;

    cJSON* items = cJSON_GetObjectItem(root, "items");
    if (!cJSON_IsArray(items)) {
        cJSON_Delete(root);
        return 0;
    }

    g_item_count = 0;
    if (g_menu) DestroyMenu(g_menu);
    g_menu = CreatePopupMenu();

    avis_parse_items(items, g_menu);

    // Append Reload and Exit options
    AppendMenuA(g_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(g_menu, MF_STRING, AVIS_RELOAD_ID, "Reload Menu");
    AppendMenuA(g_menu, MF_STRING, AVIS_EXIT_ID, "Exit");

    cJSON_Delete(root);
    return 1;
}

void avis_run_command(const char *command) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmdline[1024];

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    strcpy(cmdline, command);

    if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

LRESULT CALLBACK AvisWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            UINT id = LOWORD(wParam);
            if (id == AVIS_EXIT_ID) {
                Shell_NotifyIconA(NIM_DELETE, &g_nid);
                PostQuitMessage(0);
                return 0;
            }
            if (id == AVIS_RELOAD_ID) {
                avis_load_json_menu("menu.json");
                return 0;
            }
            int index = id - AVIS_FIRST_ID;
            if (index >= 0 && index < g_item_count) {
                avis_run_command(g_items[index].command);
            }
            return 0;
        }

        case AVIS_WM_TRAYICON: {
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);

                TrackPopupMenu(
                    g_menu,
                    TPM_LEFTALIGN | TPM_BOTTOMALIGN,
                    pt.x,
                    pt.y,
                    0,
                    hwnd,
                    NULL
                );

                PostMessage(hwnd, WM_NULL, 0, 0);
            }
            return 0;
        }

        case WM_DESTROY: {
            Shell_NotifyIconA(NIM_DELETE, &g_nid);
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void avis_parse_items(cJSON* items, HMENU parentMenu) {
    cJSON* item;
    cJSON_ArrayForEach(item, items) {
        cJSON* text = cJSON_GetObjectItem(item, "text");
        cJSON* cmd  = cJSON_GetObjectItem(item, "command");
        cJSON* submenu = cJSON_GetObjectItem(item, "submenu");

        if (cJSON_IsArray(submenu)) {
            HMENU subMenu = CreatePopupMenu();
            avis_parse_items(submenu, subMenu);
            AppendMenuA(parentMenu, MF_POPUP, (UINT_PTR)subMenu, text->valuestring);
        } else if (cJSON_IsString(text) && cJSON_IsString(cmd)) {
            if (g_item_count < AVIS_MAX_ITEMS) {
                strcpy(g_items[g_item_count].text, text->valuestring);
                strcpy(g_items[g_item_count].command, cmd->valuestring);
                AppendMenuA(parentMenu, MF_STRING, AVIS_FIRST_ID + g_item_count, g_items[g_item_count].text);
                g_item_count++;
            }
        }
    }
}

void avis_set_working_directory(void) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);

    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
        SetCurrentDirectoryA(path);
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    avis_set_working_directory();

    // Ensure system loader initializes the engine DLL in background memory
    g_hHeadlessDll = LoadSystemDarkComHeadless();

    int headless = 0;
    if (lpCmdLine && (strstr(lpCmdLine, "--headless") != NULL || strstr(lpCmdLine, "/headless") != NULL)) {
        headless = 1;
    }

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = AvisWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "AVIS_TRAY_CLASS";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA(
        "AVIS_TRAY_CLASS", 
        "Eclipse Tray",
        WS_OVERLAPPEDWINDOW, 
        0, 0, 0, 0,
        headless ? HWND_MESSAGE : NULL, 
        NULL, 
        hInst, 
        NULL
    );

    avis_load_json_menu("menu.json");

    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = AVIS_WM_TRAYICON;
    
    HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP_ICON));
    if (!hIcon) {
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    g_nid.hIcon = hIcon;
    
    strcpy(g_nid.szTip, headless ? "Eclipse Tray (Headless)" : "Eclipse Tray Menu");

    Shell_NotifyIconA(NIM_ADD, &g_nid);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Gracefully call DLL engine shutdown before freeing library handle
    if (g_hHeadlessDll) {
        pfnRunHeadlessShutdown RunHeadlessShutdown = (pfnRunHeadlessShutdown)GetProcAddress(g_hHeadlessDll, "RunHeadlessShutdown");
        if (RunHeadlessShutdown) {
            RunHeadlessShutdown();
        }
        FreeLibrary(g_hHeadlessDll);
    }

    return 0;
}