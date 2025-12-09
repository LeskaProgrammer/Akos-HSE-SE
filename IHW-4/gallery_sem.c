#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <getopt.h>
#include <time.h>
#include <stdarg.h>

// --- КОНСТАНТЫ ЗАДАЧИ ---
#define MAX_GALLERY_CAPACITY 25
#define PICTURES_COUNT 5
#define MAX_AT_PICTURE 5

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ СИНХРОНИЗАЦИИ ---
sem_t gallery_sem;                    // Семафор входа (ограничивает до 25)
sem_t painting_sems[PICTURES_COUNT];  // Семафоры картин (ограничивают до 5)
pthread_mutex_t log_mutex;            // Мьютекс для чистого вывода в консоль

// --- ПАРАМЕТРЫ ---
int total_visitors = 50;
FILE *output_file = NULL;
volatile int simulation_running = 1;

// --- ФУНКЦИЯ ЛОГИРОВАНИЯ ---
// Потокобезопасный вывод в консоль и файл
void logger(const char *format, ...) {
    va_list args, args_file;
    
    pthread_mutex_lock(&log_mutex);
    
    va_start(args, format);
    va_copy(args_file, args);
    
    vprintf(format, args);                    // Вывод на экран
    if (output_file) {
        vfprintf(output_file, format, args_file); // Вывод в файл
    }
    
    va_end(args);
    va_end(args_file);
    
    pthread_mutex_unlock(&log_mutex);
}

// Перемешивание маршрута (Алгоритм Фишера-Йетса)
void shuffle(int *array, size_t n) {
    for (size_t i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        int t = array[j]; array[j] = array[i]; array[i] = t;
    }
}

// --- ПОТОК ВАХТЕРА ---
void* watchman_routine(void* arg) {
    logger("[ВАХТЕР]: Заступил на смену. Лимит: %d человек.\n", MAX_GALLERY_CAPACITY);
    while (simulation_running) {
        int val;
        // Получаем текущее значение семафора
        sem_getvalue(&gallery_sem, &val);
        int current_visitors = MAX_GALLERY_CAPACITY - val;
        
        // Вахтер может периодически сообщать о статусе
        // logger("[ВАХТЕР]: Сейчас в галерее %d человек.\n", current_visitors);
        
        sleep(2); // Проверка раз в 2 секунды
    }
    return NULL;
}

// --- ПОТОК ПОСЕТИТЕЛЯ ---
void* visitor_routine(void* arg) {
    int id = *(int*)arg;
    free(arg);

    // Имитация случайного прибытия
    usleep(rand() % 50000);

    logger("Посетитель %d: Пришел к галерее.\n", id);

    // 1. Вход в галерею (P-операция)
    sem_wait(&gallery_sem);
    logger("Посетитель %d: --> ВОШЕЛ в галерею.\n", id);

    // 2. Генерация случайного маршрута
    int route[PICTURES_COUNT];
    for(int i=0; i<PICTURES_COUNT; i++) route[i] = i;
    shuffle(route, PICTURES_COUNT);

    // 3. Осмотр картин
    for (int i = 0; i < PICTURES_COUNT; i++) {
        int pic = route[i];
        
        // Занять место у картины
        sem_wait(&painting_sems[pic]);
        logger("Посетитель %d: Смотрит картину №%d.\n", id, pic + 1);
        
        // Время просмотра (50-150 мс)
        usleep((rand() % 100 + 50) * 1000);
        
        // Освободить место
        sem_post(&painting_sems[pic]);
        logger("Посетитель %d: Отошел от картины №%d.\n", id, pic + 1);
    }

    // 4. Выход (V-операция)
    logger("Посетитель %d: <-- ВЫШЕЛ из галереи.\n", id);
    sem_post(&gallery_sem);

    return NULL;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int opt;
    char *config_file = NULL;
    char *out_filename = NULL;

    // Парсинг аргументов
    while ((opt = getopt(argc, argv, "n:f:o:")) != -1) {
        switch (opt) {
            case 'n': total_visitors = atoi(optarg); break;
            case 'f': config_file = optarg; break;
            case 'o': out_filename = optarg; break;
            default: exit(EXIT_FAILURE);
        }
    }

    // Чтение конфига
    if (config_file) {
        FILE *f = fopen(config_file, "r");
        if (f) {
            fscanf(f, "%d", &total_visitors);
            fclose(f);
        } else {
            perror("Ошибка чтения конфига");
            return 1;
        }
    }

    if (out_filename) output_file = fopen(out_filename, "w");

    // Инициализация
    pthread_mutex_init(&log_mutex, NULL);
    sem_init(&gallery_sem, 0, MAX_GALLERY_CAPACITY);
    for(int i=0; i<PICTURES_COUNT; i++) sem_init(&painting_sems[i], 0, MAX_AT_PICTURE);

    logger("=== ЗАПУСК СИМУЛЯЦИИ (СЕМАФОРЫ) ===\nПосетителей: %d\n", total_visitors);

    // Запуск Вахтера
    pthread_t watchman;
    pthread_create(&watchman, NULL, watchman_routine, NULL);

    // Запуск Посетителей
    pthread_t *visitors = malloc(sizeof(pthread_t) * total_visitors);
    for (int i = 0; i < total_visitors; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&visitors[i], NULL, visitor_routine, id);
    }

    // Ожидание завершения всех посетителей
    for (int i = 0; i < total_visitors; i++) {
        pthread_join(visitors[i], NULL);
    }

    // Остановка Вахтера
    simulation_running = 0;
    pthread_cancel(watchman); // Прерываем sleep вахтера
    pthread_join(watchman, NULL);

    logger("=== ГАЛЕРЕЯ ЗАКРЫТА ===\n");

    // Очистка
    free(visitors);
    if (output_file) fclose(output_file);
    sem_destroy(&gallery_sem);
    for(int i=0; i<PICTURES_COUNT; i++) sem_destroy(&painting_sems[i]);
    pthread_mutex_destroy(&log_mutex);

    return 0;
}
