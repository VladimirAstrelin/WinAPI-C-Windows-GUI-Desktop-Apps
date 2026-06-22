// ================================================================
// main.c – точка входа в приложение (WinMain).
// Здесь мы:
//   1) Загружаем библиотеку Msftedit.dll, необходимую для Rich Edit.
//   2) Инициализируем общие элементы управления (Common Controls).
//   3) Создаём немодальное диалоговое окно.
//   4) Запускаем главный цикл обработки сообщений.
// 
// Rich Edit — это сложный контрол, требующий специальной инициализации.
// Без загрузки Msftedit.dll и InitCommonControlsEx он не работает.
// Мы загружаем библиотеку ДО создания любого окна, чтобы класс
// RICHEDIT50W был зарегистрирован в системе.
// ================================================================

#include <windows.h>          // основной заголовок Windows API
#include <commctrl.h>         // для InitCommonControlsEx
#include "resource.h"         // наши ID (IDD_MAIN_DIALOG)
#include "dialog_proc.h"      // прототип оконной процедуры

// ----------------------------------------------------------------
// Подключаем библиотеку comctl32.lib, в которой реализована
// функция InitCommonControlsEx. Без этого линковщик выдаст ошибку.
// ----------------------------------------------------------------
#pragma comment(lib, "comctl32.lib")

// ----------------------------------------------------------------
// WinMain – стандартная точка входа для Windows-приложений.
// 
// Параметры:
//   hInstance      – дескриптор текущего экземпляра приложения.
//   hPrevInstance  – всегда NULL (оставлен для совместимости).
//   lpCmdLine      – командная строка (мы её не используем).
//   nCmdShow       – флаг показа окна (обычно SW_SHOWNORMAL).
// 
// Аннотации _In_ и _In_opt_ – SAL-аннотации для статического анализа.
// ----------------------------------------------------------------
int WINAPI WinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPSTR     lpCmdLine,
    _In_     int       nCmdShow)
{
    // -------------------------------------------------------------
    // 1. Загружаем библиотеку Msftedit.dll.
    //    Эта библиотека содержит реализацию Rich Edit контрола.
    //    Если загрузка не удалась – программа не сможет работать,
    //    поэтому показываем ошибку и завершаемся.
    // -------------------------------------------------------------
    HMODULE hMod = LoadLibrary(L"Msftedit.dll");
    if (!hMod)
    {
        MessageBox(NULL, L"Failed to load Msftedit.dll", L"Critical Error", MB_ICONERROR);
        return 1;
    }

    // -------------------------------------------------------------
    // 2. Инициализация общих элементов управления.
    //    Common Controls – это набор стандартных контролов Windows,
    //    включая Rich Edit. Мы передаём флаги:
    //      ICC_STANDARD_CLASSES – стандартные классы (Edit, ComboBox и др.)
    //      ICC_USEREX_CLASSES   – расширенные классы (включая Rich Edit)
    //      ICC_WIN95_CLASSES    – для совместимости со старыми версиями
    //    Это гарантирует, что классы будут зарегистрированы.
    // -------------------------------------------------------------
    INITCOMMONCONTROLSEX icex = { 0 };
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // Дополнительно вызываем старую функцию InitCommonControls
    // для максимальной совместимости (не обязательно, но не помешает).
    InitCommonControls();

    // -------------------------------------------------------------
    // 3. Создание немодального диалогового окна.
    //    CreateDialog загружает шаблон из ресурса и создаёт окно,
    //    но не показывает его (если в ресурсе не установлен WS_VISIBLE).
    //    Мы покажем его явно.
    // -------------------------------------------------------------
    HWND hDlg = CreateDialog(
        hInstance,
        MAKEINTRESOURCE(IDD_MAIN_DIALOG),
        NULL,
        MainDialogProc
    );

    if (!hDlg)
    {
        // Если не удалось создать диалог – показываем код ошибки.
        DWORD err = GetLastError();
        wchar_t buf[256];
        wsprintf(buf, L"CreateDialog failed with error %d", err);
        MessageBox(NULL, buf, L"Error", MB_ICONERROR);
        return 1;
    }

    // Показываем окно на экране.
    ShowWindow(hDlg, nCmdShow);
    UpdateWindow(hDlg);   // принудительно перерисовываем

    // -------------------------------------------------------------
    // 4. Главный цикл обработки сообщений.
    //    GetMessage извлекает сообщения из очереди.
    //    TranslateMessage преобразует нажатия клавиш в символы.
    //    DispatchMessage отправляет сообщение в оконную процедуру.
    //    Когда приходит WM_QUIT, GetMessage возвращает 0, и цикл завершается.
    // -------------------------------------------------------------
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Возвращаем код завершения.
    return (int)msg.wParam;
}