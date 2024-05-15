// libgomp.cpp
//
// A subset of libgomp for bare metal.
// This is a simplistic implementation for a single-core processor for learning, development, and testing.
//
// Target is an ARM CPU with very simple bare metal threading support.
//
// These implementations only work on a single-core, non-preemtive system,
// until they is made thread-safe.

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <context.hpp>
#include <ContextFIFO.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>
#include "FIFO.hpp"
#include "libgomp.hpp"
#include "boundaries.h"
#include "tim.h"


// The threads' stacks
// thread 0 (background) uses the stack defined in the linker script

static char InterpStack[3072];                          // the stack for the interpreter thread
static char gomp_stacks[GOMP_MAX_NUM_THREADS-2][GOMP_STACK_SIZE] __ALIGNED(16);

// An array of tasks.
static task tasks[GOMP_NUM_TASKS];

// A pool of idle tasks
static FIFO<task *, GOMP_NUM_TASKS> task_pool;

// an array of omp_threads
omp_thread omp_threads[GOMP_MAX_NUM_THREADS];

// A pool of idle threads
static FIFO<omp_thread *, GOMP_MAX_NUM_THREADS> thread_pool;


int dyn_var = 0;

// the default number of parallel threads
static int gomp_nthreads_var = OMP_NUM_THREADS;

int omp_verbose = OMP_VERBOSE_DEFAULT;
#define DPRINT(level) if(omp_verbose>=level)printf

// either run the implicit task, or try to get one from the pool of ready tasks
// TODO -- each team needs its own private ready task pool

void run_implicit(task *task)
    {
    omp_thread &thread = *omp_this_thread();
    omp_thread &team = *omp_this_team();

    TASKFN *fn;
    char *data;

    DPRINT(2)("start implicit task %8p, id = %d(%d)\n", task, thread.team_id, thread.id);
    fn = task->fn;                      // run the assigned implicit task
    data = task->data;
    fn(data);
    DPRINT(2)("end   implicit task %8p, id = %d(%d)\n", task, thread.team_id, thread.id);
    task_pool.add(task);                // return the task to the pool
    team.task_count--;
    thread.task = 0;                    // forget the completed task
    }

void run_explicit(task *task)
    {
    omp_thread &thread = *omp_this_thread();
    omp_thread &team = *omp_this_team();

    TASKFN *fn;
    char *data;

    DPRINT(2)("start explicit task %8p, id = %d(%d)\n", task, thread.team_id, thread.id);
    fn = task->fn;                      // run the explicit task
    data = task->data;
    fn(data);
    DPRINT(2)("end   explicit task %8p, id = %d(%d)\n", task, thread.team_id, thread.id);
    data = data - data[-1];             // undo the arg alignment to recover the address returned from malloc
    free(data);                         // free the data
    task_pool.add(task);                // free the task
    team.task_count--;
    }


// this is the code for every member of the thread pool, except the initial thread
// The current algorithm is pretty brute force, to be fine tuned later

static uint32_t gomp_worker(uintptr_t id)
    {
    omp_thread &thread = *omp_this_thread();

    while(true)
        {
        thread.twaiting = true;
        Context::suspend();
        thread.twaiting = false;

        task *task;

        if((task=thread.task) != 0)
            {
            run_implicit(task);
            }
        else
            {
            omp_thread &team = *omp_this_team();
            if(team.task_list.take(task))         // if there are any explicit tasks waiting for a context
                {
                run_explicit(task);
                }
            }
        }

    return 0;
    }


// called by background to poll all the threads and resume them if they have something to do
void gomp_poll_threads()
    {
    for(int i=1; i<GOMP_MAX_NUM_THREADS; i++)
        {
        omp_thread *thread = &omp_threads[i];

        if(thread->twaiting)
            {
            if(thread->task)
                {
                thread->context.resume();
                }
            else
                {
                omp_thread *team = thread->team_id == 0 ? thread : thread->team;

                if(team->task_list)
                    {
                    thread->context.resume();
                    }
                }
            }
        }
    }


const char *thread_names[] =
    {
    "background",
    "interp",
    "thread 2",
    "thread 3",
    "thread 4",
    "thread 5",
    "thread 6",
    "thread 7",
    "thread 8",
    "thread 9"
    };


// clean up and re-initialize between tests
void libgomp_reinit()
    {
    gomp_nthreads_var = OMP_NUM_THREADS;
    }

// Powerup initialization of libgomp.
// Must be called after thread and threadFIFO are setup.

