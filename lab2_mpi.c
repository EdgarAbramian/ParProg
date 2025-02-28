#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NMAX 9000000 // Максимальный размер массивов
#define VNUM 2       // Количество массивов

int main(int argc, char* argv[]) {
    int Q = 21; // Число итераций во вложенном цикле
    int num_threads = 4; // Число потоков (MPI процессов)

    // Проверяем аргументы командной строки, если указан, то переопределяем значение Q
    if (argc > 1) {
        Q = atoi(argv[1]);
    }

    double st_time, end_time; // Временные метки для измерения времени выполнения

    // Переменные для хранения сумм и времен выполнения различных подходов
    double sum_consistent = 0, sum_reduce = 0, sum_recv = 0;
    double time_consistent = 0, time_reduce = 0, time_recv = 0;
    double time_init = 0; // Время на инициализацию (например, передачу данных)
    double S_reduce, S_recv; // Ускорение для алгоритмов Reduce и Recv
    double* a = (double*)malloc(sizeof(double) * NMAX); // Массив a
    double* b = (double*)malloc(sizeof(double) * NMAX); // Массив b

    int i, j, k, t; // Индексы и вспомогательные переменные
    int proc_num, proc_rank; // Число процессов и ранг текущего процесса
    MPI_Status status; // Статус MPI для операций отправки/получения

    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_num); // Получаем общее число процессов
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank); // Получаем ранг текущего процесса

    // Если процесс главный (ранг 0), инициализируем массивы и выводим параметры
    if (proc_rank == 0) {
        printf("\nParameters:\n \ntype: double, \narrays: %u, \nsize: %u, \nthreads: %u, \nQ: %u \n", VNUM, NMAX, proc_num, Q);

        // Инициализируем массивы значениями 1
        for (i = 0; i < NMAX; i++) {
            a[i] = 1;
            b[i] = 1;
        }

        // Последовательный алгоритм: вычисляем сумму без MPI
        for (k = 0; k < 20; k++) { // 20 повторов для усреднения времени
            sum_consistent = 0; // Обнуляем сумму

            st_time = MPI_Wtime(); // Засекаем время начала

            for (i = 0; i < NMAX; i++) {
                for (j = 0; j < Q; j++) {
                    sum_consistent += a[i] + b[i]; // Суммируем элементы
                }
            }

            end_time = MPI_Wtime(); // Засекаем время конца
            time_consistent += end_time - st_time; // Считаем общее время
        }
    }

    // Основной цикл: параллельное вчёыполнение
    for (t = 0; t < 20; t++) { // 20 повторов
        if (proc_rank == 0) {
            printf("\nCurrent iteration: %d", t); // Печатаем текущую итерацию
        }

        sum_recv = 0; // Сброс суммы для Recv
        sum_reduce = 0; // Сброс суммы для Reduce

        // Делим данные на участки для процессов
        int k = NMAX / proc_num; // Размер участка для одного процесса
        int i1 = k * proc_rank; // Начальный индекс текущего процесса
        int i2 = k * (proc_rank + 1); // Конечный индекс текущего процесса
        double sum_proc = 0; // Локальная сумма для процесса

        // Передача данных с помощью MPI_Bcast
        st_time = MPI_Wtime(); // Засекаем время начала

        MPI_Bcast(a, NMAX, MPI_DOUBLE, 0, MPI_COMM_WORLD); // Передаем массив a всем процессам
        MPI_Bcast(b, NMAX, MPI_DOUBLE, 0, MPI_COMM_WORLD); // Передаем массив b всем процессам

        MPI_Barrier(MPI_COMM_WORLD); // Синхронизируем процессы

        end_time = MPI_Wtime(); // Засекаем время конца
        time_init += end_time - st_time; // Считаем время передачи данных

        // Параллельное вычисление суммы (алгоритм Reduce)
        st_time = MPI_Wtime();

        if (proc_rank == proc_num - 1) {
            i2 = NMAX; // Последний процесс обрабатывает оставшиеся элементы
        }
        for (i = i1; i < i2; i++) { // Суммируем элементы своего участка
            for (j = 0; j < Q; j++) {
                sum_proc += a[i] + b[i];
            }
        }

        MPI_Reduce(&sum_proc, &sum_reduce, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); // Суммируем результаты всех процессов

        MPI_Barrier(MPI_COMM_WORLD); // Синхронизируем процессы

        end_time = MPI_Wtime();
        time_reduce += end_time - st_time; // Учитываем время Reduce

        // Параллельное вычисление суммы (алгоритм Recv)
        sum_proc = 0; // Сбрасываем локальную сумму
        st_time = MPI_Wtime();

        if (proc_rank == proc_num - 1) {
            i2 = NMAX; // Последний процесс обрабатывает оставшиеся элементы
        }
        for (i = i1; i < i2; i++) { // Суммируем элементы своего участка
            for (j = 0; j < Q; j++) {
                sum_proc += a[i] + b[i];
            }
        }

        if (proc_rank == 0) { // Главный процесс собирает данные
            sum_recv = sum_proc; // Добавляет свою часть
            for (i = 1; i < proc_num; i++) { // Получает данные от других процессов
                MPI_Recv(&sum_proc, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status);
                sum_recv += sum_proc; // Суммирует их
            }
        } else { // Остальные процессы отправляют данные главному
            MPI_Send(&sum_proc, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }

        MPI_Barrier(MPI_COMM_WORLD); // Синхронизируем процессы

        end_time = MPI_Wtime();
        time_recv += end_time - st_time; // Учитываем время Recv
    }

    // Результаты и ускорение
    if (proc_rank == 0) {
        time_consistent /= 20; // Среднее время последовательного алгоритма
        time_reduce /= 20; // Среднее время Reduce
        time_recv /= 20; // Среднее время Recv
        time_init /= 20; // Среднее время инициализации

        S_reduce = time_consistent / time_reduce; // Ускорение для Reduce
        S_recv = time_consistent / time_recv; // Ускорение для Recv

        // Вывод результатов
        printf("\nTime of work consistent algorithm is %f. \t Result sum: %f ", time_consistent, sum_consistent);
        printf("\nTime of work reduce algorithm is %f. S is %f \t Result sum: %f ", time_reduce, S_reduce, sum_reduce);
        printf("\nTime of work recv algorithm is %f. S is %f \t Result sum: %f \n", time_recv, S_recv, sum_recv);
        printf("Init time: %f \n", time_init);
    }

    // Завершаем работу MPI
    MPI_Finalize();

    return 0;
}
