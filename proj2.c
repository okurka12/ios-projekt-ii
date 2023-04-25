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

#include <stdio.h>
#include <errno.h>  // errno

#include <unistd.h>     // fork
#include <sys/ipc.h>    // IPC ctl symboly
#include <sys/shm.h>    // shmget shmat shmctl 
#include <sys/types.h>  // fork
#include <sys/wait.h>   // wait
#include <semaphore.h>

/* pres tuto strukturu se zakaznici a urednici ocisluji 
   a zjisti jestli je posta otevrena*/
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


/* vrati ukazatel na vytvorenou a pripojenou sdilenou pamet, zapise jeji ID do
   `shmid`, pri selhani vraci null */
char *get_shm(size_t size, int *shmid) {
    // ziskani segmentu shm
    key_t key;             // sem nam da ftok IPC klic
    char *shm;             // pointer na shared memory segment

    // vytvoreni klice
    if ((key = ftok(".", IPC_CREAT | 0777)) == -1) {
        perror("ftok failed");
        return NULL;
    }

    // vytvoreni segmentu sdilene pameti
    if ((*shmid = shmget(key, size, IPC_CREAT | 0777)) == -1) {
        perror("shmget failed");
        return NULL;
    }

    // pripojeni onoho segmentu k tomuto procesu (a jeho potomkum)
    if ((shm = shmat(*shmid, NULL, 0)) == (void *) -1) {
        perror("shmat failed");
        return NULL;
    }

    logv("vytvoren+pripojen shm segment (ID %d velikost %lu)", *shmid, size);
    return shm;
}



int main() {

    // TODO: parsnout argumenty
    
    unsigned int pocet_zakazniku = 3;
    unsigned int pocet_uredniku = 3;
    int shmid;
    

    // vytvoreni ukazatele na strukturu ve sdilene pameti 
    control_t *ctl = (control_t *)get_shm(sizeof(control_t), &shmid);

    // null check
    if (ctl == NULL) {
        return 1;
    }

    // inicializace semaforu pro zakazniky i uredniky
    // nonzero pshared -> semafor je sdilen mezi procesy
    sem_init(&(ctl->zakaznici_sem), 1, 1);
    sem_init(&(ctl->urednici_sem), 1, 1);

    // inicializace poctu zakazniku a uredniku
    ctl->z = 0;
    ctl->u = 0;

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

    log("odstranuji shm segment");
    shmdt(ctl);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
