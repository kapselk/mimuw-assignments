#include "mio.h"

#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

#include "debug.h"
#include "executor.h"
#include "waker.h"

// Maximum number of events to handle per epoll_wait call.
#define MAX_EVENTS 64

typedef struct MioRegistration {
    int fd;
    uint32_t events;
    Waker waker;
    struct MioRegistration* next;
} MioRegistration;

struct Mio {
    int epoll_fd;
    MioRegistration* registrations;
};

// TODO: delete this once not needed.
// #define UNIMPLEMENTED (exit(42))

Mio* mio_create(Executor* executor) {
    Mio* mio = (Mio*)malloc(sizeof(Mio));
    if (!mio)
        return NULL;
    mio->epoll_fd = epoll_create1(0);
    if (mio->epoll_fd == -1) {
        free(mio);
        return NULL;
    }
    mio->registrations = NULL;
    return mio;
}

void mio_destroy(Mio* mio) {
    if (!mio)
        return;
    MioRegistration* reg = mio->registrations;
    while (reg) {
        MioRegistration* next = reg->next;
        free(reg);
        reg = next;
    }
    close(mio->epoll_fd);
    free(mio);
}

int mio_register(Mio* mio, int fd, uint32_t events, Waker waker)
{
    debug("Registering (in Mio = %p) fd = %d with", mio, fd);

    if (!mio)
        return -1;
    MioRegistration* reg = (MioRegistration*)malloc(sizeof(MioRegistration));
    if (!reg)
        return -1;
    reg->fd = fd;
    reg->events = events;
    reg->waker = waker;
    reg->next = NULL;

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.ptr = reg;
    if (epoll_ctl(mio->epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        free(reg);
        return -1;
    }
    reg->next = mio->registrations;
    mio->registrations = reg;
    return 0;
}

int mio_unregister(Mio* mio, int fd)
{
    debug("Unregistering (from Mio = %p) fd = %d\n", mio, fd);

    if (!mio)
        return -1;
    MioRegistration* prev = NULL, *cur = mio->registrations;
    while (cur) {
        if (cur->fd == fd)
            break;
        prev = cur;
        cur = cur->next;
    }
    if (!cur)
        return -1;
    if (epoll_ctl(mio->epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0)
        return -1;
    if (prev)
        prev->next = cur->next;
    else
        mio->registrations = cur->next;
    free(cur);
    return 0;
}

void mio_poll(Mio* mio)
{
    debug("Mio (%p) polling\n", mio);

    if (!mio)
        return;
    struct epoll_event events[MAX_EVENTS];
    int n = epoll_wait(mio->epoll_fd, events, MAX_EVENTS, -1);
    if (n < 0)
        return;
    for (int i = 0; i < n; i++) {
        MioRegistration* reg = (MioRegistration*)events[i].data.ptr;
        debug("Mio: Event on fd %d; waking future %p", reg->fd, reg->waker.future);
        waker_wake(&reg->waker);
    }
}
