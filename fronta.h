/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-25  **
**              **
** Last edited: **
**  2023-04-26  **
*****************/
// Fakulta: FIT VUT
// Vyvijeno s gcc 10.2.1 na Debian GNU/Linux 11

/* hlavickovy soubor pro operace s frontou */

#include <unistd.h>     // fork
#include <sys/ipc.h>    // IPC ctl symboly
#include <sys/shm.h>    // shmget shmat shmctl 
#include <sys/types.h>  // fork
#include <sys/wait.h>   // wait
#include <semaphore.h>

#include "proj2.h"

#ifndef _FRONTA_H
#define _FRONTA_H

typedef struct {

    // semafor pro prvek fronty - proces si ho muze pri zarazeni inicializovat
    // na nulu a cekat nez ho nekdo obslozui
    sem_t qele_sem;

    // id prvku (ktery je to proces)
    unsigned int n;

} queue_ele_t;

/* datovy typ fronty: prvky fronty budou ulozeny v poli,*/
typedef struct {
    sem_t queue_sem;     // semafor pro pristupovani k fronte
    unsigned int len;    // maximalni mozna delka fronty
    unsigned int start;  // zacatek
    unsigned int end;    // konec fronty (kolik je v ni prvku)
    shm_t shm;           // sdilena pamet v niz se tato struktura bude nachazet
    queue_ele_t arr[];   // samotna data fronty (flexible array member)
} queue_t;

/* vytvori, pripoji a inicializuje frontu ve sdilene pameti, vrati na ni 
   ukazatel, pri neuspechu vrati null */
queue_t *queue_init(const unsigned int size);

/* prida do fronty prvek s cislem `id`, vrati na nej ukazatel. semafor prvku
   ponecha neinicializovany. pri neuspechu vraci null */
queue_ele_t *queue_add(queue_t *q, unsigned int id);

/* vrati ukazatel na prvek fronty, ktery je na rade, pri neuspechu null */
queue_ele_t *queue_serve(queue_t *q);

/* vrati pocet cekajicich prvku ve fronte */
unsigned int queue_length(queue_t *q);

/* destruktor fronty: odpoji sdilenou pamet a pak ji zrusi */
void queue_free(queue_t *q);

#endif  // ifndef _FRONTA_H
