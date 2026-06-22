// ================================================================
// dialog_proc.c – реализация оконной процедуры для Logging Demo.
// 
// ЧТО МЫ ЗДЕСЬ ДЕЛАЕМ?
// -------------------------------------------------------------------
// Мы создаём диалоговое окно, которое ведёт лог (журнал) действий
// пользователя. Лог отображается в многострочном поле Edit Control,
// а все действия (нажатие кнопок, перемещение ползунка, изменение
// Spin, выбор в ComboBox) записываются в этот лог с временной меткой.
// 
// КЛЮЧЕВЫЕ ОСОБЕННОСТИ:
//   1. Лог можно включать/выключать через Check Box.
//   2. Лог можно очистить кнопкой "Clear Log".
//   3. Лог можно сохранить в текстовый файл через стандартный диалог
//      сохранения (GetSaveFileName).
//   4. Spin Control работает ПО ПРОВЕРЕННОЙ СХЕМЕ (без глюков):
//      - Нет UDS_SETBUDDYINT (автообновление глючное).
//      - Инвертируем знак iDelta, чтобы стрелка вверх увеличивала.
//      - Ручной ввод в Edit синхронизируется со Spin.
// ================================================================

#include <windows.h>          // основной заголовок Windows API
#include <commctrl.h>         // для расширенных контролов (Spin, Slider, ComboBox)
#include "resource.h"         // наши ID
#include "dialog_proc.h"      // прототип оконной процедуры

// Подключаем библиотеку comdlg32.lib, где лежат GetSaveFileName и другие
// диалоги выбора файлов. Без этого линковщик выдаст ошибку.
#pragma comment(lib, "comdlg32.lib")

// ----------------------------------------------------------------
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ (статичные – видны только внутри этого файла)
// 
// Аналогия: это "память" программы – мы храним здесь состояние,
// которое должно сохраняться между вызовами оконной процедуры.
// ----------------------------------------------------------------

// --- Переменные для логирования ---
static BOOL g_bLoggingEnabled = TRUE;   // включено ли логирование (по умолчанию включено)

// --- Переменные для Spin Control (по аналогии со старым проектом) ---
static int g_spin_value = 50;          // текущее значение Spin
static int g_spin_min = 0;             // минимальное значение диапазона
static int g_spin_max = 100;           // максимальное значение диапазона
static BOOL g_spin_updating = FALSE;   // флаг: идёт ли обновление из кода?
// (нужен, чтобы не вызвать рекурсию при изменении Edit)

// ----------------------------------------------------------------
// ФУНКЦИИ ДЛЯ РАБОТЫ СО SPIN CONTROL (проверенная схема)
// 
// Зачем они нужны?
//   - UDS_SETBUDDYINT (автоматическое обновление) часто глючит,
//     поэтому мы обновляем Edit ВРУЧНУЮ.
//   - Мы используем глобальную переменную g_spin_value как единый
//     источник истины – это упрощает отладку.
// ----------------------------------------------------------------

// Обновить Edit Control из глобальной переменной g_spin_value.
// Просто преобразуем число в строку и устанавливаем в Edit.
static void UpdateSpinEdit(HWND hEdit)
{
    wchar_t buf[32];
    wsprintf(buf, L"%d", g_spin_value);
    SetWindowText(hEdit, buf);
}

// Синхронизировать Spin Control с глобальным значением g_spin_value.
// Отправляем Spin сообщение UDM_SETPOS – его внутренняя позиция
// становится равной нашему значению.
static void SyncSpin(HWND hSpin)
{
    SendMessage(hSpin, UDM_SETPOS, 0, MAKELONG(g_spin_value, 0));
}

// Попытаться обновить g_spin_value из текста Edit Control.
// Вызывается, когда пользователь ввёл число вручную.
// 
// Алгоритм:
//   1) Читаем текст из Edit.
//   2) Преобразуем в целое число (_wtoi).
//   3) Ограничиваем диапазоном g_spin_min..g_spin_max.
//   4) Если новое значение отличается от текущего – обновляем g_spin_value.
// 
// Возвращает TRUE, если значение изменилось (тогда нужно синхронизировать Spin).
static BOOL TryUpdateSpinFromEdit(HWND hEdit)
{
    wchar_t buf[32];
    GetWindowText(hEdit, buf, 32);
    int val = _wtoi(buf);   // _wtoi – преобразует строку wchar_t в int

    // Ограничиваем диапазоном
    if (val < g_spin_min) val = g_spin_min;
    if (val > g_spin_max) val = g_spin_max;

    // Если значение изменилось – обновляем
    if (val != g_spin_value)
    {
        g_spin_value = val;
        return TRUE;
    }
    return FALSE;
}

