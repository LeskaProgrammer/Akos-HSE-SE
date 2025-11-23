// file: gallery_init.c
#include "gallery_common.h"

static void make_paint_name(char *buf, size_t sz, int idx) {
    snprintf(buf, sz, "%s%d", SEM_PAINTING_BASE, idx);
}

int main(int argc, char *argv[]) {
    int total_visitors = 50;
    if (argc > 1) {
        total_visitors = atoi(argv[1]);
        if (total_visitors <= 0) {
            fprintf(stderr, "Неверное число посетителей\n");
            return 1;
        }
    }

    // удалить старые объекты, если остались
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_GALLERY_NAME);
    for (int i = 0; i < PAINTINGS; ++i) {
        char name[64];
        make_paint_name(name, sizeof(name), i);
        sem_unlink(name);
    }

    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(fd, sizeof(SharedState)) == -1) {
        perror("ftruncate");
        return 1;
    }

    SharedState *sh = mmap(NULL, sizeof(SharedState),
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    memset(sh, 0, sizeof(SharedState));
    sh->total_visitors = total_visitors;
    sh->inside = 0;
    sh->finished_visitors = 0;
    sh->day_over = 0;
    sh->log_tail = 0;

    sem_t *mutex = sem_open(SEM_MUTEX_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open mutex");
        return 1;
    }

    sem_t *gallery = sem_open(SEM_GALLERY_NAME, O_CREAT | O_EXCL, 0666, MAX_INSIDE);
    if (gallery == SEM_FAILED) {
        perror("sem_open gallery");
        return 1;
    }

    sem_t *paint[PAINTINGS];
    for (int i = 0; i < PAINTINGS; ++i) {
        char name[64];
        make_paint_name(name, sizeof(name), i);
        paint[i] = sem_open(name, O_CREAT | O_EXCL, 0666, 5);
        if (paint[i] == SEM_FAILED) {
            perror("sem_open painting");
            return 1;
        }
    }

    printf("Инициализация завершена: ожидается %d посетителей.\n", total_visitors);

    // закрываем дескрипторы (объекты продолжают существовать)
    sem_close(mutex);
    sem_close(gallery);
    for (int i = 0; i < PAINTINGS; ++i) {
        sem_close(paint[i]);
    }
    munmap(sh, sizeof(SharedState));
    close(fd);

    return 0;
}
