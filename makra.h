/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-24  **
**              **
** Last edited: **
**  2023-04-28  **
*****************/
// Fakulta: FIT VUT
// Vyvijeno s gcc 10.2.1 na Debian GNU/Linux 11

/* uzitecna makra pro druhy projekt predmetu IOS LS 2022/2023 */

#ifndef __MAKRA_H__
#define __MAKRA_H__


/* toto je potreba pro nanoslep, v GNU99 to je definovane implicitne */
#ifndef __USE_POSIX199309
#define __USE_POSIX199309 
#endif  // ifndef __USE_POSIX199309


#include <stdio.h>         // fprintf
#include <time.h>          // nanosleep
#include <stdlib.h>        // rand

/**
 * Poznamka:
 * Zamerne nepouzivam usleep, protoze POSIX pravi ze je deprecated.
*/

/* a bilion (for use in sleep_s macro definition) */
#define BLN 1000000000

/* generates a random integer betweeen `a` and `b` */
#define randint(a, b) (rand() % ((b) - (a) + 1) + (a))

/* pauses the execution of the program for `t` seconds (`t` can be float) */
#define sleep_s(t) \
   nanosleep(&(struct timespec){ \
                                .tv_sec  = ((long)((float)BLN * (t))) / BLN, \
                                .tv_nsec = ((long)((float)BLN * (t))) % BLN \
                               }, NULL)

/* sleeps `t` milliseconds */
#define sleep_ms(t) sleep_s(0.001 * t)

/* sleeps for a random time between `a` and `b` seconds */
#define sleep_rand(a, b) \
   sleep_s((float)(a) + ((float)(rand()) / RAND_MAX) * ((float)(b) - (a)))

/* sleeps for random amount of milliseconds between `a` and `b` */
#define sleep_rand_ms(a, b) sleep_rand(0.001 * (a), 0.001 * (b))

/* zkontroluje co vratil semaphore lock/unlock a kdyztak vola exit */
#define check_semaphore(s) if ((s) == -1) \
    { \
        perror("semaphore_{post|wait|init} failed"); \
        fprintf(stderr, "abort\n"); \
        exit(2); \
    }

/* uzamce `access_sem` struktury `ctl` a jestli se to nepovede zavola exit */
#define lock_all(ctl) \
    check_semaphore(sem_wait(&(ctl->access_sem)))

/* odemce `access_sem` struktury `ctl` a jestli se to nepovede zavola exit */
#define unlock_all(ctl) \
    check_semaphore(sem_post(&(ctl->access_sem)))

/* vytiskne akci do `file`, pozor, muze zavolat exit */
#define print_file(ctl, file, msg, ...) \
    check_semaphore(sem_wait(&(ctl->action_sem))); \
    ctl->n_action++; \
    fprintf(file, "%u: " msg, ctl->n_action, __VA_ARGS__); \
    logv("%u: " msg, ctl->n_action, __VA_ARGS__); \
    fflush(file); \
    check_semaphore(sem_post(&(ctl->action_sem)))


/*----------------------------------------------------------------------------*/
/* tato makra jsou jenom pro debugovani */
/*----------------------------------------------------------------------------*/
#ifndef NDEBUG

/* logs plain string or does nothing if NDEBUG is defined */
#define log(msg) fprintf(stderr, __FILE__ ":%03d: " msg "\n", __LINE__)

/* logs variable(s) or does nothing if NDEBUG is defined */
#define logv(msg, ...) fprintf(stderr, __FILE__ ":%03d: " msg "\n", \
                               __LINE__, __VA_ARGS__)

/* sleeps `t` (float) seconds or does nothing if NDEBUG is defined */
#define db_sleep(t) sleep_s(t)

/* sleeps for a random time between `a` and `b` seconds
   or does nothing if NDEBUG is defined */
#define db_sleep_rand(a, b) sleep_rand(a, b)

#else  // ifndef NDEBUG

#define log(msg) {}
#define logv(msg, ...) {}
#define db_sleep(t) {}
#define db_sleep_rand(a, b) {}

#endif  // ifndef NDEBUG
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#endif  // ifndef __MAKRA_H__
