/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-24  **
**              **
** Last edited: **
**  2023-04-29  **
*****************/
// Fakulta: FIT VUT
// Vyvijeno s gcc 10.2.1 na Debian GNU/Linux 11

/* hlavni soubor pro druhy projekt predmetu IOS LS 2022/2023 */

#include "makra.h"
#include "proj2.h"
#include "fronta.h"
#include "control_struct.h"

#include <stdio.h>
#include <errno.h>  // errno

#include <unistd.h>     // fork
#include <sys/ipc.h>    // IPC ctl symboly
#include <sys/shm.h>    // shmget shmat shmctl 
#include <sys/types.h>  // fork
#include <sys/wait.h>   // wait
#include <semaphore.h>  // semafory
#include <signal.h>     // kill, SIGKILL
#include <string.h>     // memset

/* struktura pro argumenty programu */
typedef struct {
    unsigned int nz;
    unsigned int nu;
    unsigned int tz;
    unsigned int tu;
    unsigned int f;
} args_t;


/* ziska veskerou sdilenou pamet: struktura control_t a vsechny fronty v ni,
   vsechno inicializuje, pri neuspechu vraci NULL*/
control_t *ctl_init(unsigned int pocet_zakazniku, shm_t *control_p) {

    // ziskani sdilene pameti pro control_t
    if (get_shm(sizeof(control_t), control_p) == NULL) {
        return NULL;
    }

    // cast do ukazatele (ten od nyni ukazuje do sdilene pameti)
    control_t *ctl = (control_t *)(control_p->shm);
    
    // inicializace access_sem
    // nonzero pshared -> semafor je sdilen mezi procesy
    if (sem_init(&(ctl->access_sem), 1, 1) == -1) {
        perror("sem_init failed");
        return NULL;
    }
    if (sem_init(&(ctl->action_sem), 1, 1) == -1) {
        perror("sem_init failed");
        return NULL;
    }

    // inicializace poctu zakazniku a uredniku v control_t a otevreni posty
    ctl->z = 0;
    ctl->u = 0;
    ctl->posta_otevrena = 1;
    ctl->n_action = 0;

    // inicializace tri front (tzn ziskani sdilene pameti pro ne)
    ctl->listovni_sluzby = queue_init(pocet_zakazniku);
    if (ctl->listovni_sluzby == NULL) {
        free_shm(control_p);
        return NULL;
    }
    logv("inicializovana fronta listovni sluzby (1): %p", 
         (void *)ctl->listovni_sluzby);

    ctl->baliky = queue_init(pocet_zakazniku);
    if (ctl->baliky == NULL) {
        free_shm(control_p);
        return NULL;
    }
    logv("inicializovana fronta baliky (2): %p", 
         (void *)ctl->baliky);

    ctl->penezni_sluzby = queue_init(pocet_zakazniku);
    if (ctl->penezni_sluzby == NULL) {
        free_shm(control_p);
        return NULL;
    }
    logv("inicializovana front penezni sluzby (2): %p", 
         (void *)ctl->penezni_sluzby);

    return ctl;
}


/* zpracuje argumenty programu, pri uspechu vraci 1, jinak 0 */
int parse_args(int argc, char **argv, args_t *arg_struct) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s NZ NU TZ TU F\n", argv[0]);
        return 0;
    }
    int error = 0;
    if (sscanf(argv[1], "%u", &(arg_struct->nz)) != 1) {
        log("nespravny format 1. argumentu");
        error = 1;
    }
    if (sscanf(argv[2], "%u", &(arg_struct->nu)) != 1) {
        log("nespravny format 2. argumentu");
        error = 1;
    }
    if (sscanf(argv[3], "%u", &(arg_struct->tz)) != 1) {
        log("nespravny format 3. argumentu");
        error = 1;
    }
    if (sscanf(argv[4], "%u", &(arg_struct->tu)) != 1) {
        log("nespravny format 4. argumentu");
        error = 1;
    }
    if (sscanf(argv[5], "%u", &(arg_struct->f)) != 1) {
        log("nespravny format 5. argumentu");
        error = 1;
    }
    logv("nz=%u nu=%u tz=%u tu=%u f=%u", arg_struct->nz, arg_struct->nu, 
         arg_struct->tz, arg_struct->tu, arg_struct->f);
    if (
        !(arg_struct->nu > 0) ||
        !(arg_struct->tz <= 10000) ||
        !(arg_struct->tu <= 100) ||
        !(arg_struct->f <= 10000)
    ) {
        error = 1;
    }
    if (!error) {
        return 1;
    } else {
        fprintf(stderr, "Invalid args\n");
        return 0;
    }

}


/* zabije vsechny procesy v `pids` */
void kill_all(pid_t pids[], unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        logv("zabijim potomka %d", pids[i]);
        kill(pids[i], SIGKILL);
    }
}