void libgomp_init()
    {
    // put all tasks into the idle task pool
    for(auto &task: tasks)task_pool.add(&task);

    // init the omp_threads
    // put all threads except 0 (background) into the idle thread pool and start each one
    for(unsigned i=0; i<GOMP_MAX_NUM_THREADS; i++)
        {
        omp_threads[i].id = i;

        if(i<NUM_ELEMENTS(thread_names))
            {
            omp_threads[i].name = thread_names[i];
            }

        if(i == 0)
            {
            __asm__ __volatile__(
            "   mov r9, %[bgctx]"           // init the thread pointer to the background thread
            :
            : [bgctx]"r"(&omp_threads[0])
            :
            );

            omp_threads[i].team = (omp_thread *)0xFFFFFFFF;         // background's team pointer must never be used, since background cannot be a member of a team
            omp_threads[i].stack_low = (char *)&_stack_start;
            omp_threads[i].stack_high = (char *)&_stack_end;
            }
        else if(i == 1)
            {
            libgomp_start_thread(omp_threads[i], gomp_worker, InterpStack, i);
            thread_pool.add(&omp_threads[i]);
            }
        else
            {
            libgomp_start_thread(omp_threads[i], gomp_worker, gomp_stacks[i-2], i);
            thread_pool.add(&omp_threads[i]);
            }
        }

    libgomp_reinit();
    }



extern "C"
void GOMP_parallel(
    TASKFN *fn,                                     // the context code
    char *data,                                     // the context local data
    unsigned num_threads,                           // the requested number of threads
    unsigned flags __attribute__((__unused__)))     // flags (ignored for now)
    {
    omp_thread &team = *omp_this_thread();

    if(num_threads == 0)
        {
        num_threads = gomp_nthreads_var;
        }

    team.mutex = false;
    team.tsingle = 0;
    team.sections_count = 0;
    team.sections = 0;
    team.section= 0;
    team.copyprivate = 0;
    team.team_count = 0;
    team.task_count = 0;
    team.members.init();
    team.task_list.init();

    // create a team, give each member a task, and start it
    for(unsigned i=0; i<num_threads; i++)
        {
        omp_thread *thread = 0;
        task *task = 0;
        bool ok;

        if(i == 0)
            {
            thread = &team;
            }
        else
            {
            ok = thread_pool.take(thread);
            if(!ok)
                {
                printf("thread_pool is empty\n");
                break;
                }
            thread->team = &team;
            team.members.add(thread);
            }

        thread->team_id = i;
        thread->arrived = false;
        thread->mwaiting = false;
        thread->single = 0;

        ok = task_pool.take(task);
        if(!ok)
            {
            DPRINT(2)("task_pool is empty\n");
            break;
            }

        task->fn = fn;
        task->data = data;
        team.team_count++;
        team.task_count++;
        thread->task = task;                     // this field becoming non-zero kicks off the implicit task

        DPRINT(2)("create implicit task %8p, id = %d(%d)\n", task, i, thread->id);
        }

    // since the master is also a member of this team, execute my task
    run_implicit(team.task);
    yield();

    // now that my task is done, wait for each of the other team members and any explicit tasks to complete
    while(team.task_count)
        {
        task *task;
        if(team.task_list.take(task))         // if there are any explicit tasks waiting for a context
            {
            run_explicit(task);
            }
        yield();
        }

    // disassemble the team
    while(true)
        {
        omp_thread *thread;
        if(!team.members.take(thread))break;
        thread_pool.add(thread);
        }
    }


extern "C"
void GOMP_barrier()
    {
    omp_thread &thread = *omp_this_thread();
    omp_thread &team = *omp_this_team();
    omp_thread *member;
    omp_thread **pnext;

    thread.arrived = true;                      // signal that this thread has reached the barrier

    // walk the list of all team members. If any member has not arrived yet, suspend myself.
    member = &team;
    pnext = &team.members.head;
    while(member)
        {
        if(member->arrived == false)
            {                                   // get here if any team member has not yet arrived
            thread.context.suspend();           // suspend this thread until all other threads have arrived
            return;                             // when resumed, some other thread has done all the barrier cleanup work, so just keep going
            }
        member = *pnext;
        pnext = &member->next;
        }

    // Only get here if all other team members have arrived, which mean this thread
    // is the last to arrive, and all other team members are suspended.
    // When the last team member arrives it walks the list again, clears all the "arrived" flags,
    // and resumes all the other members.

    member = &team;
    pnext = &team.members.head;
    while(member)
        {
        member->arrived = false;
        if(member != &thread)                   // don't try to resume myself
            {
            member->context.resume();           // resume any other thread in this team
            }
        member = *pnext;
        pnext = &member->next;
        }
    }


