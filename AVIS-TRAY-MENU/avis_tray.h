#ifndef AVIS_TRAY_H
#define AVIS_TRAY_H

#include <windows.h>

#define AVIS_WM_TRAYICON (WM_USER + 100)
#define AVIS_MAX_ITEMS 64
#define AVIS_FIRST_ID 1000
#define AVIS_EXIT_ID 9999

typedef struct
{
    char text[128];
    char command[512];
}
AVIS_MENU_ITEM;

extern AVIS_MENU_ITEM g_items[AVIS_MAX_ITEMS];
extern int g_item_count;

int avis_load_menu(const char *filename);

void avis_build_menu(void);

void avis_run_command(const char *cmd);
static void avis_set_working_directory(void);
LRESULT CALLBACK AvisWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

#endif