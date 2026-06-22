// ================================================================
// dialog_proc.c – демонстрация работы с ComboBox и ListBox.
// Все контролы созданы в редакторе ресурсов.
// В коде мы управляем содержимым списков и обрабатываем выбор.
// ================================================================

#include "dialog_proc.h"
#include "resource.h"
#include <windows.h>

// Дескрипторы контролов (получаем один раз в WM_INITDIALOG)
static HWND g_hComboSelect = NULL;
static HWND g_hComboEdit = NULL;
static HWND g_hListBox = NULL;

// -----------------------------------------------------------------
// Вспомогательные функции для работы со списками
// -----------------------------------------------------------------

// Добавить элемент в ComboBox (в конец)
static void ComboBox_AddString(HWND hCombo, LPCWSTR text)
{
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)text);
}

// Добавить элемент в ListBox (в конец)
static void ListBox_AddString(HWND hList, LPCWSTR text)
{
    SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)text);
}

// Очистить ComboBox
static void ComboBox_Clear(HWND hCombo)
{
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
}

// Очистить ListBox
static void ListBox_Clear(HWND hList)
{
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
}

// Получить выбранный текст из ComboBox (если ничего не выбрано – возвращает пустую строку)
static void ComboBox_GetSelectedText(HWND hCombo, wchar_t* buffer, int bufferSize)
{
    int index = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (index != CB_ERR)
    {
        SendMessage(hCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
    }
    else
    {
        // Если ничего не выбрано, но поле редактируемое (DropDown), можно получить текст из поля ввода.
        // Но для простоты будем считать, что выбрано.
        // В случае DropDownList это вообще не произойдёт.
        wcscpy_s(buffer, bufferSize, L"");
    }
}

// Получить выбранный текст из ListBox
static void ListBox_GetSelectedText(HWND hList, wchar_t* buffer, int bufferSize)
{
    int index = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
    if (index != LB_ERR)
    {
        SendMessage(hList, LB_GETTEXT, index, (LPARAM)buffer);
    }
    else
    {
        wcscpy_s(buffer, bufferSize, L"");
    }
}

// -----------------------------------------------------------------
// Обработчики кнопок
// -----------------------------------------------------------------

// Добавить новый элемент (используем диалог ввода, но для простоты добавим предопределённый)
static void OnAddItem(HWND hwnd)
{
    // Запрашиваем у пользователя текст через диалог ввода (InputBox)
    // В WinAPI нет стандартного InputBox, поэтому используем простой способ:
    // предложим пользователю ввести текст, но для простоты добавим стандартный элемент с номером.
    // Можно было бы использовать диалог, но это выходит за рамки урока.
    // Мы добавим предопределённый текст с номером.
    static int counter = 1;
    wchar_t buffer[64];
    wsprintfW(buffer, L"Item %d", counter++);

    // Добавляем в оба ComboBox и в ListBox (для демонстрации)
    ComboBox_AddString(g_hComboSelect, buffer);
    ComboBox_AddString(g_hComboEdit, buffer);
    ListBox_AddString(g_hListBox, buffer);
}

// Удалить выбранный элемент из ListBox
static void OnRemoveItem(HWND hwnd)
{
    int index = (int)SendMessage(g_hListBox, LB_GETCURSEL, 0, 0);
    if (index != LB_ERR)
    {
        SendMessage(g_hListBox, LB_DELETESTRING, index, 0);
    }
    else
    {
        MessageBoxW(hwnd, L"No item selected in ListBox.", L"Info", MB_OK);
    }
}

// Очистить все списки
static void OnClearAll(HWND hwnd)
{
    ComboBox_Clear(g_hComboSelect);
    ComboBox_Clear(g_hComboEdit);
    ListBox_Clear(g_hListBox);
}

// Показать выбранные элементы
static void OnShowSelection(HWND hwnd)
{
    wchar_t comboText[256], listText[256];
    ComboBox_GetSelectedText(g_hComboSelect, comboText, 256);
    ListBox_GetSelectedText(g_hListBox, listText, 256);

    wchar_t message[512];
    wsprintfW(message, L"ComboBox selection: %s\r\nListBox selection: %s",
        comboText, listText);

    MessageBoxW(hwnd, message, L"Current Selection", MB_OK);
}

// Переместить выбранный элемент из ComboBox (первого) в ListBox
static void OnMoveComboToList(HWND hwnd)
{
    int index = (int)SendMessage(g_hComboSelect, CB_GETCURSEL, 0, 0);
    if (index != CB_ERR)
    {
        wchar_t buffer[256];
        SendMessage(g_hComboSelect, CB_GETLBTEXT, index, (LPARAM)buffer);
        // Добавляем в ListBox
        ListBox_AddString(g_hListBox, buffer);
        // Удаляем из ComboBox (первого)
        SendMessage(g_hComboSelect, CB_DELETESTRING, index, 0);
    }
    else
    {
        MessageBoxW(hwnd, L"No item selected in ComboBox.", L"Info", MB_OK);
    }
}

// -----------------------------------------------------------------
// Диалоговая процедура
// -----------------------------------------------------------------
INT_PTR CALLBACK ListDemoProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // Получаем дескрипторы контролов
        g_hComboSelect = GetDlgItem(hwnd, IDC_COMBO_SELECT);
        g_hComboEdit = GetDlgItem(hwnd, IDC_COMBO_EDIT);
        g_hListBox = GetDlgItem(hwnd, IDC_LIST_BOX);

        // Заполним ComboBox'ы и ListBox начальными данными для демонстрации
        ComboBox_AddString(g_hComboSelect, L"Red");
        ComboBox_AddString(g_hComboSelect, L"Green");
        ComboBox_AddString(g_hComboSelect, L"Blue");

        ComboBox_AddString(g_hComboEdit, L"Monday");
        ComboBox_AddString(g_hComboEdit, L"Tuesday");
        ComboBox_AddString(g_hComboEdit, L"Wednesday");

        ListBox_AddString(g_hListBox, L"Apple");
        ListBox_AddString(g_hListBox, L"Banana");
        ListBox_AddString(g_hListBox, L"Cherry");

        // Установим начальный выбор (для удобства)
        SendMessage(g_hComboSelect, CB_SETCURSEL, 0, 0);
        SendMessage(g_hComboEdit, CB_SETCURSEL, 0, 0);
        SendMessage(g_hListBox, LB_SETCURSEL, 0, 0);

        return TRUE;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        switch (id)
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            return TRUE;

        case IDC_BTN_ADD:
            OnAddItem(hwnd);
            return TRUE;

        case IDC_BTN_REMOVE:
            OnRemoveItem(hwnd);
            return TRUE;

        case IDC_BTN_CLEAR:
            OnClearAll(hwnd);
            return TRUE;

        case IDC_BTN_SHOW_SELECTION:
            OnShowSelection(hwnd);
            return TRUE;

        case IDC_BTN_MOVE_TO_LIST:
            OnMoveComboToList(hwnd);
            return TRUE;

            // Обработка выбора в ComboBox (уведомление CBN_SELCHANGE)
        case IDC_COMBO_SELECT:
        case IDC_COMBO_EDIT:
            if (code == CBN_SELCHANGE)
            {
                // Можно что-то сделать при выборе, например, показать в заголовке.
                // Для демонстрации просто поменяем заголовок.
                SetWindowTextW(hwnd, L"ComboBox selection changed");
                return TRUE;
            }
            break;

            // Обработка выбора в ListBox (уведомление LBN_SELCHANGE)
        case IDC_LIST_BOX:
            if (code == LBN_SELCHANGE)
            {
                SetWindowTextW(hwnd, L"ListBox selection changed");
                return TRUE;
            }
            break;
        }
        break;
    }
    }
    return FALSE;
}