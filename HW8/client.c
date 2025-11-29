// client.c — клиент под Windows.
// - подключается к уже созданной сервером разделяемой памяти и семафорам;
// - генерирует случайные числа и пишет их в общий буфер;
// - Ctrl+C в любом процессе приводит к корректному завершению обоих.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "shared.h"

static volatile LONG g_stop = 0;   // локальный флаг завершения
static struct Shared *g_sh = NULL;
static HANDLE g_map = NULL;
static HANDLE g_sem_space = NULL;
static HANDLE g_sem_data  = NULL;

// Обработчик Ctrl+C / закрытия консоли.
// Логика такая же, как у сервера: ставим флаги и будим всех.
static BOOL WINAPI console_handler(DWORD type) {
    switch (type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            g_stop = 1;
            if (g_sh) {
                g_sh->stop = 1;
            }
            if (g_sem_space) {
                ReleaseSemaphore(g_sem_space, 1, NULL);
            }
            if (g_sem_data) {
                ReleaseSemaphore(g_sem_data, 1, NULL);
            }
            return TRUE;
        default:
            return FALSE;
    }
}

int main(void) {
    if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
        fprintf(stderr, "client: SetConsoleCtrlHandler failed\n");
        return 1;
    }

    // 1) Подключаемся к уже созданному сервером объекту
    //    разделяемой памяти по имени SHM_NAME.
    g_map = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS, // хотим читать и писать
        FALSE,               // дескриптор не наследуется дочерними процессами
        SHM_NAME
    );
    if (!g_map) {
        fprintf(stderr,
                "client: OpenFileMapping failed (start server first, err=%lu)\n",
                GetLastError());
        return 1;
    }

    g_sh = (struct Shared *)MapViewOfFile(
        g_map,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(struct Shared)
    );
    if (!g_sh) {
        fprintf(stderr, "client: MapViewOfFile failed (err=%lu)\n", GetLastError());
        CloseHandle(g_map);
        return 1;
    }

    // 2) Открываем существующие семафоры по их именам.
    g_sem_space = OpenSemaphoreA(
        SEMAPHORE_ALL_ACCESS, // достаточно полный набор прав
        FALSE,
        SEM_SPACE_NAME
    );
    g_sem_data = OpenSemaphoreA(
        SEMAPHORE_ALL_ACCESS,
        FALSE,
        SEM_DATA_NAME
    );
    if (!g_sem_space || !g_sem_data) {
        fprintf(stderr, "client: OpenSemaphore failed (err=%lu)\n", GetLastError());
        if (g_sem_space) {
            CloseHandle(g_sem_space);
        }
        if (g_sem_data) {
            CloseHandle(g_sem_data);
        }
        UnmapViewOfFile(g_sh);
        CloseHandle(g_map);
        return 1;
    }

    // Инициализируем генератор случайных чисел:
    // смешиваем время и PID процесса, чтобы клиенты (если вдруг будет несколько)
    // генерировали разные последовательности.
    srand((unsigned)time(NULL) ^ (unsigned)GetCurrentProcessId());

    puts("client: running (Ctrl+C to stop)");
    fflush(stdout);

    // Основной цикл клиента:
    // ждём, пока буфер освободится, записываем число, сигналим серверу.
    while (!g_stop) {
        if (g_sh->stop) {
            // Сервер уже решил завершиться — выходим.
            break;
        }

        // Ждём, пока семафор "место свободно" станет > 0.
        DWORD wait_res = WaitForSingleObject(g_sem_space, INFINITE);
        if (wait_res != WAIT_OBJECT_0) {
            // Разбудили не по обычному пути, проверим флаги и продолжим/выйдем.
            continue;
        }

        if (g_sh->stop || g_stop) {
            // Пока ждали семафор, кто-то мог попросить остановиться.
            ReleaseSemaphore(g_sem_space, 1, NULL);
            break;
        }

        // Генерируем число и записываем его в разделяемую память.
        int val = rand() % 100; // здесь тот же диапазон, что был на семинаре (0..99)
        g_sh->value = val;

        // Сообщаем серверу, что новое число лежит в буфере.
        ReleaseSemaphore(g_sem_data, 1, NULL);

        // Небольшая задержка, чтобы вывод сервера не был слишком "жидким".
        Sleep(300); // миллисекунды
    }

    // Мягкий выход: ещё раз выставим stop и разбудим ожидания серверной стороны.
    g_sh->stop = 1;
    ReleaseSemaphore(g_sem_data, 1, NULL);
    ReleaseSemaphore(g_sem_space, 1, NULL);

    // Освобождаем все свои дескрипторы и отображения.
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

    puts("client: bye");
    return 0;
}
