// ================================================================
// dialog_proc.c – реализация оконной процедуры.
// Здесь сосредоточена вся логика работы с Rich Edit:
//   - Создание контрола (программно, т.к. в ресурсах его нет).
//   - Форматирование текста (жирный, курсив, подчёркнутый).
//   - Выбор цвета и шрифта через стандартные диалоги.
//   - Сохранение и загрузка RTF-файлов.
// 
// ВАЖНО: Rich Edit требует, чтобы библиотека Msftedit.dll была
// загружена ДО создания контрола. Это делается в main.c.
// Кроме того, для работы диалогов выбора цвета и шрифта мы
// подключаем библиотеку comdlg32.lib.
// ================================================================

#include <windows.h>          // основной заголовок Windows API
#include <commctrl.h>         // для структур (не обязательно)
#include <richedit.h>         // для CHARFORMAT2, EM_* сообщений
#include <commdlg.h>          // для ChooseColor, ChooseFont
#include "resource.h"         // наши ID
#include "dialog_proc.h"      // прототип оконной процедуры

// ----------------------------------------------------------------
// Подключаем библиотеку comdlg32.lib, в которой реализованы
// функции ChooseColor и ChooseFont. Без этого линковщик выдаст ошибку.
// ----------------------------------------------------------------
#pragma comment(lib, "comdlg32.lib")