// ----------------------------------------------------------------
// ФУНКЦИИ ДЛЯ ЛОГИРОВАНИЯ
// ----------------------------------------------------------------

// Получить текущее системное время в виде строки.
// Формат: [YYYY-MM-DD HH:MM:SS] – например, [2025-06-21 17:30:47]
// Это стандартный формат для логов – он читаем и легко парсится.
static void GetTimeStamp(wchar_t* buffer, int size)
{
    SYSTEMTIME st;                  // структура с датой/временем
    GetLocalTime(&st);              // получаем локальное (не UTC) время

    // wsprintf – форматирует строку в буфер
    // %04d – число из 4 цифр с ведущими нулями (год)
    // %02d – число из 2 цифр с ведущими нулями (месяц, день, час, минута, секунда)
    wsprintf(buffer, L"[%04d-%02d-%02d %02d:%02d:%02d]",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
}

// Добавить запись в лог (в Edit Control).
// 
// Алгоритм:
//   1) Если логирование выключено – выходим (ничего не пишем).
//   2) Получаем текущий текст в Edit.
//   3) Если лог не пуст – добавляем символ новой строки \r\n.
//   4) Формируем строку: временная метка + сообщение.
//   5) Добавляем строку в конец Edit.
//   6) Прокручиваем вниз, чтобы была видна последняя запись.
// 
// Почему EM_REPLACESEL, а не SetWindowText?
//   SetWindowText заменяет ВЕСЬ текст. А нам нужно ДОБАВИТЬ в конец.
//   EM_REPLACESEL заменяет выделенный текст. Мы выделяем конец
//   (EM_SETSEL) и заменяем его на новую строку – это эффективнее.
static void AddLogEntry(HWND hDlg, const wchar_t* message)
{
    // Если логирование выключено – ничего не пишем
    if (!g_bLoggingEnabled)
        return;

    // Получаем дескриптор Edit Control для лога
    HWND hEdit = GetDlgItem(hDlg, IDC_LOG_EDIT);
    if (!hEdit)
        return;   // если контрол не найден – выходим

    // Получаем длину текущего текста в Edit
    int len = GetWindowTextLength(hEdit);
    if (len > 0)
    {
        // Если лог не пуст, добавляем символ новой строки
        // EM_SETSEL – выделяем текст (устанавливаем позицию курсора в конец)
        SendMessage(hEdit, EM_SETSEL, len, len);
        // EM_REPLACESEL – заменяем выделенный текст (т.е. вставляем в конец)
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)L"\r\n");
    }

    // Получаем временную метку
    wchar_t timestamp[64];
    GetTimeStamp(timestamp, 64);

    // Формируем полную строку: временная метка + сообщение
    wchar_t fullLine[512];
    wsprintf(fullLine, L"%s %s", timestamp, message);

    // Добавляем строку в конец Edit
    int newLen = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, newLen, newLen);
    SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)fullLine);

    // Прокручиваем вниз, чтобы последняя запись была видна
    // EM_SCROLLCARET – перемещает каретку в конец
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
    // WM_VSCROLL, SB_BOTTOM – прокручиваем вертикальную полосу в самый низ
    SendMessage(hEdit, WM_VSCROLL, SB_BOTTOM, 0);
}

// Обработчик для Radio-кнопок.
// Вызывается, когда пользователь выбрал одну из радио-кнопок.
// Проверяем, что кнопка действительно включена, и добавляем запись в лог.
static void OnRadioButton(HWND hDlg, int id, const wchar_t* label)
{
    // IsDlgButtonChecked – возвращает состояние кнопки (BST_CHECKED = включена)
    if (IsDlgButtonChecked(hDlg, id) == BST_CHECKED)
    {
        wchar_t msg[128];
        wsprintf(msg, L"Radio selected: %s", label);
        AddLogEntry(hDlg, msg);
    }
}

