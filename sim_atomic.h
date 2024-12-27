/* Atomic get/put/add/sub/inc/dec support. Fashioned after the SDL2 approach to
 * atomic variables. Uses compiler and platform intrinsics whenever possible,
 * falling back to a mutex-based approach when necessary.
 *
 * Supported compilers: GCC and Clang across all platforms (including Windows,
 *   if compiling within a MinGW environment.)
 *
 * Platforms: Windows with MSVC.
 *
 * Types: 
 * 
 * sim_atomic_value_t: The wrapper structure that encapsulates the atomic
 *   value. Normally, if using GCC, Clang or MSVC, the value is managed via the
 *   appropriate intrinsics. The fallback is a pthread mutex.
 *
 * sim_atomic_type_t: The underlying type inside the sim_atomic_value_t wrapper.
 *   This is a long for GCC and Clang, LONG for Windows.
 *
 * Clang's atomic intrinsics are slightly different as compared to GCC: For add,
 * subtract operations, Clang's atomics fetch and return the old value. So,
 * Clang takes an extra atomic load to fetch the new value.
 *
 * sim_atomic_init(sim_atomic_value_t *): Initialize an atomic wrapper. The
 *   value is set to 0 and the mutex is initialized (when necessary.)
 *
 * sim_atomic_destroy(sim_atomic_value_t *p): The inverse of sim_atomic_init().
 *   The value is set to -1. When necessary, the mutex is destroyed.
 *
 * sim_atomic_get(sim_atomic_value_t *p): Atomically returns the
 * sim_atomic_type_t value in the wrapper.
 *
 * sim_atomic_put(sim_atomic_value_t *p, sim_atomic_type_t newval): Atomically
 * stores a new value in the wrapper.
 *
 * sim_atomic_add, sim_atomic_sub(sim_atomic_value_t *p, sim_atomic_type_t x):
 *   Atomically add or subtract a quantity to or from the wrapper's value.
 * 
 * sim_atomic_inc, sim_atomic_dec(sim_atomic_value_t *p): Atomically increment or
 *   decrement the wrapper's value.
 */

#if !defined(SIM_ATOMIC_H)

#include <pthread.h>

/* TODO:  defined(__DECC_VER) && defined(_IA64) -- DEC C on Itanium looks like it
 * might be the same as Windows' interlocked API. Contribtion/correction needed. */

/* NON-FEATURE: Older GCC __sync_* primitives. These have been deprecated for a long
 * time. Explicitly not supported. */

#if (defined(_WIN32) || defined(_WIN64)) || \
     (defined(__ATOMIC_ACQ_REL) && defined(__ATOMIC_SEQ_CST) && defined(__ATOMIC_ACQUIRE))
/* Atomic operations available! */
#define HAVE_ATOMIC_PRIMS 1
#else
#define HAVE_ATOMIC_PRIMS 0
#endif

/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * Value type and wrapper for atomics:
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

#if !defined(_WIN32) && !defined(_WIN64)
typedef long sim_atomic_type_t;
#else
typedef LONG sim_atomic_type_t;
#endif

typedef struct {
#if !HAVE_ATOMIC_PRIMS
    /* If the compiler doesn't support atomic intrinsics, the backup plan is
     * a mutex. */
    pthread_mutex_t value_lock;
#endif

    volatile sim_atomic_type_t value;
} sim_atomic_value_t;

/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * Initialization, destruction:
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

static SIM_INLINE void sim_atomic_init(sim_atomic_value_t *p)
{
#if !HAVE_ATOMIC_PRIMS
    pthread_mutex_init(&p->value_lock, NULL);
#endif
    p-> value = 0;
}

static SIM_INLINE void sim_atomic_destroy(sim_atomic_value_t *p)
{
#if !HAVE_ATOMIC_PRIMS
    pthread_mutex_destroy(&p->value_lock);
#endif
    p-> value = -1;
}

/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * Primitives:
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

static SIM_INLINE sim_atomic_type_t sim_atomic_get(sim_atomic_value_t *p)
{
    sim_atomic_type_t retval;

#if HAVE_ATOMIC_PRIMS
#  if defined(__ATOMIC_ACQUIRE) && (defined(__GNUC__) || defined(__clang__))
        __atomic_load(&p->value, &retval, __ATOMIC_ACQUIRE);
#  elif defined(_WIN32) || defined(_WIN64)
#    if defined(_M_IX86) || defined(_M_X64)
        /* Intel Total Store Ordering optimization. */
        retval = p->value;
#    else
        retval = InterlockedOr(&p->value, 0);
#    endif
#  else
#    error "sim_atomic_get: No intrinsic?"
        retval = -1;
#  endif
#else
    pthread_mutex_lock(&p->value_lock);
    retval = p->value;
    pthread_mutex_unlock(&p->value_lock);
#endif

    return retval;
}

