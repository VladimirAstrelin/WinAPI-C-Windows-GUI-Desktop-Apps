// ================================================================
// dialog_proc.c – реализация диалоговых процедур.
// В этом файле мы управляем деревом: создаём узлы, обрабатываем
// выбор узла, показываем соответствующие страницы справа.
// ================================================================

#include "dialog_proc.h"      // Прототипы функций
#include "resource.h"         // Идентификаторы ресурсов (IDD_PAGE_GENERAL, IDC_TREE и т.д.)
#include <windows.h>
#include <commctrl.h>         // Для Tree Control (константы, структуры)

// -----------------------------------------------------------------
// Глобальные (статические) переменные для хранения дескрипторов окон.
// static означает, что они видны только внутри этого файла.
// -----------------------------------------------------------------

// Дескриптор самого Tree Control.
static HWND g_hTree = NULL;

// Массив дескрипторов для страниц (дочерних диалогов).
// Всего у нас 3 страницы, поэтому массив на 3 элемента.
static HWND g_hPages[3] = { 0 };

// Индекс текущей активной страницы (-1 означает, что ничего не выбрано).
static int g_currentPage = -1;

// Массив идентификаторов ресурсов для каждой страницы.
// IDD_PAGE_GENERAL, IDD_PAGE_DISPLAY, IDD_PAGE_ADVANCED – определены в resource.h.
// const означает, что массив нельзя изменить.
static const int pageIDs[] = { IDD_PAGE_GENERAL, IDD_PAGE_DISPLAY, IDD_PAGE_ADVANCED };

// Массив текстов (заголовков) для узлов дерева.
static const wchar_t* pageTitles[] = { L"General", L"Display", L"Advanced" };

// Количество страниц (определяем как константу, чтобы легко менять).
#define NUM_PAGES 3

// -----------------------------------------------------------------
// Процедура для дочерних страниц
// Эта функция вызывается Windows при каждом событии, связанном с дочерним диалогом.
// Например, когда пользователь нажимает кнопку на странице General.
// -----------------------------------------------------------------
INT_PTR CALLBACK PageChildProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Анализируем тип сообщения
    switch (uMsg)
    {
        // WM_COMMAND – приходит, когда пользователь взаимодействует с элементом на странице:
        // нажал кнопку, изменил текст в поле и т.д.
    case WM_COMMAND:
    {
        // LOWORD(wParam) – идентификатор элемента (например, IDC_PAGE_BTN_SHOW)
        // HIWORD(wParam) – код уведомления (например, BN_CLICKED для кнопок)
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        // Если нажата кнопка "Show" на первой странице (IDC_PAGE_BTN_SHOW)
        // и это уведомление именно о нажатии (BN_CLICKED)
        if (id == IDC_PAGE_BTN_SHOW && code == BN_CLICKED)
        {
            // Получаем дескриптор поля ввода на этой же странице.
            // GetDlgItem – возвращает HWND контрола по его идентификатору.
            HWND hEdit = GetDlgItem(hwnd, IDC_PAGE_EDIT);

            // Буфер для текста (256 символов достаточно).
            wchar_t buffer[256];

            // Читаем текст из поля ввода.
            // GetWindowTextW – копирует текст из окна в буфер.
            GetWindowTextW(hEdit, buffer, 256);

            // Показываем MessageBox с введённым текстом.
            // Параметры: родительское окно, текст, заголовок, кнопки.
            MessageBoxW(hwnd, buffer, L"Input from General", MB_OK);

            // Сообщение обработано – возвращаем TRUE.
            return TRUE;
        }
        // Здесь можно добавить обработку других элементов на других страницах.
        // Например, для страницы Display можно обработать CheckBox и RadioButton.
        break;
    }
    }
    // Если сообщение не было обработано, возвращаем FALSE – система обработает его сама.
    return FALSE;
}