extern "C"
void GOMP_critical_start()
    {
    omp_thread &thread = *omp_this_thread();
    omp_thread &team = *omp_this_team();

    while(team.mutex == true)
        {
        thread.mwaiting = true;
        thread.context.suspend();      // suspend this thread until it can grab the mutex
        thread.mwaiting = false;
        }

    team.mutex = true;
    }

extern "C"
void GOMP_critical_end()
    {
    omp_thread &team = *omp_this_team();
    omp_thread *member = &team;
    omp_thread **pnext = &team.members.head;

    team.mutex = false;

    while(member)                               // resume a waiting team member
        {
        if(member->mwaiting == true)
            {
            member->context.resume();           // resume the next context in the rotation
            return;
            }
        else
            {
            member = *pnext;
            pnext = &member->next;
            }
        }
    }



#if 0
extern "C"
void GOMP_atomic_start()
    {
    GOMP_critical_start();
    }

extern "C"
void GOMP_atomic_end()
    {
    GOMP_critical_end();
    }
#endif


extern "C"
bool GOMP_single_start()
    {
    omp_thread &thread = *omp_this_thread();
    omp_thread &team = *omp_this_team();

    if(thread.single++ == team.tsingle)
        {
        team.tsingle++;
        return true;
        }
    else
        {
        return false;
        }
    }



extern "C"
void *GOMP_single_copy_start()
    {
    omp_thread &thread = *omp_this_thread();
    omp_thread &team = *omp_this_team();

    if(thread.single++ == team.tsingle)
        {
        team.tsingle++;
        return 0;
        }
    else
        {
        return team.copyprivate;
        }
    }


extern "C"
void GOMP_single_copy_end(void *data)
    {
    omp_thread &team = *omp_this_team();

    team.copyprivate = data;
    }




extern "C"
int GOMP_sections_next()                // for each thread that iterates the "sections"
    {
    omp_thread &team = *omp_this_team();

    if(team.section > team.sections)    // if all sections have been executed
        {
        team.section = 0;               // clear the index, it latches at zero
        }

    if(team.section == 0)               // if all sections have been run
        {
        return 0;                       // return 0, select the "end" action and stop iterating
        }

    return team.section++;          // otherwise return the current index and increment it
    }

extern "C"
int GOMP_sections_start(int num)        // each team member calls this once at the beginning of sections
    {
    omp_thread &team = *omp_this_team();

    if(team.sections_count == 0)        // when the first context gets here
        {
        team.sections = num;            // capture the number of sections
        team.section = 1;               // init to the first section
        }

    ++team.sections_count;              // count the number of threads that have started the sections

    return GOMP_sections_next();        // for the rest, start is the same as next
    }

extern "C"
void GOMP_sections_end()                // each thread runs this once when all the sections hae been executed
    {
    omp_thread &team = *omp_this_team();
    int num = omp_get_num_threads();

    if(team.sections_count == num)      // if all team members have encountered the "start"
        {
        team.sections_count = 0;        // re-arm the sections start, though note that some may still be in a section
        }

    GOMP_barrier();                     // hold everyone here until all have arrived
    }


#if 0

/////////////
// LOCKING //
/////////////

extern "C"
void omp_init_lock(omp_lock_t *lock)
    {
    *lock = 0;
    }

extern "C"
void omp_set_lock(omp_lock_t *lock)
    {
    while(*lock)
        {
        yield();
        }

    *lock = 1;
    }

extern "C"
void omp_unset_lock(omp_lock_t *lock)
    {
    *lock = 0;
    }

extern "C"
int omp_test_lock(omp_lock_t *lock)
    {
    if(*lock)
        {
        return 0;
        }

    *lock = 1;
    return 1;
    }

extern "C"
void omp_destroy_lock(omp_lock_t *lock)
    {
    *lock = 0;
    }



////////////////////
// NESTED LOCKING //
////////////////////

extern "C"
void omp_init_nest_lock(omp_nest_lock_t *lock)
    {
    lock->lock = 0;
    lock->count = 0;
    lock->owner = 0;
    }

extern "C"
void omp_set_nest_lock(omp_nest_lock_t *lock)
    {
    int id = omp_get_thread_num();

    if(lock->lock && lock->owner == id)
        {
        ++lock->count;
        return;
        }
 
    while(lock->lock==1)
        {
        yield();
        }

    lock->lock = 1;
    lock->count = 1;
    lock->owner = id;
    }

