// ================================================================
// dialog_proc.c – реализация оконной процедуры.
// Здесь мы управляем Spin Control и связываем его с Edit Control.
// 
// ОСОБЕННОСТИ:
//   - Мы НЕ используем UDS_SETBUDDYINT, потому что автоматическое
//     обновление Edit часто глючит и даёт смещение.
//   - Мы обновляем Edit ВРУЧНУЮ при каждом изменении.
//   - Мы инвертируем знак iDelta, потому что в Windows стрелка
//     вверх даёт iDelta = -1, а стрелка вниз = +1 (историческое
//     решение Microsoft, которое мы обходим).
//   - Мы синхронизируем Spin с глобальной переменной g_value,
//     а также обрабатываем ручной ввод в Edit через EN_CHANGE.
// ================================================================

#include <windows.h>          // основной заголовок Windows API
#include <commctrl.h>         // для Spin Control сообщений (UDM_*, UDN_*)
#include "resource.h"         // наши ID
#include "dialog_proc.h"      // прототип оконной процедуры

// ----------------------------------------------------------------
// Глобальные (статические) переменные для хранения состояния.
// Они видны только внутри этого файла.
// ----------------------------------------------------------------
static int g_value = 50;          // текущее значение числа
static int g_min = 0;             // минимальное значение диапазона
static int g_max = 100;           // максимальное значение диапазона
static BOOL g_bUpdating = FALSE;  // флаг: идёт ли обновление из кода
// (чтобы не вызвать рекурсию при изменении Edit)

// ----------------------------------------------------------------
// Вспомогательная функция: обновить текст в Edit Control
// из глобальной переменной g_value.
// ----------------------------------------------------------------
static void UpdateEdit(HWND hEdit)
{
    wchar_t buf[32];
    wsprintf(buf, L"%d", g_value);
    SetWindowText(hEdit, buf);
}

// ----------------------------------------------------------------
// Вспомогательная функция: синхронизировать Spin Control
// с глобальным значением g_value.
// Мы отправляем Spin сообщение UDM_SETPOS, чтобы его внутренняя
// позиция соответствовала нашему значению.
// ----------------------------------------------------------------
static void SyncSpin(HWND hSpin)
{
    SendMessage(hSpin, UDM_SETPOS, 0, MAKELONG(g_value, 0));
}

// ----------------------------------------------------------------
// Функция: попытаться обновить g_value из текста Edit Control.
// Используется при ручном вводе пользователем.
// 
// Алгоритм:
//   1) Читаем текст из Edit.
//   2) Преобразуем в целое число (с помощью _wtoi).
//   3) Ограничиваем диапазоном g_min..g_max.
//   4) Если новое значение отличается от текущего – обновляем g_value.
// 
// Возвращает TRUE, если значение изменилось.
// ----------------------------------------------------------------
static BOOL TryUpdateFromEdit(HWND hEdit)
{
    wchar_t buf[32];
    GetWindowText(hEdit, buf, 32);
    int val = _wtoi(buf);   // преобразуем строку в целое число

    // Ограничиваем диапазоном
    if (val < g_min) val = g_min;
    if (val > g_max) val = g_max;

    // Если значение изменилось – обновляем
    if (val != g_value)
    {
        g_value = val;
        return TRUE;
    }
    return FALSE;
}

