// hw7_copy_v2.cpp
// Копирование файлов через системные вызовы:
//   ./hw7_copy <src> <dst>
//   ./hw7_copy <src> <dst> -s / --small  — использовать маленький буфер (32 байта)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum {
    FD_STDIN  = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
};

static void write_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t w = write(fd, buf, len);
        if (w <= 0) {
            // Ошибку здесь не репортим дополнительно, просто выходим.
            return;
        }
        buf += (size_t)w;
        len -= (size_t)w;
    }
}

static void print_errno_msg(const char *prefix) {
    const char *errmsg = strerror(errno);
    write_all(FD_STDERR, prefix, strlen(prefix));
    write_all(FD_STDERR, ": ", 2);
    write_all(FD_STDERR, errmsg, strlen(errmsg));
    write_all(FD_STDERR, "\n", 1);
}

static void print_usage(const char *prog) {
    if (!prog) {
        prog = "hw7_copy";
    }
    const char *msg1 = "Usage:\n  ";
    const char *msg2 = " <src> <dst> [--small | -s]\n";
    write_all(FD_STDERR, msg1, strlen(msg1));
    write_all(FD_STDERR, prog, strlen(prog));
    write_all(FD_STDERR, msg2, strlen(msg2));
}

// Копирование файла, когда буфер заведомо больше размера файла.
static int copy_with_big_buffer(int in_fd, int out_fd, off_t file_size) {
    if (file_size < 0) {
        write_all(FD_STDERR, "Invalid file size\n", 18);
        return -1;
    }

    // Делаем буфер немного больше размера файла.
    size_t buf_size = (size_t)file_size + 1u;
    if (buf_size == 0) {
        // На случай переполнения, хотя в реальности для таких файлов мы уже ничем не поможем.
        buf_size = 1u;
    }

    char *buffer = (char*)malloc(buf_size);
    if (!buffer) {
        write_all(FD_STDERR, "Failed to allocate buffer\n", 26);
        return -1;
    }

    size_t total_used = 0;
    for (;;) {
        size_t remain = buf_size - total_used;
        if (remain == 0) {
            break;
        }

        ssize_t r = read(in_fd, buffer + total_used, remain);
        if (r < 0) {
            print_errno_msg("Error reading input file");
            free(buffer);
            return -1;
        }
        if (r == 0) { // EOF
            break;
        }

        total_used += (size_t)r;
    }

    size_t offset = 0;
    while (offset < total_used) {
        ssize_t w = write(out_fd, buffer + offset, total_used - offset);
        if (w < 0) {
            print_errno_msg("Error writing output file");
            free(buffer);
            return -1;
        }
        offset += (size_t)w;
    }

    free(buffer);
    return 0;
}

// Копирование с небольшим фиксированным буфером (32 байта).
static int copy_with_small_buffer(int in_fd, int out_fd) {
    enum { SMALL_BUF = 32 };
    char buf[SMALL_BUF];

    for (;;) {
        ssize_t r = read(in_fd, buf, SMALL_BUF);
        if (r < 0) {
            print_errno_msg("Error reading input file");
            return -1;
        }
        if (r == 0) {
            break; // EOF
        }

        size_t written = 0;
        while (written < (size_t)r) {
            ssize_t w = write(out_fd, buf + written, (size_t)r - written);
            if (w < 0) {
                print_errno_msg("Error writing output file");
                return -1;
            }
            written += (size_t)w;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        print_usage(argc > 0 ? argv[0] : NULL);
        return 1;
    }

    const char *src_path = argv[1];
    const char *dst_path = argv[2];

    int use_small = 0;
    if (argc == 4) {
        const char *opt = argv[3];
        if (strcmp(opt, "-s") == 0 || strcmp(opt, "--small") == 0) {
            use_small = 1;
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    struct stat st;
    if (stat(src_path, &st) < 0) {
        print_errno_msg("Cannot stat source file");
        return 1;
    }

    int in_fd = open(src_path, O_RDONLY);
    if (in_fd < 0) {
        print_errno_msg("Cannot open source file");
        return 1;
    }

    // Права результата берем из исходного файла.
    mode_t mode = st.st_mode & 0777;

    int out_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (out_fd < 0) {
        print_errno_msg("Cannot open destination file");
        close(in_fd);
        return 1;
    }

    int result;
    if (use_small) {
        result = copy_with_small_buffer(in_fd, out_fd);
    } else {
        result = copy_with_big_buffer(in_fd, out_fd, st.st_size);
    }

    // Пытаемся явно выставить права (важно для разных платформ).
    if (chmod(dst_path, mode) < 0) {
        print_errno_msg("Warning: cannot set permissions on destination file");
    }

    int exit_code = 0;

    if (close(in_fd) < 0) {
        print_errno_msg("Error closing input file");
        exit_code = 1;
    }
    if (close(out_fd) < 0) {
        print_errno_msg("Error closing output file");
        exit_code = 1;
    }

    if (result != 0) {
        exit_code = 1;
    }

    return exit_code;
}
