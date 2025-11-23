// file: gallery_guard.c
#include "gallery_common.h"

static SharedState *sh = NULL;
static int shm_fd = -1;
static sem_t *mutex = NULL;

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
    return 0;
}

static void detach_all(void) {
    if (mutex) sem_close(mutex);
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

    // печать сразу
    printf("%s\n", line);
    fflush(stdout);

    // запись в общий лог
    sem_wait(mutex);
    int idx = sh->log_tail % LOG_LINES;
    snprintf(sh->log[idx], LOG_LINE_LEN, "%s", line);
    sh->log_tail++;
    sem_post(mutex);
}

int main(void) {
    if (attach_all() != 0) {
        return 1;
    }

    for (;;) {
        sleep(1);

        sem_wait(mutex);
        int inside = sh->inside;
        int finished = sh->finished_visitors;
        int total = sh->total_visitors;
        int at[PAINTINGS];
        for (int i = 0; i < PAINTINGS; ++i) {
            at[i] = sh->at_painting[i];
        }
        int done = (finished >= total && inside == 0);
        if (done) {
            sh->day_over = 1;
        }
        sem_post(mutex);

        log_msg("GUARD", 0,
                "контроль: внутри %d, завершило %d из %d",
                inside, finished, total);
        for (int i = 0; i < PAINTINGS; ++i) {
            log_msg("GUARD", 0,
                    "картина %d: возле нее %d посетителей",
                    i, at[i]);
        }

        if (done) {
            log_msg("GUARD", 0, "день окончен, галерея закрывается");
            break;
        }
    }

    detach_all();
    return 0;
}