extern "C"
void omp_unset_nest_lock(omp_nest_lock_t *lock)
    {
    // It is assumed that the caller matches the owner. This is not checked.
    // It is assumed that count>0. This is not checked.

    --lock->count;
    if(lock->count == 0)
        {
        lock->owner = 0;
        lock->lock = 0;
        }
    }

extern "C"
int omp_test_nest_lock(omp_nest_lock_t *lock)
    {
    int id = omp_get_thread_num();

    if(lock->lock && lock->owner == id)
        {
        ++lock->count;
        return lock->count;
        }
 
    if(lock->lock == 0)
        {
        lock->lock = 1;
        lock->count = 1;
        lock->owner = id;
        return 1;
        }

    return 0;
    }

extern "C"
void omp_destroy_nest_lock(omp_nest_lock_t *lock)
    {
    lock->lock = 0;
    lock->count = 0;
    lock->owner = 0;
    }

#endif


///////////
// Tasks //
///////////

extern "C"
void GOMP_task (    void (*fn) (void *),
                    void *data,
                    void (*cpyfn) (void *, void *),
                    long arg_size,
                    long arg_align,
                    bool if_clause,
                    unsigned flags,
                    void **depend,
                    int priority_arg,
                    void *detach)
    {
    omp_thread &thread = *omp_this_thread();
    omp_thread &team = *omp_this_team();

    if(!if_clause                               // if if_clause if false
    || !task_pool)                              // or the task pool is empty, we have to run the task right now
        {
        if(cpyfn)                               // if a copy function is defined, copy the data to a private buffer first
            {
            char buf[arg_size + arg_align - 1];
            char *dst = &buf[arg_align-1];
            dst = (char *)((uintptr_t)dst & ~(arg_align-1));
            cpyfn(dst, data);
            DPRINT(2)("call explicit task, id = %d(%d), code = %8p, data = %8p\n", thread.team_id, thread.id, fn, data);
            fn(dst);            
            }
        else
            {
            DPRINT(2)("call explicit task, id = %d(%d), code = %8p, data = %8p\n", thread.team_id, thread.id, fn, data);
            fn(data);
            }
        }
    else                                        // else queue the task to be executed by another context later
        {
        char *argmem = (char *)malloc(arg_size + arg_align);                                    // allocate memory for data
        if(argmem == 0)
            {
            printf("malloc returned 0\n");
            }
        char *arg = (char *)((uintptr_t)(argmem + arg_align) & ~(uintptr_t)(arg_align - 1));    // align the data memory
        arg[-1] = arg-argmem;                                                                   // save the alignment offset at arg-1 so we can calc the addr for free later

        if(cpyfn)
            {
            cpyfn(arg, data);
            }
        else
            {
            memcpy(arg, data, arg_size);
            } 
        
        task *task = 0;

        bool ok = task_pool.take(task);        // create a new task
        if(!ok)
            {
            printf("task_pool.take failed\n");
            return;
            }
        team.task_count++;

        task->fn = fn;                          // give it code
        task->data = arg;                       // and data

        DPRINT(2)("create explicit task %8p, id = %d(%d)\n", task, thread.team_id, thread.id);
        team.task_list.add(task);                  // add it to the list of explicit tasks
        }
    }



/////////////////////////////////
// Explicitly called functions //
/////////////////////////////////




// set the default number of threads kicked off by "parallel"
extern "C"
void omp_set_num_threads(int num)
    {
    if(num > OMP_NUM_THREADS)
        {
        num = OMP_NUM_THREADS;
        }

    gomp_nthreads_var = num;
    }

// return the number of threads in the current team
extern "C"
int omp_get_num_threads()
    {
    omp_thread &team = *omp_this_team();

    return team.team_count;
    }


// return the thread number within the current team
extern "C"
int omp_get_thread_num()
    {
    omp_thread *thrd = omp_this_thread();

    return thrd->team_id;
    }

// return the index of the current thread into the thread table
// (not an OMP function)
extern "C"
int gomp_get_thread_id()
    {
    omp_thread *thrd = omp_this_thread();

    return thrd->id;
    }


// extern "C" void omp_set_num_threads (int);
// extern "C" int omp_get_max_threads (void);



extern "C"
void omp_set_dynamic(int dyn)
    {
    dyn_var = dyn;
    }

extern "C"
int omp_get_dynamic(void)
    {
    return dyn_var;
    }


