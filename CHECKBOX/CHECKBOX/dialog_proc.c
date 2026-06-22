#include "dialog_proc.h"
#include "resource.h"

// ============================================================================
// Helper function to get the current state of a check box
static BOOL IsChecked(HWND hwnd, int controlId)
{
    return IsDlgButtonChecked(hwnd, controlId) == BST_CHECKED;
}

// Helper function to set check state
static void SetChecked(HWND hwnd, int controlId, BOOL checked)
{
    CheckDlgButton(hwnd, controlId, checked ? BST_CHECKED : BST_UNCHECKED);
}

// ============================================================================
// Dialog procedure
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Optional: set initial states
        SetChecked(hwnd, CHK_OPTION1, TRUE);   // Enable Logging is ON by default
        SetChecked(hwnd, CHK_OPTION2, FALSE);
        SetChecked(hwnd, CHK_OPTION3, FALSE);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
            case IDCANCEL:
                EndDialog(hwnd, FALSE);
                return TRUE;

            case BTN_SHOW:
            {
                // Build a message showing which options are selected
                wchar_t msg[256] = L"Selected options:\r\n";

                if (IsChecked(hwnd, CHK_OPTION1))
                    wcscat_s(msg, sizeof(msg) / sizeof(wchar_t), L"- Enable Logging\r\n");
                if (IsChecked(hwnd, CHK_OPTION2))
                    wcscat_s(msg, sizeof(msg) / sizeof(wchar_t), L"- Auto Save\r\n");
                if (IsChecked(hwnd, CHK_OPTION3))
                    wcscat_s(msg, sizeof(msg) / sizeof(wchar_t), L"- Show Confirmation\r\n");

                MessageBoxW(hwnd, msg, L"Current Selections", MB_OK);
                return TRUE;
            }

            case BTN_CHECK_ALL:
                SetChecked(hwnd, CHK_OPTION1, TRUE);
                SetChecked(hwnd, CHK_OPTION2, TRUE);
                SetChecked(hwnd, CHK_OPTION3, TRUE);
                return TRUE;

            case BTN_UNCHECK_ALL:
                SetChecked(hwnd, CHK_OPTION1, FALSE);
                SetChecked(hwnd, CHK_OPTION2, FALSE);
                SetChecked(hwnd, CHK_OPTION3, FALSE);
                return TRUE;

            // Handle individual check box clicks (optional)
            case CHK_OPTION1:
            case CHK_OPTION2:
            case CHK_OPTION3:
            {
                // You can react to a checkbox being toggled
                if (LOWORD(wParam) == CHK_OPTION1)
                {
                    BOOL bNowChecked = IsChecked(hwnd, CHK_OPTION1);
                    if (bNowChecked)
                        MessageBoxW(hwnd, L"Logging enabled", L"Info", MB_OK);
                    else
                        MessageBoxW(hwnd, L"Logging disabled", L"Info", MB_OK);
                }
                if (LOWORD(wParam) == CHK_OPTION2)
                {
                    BOOL bNowChecked = IsChecked(hwnd, CHK_OPTION2);
                    if (bNowChecked)
                        MessageBoxW(hwnd, L"Auto Save is enabled", L"Info", MB_OK);
                    else
                        MessageBoxW(hwnd, L"Auto Save is disabled", L"Info", MB_OK);
                }
                if (LOWORD(wParam) == CHK_OPTION3)
                {
                    BOOL bNowChecked = IsChecked(hwnd, CHK_OPTION3);
                    if (bNowChecked)
                        MessageBoxW(hwnd, L"Show Confirmation is enabled", L"Info", MB_OK);
                    else
                        MessageBoxW(hwnd, L"Show Confirmation is disabled", L"Info", MB_OK);
                }
                return TRUE;
            }
        }
        break;
    }
    return FALSE;
}