// ================================================================
// dialog_proc.c – реализация оконной процедуры для Progress Control.
// Здесь мы:
//   1) При инициализации настраиваем Progress Control (диапазон, шаг, начальное значение).
//   2) Обрабатываем кнопки: Start, Stop, Reset, Set 50%.
//   3) Используем таймер (SetTimer/KillTimer) для автоматического увеличения прогресса.
//   4) Обновляем Static Text с отображением процентов.
// 
// КЛЮЧЕВЫЕ МОМЕНТЫ:
//   - Для работы Progress Control необходимо инициализировать Common Controls
//     с флагом ICC_PROGRESS_CLASS (делается в main.c).
//   - Таймер отправляет сообщения WM_TIMER, которые мы обрабатываем.
//   - Важно не забывать останавливать таймер при закрытии окна и при достижении 100%.
//   - Используем IsDialogMessage в главном цикле сообщений (см. main.c) для
//     корректной обработки клавиш Tab, Enter, Escape.
// ================================================================

#include <windows.h>          // основной заголовок Windows API
#include <commctrl.h>         // для Progress Control сообщений (PBM_*)
#include "resource.h"         // наши ID
#include "dialog_proc.h"      // прототип оконной процедуры

// --------------------------------------------
// Глобальные (статические) переменные состояния.
// Они видны только внутри этого файла.
// --------------------------------------------
static int g_progress = 0;          // текущее значение прогресса (0..100)
static int g_step = 1;              // шаг увеличения при каждом тике таймера
static UINT_PTR g_timerId = 0;      // идентификатор таймера (0 = таймер не запущен)

// --------------------------------------------
// Вспомогательная функция: обновить Progress Control и Static-текст.
// Она вызывается при изменении g_progress.
// --------------------------------------------
static void UpdateProgress(HWND hProgress, HWND hStatic)
{
    // Устанавливаем позицию прогресс-бара.
    // PBM_SETPOS – сообщение для установки текущего значения.
    SendMessage(hProgress, PBM_SETPOS, g_progress, 0);

    // Формируем строку с процентами (например, "50%").
    wchar_t buf[16];
    wsprintf(buf, L"%d%%", g_progress);
    // Устанавливаем текст в Static Control.
    SetWindowText(hStatic, buf);
}

