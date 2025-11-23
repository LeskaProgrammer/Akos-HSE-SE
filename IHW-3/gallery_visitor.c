// file: gallery_visitor.c
#include "gallery_common.h"

static SharedState *sh = NULL;
static int shm_fd = -1;
static sem_t *mutex = NULL;
static sem_t *gallery_slots = NULL;
static sem_t *paint_sem[PAINTINGS];

static void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static void make_paint_name(char *buf, size_t sz, int idx) {
    snprintf(buf, sz, "%s%d", SEM_PAINTING_BASE, idx);
}

static int attach_all(void) {
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd == -1) {
        perror("shm_open");
        return -1;
    }
    sh = mmap(NULL, sizeof(SharedState), PROT_READ | PROT_WRITE,
              MAP_SHARED, shm_fd, 0);
    if (sh == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    mutex = sem_open(SEM_MUTEX_NAME, 0);
    if (mutex == SEM_FAILED) {
        perror("sem_open mutex");
        return -1;
    }
    gallery_slots = sem_open(SEM_GALLERY_NAME, 0);
    if (gallery_slots == SEM_FAILED) {
        perror("sem_open gallery");
        return -1;
    }
    for (int i = 0; i < PAINTINGS; ++i) {
        char name[64];
        make_paint_name(name, sizeof(name), i);
        paint_sem[i] = sem_open(name, 0);
        if (paint_sem[i] == SEM_FAILED) {
            perror("sem_open painting");
            return -1;
        }
    }
    return 0;
}

static void detach_all(void) {
    if (mutex) sem_close(mutex);
    if (gallery_slots) sem_close(gallery_slots);
    for (int i = 0; i < PAINTINGS; ++i) {
        if (paint_sem[i]) sem_close(paint_sem[i]);
    }
    if (sh && sh != MAP_FAILED) munmap(sh, sizeof(SharedState));
    if (shm_fd != -1) close(shm_fd);
}

static void log_msg(const char *prefix, int id, const char *fmt, ...) {
    char line[LOG_LINE_LEN];
    int off = 0;
    if (prefix) {
        off += snprintf(line + off, sizeof(line) - off,
                        "[%s %d] ", prefix, id);
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line + off, sizeof(line) - off, fmt, ap);
    va_end(ap);

    // печатаем сразу в терминал
    printf("%s\n", line);
    fflush(stdout);

    // записываем в общий лог под защитой mutex
    sem_wait(mutex);
    int idx = sh->log_tail % LOG_LINES;
    snprintf(sh->log[idx], LOG_LINE_LEN, "%s", line);
    sh->log_tail++;
    sem_post(mutex);
}

int main(int argc, char *argv[]) {
    int id = (argc > 1) ? atoi(argv[1]) : (int)getpid();

    if (attach_all() != 0) {
        return 1;
    }

    srand((unsigned int)(getpid() ^ time(NULL)));

    int visited[PAINTINGS] = {0};
    int remaining = PAINTINGS;

    // войти в галерею: ждём свободный слот
    sem_wait(gallery_slots);

    // обновляем счётчик внутри под mutex
    int inside_now;
    sem_wait(mutex);
    sh->inside++;
    inside_now = sh->inside;
    sem_post(mutex);

    log_msg("VISITOR", id,
            "зашел в галерею, сейчас внутри %d", inside_now);

    while (remaining > 0) {
        int p = rand() % PAINTINGS;
        if (visited[p]) continue;

        // ждём место у картины
        sem_wait(paint_sem[p]);

        // подходим к картине: обновляем счётчик под mutex
        int at_now;
        sem_wait(mutex);
        sh->at_painting[p]++;
        at_now = sh->at_painting[p];
        sem_post(mutex);

        log_msg("VISITOR", id,
                "подошел к картине %d, сейчас у нее %d посетителей",
                p, at_now);

        // смотрим картину
        int delay_ms = (rand() % 1000) + 500;
        sleep_ms(delay_ms);

        // отходим от картины
        sem_wait(mutex);
        sh->at_painting[p]--;
        at_now = sh->at_painting[p];
        sem_post(mutex);

        log_msg("VISITOR", id,
                "отошел от картины %d, теперь у нее %d посетителей",
                p, at_now);

        sem_post(paint_sem[p]);

        visited[p] = 1;
        remaining--;
    }

    // выходим из галереи: уменьшаем inside, увеличиваем finished
    int inside_after, finished_after, total;
    sem_wait(mutex);
    sh->inside--;
    sh->finished_visitors++;
    inside_after    = sh->inside;
    finished_after  = sh->finished_visitors;
    total           = sh->total_visitors;
    sem_post(mutex);

    log_msg("VISITOR", id,
            "вышел из галереи. Внутри %d, завершило %d из %d",
            inside_after, finished_after, total);

    sem_post(gallery_slots);

    detach_all();
    return 0;
}
