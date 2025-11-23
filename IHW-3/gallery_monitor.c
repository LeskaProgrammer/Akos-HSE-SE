// file: gallery_monitor.c
#include "gallery_common.h"

static SharedState *sh = NULL;
static int shm_fd = -1;
static sem_t *mutex = NULL;

static void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
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
    return 0;
}

static void detach_all(void) {
    if (mutex) sem_close(mutex);
    if (sh && sh != MAP_FAILED) munmap(sh, sizeof(SharedState));
    if (shm_fd != -1) close(shm_fd);
}

int main(void) {
    if (attach_all() != 0) {
        return 1;
    }

    int last = 0;
    for (;;) {
        sem_wait(mutex);
        int tail = sh->log_tail;
        int finished = sh->finished_visitors;
        int total = sh->total_visitors;
        int inside = sh->inside;
        int day_over = sh->day_over;

        while (last < tail) {
            int idx = last % LOG_LINES;
            printf("[MONITOR %d] %s\n", getpid(), sh->log[idx]);
            last++;
        }

        int done = day_over && finished >= total && inside == 0;
        sem_post(mutex);

        if (done) break;
        sleep_ms(200);   // 200 мс
    }

    detach_all();
    return 0;
}
