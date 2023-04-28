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


/* zkontroluje co vratil semaphore lock/unlock POZOR toto makro vola exit */
#define check_semaphore(s) if ((s) == -1) \
    { \
        perror("semaphore_{post|wait|init} failed"); \
        fprintf(stderr, "abort\n"); \
        exit(2); \
    }

/* uzamce frontu `q`, kdyz selze semafor vola exit */
#define lock_queue(q) check_semaphore(sem_wait(&((q)->queue_sem)))

/* odemce frontu `q`, kdyz selze semafor vola exit */
#define unlock_queue(q) check_semaphore(sem_post(&((q)->queue_sem)))

/* uzamce `access_sem` struktury `ctl` a jestli se to nepovede zavola exit */
#define lock_all(ctl) \
    check_semaphore(sem_wait(&(ctl->access_sem))) \
    lock_queue(ctl->listovni_sluzby) \
    lock_queue(ctl->baliky) \
    lock_queue(ctl->penezni_sluzby)

/* odemce `access_sem` struktury `ctl` a jestli se to nepovede zavola exit */
#define unlock_all(ctl) \
    check_semaphore(sem_post(&(ctl->access_sem))) \
    unlock_queue(ctl->listovni_sluzby) \
    unlock_queue(ctl->baliky) \
    unlock_queue(ctl->penezni_sluzby)
    


/* vytiskne akci do `file`, pozor, muze zavolat exit */
#define print_file(ctl, file, msg, ...) \
    check_semaphore(sem_wait(&(ctl->action_sem))); \
    ctl->n_action++; \
    fprintf(file, "%u: " msg, ctl->n_action, __VA_ARGS__); \
    logv("%u: " msg, ctl->n_action, __VA_ARGS__); \
    fflush(file); \
    check_semaphore(sem_post(&(ctl->action_sem)))

/**
 * Poznamka:
 * tato struktura musi byt tady protoze kdybych ji dal do proj2.h 
 * tak je to cyklicky import a davat to do fronta.h je nesmysl
*/

/* pres tuto strukturu se zakaznici a urednici ocisluji a zjisti jestli je 
posta otevrena, a taky si najdou ukazatel na frontu, do ktere chteji jit */
typedef struct {

    // jenom jeden proces bude pristupovat k teto strukture (az na action)
    sem_t access_sem;

    // kolikata je to akce
    sem_t action_sem;
    unsigned int n_action;

    // pocet zakazniku (kazdy prichozi zakaznik si vybere cislo o 1 vetsi)
    unsigned int z;

    // pocet uredniku (taktez)
    unsigned int u;

    // otevreni posty
    char posta_otevrena;

    // --- fronty maji pristupovy semafor v sobe ---
    // fronta listovni sluzby
    queue_t *listovni_sluzby;

    // fronta baliku
    queue_t *baliky;

    // fronta peneznich slzueb
    queue_t *penezni_sluzby;

} control_t;


/* struktura pro argumenty programu */
typedef struct {
    unsigned int nz;
    unsigned int nu;
    unsigned int tz;
    unsigned int tu;
    unsigned int f;
} args_t;


/* vybere nahodne neprazdnou frontu (1-3) nebo indikuje prazdnotu vsech (4) */
unsigned int vyber_frontu(control_t *ctl) {
    unsigned int fronta1 = queue_length(ctl->listovni_sluzby);
    unsigned int fronta2 = queue_length(ctl->baliky);
    unsigned int fronta3 = queue_length(ctl->penezni_sluzby);

    // jestli se nestoji fronta na zadnou sluzbu
    if (fronta1 == 0 && fronta2 == 0 && fronta3 == 0) {
        return 4;
    }

    // jestli se stoji fronta jen na jednu sluzbu
    if (fronta1 == 0 && fronta2 == 0) {
        return 3;
    }
    if (fronta2 == 0 && fronta3 == 0) {
        return 1;
    }
    if (fronta1 == 0 && fronta3 == 0) {
        return 2;
    }

    // jestli se stoji fronty na dve sluzby
    if (fronta1 == 0) {
        return rand() < (RAND_MAX / 2) ? 2 : 3;
    }
    if (fronta2 == 0) {
        return rand() < (RAND_MAX / 2) ? 1 : 3;
    }
    if (fronta3 == 0) {
        return rand() < (RAND_MAX / 2) ? 1 : 2;
    }

    // jestli se stoji vsechny fronty
    return (unsigned int)randint(1, 3);
}


