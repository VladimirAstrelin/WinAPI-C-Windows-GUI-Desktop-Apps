#include "dialog_proc.h"
#include "resource.h"

// Helper: get the ID of the currently selected radio button in a group
// Assumes IDs are consecutive: firstId, firstId+1, ..., lastId
static int GetSelectedRadioButton(HWND hwnd, int firstId, int lastId)
{
    for (int id = firstId; id <= lastId; id++)
    {
        if (IsDlgButtonChecked(hwnd, id) == BST_CHECKED)
            return id;
    }
    return -1;  // none selected (should not happen if one is always checked)
}

// Helper: update the static text to show current color choice
static void UpdateSelectionDisplay(HWND hwnd, int firstId, int lastId)
{
    int selectedId = GetSelectedRadioButton(hwnd, firstId, lastId);
    LPCWSTR colorName;

    switch (selectedId)
    {
        case RADIO_RED:   colorName = L"Red"; break;
        case RADIO_GREEN: colorName = L"Green"; break;
        case RADIO_BLUE:  colorName = L"Blue"; break;
        default:          colorName = L"(none)"; break;
    }
	// ОБНОВЛЯЕМ ТЕКСТ В STATIC CONTROL:
    wchar_t buffer[128];
    wsprintfW(buffer, L"Selected color: %s", colorName);
    SetDlgItemTextW(hwnd, IDC_SELECTION_TEXT, buffer);
}

// Dialog procedure for radio button demo
INT_PTR CALLBACK RadioDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // Set default selection: Red (first radio button)
            CheckRadioButton(hwnd, RADIO_RED, RADIO_BLUE, RADIO_RED);
            // Update the display text
            UpdateSelectionDisplay(hwnd, RADIO_RED, RADIO_BLUE);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwnd, FALSE);
                    return TRUE;

                case IDC_SHOW:
                {
                    // When "Show Selection" button is clicked, just show a message box
                    int selected = GetSelectedRadioButton(hwnd, RADIO_RED, RADIO_BLUE);
                    LPCWSTR msg;
                    switch (selected)
                    {
                        case RADIO_RED:   msg = L"You selected Red"; break;
                        case RADIO_GREEN: msg = L"You selected Green"; break;
                        case RADIO_BLUE:  msg = L"You selected Blue"; break;
                        default:          msg = L"No color selected (error)"; break;
                    }
                    MessageBoxW(hwnd, msg, L"Radio Demo", MB_OK);
                    return TRUE;
                }

                // Handle clicks on any radio button in the group (optional)
                case RADIO_RED:
                case RADIO_GREEN:
                case RADIO_BLUE:
                {
                    // Update the display text immediately when user clicks
                    UpdateSelectionDisplay(hwnd, RADIO_RED, RADIO_BLUE);
                    return TRUE;
                }
            }
            break;
        }
    }
    return FALSE;
}