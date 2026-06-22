#include "dialog_proc.h"
#include "resource.h"

// Dialog procedure – handles all messages for the dialog window
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_HELP_ABOUT:
            MessageBoxW(hwnd, L"ABOUT ABOUT ABOUT", L"Button Demo", MB_OK);
            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            return TRUE;

        }
        break;
    }
    return FALSE;
}