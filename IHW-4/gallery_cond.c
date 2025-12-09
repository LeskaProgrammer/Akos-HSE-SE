#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <stdarg.h>

#define MAX_GALLERY_CAPACITY 25
#define MAX_PAINTING_CAPACITY 5
#define PAINTINGS_COUNT 5

// Структура Монитора Галереи
typedef struct {
    int inside_count;                      // Текущее число людей
    int painting_counts[PAINTINGS_COUNT];  // Число людей у каждой картины
    
    pthread_mutex_t mutex;                 // Мьютекс для защиты данных
    pthread_cond_t cond_enter;             // Очередь на вход
    pthread_cond_t cond_painting[PAINTINGS_COUNT]; // Очереди к картинам
} GalleryMonitor;

GalleryMonitor GM;
pthread_mutex_t log_mutex;

int total_visitors = 50;
FILE *output_file = NULL;
volatile int simulation_running = 1;

void logger(const char *format, ...) {
    va_list args, args_f;
    pthread_mutex_lock(&log_mutex);
    va_start(args, format);
    va_copy(args_f, args);
    vprintf(format, args);
    if (output_file) vfprintf(output_file, format, args_f);
    va_end(args);
    va_end(args_f);
    pthread_mutex_unlock(&log_mutex);
}

// Инициализация монитора
void init_monitor() {
    GM.inside_count = 0;
    for(int i=0; i<PAINTINGS_COUNT; i++) GM.painting_counts[i] = 0;
    pthread_mutex_init(&GM.mutex, NULL);
    pthread_cond_init(&GM.cond_enter, NULL);
    for(int i=0; i<PAINTINGS_COUNT; i++) pthread_cond_init(&GM.cond_painting[i], NULL);
}

// --- МЕТОДЫ МОНИТОРА ---

// Вход в галерею
void enter_gallery(int id) {
    pthread_mutex_lock(&GM.mutex);
    while (GM.inside_count >= MAX_GALLERY_CAPACITY) {
        // Условие не выполнено, ждем сигнала (освобождения места)
        pthread_cond_wait(&GM.cond_enter, &GM.mutex);
    }
    GM.inside_count++;
    logger("Посетитель %d: --> ВОШЕЛ (Внутри: %d)\n", id, GM.inside_count);
    pthread_mutex_unlock(&GM.mutex);
}

// Выход из галереи
void leave_gallery(int id) {
    pthread_mutex_lock(&GM.mutex);
    GM.inside_count--;
    logger("Посетитель %d: <-- ВЫШЕЛ (Внутри: %d)\n", id, GM.inside_count);
    // Сигнализируем, что место освободилось
    pthread_cond_signal(&GM.cond_enter);
    pthread_mutex_unlock(&GM.mutex);
}

// Начало просмотра картины
void start_viewing(int id, int p_idx) {
    pthread_mutex_lock(&GM.mutex);
    while (GM.painting_counts[p_idx] >= MAX_PAINTING_CAPACITY) {
        pthread_cond_wait(&GM.cond_painting[p_idx], &GM.mutex);
    }
    GM.painting_counts[p_idx]++;
    logger("Посетитель %d: Смотрит картину %d\n", id, p_idx+1);
    pthread_mutex_unlock(&GM.mutex);
}

// Конец просмотра картины
void stop_viewing(int id, int p_idx) {
    pthread_mutex_lock(&GM.mutex);
    GM.painting_counts[p_idx]--;
    logger("Посетитель %d: Отошел от картины %d\n", id, p_idx+1);
    // Сигнализируем, что место у картины освободилось
    pthread_cond_signal(&GM.cond_painting[p_idx]);
    pthread_mutex_unlock(&GM.mutex);
}

void shuffle(int *array, size_t n) {
    for (size_t i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        int t = array[j]; array[j] = array[i]; array[i] = t;
    }
}

// Поток Вахтера
void* watchman_routine(void* arg) {
    logger("[ВАХТЕР]: Наблюдение запущено.\n");
    while (simulation_running) {
        pthread_mutex_lock(&GM.mutex);
        int cnt = GM.inside_count;
        pthread_mutex_unlock(&GM.mutex);
        
        // logger("[ВАХТЕР]: Внутри %d человек.\n", cnt);
        sleep(2);
    }
    return NULL;
}

// Поток Посетителя
void* visitor_routine(void* arg) {
    int id = *(int*)arg;
    free(arg);
    usleep(rand() % 50000);

    enter_gallery(id);

    int route[PAINTINGS_COUNT];
    for(int i=0; i<PAINTINGS_COUNT; i++) route[i] = i;
    shuffle(route, PAINTINGS_COUNT);

    for(int i=0; i<PAINTINGS_COUNT; i++) {
        start_viewing(id, route[i]);
        usleep((rand() % 100 + 50) * 1000); // Осмотр
        stop_viewing(id, route[i]);
    }

    leave_gallery(id);
    return NULL;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int opt;
    char *cfg = NULL, *out = NULL;

    while ((opt = getopt(argc, argv, "n:f:o:")) != -1) {
        switch (opt) {
            case 'n': total_visitors = atoi(optarg); break;
            case 'f': cfg = optarg; break;
            case 'o': out = optarg; break;
            default: exit(EXIT_FAILURE);
        }
    }

    if (cfg) {
        FILE *f = fopen(cfg, "r");
        if (f) { fscanf(f, "%d", &total_visitors); fclose(f); }
    }
    if (out) output_file = fopen(out, "w");

    init_monitor();
    pthread_mutex_init(&log_mutex, NULL);

    logger("=== ЗАПУСК СИМУЛЯЦИИ (COND VARS) ===\nПосетителей: %d\n", total_visitors);

    pthread_t watchman;
    pthread_create(&watchman, NULL, watchman_routine, NULL);

    pthread_t *th = malloc(sizeof(pthread_t) * total_visitors);
    for(int i=0; i<total_visitors; i++) {
        int *id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&th[i], NULL, visitor_routine, id);
    }

    for(int i=0; i<total_visitors; i++) pthread_join(th[i], NULL);

    simulation_running = 0;
    pthread_cancel(watchman);
    pthread_join(watchman, NULL);

    logger("=== КОНЕЦ СИМУЛЯЦИИ ===\n");

    free(th);
    if (output_file) fclose(output_file);
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&GM.mutex);
    pthread_cond_destroy(&GM.cond_enter);
    for(int i=0; i<PAINTINGS_COUNT; i++) pthread_cond_destroy(&GM.cond_painting[i]);

    return 0;
}
