// ================================================================
// dialog_proc.c – реализация оконной процедуры для Date Time Picker.
// Здесь мы:
//   1) При инициализации (WM_INITDIALOG) настраиваем Date Time Picker.
//      (В нашем случае настройка не требуется, но оставляем для будущих расширений.)
//   2) Обрабатываем чекбокс "Show time" – переключаем режим отображения
//      контрола (только дата или дата+время) путём изменения стилей.
//   3) Кнопка "Get Date" – показывает текущее системное время в MessageBox
//      в формате "Date: DD-MM-YYYY\nTime: HH:MM:SS".
//   4) Кнопка "Set Date" – устанавливает фиксированную дату (1 января 2025, 12:00)
//      в контрол.
// 
// КЛЮЧЕВЫЕ МОМЕНТЫ:
//   - Для работы Date Time Picker необходима инициализация Common Controls
//     (делается в main.c) с флагом ICC_STANDARD_CLASSES.
//   - Основные сообщения: DTM_GETSYSTEMTIME, DTM_SETSYSTEMTIME, DTM_SETFORMAT.
//   - Для получения текущего системного времени используем GetLocalTime().
//   - При изменении стилей контрола необходимо вызывать SetWindowPos
//     с флагом SWP_FRAMECHANGED, чтобы изменения применились.
// ================================================================

#include <windows.h>          // основной заголовок Windows API
#include <commctrl.h>         // для DateTimePicker (DATETIMEPICK_CLASS, DTM_* сообщения)
#include "resource.h"         // наши ID
#include "dialog_proc.h"      // прототип оконной процедуры

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
        // Здесь можно выполнить начальную настройку контролов.
        // В нашем случае контрол уже настроен по умолчанию, но мы оставляем
        // место для будущих расширений.
        // -------------------------------------------------------------
    case WM_INITDIALOG:
        return TRUE;

        // -------------------------------------------------------------
        // WM_COMMAND – приходит при взаимодействии с элементами управления:
        //   - кнопки (BN_CLICKED)
        //   - чекбоксы (BN_CLICKED)
        // 
        // LOWORD(wParam) содержит ID команды.
        // HIWORD(wParam) содержит код уведомления (например, BN_CLICKED).
        // -------------------------------------------------------------
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);   // идентификатор контрола
        int code = HIWORD(wParam); // код уведомления (не используется, но оставлено для справки)

        // Получаем дескриптор Date Time Picker для всех операций с ним.
        HWND hDT = GetDlgItem(hDlg, IDC_DATETIMEPICKER);
        if (!hDT) break; // если контрол не найден – выходим

        switch (id)
        {
            // ---- Чекбокс "Show time" ----
        case IDC_CHECK_SHOWTIME:
        {
            // Проверяем, установлен ли флажок (BST_CHECKED) или снят (BST_UNCHECKED).
            BOOL bChecked = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOWTIME) == BST_CHECKED;

            // Получаем текущий стиль контрола.
            LONG style = GetWindowLong(hDT, GWL_STYLE);

            if (bChecked)
            {
                // Если флажок установлен – включаем отображение времени.
                // DTS_TIMEFORMAT – добавляет время к отображению.
                style |= DTS_TIMEFORMAT;
                // Убираем DTS_LONGDATEFORMAT, если он был (чтобы не было конфликта).
                style &= ~DTS_LONGDATEFORMAT;
            }
            else
            {
                // Если флажок снят – отображаем только дату (короткий формат).
                // Убираем DTS_TIMEFORMAT.
                style &= ~DTS_TIMEFORMAT;
                // Убеждаемся, что установлен DTS_SHORTDATEFORMAT или DTS_LONGDATEFORMAT.
                style |= DTS_SHORTDATEFORMAT;
            }

            // Применяем новый стиль.
            SetWindowLong(hDT, GWL_STYLE, style);

            // Перерисовываем контрол с новым стилем.
            // SWP_FRAMECHANGED – сообщает системе, что изменилась рамка/стиль,
            // и контрол должен перерисоваться с новыми параметрами.
            SetWindowPos(hDT, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

            return TRUE;
        }

        // ---- Кнопка "Get Date" ----
        case IDC_BUTTON_GETDATE:
        {
            // Получаем текущее системное время (локальное) с помощью GetLocalTime().
            // Эта функция заполняет структуру SYSTEMTIME текущими датой и временем.
            SYSTEMTIME st;
            GetLocalTime(&st);

            // Форматируем строку в нужном формате:
            // Date: DD-MM-YYYY
            // Time: HH:MM:SS
            wchar_t buf[256];
            wsprintf(buf, L"Date: %02d-%02d-%04d\nTime: %02d:%02d:%02d",
                st.wDay, st.wMonth, st.wYear,
                st.wHour, st.wMinute, st.wSecond);

            // Показываем MessageBox с текущей датой и временем.
            MessageBox(hDlg, buf, L"Current Date/Time", MB_OK);
            return TRUE;
        }

        // ---- Кнопка "Set Date" ----
        case IDC_BUTTON_SETDATE:
        {
            // Устанавливаем в контрол фиксированную дату: 1 января 2025 года, 12:00:00.
            // Заполняем структуру SYSTEMTIME.
            SYSTEMTIME st = { 0 };
            st.wYear = 2025;
            st.wMonth = 1;
            st.wDay = 1;
            st.wHour = 12;
            st.wMinute = 0;
            st.wSecond = 0;

            // DTM_SETSYSTEMTIME – сообщение для установки даты/времени в контроле.
            // GDT_VALID – говорит, что передаваемая структура корректна.
            SendMessage(hDT, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);

            return TRUE;
        }

        default:
            break;
        }
        break;
    }

    // -------------------------------------------------------------
    // WM_CLOSE – при попытке закрыть окно (крестик, Alt+F4).
    // -------------------------------------------------------------
    case WM_CLOSE:
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