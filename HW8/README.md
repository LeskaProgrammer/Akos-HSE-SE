# Отчет по Домашнему заданию №8

**Студент:** Симонов Алексей

## Постановка задачи
Разработать комплект из двух независимых программ (клиент и сервер), взаимодействующих через механизм разделяемой памяти (Shared Memory) с использованием Windows API.

1. **Клиент**: генерирует случайные числа и передает их серверу.
2. **Сервер**: считывает числа из общей памяти и выводит в консоль.
3. **Условие**: корректное завершение обоих процессов при получении сигнала прерывания (Ctrl+C) в любом из терминалов.

## Описание реализации

Для решения задачи использованы системные вызовы Windows:

* **Shared Memory (`CreateFileMapping`, `MapViewOfFile`)**: 
    Создается именованный объект в памяти, доступный обоим процессам. В нем хранится структура с данными и флагом остановки.
* **Семафоры (`CreateSemaphore`)**: 
    Используются для синхронизации записи и чтения, чтобы избежать состояния гонки:
    * `SEM_SPACE` (начальное 1): разрешает запись клиенту.
    * `SEM_DATA` (начальное 0): разрешает чтение серверу.
* **Обработка сигналов (`SetConsoleCtrlHandler`)**: 
    Перехватывает нажатие `Ctrl+C`. При событии процесс выставляет флаг остановки в разделяемой памяти и «буддет» другой процесс через семафоры, чтобы корректно освободить ресурсы (закрыть хендлы и отмонтировать память).

## Результаты работы программы

Ниже приведен протокол работы системы. Сначала был запущен сервер, затем клиент. После передачи нескольких чисел на клиенте был нажат `Ctrl+C`.

### Терминал 1 (Сервер)
Сервер ожидал подключения, выводил полученные числа, а после сигнала от клиента корректно завершил работу.

```text
> server.exe
server: ready (Ctrl+C to stop)
server: 41
server: 85
server: 7
server: 63
server: 29
server: 92
server: 0
server: bye
```

### Терминал 2 (Клиент)
Клиент подключился, начал генерировать данные. Работа была прервана пользователем.

```text
> client.exe
client: running (Ctrl+C to stop)
client: bye
```

## Исходный код

### shared.h
Общие константы и структура данных.

```c
#pragma once

#define SHM_NAME        "akos7_shm_Simonov"
#define SEM_SPACE_NAME  "akos7_sem_space_Simonov"
#define SEM_DATA_NAME   "akos7_sem_data_Simonov"

struct Shared {
    int stop;   // Флаг остановки
    int value;  // Передаваемое число
};
```

### server.c
Код сервера (читатель).

```c
#include <windows.h>
#include <stdio.h>
#include "shared.h"

static volatile LONG g_stop = 0;
static struct Shared *g_sh = NULL;
static HANDLE g_map = NULL;
static HANDLE g_sem_space = NULL;
static HANDLE g_sem_data  = NULL;

// Обработчик Ctrl+C
static BOOL WINAPI console_handler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_CLOSE_EVENT) {
        g_stop = 1;
        // Сообщаем клиенту об остановке и будим его
        if (g_sh) g_sh->stop = 1;
        if (g_sem_space) ReleaseSemaphore(g_sem_space, 1, NULL);
        if (g_sem_data) ReleaseSemaphore(g_sem_data, 1, NULL);
        return TRUE;
    }
    return FALSE;
}

int main(void) {
    SetConsoleCtrlHandler(console_handler, TRUE);

    // Создание разделяемой памяти
    g_map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct Shared), SHM_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "server: shared memory already exists\n");
        return 1;
    }
    g_sh = (struct Shared *)MapViewOfFile(g_map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(struct Shared));
    
    // Инициализация памяти
    g_sh->stop = 0;
    g_sh->value = 0;

    // Создание семафоров
    g_sem_space = CreateSemaphoreA(NULL, 1, 1, SEM_SPACE_NAME);
    g_sem_data = CreateSemaphoreA(NULL, 0, 1, SEM_DATA_NAME);

    puts("server: ready (Ctrl+C to stop)");

    // Основной цикл чтения
    while (!g_stop) {
        WaitForSingleObject(g_sem_data, INFINITE);
        
        if (g_sh->stop || g_stop) break;

        printf("server: %d\n", g_sh->value);
        
        // Освобождаем место для следующей записи
        ReleaseSemaphore(g_sem_space, 1, NULL);
    }

    // Завершение работы
    g_sh->stop = 1;
    ReleaseSemaphore(g_sem_space, 1, NULL); // На случай если клиент ждет места
    ReleaseSemaphore(g_sem_data, 1, NULL);
    
    CloseHandle(g_sem_space);
    CloseHandle(g_sem_data);
    UnmapViewOfFile(g_sh);
    CloseHandle(g_map);

    puts("server: bye");
    return 0;
}
```

### client.c
Код клиента (писатель).

```c
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "shared.h"

static volatile LONG g_stop = 0;
static struct Shared *g_sh = NULL;
static HANDLE g_map = NULL;
static HANDLE g_sem_space = NULL;
static HANDLE g_sem_data  = NULL;

// Обработчик Ctrl+C
static BOOL WINAPI console_handler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_CLOSE_EVENT) {
        g_stop = 1;
        if (g_sh) g_sh->stop = 1;
        if (g_sem_space) ReleaseSemaphore(g_sem_space, 1, NULL);
        if (g_sem_data) ReleaseSemaphore(g_sem_data, 1, NULL);
        return TRUE;
    }
    return FALSE;
}

int main(void) {
    SetConsoleCtrlHandler(console_handler, TRUE);

    // Подключение к существующей памяти
    g_map = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
    if (!g_map) {
        fprintf(stderr, "client: start server first\n");
        return 1;
    }
    g_sh = (struct Shared *)MapViewOfFile(g_map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(struct Shared));
    
    // Подключение к семафорам
    g_sem_space = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, SEM_SPACE_NAME);
    g_sem_data = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, SEM_DATA_NAME);

    srand((unsigned)time(NULL) ^ (unsigned)GetCurrentProcessId());
    puts("client: running (Ctrl+C to stop)");

    // Основной цикл записи
    while (!g_stop) {
        if (g_sh->stop) break;
        
        // Ждем свободного места
        WaitForSingleObject(g_sem_space, INFINITE);
        
        if (g_sh->stop || g_stop) {
            ReleaseSemaphore(g_sem_space, 1, NULL);
            break;
        }

        // Записываем данные
        g_sh->value = rand() % 100;
        
        // Сигнализируем серверу о данных
        ReleaseSemaphore(g_sem_data, 1, NULL);
        
        Sleep(300); // Имитация работы
    }

    // Завершение работы
    g_sh->stop = 1;
    ReleaseSemaphore(g_sem_data, 1, NULL);
    ReleaseSemaphore(g_sem_space, 1, NULL);

    CloseHandle(g_sem_space);
    CloseHandle(g_sem_data);
    UnmapViewOfFile(g_sh);
    CloseHandle(g_map);

    puts("client: bye");
    return 0;
}
```