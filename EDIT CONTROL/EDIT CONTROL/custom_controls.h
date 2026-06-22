#pragma once
#include <windows.h>

// Создаёт поле ввода (Edit Control) с заданными стилями.
// Параметры:
//   hwndParent  - родительское окно
//   hInst       - экземпляр приложения
//   id          - идентификатор контрола
//   x, y, w, h  - координаты и размер
//   dwStyle     - дополнительные стили (ES_MULTILINE, ES_PASSWORD и т.д.)
//   text        - начальный текст (может быть NULL)
// Возвращает HWND созданного элемента.
HWND CreateEditControl(HWND hwndParent, HINSTANCE hInst, int id,
    int x, int y, int w, int h, DWORD dwStyle, LPCWSTR text);
