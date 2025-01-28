#include "executor.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "future.h"
#include "mio.h"
#include "waker.h"

/**
 * @brief Structure to represent the current-thread executor.
 */
struct Executor {
    Mio *mio;
    Future **queue;
    size_t max_size;
    size_t count;
    size_t head;
    size_t tail;
    size_t pending_count;
    bool running;
};

// TODO: delete this once not needed.
// #define UNIMPLEMENTED (exit(42))

Executor* executor_create(size_t max_queue_size) {
    Executor* exec = (Executor*)malloc(sizeof(Executor));
    if (!exec) {
        return NULL;
    }
    exec->mio = mio_create(exec);
    if (!exec->mio) {
        free(exec);
        return NULL;
    }
    exec->queue = (Future**)malloc(max_queue_size * sizeof(Future*));
    if (!exec->queue) {
        mio_destroy(exec->mio);
        free(exec);
        return NULL;
    }
    exec->max_size = max_queue_size;
    exec->head = 0;
    exec->tail = 0;
    exec->count = 0;
    exec->pending_count = 0;
    exec->running = false;
    return exec;
}

void waker_wake(Waker* waker) {
    Executor* exec = waker->executor;
    if (exec->count < exec->max_size) {
        exec->queue[exec->tail] = waker->future;
        exec->tail = (exec->tail + 1) % exec->max_size;
        exec->count++;
    }
}

void executor_spawn(Executor* executor, Future* fut) {
    if (!executor || !fut) return;
    fut->is_active = true;
    executor->pending_count++;
    if (executor->count < executor->max_size) {
        executor->queue[executor->tail] = fut;
        executor->tail = (executor->tail + 1) % executor->max_size;
        executor->count++;
    }
}

void executor_run(Executor* executor) {
    if (!executor) return;
    executor->running = true;
    while (executor->running) {
        if (executor->pending_count == 0) {
            executor->running = false;
            break;
        }
        if (executor->count == 0) {
            mio_poll(executor->mio);
            continue;
        }

        Future *current = executor->queue[executor->head];
        executor->head = (executor->head + 1) % executor->max_size;
        executor->count--;

        Waker waker;
        waker.executor = executor;
        waker.future = current;

        FutureState state = current->progress(current, executor->mio, waker);
        if (state != FUTURE_PENDING) {
            current->is_active = false;
            executor->pending_count--;
        }
    }
    executor->running = false;
}

void executor_destroy(Executor* executor) {
    if (!executor) return;
    mio_destroy(executor->mio);
    free(executor->queue);
    free(executor);
 }
