#pragma once
#include <windows.h>

// Создаёт статус-бар, возвращает HWND.
// hwndParent – родительское окно (диалог), hInst – экземпляр приложения.
HWND CreateStatusBar(HWND hwndParent, HINSTANCE hInst);

// Обновляет текст в указанной части (partIndex) статус-бара.
void UpdateStatusBarText(HWND hwndStatusBar, int partIndex, const wchar_t* text);

// Обработчик сообщения WM_SIZE для статус-бара (вызывать из диалога).
void OnStatusBarSize(HWND hwndStatusBar);
