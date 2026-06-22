#include "statusbar.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

HWND CreateStatusBar(HWND hwndParent, HINSTANCE hInst)
{
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    if (!InitCommonControlsEx(&icex))
        return NULL;

    HWND hwndStatus = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE,  // без SBARS_SIZEGRIP
        0, 0, 0, 0,
        hwndParent,
        (HMENU)0,
        hInst,
        NULL
    );

    if (hwndStatus)
    {
        // Настройка частей (например, 3)
        int parts[] = { 150, 300, -1 };
        SendMessage(hwndStatus, SB_SETPARTS, 3, (LPARAM)parts);
        // Начальный текст
        SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)L"Ready");
    }
    return hwndStatus;
}

void UpdateStatusBarText(HWND hwndStatusBar, int partIndex, const wchar_t* text)
{
    if (hwndStatusBar)
        SendMessage(hwndStatusBar, SB_SETTEXT, partIndex, (LPARAM)text);
}

void OnStatusBarSize(HWND hwndStatusBar)
{
    if (hwndStatusBar)
        SendMessage(hwndStatusBar, WM_SIZE, 0, 0);
}