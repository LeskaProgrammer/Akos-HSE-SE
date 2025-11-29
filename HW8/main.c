#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int re;  // действительная часть
    int im;  // мнимая часть
} Complex;

int main(void) {
    int N;
    if (scanf("%d", &N) != 1) {
        return 0;
    }

    // динамическое выделение памяти под матрицу N x N
    Complex *M = (Complex *)malloc((size_t)N * N * sizeof(Complex));
    if (M == NULL) {
        return 0; // не удалось выделить память
    }

    // чтение матрицы по строкам
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            Complex *c = &M[i * N + j];
            if (scanf("%d %d", &c->re, &c->im) != 2) {
                free(M);
                return 0;
            }
        }
    }

    // вывод эрмитово-сопряжённой: (M*)_ij = conj(M_ji)
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            Complex c = M[j * N + i]; // транспонирование
            c.im = -c.im;             // сопряжение (меняем знак мнимой части)

            if (j > 0) {
                putchar(' ');
            }
            printf("%d %d", c.re, c.im);
        }
        putchar('\n');
    }

    free(M);
    return 0;
}