// --------------------------------------------
// Оконная процедура диалога.
// Все сообщения от системы и от контролов обрабатываются здесь.
// --------------------------------------------
INT_PTR CALLBACK MainDialogProc(
    _In_ HWND   hDlg,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        // -------------------------------------------------------------
        // WM_INITDIALOG – вызывается при создании диалога.
        // Это идеальное место для инициализации контролов.
        // -------------------------------------------------------------
    case WM_INITDIALOG:
    {
        // Получаем дескрипторы контролов по их ID из ресурса.
        // GetDlgItem – функция для получения HWND контрола по ID.
        HWND hProgress = GetDlgItem(hDlg, IDC_PROGRESS);
        HWND hStatic = GetDlgItem(hDlg, IDC_STATIC_PROGRESS);

        // Проверяем, что контролы существуют (на случай, если ID в ресурсе не совпадают).
        if (!hProgress)
        {
            MessageBox(hDlg, L"Progress Control not found! Check ID.", L"Error", MB_ICONERROR);
        }
        if (!hStatic)
        {
            MessageBox(hDlg, L"Static Text not found! Check ID.", L"Error", MB_ICONERROR);
        }

        // Если оба контрола найдены – настраиваем прогресс.
        if (hProgress && hStatic)
        {
            // Устанавливаем диапазон: минимум 0, максимум 100.
            // PBM_SETRANGE – сообщение для установки диапазона.
            // MAKELONG(min, max) – упаковывает два 16-битных значения в одно 32-битное.
            SendMessage(hProgress, PBM_SETRANGE, 0, MAKELONG(0, 100));

            // Устанавливаем шаг (по умолчанию 1).
            // PBM_SETSTEP – сообщение для установки шага.
            SendMessage(hProgress, PBM_SETSTEP, g_step, 0);

            // Начальное значение прогресса – 0.
            g_progress = 0;
            // Обновляем отображение.
            UpdateProgress(hProgress, hStatic);
        }

        // Принудительно показываем окно (на случай, если в ресурсах не стоит WS_VISIBLE).
        // Это дублирует ShowWindow в WinMain, но не помешает.
        ShowWindow(hDlg, SW_SHOW);
        return TRUE;
    }

    // -------------------------------------------------------------
    // WM_COMMAND – приходит при нажатии кнопок.
    // LOWORD(wParam) содержит ID команды (кнопки).
    // -------------------------------------------------------------
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        HWND hProgress = GetDlgItem(hDlg, IDC_PROGRESS);
        HWND hStatic = GetDlgItem(hDlg, IDC_STATIC_PROGRESS);

        switch (id)
        {
            // ---- Кнопка "Start" ----
        case IDC_BUTTON_START:
            // Запускаем таймер, если он ещё не запущен.
            // SetTimer(окно, ID, интервал_в_мс, NULL) – создаёт таймер.
            // ID таймера = 1 (может быть любое число, но уникальное в рамках окна).
            // Интервал 100 мс = 0.1 секунды.
            if (g_timerId == 0)
                g_timerId = SetTimer(hDlg, 1, 100, NULL);
            return TRUE;

            // ---- Кнопка "Stop" ----
        case IDC_BUTTON_STOP:
            // Останавливаем таймер, если он запущен.
            // KillTimer(окно, ID_таймера) – удаляет таймер.
            if (g_timerId != 0)
            {
                KillTimer(hDlg, g_timerId);
                g_timerId = 0;
            }
            return TRUE;

            // ---- Кнопка "Reset" ----
        case IDC_BUTTON_RESET:
            // Останавливаем таймер (если запущен) и сбрасываем прогресс на 0.
            if (g_timerId != 0)
            {
                KillTimer(hDlg, g_timerId);
                g_timerId = 0;
            }
            g_progress = 0;
            UpdateProgress(hProgress, hStatic);
            return TRUE;

            // ---- Кнопка "Set 50%" ----
        case IDC_BUTTON_SET50:
            // Устанавливаем прогресс на 50% (независимо от таймера).
            g_progress = 50;
            UpdateProgress(hProgress, hStatic);
            return TRUE;

        default:
            break;
        }
        break;
    }

    // -------------------------------------------------------------
    // WM_TIMER – приходит от таймера через каждые 100 мс.
    // wParam содержит идентификатор таймера (у нас он равен 1).
    // -------------------------------------------------------------
    case WM_TIMER:
        // Убеждаемся, что сообщение от нашего таймера.
        if (wParam == g_timerId)
        {
            HWND hProgress = GetDlgItem(hDlg, IDC_PROGRESS);
            HWND hStatic = GetDlgItem(hDlg, IDC_STATIC_PROGRESS);

            // Если прогресс ещё не достиг 100% – увеличиваем.
            if (g_progress < 100)
            {
                g_progress += g_step;
                if (g_progress > 100) g_progress = 100;
                UpdateProgress(hProgress, hStatic);
            }
            else
            {
                // Если достигли 100% – автоматически останавливаем таймер.
                if (g_timerId != 0)
                {
                    KillTimer(hDlg, g_timerId);
                    g_timerId = 0;
                }
            }
            return TRUE;
        }
        break;

        // -------------------------------------------------------------
        // WM_CLOSE – при попытке закрыть окно (крестик, Alt+F4).
        // -------------------------------------------------------------
    case WM_CLOSE:
        // Останавливаем таймер, чтобы он не висел после закрытия.
        if (g_timerId != 0)
        {
            KillTimer(hDlg, g_timerId);
            g_timerId = 0;
        }
        // Уничтожаем окно – это вызовет WM_DESTROY.
        DestroyWindow(hDlg);
        return TRUE;

        // -------------------------------------------------------------
        // WM_DESTROY – окно действительно уничтожается.
        // Здесь мы отправляем WM_QUIT, чтобы завершить цикл сообщений.
        // -------------------------------------------------------------
    case WM_DESTROY:
        PostQuitMessage(0);
        return TRUE;
    }

    // Если мы не обработали сообщение, возвращаем FALSE.
    return FALSE;
}