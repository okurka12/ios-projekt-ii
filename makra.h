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

/* logs plain string */
#define log(msg) fprintf(stderr, __FILE__ ":%03d: " msg "\n", __LINE__)

/* logs variable(s) */
#define logv(msg, ...) fprintf(stderr, __FILE__ ":%03d: " msg "\n", \
                               __LINE__, __VA_ARGS__)

#else

#define log(msg) {}
#define logv(msg, ...) {}

#endif

/* print error */
#define perror(msg) fprintf(stderr, __FILE__ ":%03d: " msg "\n", __LINE__)

#endif  // ifndef __MAKRA_H__
