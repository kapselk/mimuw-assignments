#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "common/io.h"
#include "common/sumset.h"

#define MAX_STACK_SIZE 3000

typedef struct {
    Sumset a;
    Sumset* b;
    bool stage;
} StackFrame;

typedef struct {
    Solution localBest;
    StackFrame stack[MAX_STACK_SIZE];
    int stackTop;
} ThreadData;

typedef struct {
    StackFrame data[MAX_STACK_SIZE];
    pthread_mutex_t mutex;
} Stack;

InputData input_data;
Stack globalStack;

void* worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int stackTop = data->stackTop;
    // int solved = 0;
    while (true) {
        while (stackTop > 0) {
            // there are some tasks to do in local stack
            // printf("stackTop: %d\n", stackTop);
            StackFrame* frame = &data->stack[stackTop - 1];
            if (frame->stage) {
                stackTop--;
                continue;
            }
            Sumset* a = &frame->a;
            Sumset* b = frame->b;
            // solved++;
            if (a->sum > b->sum) {
                Sumset* tmp = a;
                a = b;
                b = tmp;
            }
            if (is_sumset_intersection_trivial(a, b)) {
                bool child = false;
                for (int i = input_data.d; i >= a->last; i--) {
                    if (!does_sumset_contain(b, i)) {
                        // stackTop++;
                        Sumset* a_with_i = &data->stack[stackTop].a;
                        sumset_add(a_with_i, a, i);
                        data->stack[stackTop].b = b;
                        data->stack[stackTop++].stage = false;
                        child = true;
                    }
                }
                if (child) {
                    frame->stage = true;
                    continue;
                }
            } else if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
                if (b->sum > data->localBest.sum) {
                    solution_build(&data->localBest, &input_data, a, b);
                }
            }
            stackTop--;
        }
        break;
    }
    // printf("Thread %lu solved %d tasks\n", pthread_self(), solved);
    return NULL;
}

int main() {
    input_data_read(&input_data);
    // ThreadData threadData[input_data.t];
    ThreadData* threadData = (ThreadData*)malloc(input_data.t * sizeof(ThreadData));
    for (int i = 0; i < input_data.t; i++) {
        solution_init(&threadData[i].localBest);
        threadData[i].stackTop = 0;
    }
    globalStack.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    Sumset* a = &input_data.a_start, *b = &input_data.b_start;
    if (a->sum > b->sum) {
        Sumset* tmp = a;
        a = b;
        b = tmp;
    }
    Sumset* to_free[input_data.d + 1];
    for (int i = a->last; i <= input_data.d; i++) {
        int tid = i % input_data.t;
        to_free[i] = NULL;
        // printf("i = %d ; tid = %d\n", i, tid);
        if (!does_sumset_contain(b, i)) {
            Sumset* a_with_i = (Sumset*)malloc(sizeof(Sumset));
            to_free[i] = a_with_i;
            sumset_add(a_with_i, a, i);
            threadData[tid].stack[threadData[tid].stackTop].a = *a_with_i;
            threadData[tid].stack[threadData[tid].stackTop].b = b;
            threadData[tid].stack[threadData[tid].stackTop++].stage = false;
        }
    }

    pthread_t threads[input_data.t];
    for (int i = 0; i < input_data.t; i++) {
        pthread_create(&threads[i], NULL, worker, &threadData[i]);
    }
    for (int i = 0; i < input_data.t; i++) {
        pthread_join(threads[i], NULL);
    }

    Solution best_solution;
    solution_init(&best_solution);
    if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
        solution_build(&best_solution, &input_data, a, b);
    }
    for (int i = 0; i < input_data.t; i++) {
        if (threadData[i].localBest.sum > best_solution.sum) {
            best_solution = threadData[i].localBest;
        }
    }
    solution_print(&best_solution);

    for (int i = a->last; i <= input_data.d; i++) {
        if (to_free[i] != NULL) {
            free(to_free[i]);
        }
    }
    free(threadData);
    return 0;
}
