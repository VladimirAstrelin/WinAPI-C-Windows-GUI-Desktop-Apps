// dialog_proc.h – объявление диалоговой процедуры.
// Этот файл включается в main.c и в dialog_proc.c,
// чтобы компилятор знал сигнатуру функции MainDialogProc.

#pragma once              // Защита от повторного включения (эквивалент #ifndef ... #endif)

#include <windows.h>      // Нужен для типов HWND, UINT, WPARAM, LPARAM и т.д.

// Прототип функции, обрабатывающей сообщения диалога.
// Возвращает INT_PTR (целое, которое может быть указателем), обычно TRUE или FALSE.
// Параметры:
//   hwnd   – дескриптор окна диалога
//   uMsg   – идентификатор сообщения (например, WM_INITDIALOG, WM_COMMAND)
//   wParam – первый параметр сообщения (зависит от uMsg)
//   lParam – второй параметр сообщения (зависит от uMsg)
INT_PTR CALLBACK MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);