// -----------------------------------------------------------------
// Вспомогательная функция для добавления узла в дерево.
// Параметры:
//   hTree     – дескриптор Tree Control
//   hParent   – дескриптор родительского узла (TVI_ROOT для корневого уровня)
//   text      – текст, который будет отображаться в узле
//   pageIndex – индекс страницы, связанной с этим узлом (-1, если страницы нет)
// Возвращает HTREEITEM – дескриптор созданного узла.
// -----------------------------------------------------------------
static HTREEITEM AddTreeItem(HWND hTree, HTREEITEM hParent, LPCWSTR text, int pageIndex)
{
    // Структура TVINSERTSTRUCT содержит информацию для вставки узла.
    TVINSERTSTRUCT tvis = { 0 };

    // hParent – родительский узел (куда добавляем).
    tvis.hParent = hParent;

    // hInsertAfter – позиция вставки: TVI_LAST означает "в конец списка".
    tvis.hInsertAfter = TVI_LAST;

    // mask – говорит системе, какие поля структуры мы заполняем.
    // TVIF_TEXT – текст узла, TVIF_PARAM – дополнительный параметр (мы храним индекс страницы).
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;

    // pszext – указатель на строку с текстом узла.
    // Приводим к типу LPWSTR, так как функция ожидает неконстантный указатель.
    tvis.item.pszText = (LPWSTR)text;

    // lParam – дополнительное 32-битное значение, которое мы можем хранить для узла.
    // Мы сохраняем индекс страницы (pageIndex), чтобы знать, какую страницу показывать
    // при выборе этого узла.
    tvis.item.lParam = (LPARAM)pageIndex;

    // Отправляем сообщение TVM_INSERTITEM в Tree Control.
    // Параметры: 0 – не используется, (LPARAM)&tvis – указатель на структуру.
    // Функция возвращает HTREEITEM – дескриптор нового узла.
    return (HTREEITEM)SendMessage(hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
}

// -----------------------------------------------------------------
// Главная диалоговая процедура – обрабатывает сообщения основного окна.
// -----------------------------------------------------------------
INT_PTR CALLBACK MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Анализируем тип сообщения
    switch (uMsg)
    {
        // -----------------------------------------------------------------
        // WM_INITDIALOG – самое первое сообщение, посылаемое диалогу перед отображением.
        // Здесь мы создаём узлы дерева, страницы, устанавливаем начальное состояние.
        // -----------------------------------------------------------------
    case WM_INITDIALOG:
    {
        // Получаем дескриптор Tree Control по его ID (IDC_TREE).
        // GetDlgItem – возвращает HWND дочернего элемента по идентификатору.
        g_hTree = GetDlgItem(hwnd, IDC_TREE);

        // -----------------------------------------------------------------
        // Вычисляем область для страниц.
        // Дерево занимает левую часть окна, страницы – правую.
        // -----------------------------------------------------------------

        // Получаем клиентскую область главного диалога (прямоугольник, ограничивающий внутреннюю часть окна).
        RECT rcClient, rcTree;
        GetClientRect(hwnd, &rcClient);

        // Получаем размеры Tree Control в экранных координатах.
        GetWindowRect(g_hTree, &rcTree);

        // Преобразуем экранные координаты дерева в клиентские (относительно главного диалога).
        // MapWindowPoints – преобразует координаты из одного окна в другое.
        // HWND_DESKTOP – дескриптор рабочего стола (экранные координаты).
        // hwnd – целевое окно (главный диалог).
        // (POINT*)&rcTree – указатель на прямоугольник (преобразуются все 4 точки).
        // 2 – количество точек (left, top, right, bottom – это 2 точки).
        MapWindowPoints(HWND_DESKTOP, hwnd, (POINT*)&rcTree, 2);

        // Вычисляем ширину дерева.
        int treeWidth = rcTree.right - rcTree.left;

        // Область для страниц – всё остальное справа от дерева с небольшими отступами.
        RECT rcPageArea;
        rcPageArea.left = treeWidth + 5;           // отступ справа от дерева
        rcPageArea.top = 5;                        // отступ сверху
        rcPageArea.right = rcClient.right - 5;     // отступ справа
        rcPageArea.bottom = rcClient.bottom - 5;   // отступ снизу

        // -----------------------------------------------------------------
        // Создаём дочерние диалоги-страницы.
        // -----------------------------------------------------------------

        // Получаем дескриптор экземпляра приложения (HINSTANCE) из окна.
        // Это нужно для создания дочерних диалогов через CreateDialog.
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

        // В цикле создаём все страницы.
        for (int i = 0; i < NUM_PAGES; i++)
        {
            // CreateDialog – создаёт диалог из ресурса, но не показывает его сразу.
            // Параметры:
            //   hInst                  – экземпляр приложения
            //   MAKEINTRESOURCE(pageIDs[i]) – ID ресурса страницы (IDD_PAGE_GENERAL и т.д.)
            //   hwnd                   – родительское окно (главный диалог)
            //   PageChildProc          – процедура, которая будет обрабатывать сообщения от этой страницы
            g_hPages[i] = CreateDialog(
                hInst,
                MAKEINTRESOURCE(pageIDs[i]),
                hwnd,          // родитель – главный диалог
                PageChildProc
            );

            // Позиционируем страницу внутри области, вычисленной ранее (rcPageArea).
            // SetWindowPos – изменяет положение и размер окна.
            // Параметры:
            //   g_hPages[i]           – дескриптор окна
            //   NULL                  – не меняем Z-порядок
            //   rcPageArea.left, rcPageArea.top – координаты верхнего левого угла
            //   rcPageArea.right - rcPageArea.left – ширина
            //   rcPageArea.bottom - rcPageArea.top – высота
            //   SWP_NOZORDER          – не изменяем Z-порядок
            SetWindowPos(g_hPages[i], NULL,
                rcPageArea.left, rcPageArea.top,
                rcPageArea.right - rcPageArea.left,
                rcPageArea.bottom - rcPageArea.top,
                SWP_NOZORDER);

            // Все страницы изначально скрыты (покажем только выбранную позже).
            ShowWindow(g_hPages[i], SW_HIDE);
        }

        // -----------------------------------------------------------------
        // Заполняем дерево иерархией.
        // -----------------------------------------------------------------

        // Создаём корневой узел "Settings". Он не связан ни с какой страницей (pageIndex = -1).
        // TVI_ROOT – специальный псевдо-узел, обозначающий корень дерева (верхний уровень).
        HTREEITEM hRoot = AddTreeItem(g_hTree, TVI_ROOT, L"Settings", -1);

        // Добавляем три дочерних узла, каждый со своим индексом страницы (0, 1, 2).
        AddTreeItem(g_hTree, hRoot, L"General", 0);
        AddTreeItem(g_hTree, hRoot, L"Display", 1);
        AddTreeItem(g_hTree, hRoot, L"Advanced", 2);

        // Раскрываем корневой узел, чтобы дочерние узлы были видны.
        // TVM_EXPAND – сообщение для раскрытия/сворачивания узла.
        // TVE_EXPAND – флаг, означающий "раскрыть".
        SendMessage(g_hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)hRoot);

        // -----------------------------------------------------------------
        // Выбираем первый узел (General) и показываем его страницу.
        // -----------------------------------------------------------------

        // Находим первый дочерний узел корня.
        // TVM_GETNEXTITEM – получает следующий узел в дереве.
        // TVGN_CHILD – флаг: получить первый дочерний узел.
        HTREEITEM hFirst = (HTREEITEM)SendMessage(g_hTree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hRoot);

        if (hFirst)
        {
            // Устанавливаем выделение на этот узел.
            // TVM_SELECTITEM – выделяет узел.
            // TVGN_CARET – флаг: установить узел как текущий (с фокусом).
            SendMessage(g_hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hFirst);

            // Получаем индекс страницы из lParam узла.
            TVITEM tvi = { 0 };
            tvi.mask = TVIF_PARAM;   // говорим, что нас интересует только поле lParam
            tvi.hItem = hFirst;      // указываем узел
            SendMessage(g_hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

            int pageIdx = (int)tvi.lParam;

            // Если индекс корректный, показываем соответствующую страницу.
            if (pageIdx >= 0 && pageIdx < NUM_PAGES)
            {
                ShowWindow(g_hPages[pageIdx], SW_SHOW);
                g_currentPage = pageIdx;
            }
        }

        // Принудительно перерисовываем дерево, чтобы избежать артефактов.
        // RedrawWindow – перерисовывает всё окно или его часть.
        RedrawWindow(g_hTree, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

        // Возвращаем TRUE, чтобы система установила фокус на первый элемент управления.
        return TRUE;
    }

    // -----------------------------------------------------------------
    // WM_SIZE – сообщение, когда окно меняет размер (при создании, растягивании и т.д.).
    // Мы должны обновить размеры дерева и страниц, чтобы они всегда соответствовали окну.
    // -----------------------------------------------------------------
    case WM_SIZE:
    {
        // Проверяем, что Tree Control создан (дескриптор не NULL).
        if (g_hTree)
        {
            // Получаем текущие размеры клиентской области главного диалога.
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);

            // Устанавливаем новую высоту дерева (она должна быть равна высоте окна),
            // ширину оставляем фиксированной (150 пикселей).
            // SetWindowPos – изменяет размер и/или позицию.
            // Параметры: NULL – не меняем Z-порядок, 0,0 – позиция (не меняем), 150 – ширина, rcClient.bottom – высота.
            // SWP_NOMOVE – не перемещать окно (оставляем текущую позицию).
            SetWindowPos(g_hTree, NULL, 0, 0, 150, rcClient.bottom, SWP_NOMOVE | SWP_NOZORDER);

            // Получаем обновлённые размеры дерева для вычисления области страниц.
            RECT rcTree;
            GetWindowRect(g_hTree, &rcTree);
            MapWindowPoints(HWND_DESKTOP, hwnd, (POINT*)&rcTree, 2);
            int treeWidth = rcTree.right - rcTree.left;

            // Область для страниц – справа от дерева.
            RECT rcPageArea;
            rcPageArea.left = treeWidth + 5;
            rcPageArea.top = 5;
            rcPageArea.right = rcClient.right - 5;
            rcPageArea.bottom = rcClient.bottom - 5;

            // Обновляем позицию и размер всех страниц.
            for (int i = 0; i < NUM_PAGES; i++)
            {
                if (g_hPages[i])
                {
                    SetWindowPos(g_hPages[i], NULL,
                        rcPageArea.left, rcPageArea.top,
                        rcPageArea.right - rcPageArea.left,
                        rcPageArea.bottom - rcPageArea.top,
                        SWP_NOZORDER);
                }
            }
        }
        return TRUE;
    }

    // -----------------------------------------------------------------
    // WM_NOTIFY – сообщение, которое приходит от дочерних элементов управления,
    // когда они хотят уведомить родителя о каких-то событиях.
    // В нашем случае Tree Control отправляет уведомление TVN_SELCHANGED,
    // когда пользователь выбирает другой узел.
    // -----------------------------------------------------------------
    case WM_NOTIFY:
    {
        // LPNMHDR – указатель на структуру NMHDR, которая содержит общую информацию об уведомлении.
        NMHDR* pnmh = (NMHDR*)lParam;

        // Проверяем, что уведомление пришло от нашего Tree Control (IDC_TREE)
        // и это событие TVN_SELCHANGED (выделенный узел изменился).
        if (pnmh->idFrom == IDC_TREE && pnmh->code == TVN_SELCHANGED)
        {
            // Структура NMTREEVIEW содержит подробную информацию о событии.
            NMTREEVIEW* pnmtv = (NMTREEVIEW*)lParam;

            // Получаем дескриптор нового выбранного узла.
            HTREEITEM hSelected = pnmtv->itemNew.hItem;

            if (hSelected)
            {
                // Получаем индекс страницы из lParam узла.
                TVITEM tvi = { 0 };
                tvi.mask = TVIF_PARAM;
                tvi.hItem = hSelected;
                SendMessage(g_hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

                int newPageIdx = (int)tvi.lParam;

                // Если индекс валидный и отличается от текущей страницы, переключаем.
                if (newPageIdx >= 0 && newPageIdx < NUM_PAGES && newPageIdx != g_currentPage)
                {
                    // Скрываем старую страницу, если она была показана.
                    if (g_currentPage != -1)
                        ShowWindow(g_hPages[g_currentPage], SW_HIDE);

                    // Показываем новую страницу.
                    ShowWindow(g_hPages[newPageIdx], SW_SHOW);

                    // Обновляем текущий индекс.
                    g_currentPage = newPageIdx;
                }
            }

            // Сообщение обработано – возвращаем TRUE.
            return TRUE;
        }
        break;
    }

    // -----------------------------------------------------------------
    // WM_COMMAND – обрабатываем команды от кнопок, расположенных вне страниц.
    // Например, кнопка "Close" с ID IDOK.
    // -----------------------------------------------------------------
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        // Если нажата кнопка OK (закрыть) или Cancel – закрываем диалог.
        if (id == IDOK || id == IDCANCEL)
        {
            // EndDialog – закрывает модальное диалоговое окно и возвращает управление в WinMain.
            // Второй параметр – значение, которое будет возвращено из DialogBox (обычно не используется).
            EndDialog(hwnd, FALSE);
            return TRUE;
        }
        break;
    }
    }
    // Для всех неперехваченных сообщений возвращаем FALSE – система обработает их сама.
    return FALSE;
}