int urednik(control_t *ctl, unsigned int tu, FILE *file) {

    /* maly delay aby cisla byla jakz takz nahodna */
    for (int i = 0; i < 100; i++) {
        rand();
    }

    // cislo ktere bude jednoznacne identifikovat urednika
    unsigned int cislo;

    // lokalni ukazatel na obsluhovaneho zakaznika
    queue_ele_t *zakaznik;

    // cislo vybrane fronty
    unsigned int vybrana_fronta;

    // ocislovat se
    lock_all(ctl);
    ctl->u++;
    cislo = ctl->u;
    print_file(ctl, file, "U %u: started\n", cislo);
    unlock_all(ctl);
    db_sleep_rand(0.1, 0.2);  // debug sleep po unlocku

    while (1) {
        
        // uzamce vsechno aby si vybral frontu
        lock_all(ctl);
        vybrana_fronta = vyber_frontu(ctl);
        switch (vybrana_fronta) {
            case 1:
                zakaznik = queue_serve(ctl->listovni_sluzby);
                logv("urednik %u si vybral %u", cislo, vybrana_fronta);
                break;
            case 2: 
                logv("urednik %u si vybral %u", cislo, vybrana_fronta);
                zakaznik = queue_serve(ctl->baliky);
                break;
            case 3:
                logv("urednik %u si vybral %u", cislo, vybrana_fronta);
                zakaznik = queue_serve(ctl->penezni_sluzby);
                break;
            case 4:
                if (!(ctl->posta_otevrena)) {
                    print_file(ctl, file, "U %u: going home\n", cislo);
                    unlock_all(ctl);
                    return 0;
                } else {
                    print_file(ctl, file, "U %u: taking break\n", cislo);
                    unlock_all(ctl);
                    sleep_rand_ms(0, tu);
                    print_file(ctl, file, "U %u: break finished\n", cislo);
                    continue;
                }
        }
        if (zakaznik == NULL) {
            exit(2);
        } else {

            // zavolani zakaznika
            print_file(ctl, file, "U %u: serving a service of type %u\n", 
                       cislo, vybrana_fronta);
            check_semaphore(sem_post(&(zakaznik->qele_sem)));

            sleep_rand_ms(0, 10);
            print_file(ctl, file, "U %u: service finished\n", cislo);

            unlock_all(ctl);
            
        }

    }

    logv("urednik %u zacal", cislo);


    logv("jsem urednik cislo %d a koncim", cislo);

    return 0;
}


int zakaznik(control_t *ctl, unsigned int tz, FILE *file) {

    // cislo ktere bude jednoznacne identifikovat zakaznika
    unsigned int cislo;
    char aktivita;

    // lok. ukazatel na prvek ve fronte na pozici, do niz se zakaznik zaradi
    queue_ele_t *q_ele;


    // ocislovat se
    lock_all(ctl);
    ctl->z++;
    cislo = ctl->z;
    print_file(ctl, file, "Z %u: started\n", cislo);
    unlock_all(ctl);

    db_sleep_rand(0.1, 0.2);
    logv("zakaznik %u zacal", cislo);

    // da slofika nez pujde na postu
    sleep_rand_ms(0, tz);

    lock_all(ctl);
    if (ctl->posta_otevrena) {
        logv("zakaznik %u zjistil ze je posta otevrena", cislo);
        aktivita = randint(1, 3);
        switch (aktivita) {
            case 1:
                logv("zakaznik %u si vybral %hhd", cislo, aktivita);
                q_ele = queue_add(ctl->listovni_sluzby, cislo);
                break;
            case 2:
                logv("zakaznik %u si vybral %hhd", cislo, aktivita);
                q_ele = queue_add(ctl->baliky, cislo);
                break;
            case 3:
                logv("zakaznik %u si vybral %hhd", cislo, aktivita);
                q_ele = queue_add(ctl->penezni_sluzby, cislo);
                break;
        }

        // logv("zakaznik %u tento radek probehne", cislo);
        print_file(ctl, file, "Z %u: entering office for a service %hhd\n", 
                   cislo, aktivita);
        // logv("zakaznik %u tento radek neprobehne", cislo);

        unlock_all(ctl);

        // inicializace sveho semaforu (bude cekat nez ho urednik odemce)
        check_semaphore(sem_init(&(q_ele->qele_sem), 1, 0));


        // cekani na urednika
        check_semaphore(sem_wait(&(q_ele->qele_sem)));
        logv("zakaznik %u byl vybran urednikem", cislo);
        print_file(ctl, file, "Z %u: called by office worker\n", cislo);

        sleep_rand_ms(0, 10);
        print_file(ctl, file, "Z %u: going home\n", cislo);
        return 0;

    // pokud je zavreno
    } else {
        logv("zakaznik %u zjistil ze je posta zavrena", cislo);
        unlock_all(ctl);
        print_file(ctl, file, "Z %u: going home\n", cislo);
        return 0;
    }
    
    return 0;
}


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
    FILE *fd = fopen("./proj2.out", "w+");
    if (fd == NULL) {
        perror("file couldn't be opened");
        return 1;
    }
    
    shm_t ctl_shm;
    control_t *ctl = ctl_init(args.nz, &ctl_shm);

    // OD TED BUDE APLIKACE VICEPROCESOVA (doted nebyla)
    // -------------------------------------------------------------------------

    // pro vytvareni potomku
    int pid, rcode;

    // vytvoreni uredniku
    for (unsigned int i = 0; i < args.nu; i++) {
        rand(); rand(); rand();
        pid = fork();

        // pokud jsem dite
        if (pid == 0) {
            rcode = urednik(ctl, args.tu, fd);
            fclose(fd);
            return rcode;
        }
    }

    // vytvoreni zakazniku
    for (unsigned int i = 0; i < args.nz; i++) {
        rand(); rand(); rand();
        pid = fork();

        // pokud jsem dite
        if (pid == 0) {
            rcode = zakaznik(ctl, args.tz, fd);
            fclose(fd);
            return rcode;  
        }
    }

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
        wait(NULL);
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

    return 0;
}
