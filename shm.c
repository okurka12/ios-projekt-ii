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

/* implementace get_shm a free_shm deklarovane v proj2.h */
/* je zadouci, aby tyto funkce volal rodic-proces */

#include <stdio.h>      // perror
#include <sys/ipc.h>    // IPC ctl symboly
#include <sys/shm.h>    // shmget shmat shmctl 
#include "makra.h"
#include "proj2.h"

shm_t *get_shm(const size_t size, shm_t *t) {
    
    // klic pro shm
    int key = ftok(".", IPC_CREAT | 0777);

    // null check
    if (key == -1) {
        perror("ftok failed");
        return NULL;
    }

    // vytvoreni segmentu
    t->shmid = shmget(key, size, IPC_CREAT | 0777);

    // -1 check
    if (t->shmid == -1) {
        perror("shmget failed");
        return NULL;
    }

    // pripojeni segmentu k tomuto procesu a jeho potomkum
    t->shm = shmat(t->shmid, NULL, 0);
    if (t->shm == (void *)-1) {
        perror("shmat failed");
        return NULL;
    }

    // vse uspelo, ted definovat hodnotu t->size
    t->size = size;

    logv("vytvoren+pripojen shm segment (ID %d, velikost %lu, adresa %p)", 
         t->shmid, t->size, t->shm);

    return t;
}

/* odpoji a zrusi shm segment */
void free_shm(shm_t *t) {
    logv("uvolnuji shm segment (ID %d, velikost %lu, adresa %p)", 
         t->shmid, t->size, t->shm);
    shmdt(t->shm);
    shmctl(t->shmid, IPC_RMID, NULL);
}
