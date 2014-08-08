/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <time.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
#include <string.h> /* memset() */
#include <setjmp.h>
#include "system-sdl.h"
#include "thread-sdl.h"
#include "../kernel-internal.h"
#include "core_alloc.h"

/* Define this as 1 to show informational messages that are not errors. */
#define THREAD_SDL_DEBUGF_ENABLED 0

#if THREAD_SDL_DEBUGF_ENABLED
#define THREAD_SDL_DEBUGF(...) DEBUGF(__VA_ARGS__)
static char __name[32];
#define THREAD_SDL_GET_NAME(thread) \
    ({ thread_get_name(__name, ARRAYLEN(__name), thread); __name; })
#else
#define THREAD_SDL_DEBUGF(...)
#define THREAD_SDL_GET_NAME(thread)
#endif

#define THREAD_PANICF(str...) \
    ({ fprintf(stderr, str); exit(-1); })

/* Thread/core entries as in rockbox core */
static struct core_entry cores[NUM_CORES];
struct thread_entry threads[MAXTHREADS];
/* Jump buffers for graceful exit - kernel threads don't stay neatly
 * in their start routines responding to messages so this is the only
 * way to get them back in there so they may exit */
static jmp_buf thread_jmpbufs[MAXTHREADS];
/* this mutex locks out other Rockbox threads while one runs,
 * that enables us to simulate a cooperative environment even if
 * the host is preemptive */
static SDL_mutex *m;
#define THREADS_RUN                 0
#define THREADS_EXIT                1
#define THREADS_EXIT_COMMAND_DONE   2
static volatile int threads_status = THREADS_RUN;

extern long start_tick;

void sim_thread_shutdown(void)
{
    int i;

    /* This *has* to be a push operation from a thread not in the pool
       so that they may be dislodged from their blocking calls. */

    /* Tell all threads jump back to their start routines, unlock and exit
       gracefully - we'll check each one in turn for it's status. Threads
       _could_ terminate via remove_thread or multiple threads could exit
       on each unlock but that is safe. */

    /* Do this before trying to acquire lock */
    threads_status = THREADS_EXIT;

    /* Take control */
    SDL_LockMutex(m);

    /* Signal all threads on delay or block */
    for (i = 0; i < MAXTHREADS; i++)
    {
        struct thread_entry *thread = &threads[i];
        if (thread->context.s == NULL)
            continue;
        SDL_SemPost(thread->context.s);
    }

    /* Wait for all threads to finish and cleanup old ones. */
    for (i = 0; i < MAXTHREADS; i++)
    {
        struct thread_entry *thread = &threads[i];
        SDL_Thread *t = thread->context.t;

        if (t != NULL)
        {
            SDL_UnlockMutex(m);
            /* Wait for it to finish */
            SDL_WaitThread(t, NULL);
            /* Relock for next thread signal */
            SDL_LockMutex(m);
            /* Already waited and exiting thread would have waited .told,
             * replacing it with t. */
            thread->context.told = NULL;
        }
        else
        {
            /* Wait on any previous thread in this location-- could be one not quite
             * finished exiting but has just unlocked the mutex. If it's NULL, the
             * call returns immediately.
             *
             * See remove_thread below for more information. */
            SDL_WaitThread(thread->context.told, NULL);
        }
    }

    SDL_UnlockMutex(m);

    /* Signal completion of operation */
    threads_status = THREADS_EXIT_COMMAND_DONE;
}

static void new_thread_id(unsigned int slot_num,
                          struct thread_entry *thread)
{
    unsigned int version =
        (thread->id + (1u << THREAD_ID_VERSION_SHIFT))
            & THREAD_ID_VERSION_MASK;

    if (version == 0)
        version = 1u << THREAD_ID_VERSION_SHIFT;

    thread->id = version | (slot_num & THREAD_ID_SLOT_MASK);
}

static struct thread_entry * find_empty_thread_slot(void)
{
    struct thread_entry *thread = NULL;
    int n;

    for (n = 0; n < MAXTHREADS; n++)
    {
        int state = threads[n].state;

        if (state == STATE_KILLED)
        {
            thread = &threads[n];
            break;
        }
    }

    return thread;
}


