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

/* hlavni soubor pro druhy projekt predmetu IOS LS 2022/2023 */

#include "makra.h"
#include "proj2.h"
#include "fronta.h"

#include <stdio.h>
#include <errno.h>  // errno

#include <unistd.h>     // fork
#include <sys/ipc.h>    // IPC ctl symboly
#include <sys/shm.h>    // shmget shmat shmctl 
#include <sys/types.h>  // fork
#include <sys/wait.h>   // wait
#include <semaphore.h>

/**
 * Poznamka:
 * tato struktura musi byt tady protoze kdybych ji dal do proj2.h 
 * tak je to cyklicky import a davat to do fronta.h je nesmysl
*/

/* pres tuto strukturu se zakaznici a urednici ocisluji a zjisti jestli je 
posta otevrena, a taky si najdou ukazatel na frontu, do ktere chteji jit */
typedef struct {
    // pocet zakazniku (kazdy prichozi zakaznik si vybere cislo o 1 vetsi)
    sem_t zakaznici_sem;
    unsigned int z;

    // pocet uredniku (taktez)
    sem_t urednici_sem;
    unsigned int u;

    // otevreni posty
    sem_t posta_otevrena_sem;
    char posta_otevrena;

    // --- fronty maji pristupovy semafor v sobe ---
    // fronta listovni sluzby
    queue_t *listovni_sluzby;

    // fronta baliku
    queue_t *baliky;

    // fronta peneznich slzueb
    queue_t *penezni_sluzby;

} control_t;

int urednik(control_t *ctl) {

    // cislo ktere bude jednoznacne identifikovat urednika
    unsigned int cislo;

    // zamknout semafor
    sem_wait(&(ctl->urednici_sem));

    ctl->u++;
    cislo = ctl->u;

    // odemknout semafor
    sem_post(&(ctl->urednici_sem));
    db_sleep_rand(0.5, 0.8);

    logv("jsem urednik cislo %d a koncim", cislo);

    return 0;
}


int zakaznik(control_t *ctl) {

    // cislo ktere bude jednoznacne identifikovat zakaznika
    unsigned int cislo;

    // zamknout semafor
    sem_wait(&(ctl->zakaznici_sem));

    ctl->z++;
    cislo = ctl->z;

    // odemknout semafor
    sem_post(&(ctl->zakaznici_sem));
    sleep_rand(0.5, 0.8);

    logv("jsem zakaznik cislo %d a koncim", cislo);

    return 0;
}

int main() {

    // TODO: parsnout argumenty
    
    unsigned int pocet_zakazniku = 3;
    unsigned int pocet_uredniku = 3;
    
    // ziskani sdilene pameti pro control_t
    shm_t control;
    if (get_shm(sizeof(control_t), &control) == NULL) {
        return 1;
    }

    // cast do ukazatele (ten od nyni ukazuje do sdilene pameti)
    control_t *ctl = (control_t *)(control.shm);
    
    // inicializace semaforu v ctl pro zakazniky i uredniky i otevreni posty
    // nonzero pshared -> semafor je sdilen mezi procesy
    sem_init(&(ctl->zakaznici_sem), 1, 1);
    sem_init(&(ctl->urednici_sem), 1, 1);
    sem_init(&(ctl->posta_otevrena_sem), 1, 1);

    // inicializace poctu zakazniku a uredniku v control_t a otevreni posty
    ctl->z = 0;
    ctl->u = 0;
    ctl->posta_otevrena = 1;

    // inicializace tri front (tzn ziskani sdilene pameti pro ne)
    ctl->listovni_sluzby = queue_init(pocet_zakazniku);
    if (ctl->listovni_sluzby == NULL) {
        free_shm(&control);
        return 1;
    }
    ctl->baliky = queue_init(pocet_zakazniku);
    if (ctl->baliky == NULL) {
        free_shm(&control);
        return 1;
    }
    ctl->penezni_sluzby = queue_init(pocet_zakazniku);
    if (ctl->penezni_sluzby == NULL) {
        free_shm(&control);
        return 1;
    }

    // OD TED BUDE APLIKACE VICEPROCESOVA (doted nebyla)
    // -------------------------------------------------------------------------

    // vytvoreni zakazniku
    int pid;
    for (unsigned int i = 0; i < pocet_zakazniku; i++) {
        pid = fork();

        // pokud jsem dite
        if (pid == 0) {
            return zakaznik(ctl);  
        }
    }

    // vytvoreni uredniku
    for (unsigned int i = 0; i < pocet_uredniku; i++) {
        pid = fork();

        // pokud jsem dite
        if (pid == 0) {
            return urednik(ctl);
        }
    }

    // pockani na vsechny zakazniky a uredniky nez skonci
    for (unsigned int i = 0; i < pocet_uredniku + pocet_zakazniku; i++) {
        wait(NULL);
    }

    logv("jsem rodic a nez skoncim, vezte ze bylo %d zakazniku a %d uredniku", 
         ctl->z, ctl->u);

    // uvolneni front
    free_shm(&(ctl->listovni_sluzby->shm));
    free_shm(&(ctl->baliky->shm));
    free_shm(&(ctl->penezni_sluzby->shm));

    // uvolneni ctl
    free_shm(&control);

    return 0;
}
