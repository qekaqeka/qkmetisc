#pragma once

#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#ifdef NDEBUG
#define pthread_assert_lockdep(mtx) do while(0)
#else
void _pthread_assert_lockdep(pthread_mutex_t *mtx) {
    if ( pthread_mutex_trylock(mtx) ) {
        if ( errno == EBUSY ) {
            return;
        } else {
            abort();
        }
    }
    abort();
    pthread_mutex_unlock(mtx);
}
#define pthread_assert_lockdep(mtx) _pthread_assert_lockdep(mtx)
#endif