/* Initialize SDL threading */
void init_threads(void)
{
    static uintptr_t main_stack[] = { DEADBEEF, 0 };
    struct thread_entry *thread;
    int n;

    memset(cores, 0, sizeof(cores));
    memset(threads, 0, sizeof(threads));

    m = SDL_CreateMutex();

    if (SDL_LockMutex(m) == -1)
    {
        fprintf(stderr, "Couldn't lock mutex\n");
        return;
    }

    /* Initialize all IDs */
    for (n = 0; n < MAXTHREADS; n++)
        threads[n].id = THREAD_ID_INIT(n);

    /* Slot 0 is reserved for the main thread - initialize it here and
       then create the SDL thread - it is possible to have a quick, early
       shutdown try to access the structure. */
    thread = &threads[0];
    thread->stack = main_stack;
    thread->stack_size = sizeof (main_stack);
    thread->name = "main";
    thread->state = STATE_RUNNING;
    thread->context.s = SDL_CreateSemaphore(0);
    thread->context.t = NULL; /* NULL for the implicit main thread */
    cores[CURRENT_CORE].running = thread;
 
    if (thread->context.s == NULL)
    {
        fprintf(stderr, "Failed to create main semaphore\n");
        return;
    }

    /* Tell all threads jump back to their start routines, unlock and exit
       gracefully - we'll check each one in turn for it's status. Threads
       _could_ terminate via remove_thread or multiple threads could exit
       on each unlock but that is safe. */

    /* Setup jump for exit */
    if (setjmp(thread_jmpbufs[0]) == 0)
    {
        THREAD_SDL_DEBUGF("Main thread: %p\n", thread);
        return;
    }

    SDL_UnlockMutex(m);

    /* Set to 'COMMAND_DONE' when other rockbox threads have exited. */
    while (threads_status < THREADS_EXIT_COMMAND_DONE)
        SDL_Delay(10);

    SDL_DestroyMutex(m);

    /* We're the main thead - perform exit - doesn't return. */
    sim_do_exit();
}

void sim_thread_exception_wait(void)
{
    while (1)
    {
        SDL_Delay(HZ/10);
        if (threads_status != THREADS_RUN)
            thread_exit();
    }
}

/* A way to yield and leave the threading system for extended periods */
void sim_thread_lock(void *me)
{
    SDL_LockMutex(m);
    cores[CURRENT_CORE].running = (struct thread_entry *)me;

    if (threads_status != THREADS_RUN)
        thread_exit();
}

void * sim_thread_unlock(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    SDL_UnlockMutex(m);
    return current;
}

struct thread_entry * thread_id_entry(unsigned int thread_id)
{
    return &threads[thread_id & THREAD_ID_SLOT_MASK];
}

static void add_to_list_l(struct thread_entry **list,
                          struct thread_entry *thread)
{
    if (*list == NULL)
    {
        /* Insert into unoccupied list */
        thread->l.next = thread;
        thread->l.prev = thread;
        *list = thread;
    }
    else
    {
        /* Insert last */
        thread->l.next = *list;
        thread->l.prev = (*list)->l.prev;
        thread->l.prev->l.next = thread;
        (*list)->l.prev = thread;
    }
}

static void remove_from_list_l(struct thread_entry **list,
                               struct thread_entry *thread)
{
    if (thread == thread->l.next)
    {
        /* The only item */
        *list = NULL;
        return;
    }

    if (thread == *list)
    {
        /* List becomes next item */
        *list = thread->l.next;
    }

    /* Fix links to jump over the removed entry. */
    thread->l.prev->l.next = thread->l.next;
    thread->l.next->l.prev = thread->l.prev;
}

unsigned int thread_self(void)
{
    return cores[CURRENT_CORE].running->id;
}

struct thread_entry* thread_self_entry(void)
{
    return cores[CURRENT_CORE].running;
}

