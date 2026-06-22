#include "dialog_proc.h"
#include "resource.h"
#include "statusbar.h"   // подключаем модуль статус-бара

static HWND g_hwndStatusBar = NULL;

INT_PTR CALLBACK MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        g_hwndStatusBar = CreateStatusBar(hwnd, hInst);
        return TRUE;
    }

    case WM_SIZE:
        OnStatusBarSize(g_hwndStatusBar);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            return TRUE;

        case BTN_UPDATE_STATUS:
            if (g_hwndStatusBar)
            {
                static int counter = 0;
                counter++;
                wchar_t buf[128];
                wsprintfW(buf, L"Updated #%d", counter);
                UpdateStatusBarText(g_hwndStatusBar, 0, buf);
                UpdateStatusBarText(g_hwndStatusBar, 1, L"Part 2");
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}