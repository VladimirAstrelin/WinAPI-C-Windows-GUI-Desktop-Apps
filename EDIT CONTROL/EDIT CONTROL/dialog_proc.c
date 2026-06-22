// ================================================================
// dialog_proc.c – реализация диалоговой процедуры.
// Здесь мы обрабатываем сообщения от диалога и его элементов управления.
// Все контролы (поля ввода, кнопки) созданы в редакторе ресурсов,
// поэтому мы не создаём их в коде, а только получаем их дескрипторы.
// ================================================================

#include "dialog_proc.h"      // Чтобы компилятор знал прототип EditDemoProc
#include "resource.h"         // Идентификаторы контролов (IDC_EDIT_NAME, IDC_BTN_SHOW и т.д.)

// -----------------------------------------------------------------
// Глобальные (статические) переменные для хранения дескрипторов полей ввода.
// static означает, что они видны только внутри этого файла.
// Мы сохраняем HWND каждого поля, чтобы быстро обращаться к ним в коде.
// -----------------------------------------------------------------
static HWND g_hEditName = NULL;
static HWND g_hEditPassword = NULL;
static HWND g_hEditAge = NULL;
static HWND g_hEditComment = NULL;

// -----------------------------------------------------------------
// Вспомогательные функции, которые мы будем вызывать по нажатию кнопок.
// Они работают с содержимым полей, используя их идентификаторы.
// -----------------------------------------------------------------

// Функция "Show All" – читает текст из всех полей и показывает его в одном сообщении.
static void ShowAllTexts(HWND hwnd)
{
    // Буферы для хранения текста из каждого поля.
    // Размеры подобраны с запасом, чтобы вместить любой ввод пользователя.
    wchar_t name[256], password[256], age[64], comment[1024];
    wchar_t message[2048];    // Общий буфер для всего сообщения

    // GetDlgItemTextW – безопасно читает текст из указанного поля в буфер.
    // Параметры: (окно, ID контрола, буфер, размер буфера в символах).
    GetDlgItemTextW(hwnd, IDC_EDIT_NAME, name, 256);
    GetDlgItemTextW(hwnd, IDC_EDIT_PASSWORD, password, 256);
    GetDlgItemTextW(hwnd, IDC_EDIT_AGE, age, 64);
    GetDlgItemTextW(hwnd, IDC_EDIT_COMMENT, comment, 1024);

    // Форматируем строку с переносами строк (\r\n) для красивого отображения.
    wsprintfW(message,
        L"Name: %s\r\n"
        L"Password: %s\r\n"
        L"Age: %s\r\n"
        L"Comment: %s",
        name, password, age, comment);

    // Показываем MessageBox с полученными данными.
    MessageBoxW(hwnd, message, L"Current Input", MB_OK);
}

// Функция "Clear All" – очищает все поля ввода.
static void ClearAllTexts(HWND hwnd)
{
    // SetDlgItemTextW – устанавливает текст в указанном поле.
    // Передаём пустую строку L"", чтобы очистить поле.
    SetDlgItemTextW(hwnd, IDC_EDIT_NAME, L"");
    SetDlgItemTextW(hwnd, IDC_EDIT_PASSWORD, L"");
    SetDlgItemTextW(hwnd, IDC_EDIT_AGE, L"");
    SetDlgItemTextW(hwnd, IDC_EDIT_COMMENT, L"");
}

// Функция "Set Custom Text" – заполняет поля тестовыми данными для демонстрации.
static void SetCustomTexts(HWND hwnd)
{
    // Устанавливаем предопределённые строки в каждое поле.
    SetDlgItemTextW(hwnd, IDC_EDIT_NAME, L"John Doe");
    SetDlgItemTextW(hwnd, IDC_EDIT_PASSWORD, L"secret123");
    SetDlgItemTextW(hwnd, IDC_EDIT_AGE, L"30");
    SetDlgItemTextW(hwnd, IDC_EDIT_COMMENT, L"This is a sample comment.\nMulti-line text works too.");
}

