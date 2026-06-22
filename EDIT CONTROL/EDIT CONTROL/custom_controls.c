#include "custom_controls.h"
#include <windows.h>

HWND CreateEditControl(HWND hwndParent, HINSTANCE hInst, int id,
    int x, int y, int w, int h, DWORD dwStyle, LPCWSTR text)
{
    // Базовые стили для Edit Control: дочерний, видимый, с рамкой (WS_BORDER) и табуляцией.
    // WS_TABSTOP позволяет переключаться по клавише Tab.
    DWORD baseStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP;

    // Создаём окно с классом "EDIT".
    HWND hwnd = CreateWindowEx(
        0,                         // Без расширенных стилей
        L"EDIT",                   // Имя класса для Edit Control
        text ? text : L"",         // Начальный текст (если передан)
        baseStyle | dwStyle,       // Комбинируем базовые стили с переданными
        x, y, w, h,
        hwndParent,
        (HMENU)(INT_PTR)id,        // Преобразуем ID в HMENU (так требует CreateWindow)
        hInst,
        NULL
    );

    return hwnd;
}