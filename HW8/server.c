// server.c — сервер под Windows.
// - создаёт разделяемую память и два семафора;
// - читает числа от клиента и выводит их в stdout;
// - по Ctrl+C в любом процессе выставляется флаг stop,
//   после чего оба процесса корректно завершатся.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "shared.h"

// Глобальные объекты, которыми пользуемся и из основного кода, и из обработчика.
static volatile LONG g_stop = 0;   // локальный флаг остановки (внутри процесса)
static struct Shared *g_sh = NULL; // указатель на область в разделяемой памяти
static HANDLE g_map = NULL;        // дескриптор объекта "file mapping"
static HANDLE g_sem_space = NULL;  // семафор "место свободно"
static HANDLE g_sem_data  = NULL;  // семафор "данные готовы"

// Обработчик событий консоли (Ctrl+C, закрытие окна и т.п.).
// Никакой тяжёлой логики тут не делаем — только выставляем флаги
// и будим второй процесс через семафоры.
static BOOL WINAPI console_handler(DWORD type) {
    switch (type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            g_stop = 1;

            // Сообщаем клиенту через разделяемую память, что надо завершаться.
            if (g_sh) {
                g_sh->stop = 1;
            }

            // Будим оба ожидания: и серверное, и клиентское.
            if (g_sem_space) {
                ReleaseSemaphore(g_sem_space, 1, NULL);
            }
            if (g_sem_data) {
                ReleaseSemaphore(g_sem_data, 1, NULL);
            }
            return TRUE; // событие обработано
        default:
            return FALSE;
    }
}

int main(void) {
    // Подписываемся на Ctrl+C и закрытие консоли.
    if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
        fprintf(stderr, "server: SetConsoleCtrlHandler failed\n");
        return 1;
    }

    // 1) Создаём объект разделяемой памяти.
    // Используем файл подкачки (INVALID_HANDLE_VALUE),
    // размер — ровно структура Shared.
    g_map = CreateFileMappingA(
        INVALID_HANDLE_VALUE,        // не настоящий файл, а память
        NULL,                        // стандартные права безопасности
        PAGE_READWRITE,              // чтение + запись
        0,                           // старшее слово размера
        (DWORD)sizeof(struct Shared),// младшее слово размера
        SHM_NAME                     // имя объекта для обоих процессов
    );
    if (!g_map) {
        fprintf(stderr, "server: CreateFileMapping failed (err=%lu)\n", GetLastError());
        return 1;
    }

    // Если объект уже существовал, значит второй сервер пытался запуститься.
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "server: shared memory already exists (another server running?)\n");
        CloseHandle(g_map);
        return 1;
    }

    // 2) Отображаем этот объект в адресное пространство процесса.
    g_sh = (struct Shared *)MapViewOfFile(
        g_map,
        FILE_MAP_ALL_ACCESS, // хотим и читать, и писать
        0, 0,
        sizeof(struct Shared)
    );
    if (!g_sh) {
        fprintf(stderr, "server: MapViewOfFile failed (err=%lu)\n", GetLastError());
        CloseHandle(g_map);
        return 1;
    }

    // Инициализируем содержимое общей памяти.
    g_sh->stop  = 0;
    g_sh->value = 0;

    // 3) Создаём два именованных семафора.

    // Семафор "место свободно": начальное значение 1 ⇒ буфер пуст,
    // максимальное значение тоже 1 (по сути булевский флажок).
    g_sem_space = CreateSemaphoreA(
        NULL, // стандартные атрибуты безопасности
        1,    // initialCount
        1,    // maximumCount
        SEM_SPACE_NAME
    );
    if (!g_sem_space) {
        fprintf(stderr, "server: CreateSemaphore(space) failed (err=%lu)\n", GetLastError());
        UnmapViewOfFile(g_sh);
        CloseHandle(g_map);
        return 1;
    }

    // Семафор "данные готовы": начальное значение 0 ⇒ пока данных нет.
    g_sem_data = CreateSemaphoreA(
        NULL,
        0,    // пока ничего не записано
        1,
        SEM_DATA_NAME
    );
    if (!g_sem_data) {
        fprintf(stderr, "server: CreateSemaphore(data) failed (err=%lu)\n", GetLastError());
        CloseHandle(g_sem_space);
        UnmapViewOfFile(g_sh);
        CloseHandle(g_map);
        return 1;
    }

    puts("server: ready (Ctrl+C to stop)");
    fflush(stdout);

    // Основной цикл сервера:
    // ждём семафор "данные готовы", читаем число, печатаем его.
    while (!g_stop) {
        DWORD wait_res = WaitForSingleObject(g_sem_data, INFINITE);
        if (wait_res != WAIT_OBJECT_0) {
            // Если нас разбудили по ошибке/закрытию — просто проверим флаги.
            continue;
        }

        // Проверяем, не попросил ли клиент или обработчик завершиться.
        if (g_sh->stop || g_stop) {
            break;
        }

        int x = g_sh->value;
        printf("server: %d\n", x);
        fflush(stdout);

        // Освобождаем "место" для следующего числа.
        ReleaseSemaphore(g_sem_space, 1, NULL);
    }

    // На всякий случай ещё раз выставляем stop в общей памяти —
    // вдруг клиент просыпается после нас.
    g_sh->stop = 1;
    ReleaseSemaphore(g_sem_space, 1, NULL);
    ReleaseSemaphore(g_sem_data, 1, NULL);

    // Корректное освобождение всех ресурсов сервера.
    if (g_sem_space) {
        CloseHandle(g_sem_space);
    }
    if (g_sem_data) {
        CloseHandle(g_sem_data);
    }
    if (g_sh) {
        UnmapViewOfFile(g_sh);
    }
    if (g_map) {
        CloseHandle(g_map);
    }

    puts("server: bye");
    return 0;
}
