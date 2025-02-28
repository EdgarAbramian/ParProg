#include "stdio.h"
#include "stdlib.h"
#include "omp.h"

#define NMAX 9000000 // Максимальный размер массивов
#define VNUM 2       // Количество массивов

int main (int argc, char** argv) {
    int Q = 1; // Количество повторов внутренних операций
    int num_threads = 4; // Количество потоков для OpenMP
    if (argc > 2) {
        num_threads = atoi(argv[1]); // Если аргументы заданы, переопределяем количество потоков
        Q = atoi(argv[2]); // и значение Q
    }

    // Вывод параметров выполнения программы
    printf("Parameters:\n \ntype: double, \narrays: %u, \nsize: %u, \nthreads: %u, \nQ: %u \n", VNUM, NMAX, num_threads, Q);

    omp_set_num_threads(num_threads); // Устанавливаем количество потоков для OpenMP

    double st_time, end_time; // Переменные для измерения времени

    // Переменные для хранения сумм и времени выполнения для разных методов
    double sum_consistent = 0, sum_reduction = 0, sum_critical = 0, sum_atomic = 0;
    double time_consistent = 0, time_reduction = 0, time_critical = 0, time_atomic = 0;
    double init_time = 0; // Время инициализации

    double S_reduction = 0, S_critical = 0, S_atomic = 0; // Ускорение для разных методов

    // Инициализируем массивы значениями 1
    double* a = (double*)malloc(sizeof(double) * NMAX);
    double* b = (double*)malloc(sizeof(double) * NMAX);
    for (int i = 0; i < NMAX; i++) {
        a[i] = 1;
        b[i] = 1;
    }

    // Основной цикл повторений для усреднения времени
    for (int k = 0; k < 20; k++) {
        printf("\nCurrent iteration: %d ", k); // Текущая итерация

        // Сброс сумм
        sum_consistent = 0, sum_reduction = 0, sum_critical = 0, sum_atomic = 0;

        // Последовательный алгоритм (без OpenMP)
        st_time = omp_get_wtime(); // Засекаем время
        for (int i = 0; i < NMAX; i++) {
            for (int j = 0; j < Q; j++) {
                sum_consistent += a[i] + b[i]; // Суммируем элементы
            }
        }
        end_time = omp_get_wtime(); // Конец времени
        time_consistent += end_time - st_time; // Время последовательного алгоритма

        // Параллельные алгоритмы с использованием OpenMP
        st_time = omp_get_wtime();
#pragma omp parallel shared(a, b) private(i, j)
        {
            end_time = omp_get_wtime();
            init_time += end_time - st_time; // Учитываем время инициализации потоков

            // Алгоритм с использованием директивы reduction
            st_time = omp_get_wtime();
#pragma omp for reduction(+: sum_reduction)
            for (int i = 0; i < NMAX; i++) {
                for (int j = 0; j < Q; j++) {
                    sum_reduction += a[i] + b[i]; // Суммируем элементы с автоматическим объединением
                }
            }
#pragma omp barrier
            end_time = omp_get_wtime();
            time_reduction += end_time - st_time; // Время метода reduction

            // Алгоритм с использованием критической секции
            st_time = omp_get_wtime();
#pragma omp for
            for (int i = 0; i < NMAX; i++) {
                for (int j = 0; j < Q; j++) {
#pragma omp critical // Обеспечиваем последовательный доступ к общей переменной
                    sum_critical += a[i] + b[i];
                 }
            }
#pragma omp barrier
            end_time = omp_get_wtime();
            time_critical += end_time - st_time; // Время метода critical

            // Алгоритм с использованием атомарной операции
#pragma omp for
            for (int i = 0; i < NMAX; i++) {
                for (int j = 0; j < Q; j++) {
#pragma omp atomic // Обеспечиваем атомарное обновление переменной
                    sum_atomic += (a[i] + b[i]);
                }
            }
#pragma omp barrier
            end_time = omp_get_wtime();
            time_atomic += end_time - st_time; // Время метода atomic
        }
    }

    // Усредняем время для каждого метода
    time_critical /= 20;
    time_reduction /= 20;
    time_consistent /= 20;
    time_atomic /= 20;
    init_time /= 20;

    // Рассчитываем ускорение для параллельных методов
    S_reduction = time_consistent / time_reduction;
    S_critical = time_consistent / time_critical;
    S_atomic = time_consistent / time_atomic;

    // Выводим результаты
    printf("\nTime of work consistent algorithm is %f. \t\t\t Result sum: %f ", time_consistent, sum_consistent);
    printf("\nTime of work reduction algorithm is %f. S is %f. \t Result sum: %f ", time_reduction, S_reduction, sum_reduction);
    printf("\nTime of work critical algorithm is %f. S is %f. \t Result sum: %f ", time_critical, S_critical, sum_critical);
    printf("\nTime of work atomic algorithm is %f. S is %f. \t Result sum: %f \n", time_atomic, S_atomic, sum_atomic);
    printf("Init time: %f \n", init_time);

    return 0;
}