// Return current time as a floating point number in seconds since powerup.
// This uses the 32-bit TIM2 timer which runs at 1 MHz.
// It will roll over every 1 hour and 11.5 seconds.
// TODO -- extend this to 64 bits using an interrupt.

extern "C"
double omp_get_wtime(void)
    {
    uint32_t ticks = __HAL_TIM_GET_COUNTER(&htim2);
    double fticks = (double)ticks;
    return fticks / 1000000.;
    }

extern "C"
float omp_get_wtime_float(void)
    {
    uint32_t ticks = __HAL_TIM_GET_COUNTER(&htim2);
    float fticks = (float)ticks;
    return fticks / 1000000.;
    }

float omp_get_wtime(int __attribute__((__unused__)))
    {
    uint32_t ticks = __HAL_TIM_GET_COUNTER(&htim2);
    float fticks = (float)ticks;
    return fticks / 1000000.;
    }

// return the value of one tick (one microsecond)
extern "C"
double omp_get_wtick (void)
    {
    return 0.000001;
    }

// return the value of one tick (one microsecond)
extern "C"
float omp_get_wtick_float (void)
    {
    return 0.000001f;
    }

// return the value of one tick (one microsecond)
float omp_get_wtick(int __attribute__((__unused__)))
    {
    return 0.000001f;
    }

// extern "C" int omp_get_num_teams (void);
// extern "C" int omp_get_team_num (void);
// extern "C" int omp_get_team_size (int);
// extern "C" int omp_get_num_procs (void);
// extern "C" int omp_in_parallel (void);
// extern "C" void omp_set_nested (int);
// extern "C" int omp_get_nested (void);
// extern "C" void omp_init_lock_with_hint (omp_lock_t *, omp_sync_hint_t);
// extern "C" void omp_init_nest_lock_with_hint (omp_nest_lock_t *, omp_sync_hint_t);
// extern "C" void omp_set_schedule (omp_sched_t, int);
// extern "C" void omp_get_schedule (omp_sched_t *, int *);
// extern "C" int omp_get_thread_limit (void);
// extern "C" void omp_set_max_active_levels (int);
// extern "C" int omp_get_max_active_levels (void);
// extern "C" int omp_get_level (void);
// extern "C" int omp_get_ancestor_thread_num (int);
// extern "C" int omp_get_active_level (void);
// extern "C" int omp_in_final (void);
// extern "C" int omp_get_cancellation (void);
// extern "C" omp_proc_bind_t omp_get_proc_bind (void);
// extern "C" int omp_get_num_places (void);
// extern "C" int omp_get_place_num_procs (int);
// extern "C" void omp_get_place_proc_ids (int, int *);
// extern "C" int omp_get_place_num (void);
// extern "C" int omp_get_partition_num_places (void);
// extern "C" void omp_get_partition_place_nums (int *);
// extern "C" void omp_set_default_device (int);
// extern "C" int omp_get_default_device (void);
// extern "C" int omp_get_num_devices (void);
// extern "C" int omp_is_initial_device (void);
// extern "C" int omp_get_initial_device (void);
// extern "C" int omp_get_max_task_priority (void);
// extern "C" void *omp_target_alloc (__SIZE_TYPE__, int);
// extern "C" void omp_target_free (void *, int);
// extern "C" int omp_target_is_present (const void *, int);
// extern "C" int omp_target_memcpy (void *, const void *, __SIZE_TYPE__, __SIZE_TYPE__, __SIZE_TYPE__, int, int);
// extern "C" int omp_target_memcpy_rect (void *, const void *, __SIZE_TYPE__, int, const __SIZE_TYPE__ *, const __SIZE_TYPE__ *, const __SIZE_TYPE__ *, const __SIZE_TYPE__ *, const __SIZE_TYPE__ *, int, int);
// extern "C" int omp_target_associate_ptr (const void *, const void *, __SIZE_TYPE__, __SIZE_TYPE__, int);
// extern "C" int omp_target_disassociate_ptr (const void *, int);
// extern "C" void omp_set_affinity_format (const char *);
// extern "C" __SIZE_TYPE__ omp_get_affinity_format (char *, __SIZE_TYPE__);
// extern "C" void omp_display_affinity (const char *);
// extern "C" __SIZE_TYPE__ omp_capture_affinity (char *, __SIZE_TYPE__, const char *);
// extern "C" int omp_pause_resource (omp_pause_resource_t, int);
// extern "C" int omp_pause_resource_all (omp_pause_resource_t);
