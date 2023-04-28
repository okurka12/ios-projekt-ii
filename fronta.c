/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-26  **
**              **
** Last edited: **
**  2023-04-28  **
*****************/
// Fakulta: FIT VUT
// Vyvijeno s gcc 10.2.1 na Debian GNU/Linux 11
/*-------------------------------------------------------------------------------------*/
/* implementace operaci s frontou */

#include <assert.h>
#include <stdio.h>  // perror
#include <semaphore.h>

#include "fronta.h"
#include "makra.h"
#include "proj2.h"

/* POZOR toto makro vola `return NULL` */
#define check_sem_succes(f) if ((f) == -1) \
    { perror(__FILE__ ": semaphore failed"); return NULL; }


queue_t *queue_init(unsigned int size) {

    // ziskat sdilenou pamet pro frontu
    shm_t queue_shm;
    size_t bytes_n = sizeof(queue_t) + size * sizeof(queue_ele_t);
    if (get_shm(bytes_n, &queue_shm) == NULL) {
        return NULL;
    }

    // inicializovat frontu do sdilene pameti
    queue_t *queue = (queue_t *)queue_shm.shm;
    queue->len   = size;
    queue->start = 0;
    queue->shm   = queue_shm;
    
    // inicializovat id prvku na nulu: vyznam: ve fronte nikdo neni
    for (unsigned int i = 0; i < size; i++) {
        queue->arr[i].n = 0;
    }

    return queue;
}


queue_ele_t *queue_add(queue_t *q, unsigned int id) {
    queue_ele_t *out;

    if (q->end < q->len) {
        q->arr[q->end].n = id;
        out = &(q->arr[q->end]);
        q->end++;
        logv("do fronty %p se zaradil prvek %u", (void *)q, id);        
    } else {
        log("nastalo neco co by nemelo nastavat");
        out = NULL;
    }

    return out;
}


queue_ele_t *queue_serve(queue_t *q) {
    logv("q=%p q->arr=%p", (void *)q, (void *)(q->arr));

    // kdyz je fronta prazdna
    if (queue_length(q) == 0) {
        return NULL;
    }

    queue_ele_t *out;

    out = &(q->arr[q->start]);
    q->start++;
    assert(q->start <= q->end);
    logv("z fronty %p byl obslouzen prvek %u", (void *)q, out->n);    

    return out;
}


unsigned int queue_length(queue_t *q) {

    unsigned int out = 0;

    out = q->end - q->start;        

    return out;
}


void queue_free(queue_t *q) {

    // detach + remove shared memory
    free_shm(&(q->shm));
}
