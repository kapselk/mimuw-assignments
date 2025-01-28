#include "future_combinators.h"
#include <stdlib.h>

#include "future.h"
#include "waker.h"

// TODO: delete this once not needed.
// #define UNIMPLEMENTED (exit(42))

/* ThenFuture */
static FutureState then_future_progress(Future* self, Mio* mio, Waker waker) {
    ThenFuture* tf = (ThenFuture*)self;
    if (!tf->fut1_completed) {
        FutureState state1 = tf->fut1->progress(tf->fut1, mio, waker);
        if (state1 == FUTURE_COMPLETED) {
            tf->fut1_completed = true;
            void* result = (tf->fut1->ok != NULL) ? tf->fut1->ok : tf->fut1->arg;
            tf->fut2->arg = result;
        } else if (state1 == FUTURE_PENDING) {
            self->errcode = THEN_FUTURE_ERR_FUT1_FAILED;
            return FUTURE_FAILURE;
        } else {
            return FUTURE_PENDING;
        }
    }
    FutureState state2 = tf->fut2->progress(tf->fut2, mio, waker);
    if (state2 == FUTURE_COMPLETED) {
        self->ok = tf->fut2->ok;
        return FUTURE_COMPLETED;
    } else if (state2 == FUTURE_FAILURE) {
        self->errcode = tf->fut2->errcode;
        return FUTURE_FAILURE;
    } else {
        return FUTURE_PENDING;
    }
}

ThenFuture future_then(Future* fut1, Future* fut2)
{
    // UNIMPLEMENTED;
    return (ThenFuture) {
        .base = future_create(then_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .fut1_completed = false
    };
}

/* JoinFuture */
static FutureState join_future_progress(Future* self, Mio* mio, Waker waker) {
    JoinFuture* jf = (JoinFuture*)self;
    if (jf->fut1_completed == FUTURE_PENDING) {
        FutureState state1 = jf->fut1->progress(jf->fut1, mio, waker);
        if (state1 != FUTURE_PENDING) {
            jf->fut1_completed = state1;
            jf->result.fut1.ok = jf->fut1->ok;
            jf->result.fut1.errcode = jf->fut1->errcode;
        }
    }
    if (jf->fut2_completed == FUTURE_PENDING) {
        FutureState state2 = jf->fut2->progress(jf->fut2, mio, waker);
        if (state2 != FUTURE_PENDING) {
            jf->fut2_completed = state2;
            jf->result.fut2.ok = jf->fut2->ok;
            jf->result.fut2.errcode = jf->fut2->errcode;
        }
    }
    if (jf->fut1_completed == FUTURE_PENDING || jf->fut2_completed == FUTURE_PENDING) {
        return FUTURE_PENDING;
    }
    if (jf->fut1_completed == FUTURE_COMPLETED && jf->fut2_completed == FUTURE_COMPLETED) {
        self->ok = &jf->result;
        return FUTURE_COMPLETED;
    }
    if (jf->fut1_completed == FUTURE_FAILURE && jf->fut2_completed == FUTURE_COMPLETED) {
        self->errcode = JOIN_FUTURE_ERR_FUT1_FAILED;
        return FUTURE_FAILURE;
    }
    if (jf->fut1_completed == FUTURE_COMPLETED && jf->fut2_completed == FUTURE_FAILURE) {
        self->errcode = JOIN_FUTURE_ERR_FUT2_FAILED;
        return FUTURE_FAILURE;
    }
    if (jf->fut1_completed == FUTURE_FAILURE && jf->fut2_completed == FUTURE_FAILURE) {
        self->errcode = JOIN_FUTURE_ERR_BOTH_FUTS_FAILED;
        return FUTURE_FAILURE;
    }
    return FUTURE_FAILURE;
}
JoinFuture future_join(Future* fut1, Future* fut2)
{
    // UNIMPLEMENTED;
    return (JoinFuture) {
        .base = future_create(join_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .fut1_completed = FUTURE_PENDING,
        .fut2_completed = FUTURE_PENDING,
        .result.fut1.errcode = 0,
        .result.fut1.ok = NULL,
        .result.fut2.errcode = 0,
        .result.fut2.ok = NULL
    };
}

/* SelectFuture */
static FutureState select_future_progress(Future* future, Mio* mio, Waker waker) {
    SelectFuture* sf = (SelectFuture*)future;
    if (sf->which_completed == SELECT_COMPLETED_FUT1 || sf->which_completed == SELECT_COMPLETED_FUT2) {
        return FUTURE_COMPLETED;
    }
    FutureState state1 = FUTURE_PENDING;
    if (sf->which_completed == SELECT_COMPLETED_NONE || sf->which_completed == SELECT_FAILED_FUT2) {
        state1 = sf->fut1->progress(sf->fut1, mio, waker);
    } else {
        state1 = FUTURE_FAILURE;
    }
    FutureState state2 = FUTURE_PENDING;
    if (sf->which_completed == SELECT_COMPLETED_NONE || sf->which_completed == SELECT_FAILED_FUT1) {
        state2 = sf->fut2->progress(sf->fut2, mio, waker);
    } else {
        state2 = FUTURE_FAILURE;
    }

    if (state1 == FUTURE_COMPLETED) {
        sf->which_completed = SELECT_COMPLETED_FUT1;
        future->ok = sf->fut1->ok;
        return FUTURE_COMPLETED;
    }
    if (state2 == FUTURE_COMPLETED) {
        sf->which_completed = SELECT_COMPLETED_FUT2;
        future->ok = sf->fut2->ok;
        return FUTURE_COMPLETED;
    }
    if (state1 == FUTURE_FAILURE && state2 == FUTURE_FAILURE) {
        future->errcode = SELECT_FAILED_BOTH;
        return FUTURE_FAILURE;
    }
    if (state1 == FUTURE_FAILURE) {
        if (sf->which_completed == SELECT_COMPLETED_NONE)
            sf->which_completed = SELECT_FAILED_FUT1;
    }
    if (state2 == FUTURE_FAILURE) {
        if (sf->which_completed == SELECT_COMPLETED_NONE)
            sf->which_completed = SELECT_FAILED_FUT2;
    }
    return FUTURE_PENDING;
}

SelectFuture future_select(Future* fut1, Future* fut2)
{
    // UNIMPLEMENTED;
    return (SelectFuture) {
        .base = future_create(select_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .which_completed = SELECT_COMPLETED_NONE
    };
}