// ----------------------------------------------------------------
// Вспомогательная функция для применения форматирования
// (жирный, курсив, подчёркнутый) к выделенному тексту.
// 
// Параметры:
//   hRich   – дескриптор Rich Edit контрола.
//   mask    – битовая маска, указывающая, какие атрибуты меняем
//             (CFM_BOLD, CFM_ITALIC, CFM_UNDERLINE).
//   effects – значения атрибутов (CFE_BOLD, CFE_ITALIC, CFE_UNDERLINE)
//             или 0 для снятия.
// ----------------------------------------------------------------
static void ApplyCharFormat(HWND hRich, DWORD mask, DWORD effects)
{
    // Структура CHARFORMAT2 описывает форматирование символа.
    // Инициализируем нулями, чтобы не было мусора.
    CHARFORMAT2 cf = { 0 };
    cf.cbSize = sizeof(CHARFORMAT2);   // обязательно указываем размер
    cf.dwMask = mask;                  // какие поля заполнены
    cf.dwEffects = effects;            // какие эффекты применить

    // EM_SETCHARFORMAT – сообщение для установки форматирования.
    // SCF_SELECTION – применить только к выделенному тексту.
    SendMessage(hRich, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

// ----------------------------------------------------------------
// Диалог выбора цвета (ChooseColor).
// Возвращает выбранный цвет (COLORREF) или CLR_INVALID при отмене.
// 
// ВАЖНО: В структуре CHOOSECOLOR обязательно нужно указать
// массив пользовательских цветов (lpCustColors). Если этого не сделать,
// программа может упасть в некоторых версиях Windows.
// Мы используем статический массив, который сохраняется между вызовами.
// ----------------------------------------------------------------
static COLORREF ChooseColorDialog(HWND hWnd, COLORREF crDefault)
{
    // Статический массив из 16 пользовательских цветов.
    // Инициализируем нулями – система сама заполнит их при первом вызове.
    static COLORREF customColors[16] = { 0 };

    CHOOSECOLOR cc = { 0 };
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = hWnd;                    // владелец – наше окно
    cc.rgbResult = crDefault;               // цвет по умолчанию
    cc.lpCustColors = customColors;         // ОБЯЗАТЕЛЬНО!
    cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
    // CC_RGBINIT – использовать rgbResult как начальный цвет.
    // CC_FULLOPEN – показывать полный диалог (с настройками пользователя).
    // CC_ANYCOLOR – разрешить выбор любого цвета (не только стандартные).

    if (ChooseColor(&cc))
        return cc.rgbResult;
    else
        return CLR_INVALID;   // пользователь нажал "Отмена" или ошибка
}

// ----------------------------------------------------------------
// Диалог выбора шрифта (ChooseFont).
// Возвращает TRUE, если шрифт выбран, и заполняет структуру LOGFONT.
// ----------------------------------------------------------------
static BOOL ChooseFontDialog(HWND hWnd, LOGFONT* plf)
{
    CHOOSEFONT cf = { 0 };
    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = hWnd;
    cf.lpLogFont = plf;                      // куда сохранить выбранный шрифт
    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
    // CF_INITTOLOGFONTSTRUCT – использовать plf как начальный шрифт.
    // CF_SCREENFONTS – показывать только шрифты, доступные на экране.

    return ChooseFont(&cf);
}

// ----------------------------------------------------------------
// Применить выбранный шрифт к выделенному тексту.
// Преобразует LOGFONT в CHARFORMAT2 и отправляет сообщение.
// ----------------------------------------------------------------
static void ApplyFont(HWND hRich, LOGFONT* plf)
{
    CHARFORMAT2 cf = { 0 };
    cf.cbSize = sizeof(CHARFORMAT2);
    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC |
        CFM_UNDERLINE | CFM_CHARSET | CFM_COLOR;

    // Копируем имя шрифта
    wcsncpy_s(cf.szFaceName, LF_FACESIZE, plf->lfFaceName, _TRUNCATE);

    // Высота шрифта в twips (1/20 пункта). LOGFONT.lfHeight – в логических пикселях.
    // Для положительного значения пересчёт: yHeight = abs(lfHeight) * 20.
    cf.yHeight = abs(plf->lfHeight) * 20;

    // Кодировка (обычно DEFAULT_CHARSET)
    cf.bCharSet = plf->lfCharSet;

    // Устанавливаем стили из LOGFONT
    cf.dwEffects = 0;
    if (plf->lfWeight >= FW_BOLD) cf.dwEffects |= CFE_BOLD;
    if (plf->lfItalic) cf.dwEffects |= CFE_ITALIC;
    if (plf->lfUnderline) cf.dwEffects |= CFE_UNDERLINE;

    // Цвет текста (по умолчанию чёрный)
    cf.crTextColor = RGB(0, 0, 0);

    // Отправляем форматирование
    SendMessage(hRich, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

// ----------------------------------------------------------------
// Callback-функции для EM_STREAMOUT (сохранение) и EM_STREAMIN (загрузка).
// Эти функции вызываются Rich Edit для чтения/записи данных блоками.
// ----------------------------------------------------------------

// Сохранение: записывает буфер в файл.
static DWORD CALLBACK StreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;   // дескриптор файла
    DWORD written;
    if (!WriteFile(hFile, pbBuff, cb, &written, NULL))
        return 1;   // ошибка
    *pcb = written; // сколько байт реально записано
    return 0;       // успех
}

// Загрузка: читает из файла в буфер.
static DWORD CALLBACK StreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;   // дескриптор файла
    DWORD read;
    if (!ReadFile(hFile, pbBuff, cb, &read, NULL))
        return 1;   // ошибка
    *pcb = read;    // сколько байт реально прочитано
    return 0;       // успех
}

// ----------------------------------------------------------------
// Оконная процедура диалога.
// Все сообщения от системы и от кнопок обрабатываются здесь.
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
        // Здесь мы создаём Rich Edit контрол программно.
        // -------------------------------------------------------------
    case WM_INITDIALOG:
    {
        // Попытка создать Rich Edit с классом RICHEDIT50W (современная версия).
        // Если не получится – пробуем RICHEDIT20W (старая версия).
        // 
        // Параметры CreateWindowEx:
        //   WS_EX_CLIENTEDGE – рамка с углублением.
        //   L"RICHEDIT50W" – имя класса.
        //   L"" – начальный текст (пустой).
        //   WS_CHILD | WS_VISIBLE ... – стили.
        //   10, 50, 400, 200 – координаты и размер (подгоните под свой диалог).
        //   hDlg – родительское окно.
        //   (HMENU)IDC_RICHEDIT – идентификатор.
        //   GetModuleHandle(NULL) – экземпляр.
        //   NULL – дополнительные данные.
        HWND hRich = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            L"RICHEDIT50W",
            L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
            10, 50, 400, 200,
            hDlg,
            (HMENU)IDC_RICHEDIT,
            GetModuleHandle(NULL),
            NULL
        );

        // Если не получилось, пробуем старый класс
        if (!hRich)
        {
            hRich = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                L"RICHEDIT20W",
                L"",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
                10, 50, 400, 200,
                hDlg,
                (HMENU)IDC_RICHEDIT,
                GetModuleHandle(NULL),
                NULL
            );
        }

        if (!hRich)
        {
            // Если не удалось создать – показываем ошибку.
            DWORD err = GetLastError();
            wchar_t buf[256];
            wsprintf(buf, L"CreateWindowEx failed with error %d", err);
            MessageBox(hDlg, buf, L"Error", MB_ICONERROR);
        }
        else
        {
            // Успешно создали – устанавливаем начальный текст.
            SetWindowText(hRich, L"Hello, Rich Edit!");
            // Ограничиваем максимальную длину (1 МБ) для безопасности.
            SendMessage(hRich, EM_LIMITTEXT, 1024 * 1024, 0);
        }

        return TRUE;
    }

    // -------------------------------------------------------------
    // WM_COMMAND – приходит при нажатии кнопок.
    // LOWORD(wParam) содержит ID команды.
    // -------------------------------------------------------------
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        HWND hRich = GetDlgItem(hDlg, IDC_RICHEDIT);
        if (!hRich) break;   // если контрол не найден – выходим

        switch (id)
        {
            // ---- Жирный ----
        case IDC_BUTTON_BOLD:
        {
            // Получаем текущее форматирование выделения
            CHARFORMAT2 cf = { 0 };
            cf.cbSize = sizeof(CHARFORMAT2);
            SendMessage(hRich, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

            // Если жирный включён – выключаем, иначе включаем
            DWORD mask = CFM_BOLD;
            DWORD effects = (cf.dwEffects & CFE_BOLD) ? 0 : CFE_BOLD;
            ApplyCharFormat(hRich, mask, effects);
            return TRUE;
        }

        // ---- Курсив ----
        case IDC_BUTTON_ITALIC:
        {
            CHARFORMAT2 cf = { 0 };
            cf.cbSize = sizeof(CHARFORMAT2);
            SendMessage(hRich, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
            DWORD mask = CFM_ITALIC;
            DWORD effects = (cf.dwEffects & CFE_ITALIC) ? 0 : CFE_ITALIC;
            ApplyCharFormat(hRich, mask, effects);
            return TRUE;
        }

        // ---- Подчёркнутый ----
        case IDC_BUTTON_UNDERLINE:
        {
            CHARFORMAT2 cf = { 0 };
            cf.cbSize = sizeof(CHARFORMAT2);
            SendMessage(hRich, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
            DWORD mask = CFM_UNDERLINE;
            DWORD effects = (cf.dwEffects & CFE_UNDERLINE) ? 0 : CFE_UNDERLINE;
            ApplyCharFormat(hRich, mask, effects);
            return TRUE;
        }

        // ---- Выбор цвета ----
        case IDC_BUTTON_COLOR:
        {
            COLORREF cr = ChooseColorDialog(hDlg, RGB(0, 0, 0));
            if (cr != CLR_INVALID)
            {
                CHARFORMAT2 cf = { 0 };
                cf.cbSize = sizeof(CHARFORMAT2);
                cf.dwMask = CFM_COLOR;
                cf.crTextColor = cr;

                // Если текст не выделен – применяем ко всему тексту,
                // иначе только к выделению.
                DWORD dwSel = (DWORD)SendMessage(hRich, EM_GETSEL, 0, 0);
                if (LOWORD(dwSel) == HIWORD(dwSel))
                    SendMessage(hRich, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
                else
                    SendMessage(hRich, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
            }
            return TRUE;
        }

        // ---- Выбор шрифта ----
        case IDC_BUTTON_FONT:
        {
            LOGFONT lf = { 0 };
            CHARFORMAT2 cf = { 0 };
            cf.cbSize = sizeof(CHARFORMAT2);
            SendMessage(hRich, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

            // Заполняем LOGFONT из текущего форматирования
            if (cf.dwMask & CFM_FACE)
                wcsncpy_s(lf.lfFaceName, LF_FACESIZE, cf.szFaceName, _TRUNCATE);
            else
                wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Times New Roman");
            lf.lfHeight = cf.yHeight / 20;
            lf.lfWeight = (cf.dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
            lf.lfItalic = (cf.dwEffects & CFE_ITALIC) ? TRUE : FALSE;
            lf.lfUnderline = (cf.dwEffects & CFE_UNDERLINE) ? TRUE : FALSE;
            lf.lfCharSet = cf.bCharSet;

            if (ChooseFontDialog(hDlg, &lf))
                ApplyFont(hRich, &lf);
            return TRUE;
        }

        // ---- Сохранить в RTF ----
        case IDC_BUTTON_SAVE:
        {
            OPENFILENAME ofn = { 0 };
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFilter = L"RTF Files\0*.rtf\0All Files\0*.*\0";
            ofn.lpstrFile = (LPTSTR)malloc(MAX_PATH * sizeof(TCHAR));
            ofn.lpstrFile[0] = 0;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT;
            ofn.lpstrDefExt = L"rtf";

            if (GetSaveFileName(&ofn))
            {
                HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    EDITSTREAM es = { 0 };
                    es.dwCookie = (DWORD_PTR)hFile;
                    es.pfnCallback = StreamOutCallback;
                    // SF_RTF – формат RTF (а не обычный текст)
                    SendMessage(hRich, EM_STREAMOUT, SF_RTF, (LPARAM)&es);
                    CloseHandle(hFile);
                    MessageBox(hDlg, L"File saved as RTF!", L"Info", MB_OK);
                }
                else
                {
                    MessageBox(hDlg, L"Failed to save file!", L"Error", MB_ICONERROR);
                }
            }
            free((void*)ofn.lpstrFile);
            return TRUE;
        }

        // ---- Загрузить RTF ----
        case IDC_BUTTON_LOAD:
        {
            OPENFILENAME ofn = { 0 };
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFilter = L"RTF Files\0*.rtf\0All Files\0*.*\0";
            ofn.lpstrFile = (LPTSTR)malloc(MAX_PATH * sizeof(TCHAR));
            ofn.lpstrFile[0] = 0;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn))
            {
                HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    EDITSTREAM es = { 0 };
                    es.dwCookie = (DWORD_PTR)hFile;
                    es.pfnCallback = StreamInCallback;
                    // SF_RTF – загружаем как RTF
                    SendMessage(hRich, EM_STREAMIN, SF_RTF, (LPARAM)&es);
                    CloseHandle(hFile);
                    MessageBox(hDlg, L"File loaded as RTF!", L"Info", MB_OK);
                }
                else
                {
                    MessageBox(hDlg, L"Failed to load file!", L"Error", MB_ICONERROR);
                }
            }
            free((void*)ofn.lpstrFile);
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


/*

Инструкция по созданию диалога в Visual Studio
Создайте проект – пустой проект C++ (Win32) с именем, например, RichEditDemo. Убедитесь, что подсистема – Windows.

Добавьте файлы из шаблона выше.

Создайте ресурс диалога:

Правой кнопкой на проекте → Добавить → Ресурс → Dialog → Создать.

В свойствах диалога установите:

ID = IDD_MAIN_DIALOG (101).

Visible = True.

Style = Popup.

Border = Dialog Frame.

Caption (заголовок) оставьте пустым (установим в коде).

Разместите кнопки (Button) с ID, соответствующими resource.h:

IDC_BUTTON_BOLD (1002) – текст B

IDC_BUTTON_ITALIC (1003) – текст I

IDC_BUTTON_UNDERLINE (1004) – текст U

IDC_BUTTON_COLOR (1005) – текст Color

IDC_BUTTON_FONT (1006) – текст Font

IDC_BUTTON_SAVE (1007) – текст Save

IDC_BUTTON_LOAD (1008) – текст Load

Не добавляйте Rich Edit на диалог – мы создаём его программно. Просто оставьте место для него (например, прямоугольник размером 400x200 в нижней части).

Соберите и запустите – всё должно работать.

Почему мы создаём Rich Edit программно?
В редакторе ресурсов Visual Studio есть элемент Rich Edit 2.0 Control, но он не всегда корректно работает (особенно в старых проектах). Программное создание даёт полный контроль над параметрами и упрощает отладку. Кроме того, мы можем попробовать разные версии класса (RICHEDIT50W и RICHEDIT20W) для совместимости.

resource.h

// ================================================================
// resource.h – файл с числовыми идентификаторами всех ресурсов.
// Этот файл генерируется редактором ресурсов Visual Studio,
// но мы дополняем его своими определениями.
//
// ВСЕ ИДЕНТИФИКАТОРЫ ДОЛЖНЫ БЫТЬ УНИКАЛЬНЫМИ в пределах проекта.
// Если нужно добавить новый контрол – добавляем новый #define.
// ================================================================

#ifndef RESOURCE_H
#define RESOURCE_H

// --------------------------------------------
// Идентификатор главного диалогового окна
// --------------------------------------------
#define IDD_MAIN_DIALOG                 101

// --------------------------------------------
// Идентификаторы элементов управления
// --------------------------------------------
#define IDC_RICHEDIT                    1001   // Rich Edit контрол (создаётся программно)
#define IDC_BUTTON_BOLD                 1002   // кнопка "B" (жирный)
#define IDC_BUTTON_ITALIC               1003   // кнопка "I" (курсив)
#define IDC_BUTTON_UNDERLINE            1004   // кнопка "U" (подчёркнутый)
#define IDC_BUTTON_COLOR                1005   // кнопка "Color" (выбор цвета)
#define IDC_BUTTON_FONT                 1006   // кнопка "Font" (выбор шрифта)
#define IDC_BUTTON_SAVE                 1007   // кнопка "Save" (сохранить RTF)
#define IDC_BUTTON_LOAD                 1008   // кнопка "Load" (загрузить RTF)

// --------------------------------------------
// Следующие макросы используются редактором ресурсов
// для автоматического присвоения ID новым объектам.
// Их НЕ УДАЛЯТЬ и НЕ ИЗМЕНЯТЬ вручную.
// --------------------------------------------
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        103
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1009
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

#endif // RESOURCE_H



*/