/* ./proj2 NZ NU TZ TU F */
int main(int argc, char **argv) {

    // toto aby to bylo jakz takz nahodne
    srand(time(NULL));

    log("zacinam program");

    // parsnuti argumentu
    args_t args;
    if (!(parse_args(argc, argv, &args))) {
        log("parse args failed");
        return 1;
    } else {
        logv("argumenty parsovany nz=%u nu=%u tz=%u ms tu=%u ms f=%u ms", 
             args.nz, args.nu, args.tz, args.tu, args.f);
    }

    // otevreni souboru
    log("oteviram ./proj2.out");
    FILE *fd = fopen("./proj2.out", "w+");
    if (fd == NULL) {
        perror("file couldn't be opened");
        return 1;
    }
    
    // pole pro PID vsech procesu
    log("alokuji pamet pro PIDs potomku");
    pid_t *pids = malloc(sizeof(pid_t) * (args.nz + args.nu));
    if (pids == NULL) {
        fprintf(stderr, "couldn't allocate memory\n");
        return 1;
    }
    memset(pids, 0, sizeof(pid_t) * (args.nz + args.nu));

    // inicializace sdilene pameti pro ridici strukturu
    log("alokuji sdilenou pamet");
    shm_t ctl_shm;
    control_t *ctl = ctl_init(args.nz, &ctl_shm);

    // null check
    if (ctl == NULL) {
        free(pids);
        fprintf(stderr, __FILE__ ":%d: abort\n", __LINE__);
        return 1;
    }

    // -------------------------------------------------------------------------
    // OD TED BUDE APLIKACE VICEPROCESOVA (doted nebyla)
    // -------------------------------------------------------------------------

    // pro vytvareni potomku
    int pid, rcode;

    // vytvoreni uredniku
    log("vytvarim uredniky");
    for (unsigned int i = 0; i < args.nu; i++) {

        // pokazde pred vytvorenim potomka zavolat tohle, jinak bude mit kazdy
        // potomek nahodou radu inicializovanou stejnym seminkem
        rand(); rand(); rand(); rand(); rand();
        
        pid = fork();

        // pokud jsem dite
        if (pid == 0) {

            // kod ditete
            rcode = urednik(ctl, args.tu, fd);

            // uvolneni nesdilenych prostredku alokovanych rodicem
            free(pids);
            fclose(fd);

            // konec ditete
            return rcode;
        }

        // pokud selhal fork
        if (pid == -1) {
            fprintf(stderr, "selhal fork, zabijim deti, abort\n");
            kill_all(pids, args.nz + args.nu);
            free(pids);
            fclose(fd);
            return 1;
        }

        // rodic: pridat dite do seznamu deti
        pids[i] = pid;
    }
    log("vsichni urednici vytvoreni");

    // vytvoreni zakazniku
    log("vytvarim zakazniky");
    for (unsigned int i = args.nu; i < args.nu + args.nz; i++) {

        // vizte tvoreni uredniku
        rand(); rand(); rand(); rand(); rand();
        
        pid = fork();

        // pokud jsem dite
        if (pid == 0) {

            // kod ditete
            rcode = zakaznik(ctl, args.tz, fd);

            // uvolneni nesdilenych prostredku alokovanych rodicem
            free(pids);
            fclose(fd);

            // konec ditete
            return rcode;
        }

        // pokud selhal fork
        if (pid == -1) {
            fprintf(stderr, "selhal fork, zabijim deti, abort\n");
            kill_all(pids, args.nz + args.nu);
            free(pids);
            fclose(fd);
            return 1;
        }

        // rodic: pridat dite do seznamu deti
        pids[i] = pid;
    }
    log("vsichni zakaznici vytvoreni");

    // spanek a pak uzavreni posty
    sleep_rand_ms(0, args.f);

    // uzavreni posty
    lock_all(ctl);
    ctl->posta_otevrena = 0;
    log("zaviram postu");
    print_file(ctl, fd, "closing%s\n", "");
    unlock_all(ctl);

    // pockani na vsechny zakazniky a uredniky nez skonci
    for (unsigned int i = 0; i < args.nu + args.nz; i++) {
        logv("cekam nez skonci potomci: %d/%d", i, args.nz + args.nu);
        wait(&rcode);

        // pokud se stala chyba a nejake dite skoncilo neuspesne
        if (rcode != 0) {
            fprintf(stderr, "stala se chyba v diteti, zabijim deti, abort\n");
            fclose(fd);
            free(pids);
            free_shm(&ctl_shm);
            kill_all(pids, args.nz + args.nu);
            return 1;
        }
    }

    logv("jsem rodic a nez skoncim, vezte, ze bylo %d zakazniku a %d uredniku", 
         ctl->z, ctl->u);

    // zavreni souboru
    fclose(fd);

    // uvolneni front
    free_shm(&(ctl->listovni_sluzby->shm));
    free_shm(&(ctl->baliky->shm));
    free_shm(&(ctl->penezni_sluzby->shm));

    // uvolneni ctl
    free_shm(&ctl_shm);

    // uvolneni pids
    free(pids);

    return 0;
}