// -----------------------------------------------------------------
// Главная диалоговая процедура.
// Windows вызывает эту функцию каждый раз, когда происходит какое-либо событие,
// связанное с нашим диалогом: создание, изменение размера, нажатие кнопки и т.д.
// -----------------------------------------------------------------
INT_PTR CALLBACK EditDemoProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Анализируем тип сообщения
    switch (uMsg)
    {
        // ------------------------------------------------------------
        // WM_INITDIALOG – самое первое сообщение, посылаемое диалогу перед его отображением.
        // Здесь мы инициализируем элементы: получаем дескрипторы, устанавливаем ограничения,
        // задаём начальный фокус.
        // ------------------------------------------------------------
        case WM_INITDIALOG:
        {
            // Получаем дескрипторы уже существующих контролов, созданных в редакторе ресурсов.
            // GetDlgItem – возвращает HWND контрола по его идентификатору.
            g_hEditName = GetDlgItem(hwnd, IDC_EDIT_NAME);
            g_hEditPassword = GetDlgItem(hwnd, IDC_EDIT_PASSWORD);
            g_hEditAge = GetDlgItem(hwnd, IDC_EDIT_AGE);
            g_hEditComment = GetDlgItem(hwnd, IDC_EDIT_COMMENT);

            // Устанавливаем ограничения на длину вводимого текста.
            // EM_LIMITTEXT – сообщение, которое ограничивает максимальное количество символов.
            // Параметры: wParam = максимальная длина (в символах), lParam = не используется.
            // Для поля "Name" – не более 20 символов.
            SendMessage(g_hEditName, EM_LIMITTEXT, 20, 0);
            // Для поля "Age" – не более 3 символов (достаточно для возраста до 999).
            SendMessage(g_hEditAge, EM_LIMITTEXT, 3, 0);

            // Устанавливаем фокус на поле "Name", чтобы пользователь сразу мог начать ввод.
            SetFocus(g_hEditName);

            // Возвращаем FALSE, чтобы система не переназначала фокус автоматически.
            // Мы уже установили фокус сами, поэтому FALSE даёт понять системе, что мы сделали это.
            return FALSE;
        }

    // ------------------------------------------------------------
    // WM_COMMAND – приходит, когда пользователь взаимодействует с элементом:
    // нажимает кнопку, изменяет текст в поле, выбирает пункт меню и т.п.
    // ------------------------------------------------------------
        case WM_COMMAND:
        {
            // LOWORD(wParam) содержит идентификатор элемента, вызвавшего событие.
            // HIWORD(wParam) содержит код уведомления (например, BN_CLICKED для кнопок, EN_CHANGE для полей).
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            // Обрабатываем команду в зависимости от идентификатора.
            switch (id)
            {
                // IDOK и IDCANCEL – стандартные идентификаторы для кнопок OK и Cancel.
                // В нашем диалоге они закрывают окно.
            case IDOK:
            case IDCANCEL:
                EndDialog(hwnd, FALSE);   // Закрываем модальное окно
                return TRUE;

                // Нажатие на кнопку "Show All"
            case IDC_BTN_SHOW:
                ShowAllTexts(hwnd);
                return TRUE;

                // Нажатие на кнопку "Clear All"
            case IDC_BTN_CLEAR:
                ClearAllTexts(hwnd);
                return TRUE;

                // Нажатие на кнопку "Set Custom Text"
            case IDC_BTN_SET_TEXT:
                SetCustomTexts(hwnd);
                return TRUE;

                // Уведомления от полей ввода (когда пользователь изменяет текст).
                // Мы обрабатываем все четыре поля одинаково – просто меняем заголовок окна.
                // Это только пример реакции на изменение текста.
            case IDC_EDIT_NAME:
            case IDC_EDIT_PASSWORD:
            case IDC_EDIT_AGE:
            case IDC_EDIT_COMMENT:
                // Проверяем, что уведомление именно об изменении текста (EN_CHANGE).
                if (code == EN_CHANGE)
                {
                    // Меняем заголовок окна, чтобы показать, что событие произошло.
                    SetWindowTextW(hwnd, L"Edit Demo - Text changed");
                    return TRUE;
                }
                break;   // Если это другое уведомление, просто выходим из case
            }
            break;   // Выход из switch по командам
        }
    }

    // Для всех неперехваченных сообщений возвращаем FALSE – система обработает их сама.
    return FALSE;
}