void switch_thread(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;

    enable_irq();

    switch (current->state)
    {
    case STATE_RUNNING:
    {
        SDL_UnlockMutex(m);
        /* Any other thread waiting already will get it first */
        SDL_LockMutex(m);
        break;
        } /* STATE_RUNNING: */

    case STATE_BLOCKED:
    {
        int oldlevel;

        SDL_UnlockMutex(m);
        SDL_SemWait(current->context.s);
        SDL_LockMutex(m);

        oldlevel = disable_irq_save();
        current->state = STATE_RUNNING;
        restore_irq(oldlevel);
        break;
        } /* STATE_BLOCKED: */

    case STATE_BLOCKED_W_TMO:
    {
        int result, oldlevel;

        SDL_UnlockMutex(m);
        result = SDL_SemWaitTimeout(current->context.s, current->tmo_tick);
        SDL_LockMutex(m);

        oldlevel = disable_irq_save();

        if (current->state == STATE_BLOCKED_W_TMO)
        {
            /* Timed out */
            remove_from_list_l(current->bqp, current);

#ifdef HAVE_WAKEUP_EXT_CB
            if (current->wakeup_ext_cb != NULL)
                current->wakeup_ext_cb(current);
#endif
            current->state = STATE_RUNNING;
        }

        if (result == SDL_MUTEX_TIMEDOUT)
        {
            /* Other signals from an explicit wake could have been made before
             * arriving here if we timed out waiting for the semaphore. Make
             * sure the count is reset. */
            while (SDL_SemValue(current->context.s) > 0)
                SDL_SemTryWait(current->context.s);
        }

        restore_irq(oldlevel);
        break;
        } /* STATE_BLOCKED_W_TMO: */

    case STATE_SLEEPING:
    {
        SDL_UnlockMutex(m);
        SDL_SemWaitTimeout(current->context.s, current->tmo_tick);
        SDL_LockMutex(m);
        current->state = STATE_RUNNING;
        break;
        } /* STATE_SLEEPING: */
    }

#ifdef DEBUG
    core_check_valid();
#endif
    cores[CURRENT_CORE].running = current;

    if (threads_status != THREADS_RUN)
        thread_exit();
}

void sleep_thread(int ticks)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    int rem;

    current->state = STATE_SLEEPING;

    rem = (SDL_GetTicks() - start_tick) % (1000/HZ);
    if (rem < 0)
        rem = 0;

    current->tmo_tick = (1000/HZ) * ticks + ((1000/HZ)-1) - rem;
}

void block_thread(struct thread_entry *current, int ticks)
{
    if (ticks < 0)
        current->state = STATE_BLOCKED;
    else
    {
        current->state = STATE_BLOCKED_W_TMO;
        current->tmo_tick = (1000/HZ)*ticks;
    }

    add_to_list_l(current->bqp, current);
}

unsigned int wakeup_thread_(struct thread_entry **list)
{
    struct thread_entry *thread = *list;

    if (thread != NULL)
    {
        switch (thread->state)
        {
        case STATE_BLOCKED:
        case STATE_BLOCKED_W_TMO:
            remove_from_list_l(list, thread);
            thread->state = STATE_RUNNING;
            SDL_SemPost(thread->context.s);
            return THREAD_OK;
        }
    }

    return THREAD_NONE;
}

void thread_thaw(unsigned int thread_id)
{
    struct thread_entry *thread = thread_id_entry(thread_id);

    if (thread->id == thread_id && thread->state == STATE_FROZEN)
    {
        thread->state = STATE_RUNNING;
        SDL_SemPost(thread->context.s);
    }
}

int runthread(void *data)
{
    struct thread_entry *current;
    jmp_buf *current_jmpbuf;

    /* Cannot access thread variables before locking the mutex as the
       data structures may not be filled-in yet. */
    SDL_LockMutex(m);
    cores[CURRENT_CORE].running = (struct thread_entry *)data;
    current = cores[CURRENT_CORE].running;
    current_jmpbuf = &thread_jmpbufs[current - threads];

    /* Setup jump for exit */
    if (setjmp(*current_jmpbuf) == 0)
    {
        /* Run the thread routine */
        if (current->state == STATE_FROZEN)
        {
            SDL_UnlockMutex(m);
            SDL_SemWait(current->context.s);
            SDL_LockMutex(m);
            cores[CURRENT_CORE].running = current;
        }

        if (threads_status == THREADS_RUN)
        {
            current->context.start();
            THREAD_SDL_DEBUGF("Thread Done: %d (%s)\n",
                              current - threads, THREAD_SDL_GET_NAME(current));
            /* Thread routine returned - suicide */
        }

        thread_exit();
    }
    else
    {
        /* Unlock and exit */
        SDL_UnlockMutex(m);
    }

    return 0;
}

