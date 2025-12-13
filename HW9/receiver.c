

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

// --- КОНФИГУРАЦИЯ (Должна совпадать с Sender) ---
#define SIG_BIT_0   SIGUSR1
#define SIG_BIT_1   SIGUSR2
#define SIG_ACK     SIGUSR1

void fatal_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    printf("=== ПРИЕМНИК (RECEIVER) ===\n");
    printf("Мой PID: %d\n", getpid());

    // 1. Настройка маски сигналов
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) fatal_error("sigprocmask");

    pid_t tx_pid;
    printf("Введите PID передатчика: ");
    if (scanf("%d", &tx_pid) != 1) fatal_error("Некорректный PID");

    printf("[Связь] Ожидание подключения...\n");

    siginfo_t info;
    
    // 2. Ожидание Handshake (первый сигнал от нужного PID)
    while (1) {
        if (sigwaitinfo(&mask, &info) < 0) {
            if (errno == EINTR) continue;
            fatal_error("sigwaitinfo handshake");
        }
        
        // Фильтр "свой-чужой"
        if (info.si_pid == tx_pid) break;
    }

    // Отправляем ACK (готовность)
    kill(tx_pid, SIG_ACK);
    printf("[Связь] Соединение установлено. Принимаю поток:\n");

    uint32_t buffer = 0;
    int count = 0;
    
    // Переменные для проверки целостности (на 10 баллов)
    int my_parity = 0;
    int remote_parity = -1;
    
    int active = 1;

    // 3. Основной цикл приема
    while (active) {
        // Блокируемся до получения сигнала
        if (sigwaitinfo(&mask, &info) < 0) {
            if (errno == EINTR) continue;
            fatal_error("sigwaitinfo loop");
        }

        // Игнорируем чужие процессы
        if (info.si_pid != tx_pid) continue;

        // SIGINT - сигнал окончания передачи
        if (info.si_signo == SIGINT) {
            active = 0;
            break;
        }

        // Декодирование бита
        int bit = -1;
        if (info.si_signo == SIG_BIT_1) bit = 1;
        if (info.si_signo == SIG_BIT_0) bit = 0;

        if (bit != -1) {
            if (count < 32) {
                // Сборка числа
                buffer = (buffer << 1) | bit;
                my_parity ^= bit;
                // Вывод без перевода строки
                printf("%d", bit); fflush(stdout);
            } 
            else if (count == 32) {
                // Принят бит четности
                remote_parity = bit;
                printf(" [Parity: %d]", bit);
            }
            
            count++;
            
            // Квитирование (отправляем подтверждение)
            kill(tx_pid, SIG_ACK);
        }
    }

    // 4. Вывод результатов
    printf("\n\n--- ИТОГ ---\n");
    if (count < 32) {
        printf("[FAIL] Передача оборвалась. Получено %d бит.\n", count);
        return EXIT_FAILURE;
    }

    int32_t final_val = (int32_t)buffer;
    printf("Принято число: %d\n", final_val);
    printf("Hex значение: 0x%08X\n", buffer);

    printf("Проверка целостности: ");
    if (count > 32) {
        if (my_parity == remote_parity) {
            printf("[ OK ] Данные корректны.\n");
        } else {
            printf("[ ОШИБКА ] Несовпадение четности! (Расчет: %d, Получено: %d)\n", my_parity, remote_parity);
        }
    } else {
        printf("[ WARN ] Нет бита четности.\n");
    }

    return 0;
}