static SIM_INLINE void sim_atomic_put(sim_atomic_value_t *p, sim_atomic_type_t newval)
{
#if HAVE_ATOMIC_PRIMS
#  if defined(__ATOMIC_SEQ_CST) && (defined(__GNUC__) || defined(__clang__))
    /* Clang doesn't like __ATOMIC_ACQ_REL for atomic stores. And for good reason: sequential
     * consistency is the appropriate memory ordering for storing values. */
    __atomic_store(&p->value, &newval, __ATOMIC_SEQ_CST);
#  elif defined(_WIN32) || defined(_WIN64)
#    if defined(_M_IX86) || defined(_M_X64)
        /* Intel Total Store Ordering optimization. */
        p->value = newval;
#    else
        sim_atomic_type_t retval, cmp;

        do {
            cmp = sim_atomic_int_get(p);
            retval = InterlockedCompareExchange(&p->value, newval, cmp);
        } while (retval != cmp);
#    endif
#  else
#    error "sim_atomic_put: No intrinsic?"
#  endif
#else
    pthread_mutex_lock(&p->value_lock);
    p->value = newval;
    pthread_mutex_unlock(&p->value_lock);
#endif
}

static SIM_INLINE sim_atomic_type_t sim_atomic_add(sim_atomic_value_t *p, sim_atomic_type_t x)
{
    sim_atomic_type_t retval;

#if HAVE_ATOMIC_PRIMS
#  if defined(__ATOMIC_ACQ_REL)
#    if defined(__GNUC__)
        retval = __atomic_add_fetch(&p->value, x, __ATOMIC_ACQ_REL);
#    elif defined(__clang__)
#      if __LONG_WIDTH__ == 32
            (void) __atomic_fetch_add_4(&p->value, x, __ATOMIC_ACQ_REL);
#      else if __LONG_WIDTH__ == 64
            (void) __atomic_fetch_add_8(&p->value, x, __ATOMIC_ACQ_REL);
#      endif
            /* Return the updated value. */
            retval = sim_atomic_get(p);
#    else
#      error "sim_atomic_add: No __ATOMIC_ACQ_REL intrinsic?"
#    endif
#  elif defined(_WIN32) || defined(_WIN64)
        retval = InterlockedAdd(&p->value, x);
#  else
#    error "sim_atomic_add: No intrinsic?"
#  endif
#else
    pthread_mutex_lock(&p->value_lock);
    p->value += newval;
    retval = p->value;
    pthread_mutex_unlock(&p->value_lock);
#endif

    return retval;
}

static SIM_INLINE sim_atomic_type_t sim_atomic_sub(sim_atomic_value_t *p, sim_atomic_type_t x)
{
    sim_atomic_type_t retval;

#if HAVE_ATOMIC_PRIMS
#  if defined(__ATOMIC_ACQ_REL)
#    if defined(__GNUC__)
        retval = __atomic_sub_fetch(&p->value, x, __ATOMIC_ACQ_REL);
#    elif defined(__clang__)
#      if __LONG_WIDTH__ == 32
            __atomic_fetch_sub_4(&p->value, x, __ATOMIC_ACQ_REL);
#      else if __LONG_WIDTH__ == 64
            __atomic_fetch_sub_8(&p->value, x, __ATOMIC_ACQ_REL);
#      endif
            /* Return the updated value. */
            retval = sim_atomic_get(p);
#    else
#      error "sim_atomic_sub: No __ATOMIC_ACQ_REL intrinsic?"
#    endif
#  elif defined(_WIN32) || defined(_WIN64)
        /* No InterlockedSub() -- use the math identity. */
        retval = sim_atomic_add(p, -x);
#  else
#    error "sim_atomic_sub: No intrinsic?"
#  endif
#else
    pthread_mutex_lock(&p->value_lock);
    p->value -= newval;
    retval = p->value;
    pthread_mutex_unlock(&p->value_lock);
#endif

    return retval;
}

static SIM_INLINE sim_atomic_type_t sim_atomic_inc(sim_atomic_value_t *p)
{
    sim_atomic_type_t retval;

#if HAVE_ATOMIC_PRIMS
#  if !defined(_WIN32) && !defined(_WIN64)
        retval = sim_atomic_add(p, 1);
#  elif defined(_WIN32) || defined(_WIN64)
        retval = InterlockedIncrement(&p->value);
#  else
#    error "sim_atomic_inc: No intrinsic?"
#  endif
#else
    pthread_mutex_lock(&p->value_lock);
    retval = ++p->value;
    pthread_mutex_unlock(&p->value_lock);
#endif

    return retval;
}

static SIM_INLINE sim_atomic_type_t sim_atomic_dec(sim_atomic_value_t *p)
{
    sim_atomic_type_t retval;

#if HAVE_ATOMIC_PRIMS
#  if !defined(_WIN32) && !defined(_WIN64)
        retval = sim_atomic_sub(p, 1);
#  elif defined(_WIN32) || defined(_WIN64)
        retval = InterlockedDecrement(&p->value);
#  else
#    error "sim_atomic_dec: No intrinsic?"
#  endif
#else
    pthread_mutex_lock(&p->value_lock);
    retval = --p->value;
    pthread_mutex_unlock(&p->value_lock);
#endif

    return retval;
}


#define SIM_ATOMIC_H
#endif
