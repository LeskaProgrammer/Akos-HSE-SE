#include <iostream>
#include <vector>
#include <deque>
#include <unistd.h>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>

// --- Глобальные ресурсы ---
std::deque<int> buffer;             // Общий буфер (очередь)
pthread_mutex_t mutex_buffer;       // Мьютекс для защиты буфера
pthread_cond_t cond_buffer;         // Условная переменная для сигнала Монитору
pthread_mutex_t mutex_log;          // Мьютекс для чистого вывода в консоль

int producers_finished = 0;         // Счетчик завершивших работу источников
int active_adders = 0;              // Счетчик активных сумматоров
const int TOTAL_PRODUCERS = 100;    // Число источников (по заданию)

// Файл для имитации сохранения лога
std::ofstream log_file;

// Структура аргументов для потока-сумматора
struct AdderArgs {
    int id;
    int val1;
    int val2;
};

// --- Функция логирования (в консоль и файл) ---
void log_event(const std::string& msg) {
    pthread_mutex_lock(&mutex_log);
    
    // Вывод в консоль
    std::cout << msg << std::endl;
    
    // Вывод в файл (имитация)
    if (log_file.is_open()) {
        log_file << msg << std::endl;
    }
    
    pthread_mutex_unlock(&mutex_log);
}

// --- Поток-Источник (Producer) ---
void* producer_routine(void* arg) {
    long id = (long)arg;

    // Имитация задержки (1-7 сек)
    int delay = 1 + rand() % 7;
    sleep(delay);

    // Генерация числа (1-100)
    int value = 1 + rand() % 100;

    // Вход в критическую секцию
    pthread_mutex_lock(&mutex_buffer);
    buffer.push_back(value);
    producers_finished++;
    
    std::stringstream ss;
    ss << "[Источник " << id << "] Сгенерировал: " << value 
       << " (сон " << delay << "с). Буфер: " << buffer.size();
    log_event(ss.str());

    // Сигнализируем монитору, что есть новые данные
    pthread_cond_signal(&cond_buffer);
    pthread_mutex_unlock(&mutex_buffer);

    return NULL;
}

// --- Поток-Сумматор (Worker) ---
void* adder_routine(void* arg) {
    AdderArgs* args = (AdderArgs*)arg;
    int my_id = args->id;
    int v1 = args->val1;
    int v2 = args->val2;
    delete args; // Освобождаем память

    // Имитация вычислений (3-6 сек)
    int work_time = 3 + rand() % 4;
    sleep(work_time);

    int result = v1 + v2;

    pthread_mutex_lock(&mutex_buffer);
    buffer.push_back(result);
    active_adders--;

    std::stringstream ss;
    ss << "   -> [Сумматор " << my_id << "] Результат: " << v1 << " + " << v2 << " = " << result 
       << " (время " << work_time << "с). Буфер: " << buffer.size();
    log_event(ss.str());

    // Сигнализируем монитору (вдруг это была последняя пара)
    pthread_cond_signal(&cond_buffer);
    pthread_mutex_unlock(&mutex_buffer);

    return NULL;
}

// --- Главная функция (Монитор) ---
int main() {
    srand(time(NULL));
    
    // Открытие файла лога
    log_file.open("simulation.log");
    
    // Инициализация синхронизации
    pthread_mutex_init(&mutex_buffer, NULL);
    pthread_cond_init(&cond_buffer, NULL);
    pthread_mutex_init(&mutex_log, NULL);

    log_event("=== ЗАПУСК СИМУЛЯЦИИ ===");

    // 1. Запуск 100 потоков-источников
    pthread_t producers[TOTAL_PRODUCERS];
    for (long i = 1; i <= TOTAL_PRODUCERS; ++i) {
        if (pthread_create(&producers[i-1], NULL, producer_routine, (void*)i) != 0) {
            perror("Ошибка создания потока");
            return 1;
        }
        pthread_detach(producers[i-1]); // Отсоединяем, так как не будем ждать их через join
    }

    // 2. Цикл Монитора (бесконечный, пока не выполнится условие выхода)
    int adder_count = 1;
    
    while (true) {
        pthread_mutex_lock(&mutex_buffer);

        // Ждем, пока в буфере станет >= 2 элемента, или пока все не закончится
        while (buffer.size() < 2) {
            // Условие завершения: все источники отработали, сумматоры свободны, в буфере 1 число
            if (producers_finished == TOTAL_PRODUCERS && active_adders == 0 && buffer.size() == 1) {
                pthread_mutex_unlock(&mutex_buffer);
                goto end_simulation;
            }
            // Защита от зависания при пустом буфере в конце (на случай сбоев)
            if (producers_finished == TOTAL_PRODUCERS && active_adders == 0 && buffer.empty()) {
                pthread_mutex_unlock(&mutex_buffer);
                goto end_simulation;
            }
            
            // Эффективное ожидание сигнала (без сжигания CPU)
            pthread_cond_wait(&cond_buffer, &mutex_buffer);
        }

        // Извлекаем пару чисел (FIFO)
        int a = buffer.front(); buffer.pop_front();
        int b = buffer.front(); buffer.pop_front();
        active_adders++;

        // Лог запуска сумматора
        std::stringstream ss;
        ss << "[МОНИТОР] Найдена пара (" << a << ", " << b << "). Запуск Сумматора #" << adder_count;
        log_event(ss.str());

        // Создаем поток-сумматор
        pthread_t t_adder;
        AdderArgs* args = new AdderArgs{adder_count++, a, b};
        if (pthread_create(&t_adder, NULL, adder_routine, args) != 0) {
            perror("Ошибка создания сумматора");
        }
        pthread_detach(t_adder);

        pthread_mutex_unlock(&mutex_buffer);
    }

end_simulation:
    log_event("\n=== ВЫЧИСЛЕНИЯ ЗАВЕРШЕНЫ ===");
    if (!buffer.empty()) {
        std::cout << "ФИНАЛЬНЫЙ РЕЗУЛЬТАТ: " << buffer.front() << std::endl;
        if (log_file.is_open()) log_file << "ФИНАЛЬНЫЙ РЕЗУЛЬТАТ: " << buffer.front() << std::endl;
    } else {
        std::cout << "Ошибка: буфер пуст." << std::endl;
    }

    // Очистка ресурсов
    if (log_file.is_open()) {
        log_file.close();
        std::cout << "[Система] Файл лога 'simulation.log' успешно записан." << std::endl;
    }
    
    pthread_mutex_destroy(&mutex_buffer);
    pthread_cond_destroy(&cond_buffer);
    pthread_mutex_destroy(&mutex_log);

    return 0;
}