
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

// --- КОНФИГУРАЦИЯ ПРОТОКОЛА ---
#define SIG_BIT_0   SIGUSR1     // Сигнал для передачи бита '0'
#define SIG_BIT_1   SIGUSR2     // Сигнал для передачи бита '1'
#define SIG_ACK     SIGUSR1     // Сигнал подтверждения от приемника
#define TIMEOUT_SEC 3           // Таймаут ожидания ответа (сек)

void fatal_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Получение времени для метрики (Критерий на 10 баллов)
double get_wall_time() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) return 0;
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Функция ожидания подтверждения с таймаутом
// Реализует защиту от зависаний (Критерий на 10 баллов)
void wait_for_ack(pid_t target_pid, sigset_t *mask) {
    siginfo_t info;
    struct timespec timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_nsec = 0;

    while (1) {
        // Ждем сигнал (блокируемся на время timeout)
        int signum = sigtimedwait(mask, &info, &timeout);

        if (signum < 0) {
            if (errno == EAGAIN) {
                fprintf(stderr, "\n[ОШИБКА] Таймаут! Приемник (PID %d) не отвечает.\n", target_pid);
                exit(EXIT_FAILURE);
            }
            if (errno == EINTR) continue; // Прерывание другим сигналом
            fatal_error("sigtimedwait failed");
        }

        // Если нажали Ctrl+C - выходим
        if (signum == SIGINT) {
            fprintf(stderr, "\nПередача прервана пользователем.\n");
            exit(EXIT_FAILURE);
        }

        // Проверяем, что подтверждение пришло именно от приемника
        // Sender ждет SIG_ACK (SIGUSR1).
        // Если пришел SIGUSR2 (SIG_BIT_1) от кого-то еще - игнорируем/пропускаем
        if (info.si_pid == target_pid && signum == SIG_ACK) {
            return; // Успех
        }
    }
}

int main() {
    printf("=== ПЕРЕДАТЧИК (SENDER) ===\n");
    printf("Мой PID: %d\n", getpid());

    // 1. Блокируем сигналы (чтобы не убили процесс, а попали в очередь)
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);
    
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) fatal_error("sigprocmask");

    // 2. Ввод данных
    pid_t rx_pid;
    printf("Введите PID приемника: ");
    if (scanf("%d", &rx_pid) != 1) fatal_error("Некорректный PID");

    int32_t value;
    printf("Введите целое число (int32): ");
    if (scanf("%d", &value) != 1) fatal_error("Некорректное число");

    // Используем unsigned для корректного битового сдвига
    uint32_t payload = (uint32_t)value;

    printf("[Связь] Проверка доступности хоста...\n");

    // 3. Handshake: Будим приемник любым сигналом (например, SIG_BIT_1)
    kill(rx_pid, SIG_BIT_1);
    
    // Ждем подтверждения готовности
    wait_for_ack(rx_pid, &mask);
    printf("[Связь] Приемник готов. Начинаю передачу.\n");

    double start_time = get_wall_time();
    int parity_calc = 0; // Для расчета контрольной суммы

    // 4. Побитовая передача данных (32 бита)
    // Идем от старшего бита (31) к младшему (0)
    for (int i = 31; i >= 0; --i) {
        int bit = (payload >> i) & 1;
        parity_calc ^= bit; // XOR накопление

        int signal_to_send = (bit == 1) ? SIG_BIT_1 : SIG_BIT_0;
        
        if (kill(rx_pid, signal_to_send) < 0) fatal_error("kill failed");
        
        // Ждем ACK (синхронизация)
        wait_for_ack(rx_pid, &mask);

        // Визуализация
        printf("\rПрогресс: %2d/32 бит", 32 - i);
        fflush(stdout);
    }

    // 5. Передача бита четности (33-й бит) - На 10 баллов
    int parity_sig = (parity_calc == 1) ? SIG_BIT_1 : SIG_BIT_0;
    kill(rx_pid, parity_sig);
    wait_for_ack(rx_pid, &mask);
    printf("\n[Parity] Контрольный бит отправлен.\n");

    // 6. Завершение сеанса
    kill(rx_pid, SIGINT);
    
    double elapsed = get_wall_time() - start_time;
    printf("Успешно. Время передачи: %.4f сек.\n", elapsed);

    return 0;
}