// ----------------------------------------------------------------
// ОКОННАЯ ПРОЦЕДУРА ДИАЛОГА
// 
// Это сердце нашей программы. Windows вызывает эту функцию
// при каждом событии (пользователь кликнул, нажал клавишу, таймер и т.д.).
// 
// Аналогия: это как диспетчер в офисе – принимает все запросы
// и решает, кто их будет обрабатывать.
// ----------------------------------------------------------------
INT_PTR CALLBACK MainDialogProc(
    _In_ HWND   hDlg,    // дескриптор нашего диалога (как удостоверение личности)
    _In_ UINT   uMsg,    // номер сообщения (что случилось?)
    _In_ WPARAM wParam,  // дополнительный параметр 1
    _In_ LPARAM lParam)  // дополнительный параметр 2
{
    switch (uMsg)
    {
        // ================================================================
        // WM_INITDIALOG – диалог только что создан, но ещё не показан.
        // 
        // Это идеальное место для настройки контролов: установка
        // диапазонов, начальных значений, загрузка данных и т.д.
        // Аналог – конструктор класса в C++.
        // ================================================================
    case WM_INITDIALOG:
    {
        // -------------------------------------------------------------
        // 1. Настройка Edit Control для лога (только для чтения)
        // -------------------------------------------------------------
        HWND hLogEdit = GetDlgItem(hDlg, IDC_LOG_EDIT);
        if (hLogEdit)
        {
            // Получаем текущий стиль окна
            LONG style = GetWindowLong(hLogEdit, GWL_STYLE);
            // Добавляем ES_READONLY – чтобы пользователь не мог редактировать лог
            style |= ES_READONLY;
            // Устанавливаем новый стиль
            SetWindowLong(hLogEdit, GWL_STYLE, style);
            // Применяем изменения (перерисовываем)
            SetWindowPos(hLogEdit, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        }

        // -------------------------------------------------------------
        // 2. Настройка Radio-кнопок (группа)
        // -------------------------------------------------------------
        // CheckRadioButton – устанавливает одну кнопку в группе как выбранную.
        // Параметры: (окно, ID_первой_в_группе, ID_последней_в_группе, ID_выбранной)
        // Устанавливаем Radio1 как выбранную по умолчанию.
        CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);

        // -------------------------------------------------------------
        // 3. Настройка Spin Control (ПОЛНОСТЬЮ из старого проекта)
        // -------------------------------------------------------------
        HWND hSpin = GetDlgItem(hDlg, IDC_SPIN);
        HWND hSpinEdit = GetDlgItem(hDlg, IDC_SPIN_EDIT);
        if (hSpin && hSpinEdit)
        {
            // 3.1. Связываем Spin с Edit (Buddy)
            //     UDM_SETBUDDY – говорит Spin, с каким полем ввода он связан.
            SendMessage(hSpin, UDM_SETBUDDY, (WPARAM)hSpinEdit, 0);

            // 3.2. Устанавливаем диапазон 0..100
            //     UDM_SETRANGE – параметр: MAKELONG(min, max)
            SendMessage(hSpin, UDM_SETRANGE, 0, MAKELONG(g_spin_min, g_spin_max));

            // 3.3. Устанавливаем начальное значение 50
            g_spin_value = 50;
            SyncSpin(hSpin);          // синхронизируем Spin
            UpdateSpinEdit(hSpinEdit); // обновляем Edit

            // 3.4. Устанавливаем стили Spin Control
            //     Убираем UDS_SETBUDDYINT – автообновление глючное.
            //     Добавляем UDS_ALIGNRIGHT – прикрепить Spin справа от Edit.
            //     Добавляем UDS_ARROWKEYS – разрешить управление с клавиатуры.
            LONG style = GetWindowLong(hSpin, GWL_STYLE);
            style |= UDS_ALIGNRIGHT | UDS_ARROWKEYS;
            style &= ~UDS_SETBUDDYINT;
            SetWindowLong(hSpin, GWL_STYLE, style);
            SetWindowPos(hSpin, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        }

        // -------------------------------------------------------------
        // 4. Настройка Slider Control
        // -------------------------------------------------------------
        HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER);
        if (hSlider)
        {
            // TBM_SETRANGE – устанавливаем диапазон 0..100
            SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            // TBM_SETPOS – начальная позиция 50
            SendMessage(hSlider, TBM_SETPOS, TRUE, 50);
            // TBM_SETTICFREQ – деления каждые 10 единиц (0, 10, 20, ..., 100)
            SendMessage(hSlider, TBM_SETTICFREQ, 10, 0);
        }

        // -------------------------------------------------------------
        // 5. Настройка ComboBox (выпадающий список)
        // -------------------------------------------------------------
        HWND hCombo = GetDlgItem(hDlg, IDC_COMBO);
        if (hCombo)
        {
            // CB_ADDSTRING – добавляем элементы в конец списка
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Red");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Green");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Blue");
            // CB_SETCURSEL – устанавливаем первый элемент как выбранный по умолчанию
            SendMessage(hCombo, CB_SETCURSEL, 0, 0);
        }

        // -------------------------------------------------------------
        // 6. Check Box "Enable Logging" – по умолчанию включена
        // -------------------------------------------------------------
        CheckDlgButton(hDlg, IDC_CHECK_ENABLE, BST_CHECKED);
        g_bLoggingEnabled = TRUE;

        // -------------------------------------------------------------
        // 7. Начальная запись в лог
        // -------------------------------------------------------------
        AddLogEntry(hDlg, L"Program started");

        // -------------------------------------------------------------
        // 8. Принудительный показ окна (на случай, если в ресурсах нет WS_VISIBLE)
        // -------------------------------------------------------------
        ShowWindow(hDlg, SW_SHOW);

        return TRUE; // сообщение обработано
    }

    // ================================================================
    // WM_COMMAND – пользователь нажал кнопку, изменил Check Box,
    // выбрал Radio, изменил ComboBox или что-то ввёл в Edit.
    // 
    // wParam: LOWORD(wParam) – ID контрола (кнопки, чекбокса и т.д.)
    //         HIWORD(wParam) – код уведомления (BN_CLICKED, EN_CHANGE и т.д.)
    // ================================================================
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);   // ID контрола
        int code = HIWORD(wParam); // код уведомления

        // -------------------------------------------------------------
        // Обработка ручного ввода в Spin Buddy (Edit Control)
        // 
        // EN_CHANGE – приходит каждый раз, когда пользователь меняет текст.
        // Мы синхронизируем Edit со Spin, чтобы они всегда показывали
        // одно и то же значение.
        // -------------------------------------------------------------
        if (id == IDC_SPIN_EDIT && code == EN_CHANGE)
        {
            // Если мы НЕ в процессе обновления из кода (защита от рекурсии)
            if (!g_spin_updating)
            {
                HWND hSpin = GetDlgItem(hDlg, IDC_SPIN);
                HWND hSpinEdit = GetDlgItem(hDlg, IDC_SPIN_EDIT);
                if (hSpin && hSpinEdit)
                {
                    // Пытаемся обновить g_spin_value из текста Edit
                    if (TryUpdateSpinFromEdit(hSpinEdit))
                    {
                        // Если значение изменилось – синхронизируем Spin
                        SyncSpin(hSpin);
                    }
                }
            }
            return TRUE; // сообщение обработано
        }

        // -------------------------------------------------------------
        // Обработка Radio-кнопок (BN_CLICKED)
        // -------------------------------------------------------------
        if (code == BN_CLICKED)
        {
            switch (id)
            {
            case IDC_RADIO1: OnRadioButton(hDlg, IDC_RADIO1, L"Option 1"); return TRUE;
            case IDC_RADIO2: OnRadioButton(hDlg, IDC_RADIO2, L"Option 2"); return TRUE;
            case IDC_RADIO3: OnRadioButton(hDlg, IDC_RADIO3, L"Option 3"); return TRUE;
            }
        }

        // -------------------------------------------------------------
        // Обработка Check Box "Enable Logging"
        // -------------------------------------------------------------
        if (id == IDC_CHECK_ENABLE && code == BN_CLICKED)
        {
            // Получаем состояние чекбокса (BST_CHECKED = включен)
            g_bLoggingEnabled = IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLE) == BST_CHECKED;

            // Логируем изменение
            if (g_bLoggingEnabled)
                AddLogEntry(hDlg, L"Logging enabled");
            else
            {
                // Если логирование выключается, мы должны записать это в лог,
                // но AddLogEntry не даст этого сделать, потому что проверяет флаг.
                // Поэтому мы временно включаем флаг, пишем запись, и выключаем обратно.
                // Это маленький "хак", но он работает.
                g_bLoggingEnabled = TRUE;
                AddLogEntry(hDlg, L"Logging disabled");
                g_bLoggingEnabled = FALSE;
            }
            return TRUE;
        }

        // -------------------------------------------------------------
        // Обработка кнопок: Clear Log, Save Log, Test
        // -------------------------------------------------------------
        switch (id)
        {
        case IDC_BUTTON_CLEAR:
        {
            // Очищаем лог – просто устанавливаем пустой текст в Edit
            HWND hEdit = GetDlgItem(hDlg, IDC_LOG_EDIT);
            if (hEdit)
                SetWindowText(hEdit, L"");   // пустая строка

            // Добавляем запись в лог (после очистки)
            AddLogEntry(hDlg, L"Log cleared");
            return TRUE;
        }

        case IDC_BUTTON_SAVE:
        {
            // Сохраняем лог в текстовый файл
            HWND hEdit = GetDlgItem(hDlg, IDC_LOG_EDIT);
            if (!hEdit)
                return TRUE;

            // Получаем длину текста в Edit
            int len = GetWindowTextLength(hEdit);
            if (len == 0)
            {
                // Если лог пуст – сообщаем и выходим
                MessageBox(hDlg, L"Log is empty. Nothing to save.", L"Info", MB_OK);
                return TRUE;
            }

            // Выделяем память под текст (len + 1 для завершающего нуля)
            wchar_t* buffer = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
            if (!buffer)
            {
                MessageBox(hDlg, L"Memory allocation failed!", L"Error", MB_ICONERROR);
                return TRUE;
            }
            // Копируем текст из Edit в буфер
            GetWindowText(hEdit, buffer, len + 1);

            // -------------------------------------------------------------
            // Открываем стандартный диалог сохранения файла (GetSaveFileName)
            // -------------------------------------------------------------
            OPENFILENAME ofn = { 0 };
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hDlg;                    // владелец – наше окно
            ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
            //           ^^^^^^^^  маска файлов: "Text Files" и "*.txt"
            //                    следующая пара: "All Files" и "*.*"
            //                   \0\0 – двойной завершающий ноль (конец списка)

            // Выделяем память под имя файла (MAX_PATH = 260 символов)
            ofn.lpstrFile = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));
            ofn.lpstrFile[0] = 0;   // пустая строка
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            // OFN_OVERWRITEPROMPT – спросить, если файл уже существует
            // OFN_PATHMUSTEXIST – путь должен существовать
            ofn.lpstrDefExt = L"txt";   // расширение по умолчанию

            // Показываем диалог. Если пользователь нажал "Сохранить" – TRUE.
            if (GetSaveFileName(&ofn))
            {
                // Создаём файл (GENERIC_WRITE – для записи, CREATE_ALWAYS – создать/перезаписать)
                HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD bytesWritten;
                    // Записываем буфер в файл (UTF-16, т.к. wchar_t)
                    // len * sizeof(wchar_t) – размер в байтах
                    if (!WriteFile(hFile, buffer, len * sizeof(wchar_t), &bytesWritten, NULL))
                    {
                        MessageBox(hDlg, L"Failed to write file!", L"Error", MB_ICONERROR);
                    }
                    else
                    {
                        // Успешно сохранили – добавляем запись в лог
                        wchar_t logMsg[256];
                        wsprintf(logMsg, L"Log saved to: %s", ofn.lpstrFile);
                        AddLogEntry(hDlg, logMsg);
                        MessageBox(hDlg, L"Log saved successfully!", L"Info", MB_OK);
                    }
                    CloseHandle(hFile);
                }
                else
                {
                    MessageBox(hDlg, L"Failed to create file!", L"Error", MB_ICONERROR);
                }
            }
            // Освобождаем память
            free((void*)ofn.lpstrFile);
            free(buffer);
            return TRUE;
        }

        case IDC_BUTTON_TEST:
        {
            // Тестовая кнопка – просто добавляет запись в лог
            AddLogEntry(hDlg, L"Test button clicked");
            return TRUE;
        }

        default:
            break;
        }

        // -------------------------------------------------------------
        // Обработка изменения выбора в ComboBox (CBN_SELCHANGE)
        // -------------------------------------------------------------
        if (id == IDC_COMBO && code == CBN_SELCHANGE)
        {
            HWND hCombo = GetDlgItem(hDlg, IDC_COMBO);
            if (hCombo)
            {
                // Получаем индекс выбранного элемента
                int idx = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR)   // если выбор валидный
                {
                    // Получаем текст выбранного элемента
                    wchar_t text[128];
                    SendMessage(hCombo, CB_GETLBTEXT, idx, (LPARAM)text);
                    // Логируем
                    wchar_t msg[256];
                    wsprintf(msg, L"ComboBox selected: %s", text);
                    AddLogEntry(hDlg, msg);
                }
            }
            return TRUE;
        }

        break; // выходим из WM_COMMAND, если ничего не обработали
    }

    // ================================================================
    // WM_HSCROLL – приходит при перемещении горизонтального Slider
    // ================================================================
    case WM_HSCROLL:
    {
        // Получаем дескриптор Slider из lParam
        HWND hSlider = (HWND)lParam;
        // Проверяем, что это наш Slider
        if (hSlider && GetDlgItem(hDlg, IDC_SLIDER) == hSlider)
        {
            // Получаем текущую позицию
            int pos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
            // Логируем
            wchar_t msg[128];
            wsprintf(msg, L"Slider moved to: %d", pos);
            AddLogEntry(hDlg, msg);
        }
        // Для WM_HSCROLL нужно возвращать 0
        return 0;
    }

    // ================================================================
    // WM_NOTIFY – уведомления от сложных контролов (Spin, Month Calendar и др.)
    // 
    // Причина: WM_COMMAND не может передать достаточно данных,
    // поэтому для Spin используется WM_NOTIFY с кодом UDN_DELTAPOS.
    // ================================================================
    case WM_NOTIFY:
    {
        // lParam – указатель на структуру NMHDR (или более крупную)
        NMHDR* pnmh = (NMHDR*)lParam;

        // Проверяем: уведомление от Spin? И это изменение позиции?
        if (pnmh->idFrom == IDC_SPIN && pnmh->code == UDN_DELTAPOS)
        {
            // UDN_DELTAPOS означает, что пользователь нажал на стрелку.
            // Структура NMUPDOWN содержит:
            //   iPos   – текущая позиция ДО изменения
            //   iDelta – изменение: -1 для стрелки вверх, +1 для стрелки вниз
            //            (ЭТО ИСТОРИЧЕСКАЯ ОСОБЕННОСТЬ WINDOWS! Стрелка вверх даёт -1)
            NMUPDOWN* pnud = (NMUPDOWN*)pnmh;

            // -------------------------------------------------------------
            // КЛЮЧЕВОЙ МОМЕНТ: инвертируем знак iDelta,
            // чтобы стрелка вверх УВЕЛИЧИВАЛА, а стрелка вниз УМЕНЬШАЛА.
            // -------------------------------------------------------------
            int newValue = g_spin_value - pnud->iDelta;

            // Ограничиваем диапазоном
            if (newValue < g_spin_min) newValue = g_spin_min;
            if (newValue > g_spin_max) newValue = g_spin_max;

            // Обновляем глобальное значение
            g_spin_value = newValue;

            // Синхронизируем Spin (чтобы его внутреннее состояние совпадало)
            HWND hSpin = GetDlgItem(hDlg, IDC_SPIN);
            if (hSpin) SyncSpin(hSpin);

            // Обновляем Edit Buddy (устанавливаем флаг, чтобы не вызвать рекурсию)
            HWND hSpinEdit = GetDlgItem(hDlg, IDC_SPIN_EDIT);
            if (hSpinEdit)
            {
                g_spin_updating = TRUE;
                UpdateSpinEdit(hSpinEdit);
                g_spin_updating = FALSE;
            }

            // Логируем изменение
            wchar_t msg[128];
            wsprintf(msg, L"Spin changed to: %d", newValue);
            AddLogEntry(hDlg, msg);

            return TRUE; // сообщение обработано
        }
        break;
    }

    // ================================================================
    // WM_CLOSE – пользователь нажал крестик [X] или Alt+F4
    // ================================================================
    case WM_CLOSE:
        // Пишем в лог, что программа закрывается
        AddLogEntry(hDlg, L"Program closed");
        // Уничтожаем окно – это вызовет WM_DESTROY
        DestroyWindow(hDlg);
        return TRUE;

        // ================================================================
        // WM_DESTROY – окно уже уничтожено, контролы удалены.
        // Здесь мы завершаем цикл сообщений (PostQuitMessage).
        // ================================================================
    case WM_DESTROY:
        PostQuitMessage(0);   // завершаем цикл в main.c
        return TRUE;
    }

    // Все необработанные нами сообщения возвращаем с FALSE.
    // FALSE означает: «я это не обработал, делай что-нибудь по умолчанию, Windows».
    return FALSE;
}