unsigned int create_thread(void (*function)(void),
                           void* stack, size_t stack_size,
                           unsigned flags, const char *name)
{
    struct thread_entry *thread;
    SDL_Thread* t;
    SDL_sem *s;

    THREAD_SDL_DEBUGF("Creating thread: (%s)\n", name ? name : "");

    thread = find_empty_thread_slot();
    if (thread == NULL)
    {
        DEBUGF("Failed to find thread slot\n");
        return 0;
    }

    s = SDL_CreateSemaphore(0);
    if (s == NULL)
    {
        DEBUGF("Failed to create semaphore\n");
        return 0;
    }

    t = SDL_CreateThread(runthread, thread);
    if (t == NULL)
    {
        DEBUGF("Failed to create SDL thread\n");
        SDL_DestroySemaphore(s);
        return 0;
    }

    unsigned int stack_words = stack_size / sizeof (uintptr_t);
    for (unsigned int i = stack_words; i-- > 0;)
        ((uintptr_t *)stack)[i] = DEADBEEF;

    thread->stack = stack;
    thread->stack_size = stack_size;
    thread->name = name;
    thread->state = (flags & CREATE_THREAD_FROZEN) ?
        STATE_FROZEN : STATE_RUNNING;
    thread->context.start = function;
    thread->context.t = t;
    thread->context.s = s;

    THREAD_SDL_DEBUGF("New Thread: %d (%s)\n",
                      thread - threads, THREAD_SDL_GET_NAME(thread));

    return thread->id;
}

static void remove_thread(unsigned int thread_id)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    struct thread_entry *thread = thread_id_entry(thread_id);

    SDL_Thread *t;
    SDL_sem *s;

    if (thread->id != thread_id)
        return;

    int oldlevel = disable_irq_save();

    t = thread->context.t;
    s = thread->context.s;

    /* Wait the last thread here and keep this one or SDL will leak it since
     * it doesn't free its own library allocations unless a wait is performed.
     * Such behavior guards against the memory being invalid by the time
     * SDL_WaitThread is reached and also against two different threads having
     * the same pointer. It also makes SDL_WaitThread a non-concurrent function.
     *
     * However, see more below about SDL_KillThread.
     */
    SDL_WaitThread(thread->context.told, NULL);

    thread->context.t = NULL;
    thread->context.s = NULL;
    thread->context.told = t;

    if (thread != current)
    {
        switch (thread->state)
        {
        case STATE_BLOCKED:
        case STATE_BLOCKED_W_TMO:
            /* Remove thread from object it's waiting on */
            remove_from_list_l(thread->bqp, thread);

#ifdef HAVE_WAKEUP_EXT_CB
            if (thread->wakeup_ext_cb != NULL)
                thread->wakeup_ext_cb(thread);
#endif
            break;
        }

        SDL_SemPost(s);
    }

    THREAD_SDL_DEBUGF("Removing thread: %d (%s)\n",
        thread - threads, THREAD_SDL_GET_NAME(thread));

    new_thread_id(thread->id, thread);
    thread->state = STATE_KILLED;
    thread_queue_wake(&thread->queue);

    SDL_DestroySemaphore(s);

    if (thread == current)
    {
        /* Do a graceful exit - perform the longjmp back into the thread
           function to return */
        restore_irq(oldlevel);
        longjmp(thread_jmpbufs[current - threads], 1);
    }

    /* SDL_KillThread frees the old pointer too because it uses SDL_WaitThread
     * to wait for the host to remove it. */
    thread->context.told = NULL;
    SDL_KillThread(t);
    restore_irq(oldlevel);
}

void thread_exit(void)
{
    unsigned int id = thread_self();
    remove_thread(id);
    /* This should never and must never be reached - if it is, the
     * state is corrupted */
    THREAD_PANICF("thread_exit->K:*R (ID: %d)", id);
    while (1);
}

void thread_wait(unsigned int thread_id)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    struct thread_entry *thread = thread_id_entry(thread_id);

    if (thread->id == thread_id && thread->state != STATE_KILLED)
    {
        current->bqp = &thread->queue;
        block_thread(current, TIMEOUT_BLOCK);
        switch_thread();
    }
}

/*---------------------------------------------------------------------------
 * Suspends a thread's execution for at least the specified number of ticks.
 *
 * May result in CPU core entering wait-for-interrupt mode if no other thread
 * may be scheduled.
 *
 * NOTE: sleep(0) sleeps until the end of the current tick
 *       sleep(n) that doesn't result in rescheduling:
 *                      n <= ticks suspended < n + 1
 *       n to n+1 is a lower bound. Other factors may affect the actual time
 *       a thread is suspended before it runs again.
 *---------------------------------------------------------------------------
 */
unsigned sleep(unsigned ticks)
{
    disable_irq();
    sleep_thread(ticks);
    switch_thread();
    return 0;
}

/*---------------------------------------------------------------------------
 * Elects another thread to run or, if no other thread may be made ready to
 * run, immediately returns control back to the calling thread.
 *---------------------------------------------------------------------------
 */
void yield(void)
{
    switch_thread();
}
