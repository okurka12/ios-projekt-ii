/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-24  **
**              **
** Last edited: **
**  2023-04-24  **
*****************/
// Fakulta: FIT VUT
// Vyvijeno s gcc 10.2.1 na Debian GNU/Linux 11

/* uzitecna makra pro druhy projekt predmetu IOS LS 2022/2023 */

#ifndef __MAKRA_H__
#define __MAKRA_H__

#include <stdio.h>

#ifndef NDEBUG

/**
 * Poznamka:
 * Zamerne nepouzivam usleep, protoze POSIX pravi ze je deprecated.
*/

/* deklarace nanosleep je v `time.h` z nejakeho duvodu podminena definici 
   tohoto symbolu */
#ifndef __USE_POSIX199309
#define __USE_POSIX199309
#endif  // ifndef __USE_POSIX199309
#include <time.h>          // nanosleep
#include <limits.h>
#include <assert.h>

/* logs plain string or does nothing if NDEBUG is defined */
#define log(msg) fprintf(stderr, __FILE__ ":%03d: " msg "\n", __LINE__)

/* logs variable(s) or does nothing if NDEBUG is defined */
#define logv(msg, ...) fprintf(stderr, __FILE__ ":%03d: " msg "\n", \
                               __LINE__, __VA_ARGS__)

/* a bilion (for use in db_sleep macro definition) */
#define BLN 1000000000

/* sleeps `t` (float) seconds or does nothing if NDEBUG is defined */
#define db_sleep(t) \
    nanosleep(&(struct timespec){ \
                                 .tv_sec  = (long)(BLN * (t)) / BLN, \
                                 .tv_nsec = (long)(BLN * (t)) % BLN \
                                }, NULL)

#else  // ifndef NDEBUG

#define log(msg) {}
#define logv(msg, ...) {}
#define db_sleep(t) {}

#endif  // ifndef NDEBUG


/* print error */
#define perror(msg) fprintf(stderr, __FILE__ ":%03d: " msg "\n", __LINE__)

/* print error with format (like printf) */
#define perrorf(msg, ...) fprintf(stderr, __FILE__ ":%03d: " msg "\n", \
                                  __LINE__, __VA_ARGS__)

#endif  // ifndef __MAKRA_H__