// ----------------------------------------------------------------
// Оконная процедура диалога.
// Все сообщения от системы и от контролов обрабатываются здесь.
// ----------------------------------------------------------------
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
        // Здесь мы настраиваем Spin Control и Edit Control.
        // -------------------------------------------------------------
    case WM_INITDIALOG:
    {
        // Получаем дескрипторы контролов из ресурса по их ID
        HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_VALUE);
        HWND hSpin = GetDlgItem(hDlg, IDC_SPIN_VALUE);

        if (hEdit && hSpin)
        {
            // -----------------------------------------------------
            // 1. Связываем Spin с Edit (как Buddy).
            //    Это необходимо, чтобы Spin знал, с каким полем ввода
            //    он связан. Но мы не используем автоматическое обновление
            //    (UDS_SETBUDDYINT), потому что оно работает с багами.
            // -----------------------------------------------------
            SendMessage(hSpin, UDM_SETBUDDY, (WPARAM)hEdit, 0);

            // -----------------------------------------------------
            // 2. Устанавливаем диапазон значений.
            //    UDM_SETRANGE ожидает параметр вида MAKELONG(min, max).
            //    Мы используем глобальные переменные g_min и g_max.
            // -----------------------------------------------------
            SendMessage(hSpin, UDM_SETRANGE, 0, MAKELONG(g_min, g_max));

            // -----------------------------------------------------
            // 3. Устанавливаем начальное значение (50).
            //    Spin запоминает его внутренне, а мы ещё обновим Edit.
            // -----------------------------------------------------
            g_value = 50;
            SyncSpin(hSpin);      // синхронизируем Spin
            UpdateEdit(hEdit);    // обновляем Edit

            // -----------------------------------------------------
            // 4. Устанавливаем стили Spin Control.
            //    Мы явно убираем флаг UDS_SETBUDDYINT, чтобы
            //    автообновление не мешало. Добавляем:
            //      UDS_ALIGNRIGHT – прикрепить Spin справа от Buddy
            //      UDS_ARROWKEYS  – разрешить клавиши-стрелки для управления
            // -----------------------------------------------------
            LONG style = GetWindowLong(hSpin, GWL_STYLE);
            style |= UDS_ALIGNRIGHT | UDS_ARROWKEYS;
            style &= ~UDS_SETBUDDYINT;   // Убираем автообновление
            SetWindowLong(hSpin, GWL_STYLE, style);
            // Применяем изменения стиля (перерисовываем)
            SetWindowPos(hSpin, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        }

        return TRUE;
    }

    // -------------------------------------------------------------
    // WM_COMMAND – приходит от кнопок и от Edit Control.
    // Здесь мы:
    //   - Обрабатываем кнопки Apply и Set Range.
    //   - Обрабатываем изменение текста в Edit (EN_CHANGE).
    // -------------------------------------------------------------
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);   // идентификатор контрола
        int code = HIWORD(wParam); // код уведомления (например, EN_CHANGE)

        // ---------------------------------------------------------
        // Если уведомление пришло от Edit Control и это изменение
        // текста (EN_CHANGE), то пытаемся синхронизировать g_value
        // с введённым пользователем числом.
        // ---------------------------------------------------------
        if (id == IDC_EDIT_VALUE && code == EN_CHANGE)
        {
            // Если мы не находимся в процессе обновления из кода
            // (защита от рекурсии), то читаем текст и обновляем g_value.
            if (!g_bUpdating)
            {
                HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_VALUE);
                HWND hSpin = GetDlgItem(hDlg, IDC_SPIN_VALUE);
                if (hEdit && hSpin)
                {
                    // Если значение изменилось – синхронизируем Spin
                    if (TryUpdateFromEdit(hEdit))
                    {
                        SyncSpin(hSpin);
                    }
                }
            }
            return TRUE;
        }

        // ---------------------------------------------------------
        // Обработка кнопок
        // ---------------------------------------------------------
        switch (id)
        {
            // ---- Кнопка Apply ----
        case IDC_BUTTON_APPLY:
        {
            // Просто показываем текущее значение g_value в MessageBox.
            wchar_t buf[256];
            wsprintf(buf, L"Applied value: %d", g_value);
            MessageBox(hDlg, buf, L"Apply", MB_OK);
            return TRUE;
        }

        // ---- Кнопка Set Range ----
        case IDC_BUTTON_RANGE:
        {
            // Устанавливаем новый диапазон: 0..200.
            g_min = 0;
            g_max = 200;

            HWND hSpin = GetDlgItem(hDlg, IDC_SPIN_VALUE);
            if (hSpin)
            {
                SendMessage(hSpin, UDM_SETRANGE, 0, MAKELONG(g_min, g_max));
                // Устанавливаем значение на середину нового диапазона (100)
                g_value = 100;
                SyncSpin(hSpin);
            }

            // Обновляем Edit, устанавливая флаг g_bUpdating, чтобы
            // не вызвать повторную обработку EN_CHANGE.
            HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_VALUE);
            if (hEdit)
            {
                g_bUpdating = TRUE;
                UpdateEdit(hEdit);
                g_bUpdating = FALSE;
            }

            MessageBox(hDlg, L"Range set to 0..200", L"Range", MB_OK);
            return TRUE;
        }

        default:
            break;
        }
        break;
    }

    // -------------------------------------------------------------
    // WM_NOTIFY – приходит от Spin Control при изменении позиции.
    // Код уведомления – UDN_DELTAPOS.
    // Структура NMUPDOWN содержит:
    //   iPos   – текущая позиция до изменения
    //   iDelta – изменение: -1 для стрелки вверх, +1 для стрелки вниз
    //            (это историческая особенность Windows, которую мы обходим)
    // -------------------------------------------------------------
    case WM_NOTIFY:
    {
        NMHDR* pnmh = (NMHDR*)lParam;
        if (pnmh->idFrom == IDC_SPIN_VALUE && pnmh->code == UDN_DELTAPOS)
        {
            NMUPDOWN* pnud = (NMUPDOWN*)pnmh;

            // -----------------------------------------------------
            // ВАЖНО: инвертируем знак iDelta, чтобы стрелка вверх
            // увеличивала значение, а вниз – уменьшала.
            // По умолчанию: Up -> -1, Down -> +1.
            // Мы делаем: newValue = g_value - iDelta.
            // -----------------------------------------------------
            int newValue = g_value - pnud->iDelta;

            // Ограничиваем диапазоном
            if (newValue < g_min) newValue = g_min;
            if (newValue > g_max) newValue = g_max;

            // Обновляем глобальное значение
            g_value = newValue;

            // Синхронизируем Spin (чтобы его внутреннее состояние совпадало)
            HWND hSpin = GetDlgItem(hDlg, IDC_SPIN_VALUE);
            if (hSpin) SyncSpin(hSpin);

            // Обновляем Edit, устанавливая флаг, чтобы не вызвать рекурсию
            HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_VALUE);
            if (hEdit)
            {
                g_bUpdating = TRUE;
                UpdateEdit(hEdit);
                g_bUpdating = FALSE;
            }

            return TRUE;
        }
        break;
    }

    // -------------------------------------------------------------
    // WM_CLOSE – при попытке закрыть окно (крестик, Alt+F4).
    // -------------------------------------------------------------
    case WM_CLOSE:
        DestroyWindow(hDlg);
        PostQuitMessage(0);
        return TRUE;

        // -------------------------------------------------------------
        // WM_DESTROY – окно уничтожено.
        // -------------------------------------------------------------
    case WM_DESTROY:
        PostQuitMessage(0);
        return TRUE;
    }

    // Если мы не обработали сообщение, возвращаем FALSE,
    // чтобы система выполнила стандартную обработку.
    return FALSE;
}