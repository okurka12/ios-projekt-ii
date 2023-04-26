/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-26  **
**              **
** Last edited: **
**  2023-04-26  **
*****************/
// Fakulta: FIT VUT
// Vyvijeno s gcc 10.2.1 na Debian GNU/Linux 11

/* API pro nektere funkce pouzivane v main zdrojovem souboru proj2.c */

#ifndef __PROJ2_H_
#define __PROJ2_H_

#include <stdio.h>     // size_t
#include <sys/ipc.h>    // IPC ctl symboly
#include <sys/shm.h>    // shmget shmat shmctl 

typedef struct {
   size_t size;
   char *shm;
   int shmid;
} shm_t;

/* vytvori + pripoji sdilenou pamet, ulozi ukazatel na ni a jeji ID a velikost
   do struktury `t`, na niz vrati ukazatel, pri neuspechu vraci NULL */
shm_t *get_shm(const size_t size, shm_t *t);

/* odpoji pamet od procesu a pak ji zrusi */
void free_shm(shm_t *t);

#endif  // ifndef __PROJ2_H_
