/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-24  **
**              **
** Last edited: **
**  2023-04-27  **
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

/* zkontroluje co vratil semaphore lock/unlock POZOR toto makro vola return */
#define check_semaphore(s) if ((s) == -1) \
    {perror("semaphore_{post|wait|init} failed"); return 2;}

/* vytiskne akci do `file`, pozor, muze zavolat return */
#define print_file(ctl, file, msg, ...) \
    check_semaphore(sem_wait(&(ctl->action_sem))); \
    ctl->n_action++; \
    fprintf(file, "%u: " msg, ctl->n_action, __VA_ARGS__); \
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

    // kolikata je to akce
    sem_t action_sem;
    unsigned int n_action;

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


/* struktura pro argumenty programu */
typedef struct {
    unsigned int nz;
    unsigned int nu;
    unsigned int tz;
    unsigned int tu;
    unsigned int f;
} args_t;


/* je-li otevreno, vrati 1, jestli ne, vrati 0, doslo-li k chybe, vrati 2 */
char je_otevreno(control_t *ctl) {
    char otevreno;
    check_semaphore(sem_wait(&(ctl->posta_otevrena_sem)));  // lock
    otevreno = ctl->posta_otevrena;
    check_semaphore(sem_post(&(ctl->posta_otevrena_sem)));  // unlock
    return otevreno;
}

/* vybere nahodne neprazdnou frontu (1-3) nebo indikuje prazdnotu vsech (4) */
char vyber_frontu(control_t *ctl) {
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
    return (rand() % 3) + 1;
}


int urednik(control_t *ctl, unsigned int tu, FILE *file) {

    /* maly delay aby cisla byla jakz takz nahodna */
    for (int i = 0; i < 100; i++) {
        rand();
    }

    // cislo ktere bude jednoznacne identifikovat urednika
    unsigned int cislo;
    char otevreno;
    queue_ele_t *zakaznik;
    char vybrana_fronta;

    // ocislovat se
    check_semaphore(sem_wait(&(ctl->urednici_sem)));  // lock
    ctl->u++;
    cislo = ctl->u;
    print_file(ctl, file, "U %u: started\n", cislo);
    check_semaphore(sem_post(&(ctl->urednici_sem)));  // unlock
    db_sleep_rand(0.1, 0.2);


    do {

        switch (je_otevreno(ctl)) {
            case 1:
                otevreno = 1;
                break;
            case 0:
                otevreno = 0;
                break;
            case 2:
                return 2;
            default:
                log("nastal stav co by nemel nastavat");
        }
        vybrana_fronta = vyber_frontu(ctl);
        logv("U %u si vybral frontu %hhd", cislo, vybrana_fronta);
        switch (vybrana_fronta) {
            case 1:
                zakaznik = queue_serve(ctl->listovni_sluzby);
                break;
            case 2:
                zakaznik = queue_serve(ctl->baliky);
                break;
            case 3:
                zakaznik = queue_serve(ctl->penezni_sluzby);
                break;
            case 4:
                if (!otevreno) {
                    print_file(ctl, file, "U %u: going home\n", cislo);
                    return 0;
                }
                print_file(ctl, file, "U %u: taking break\n", cislo);
                sleep_rand_ms(0, tu);
                print_file(ctl, file, "U %u: break finished\n",  cislo);
                continue;
            default:
                log("nastal stav co by nemel nastavat");
                break;
        
        }
        if (zakaznik == NULL) {
            return 1;
        }

        // odemknout zakaznikovi semafor (zavolat ho)
        check_semaphore(sem_post(&(zakaznik->qele_sem)));
        print_file(ctl, file, "U %u: serving a service of type %hhd\n", 
               cislo, vybrana_fronta);
        sleep_rand_ms(0, 10);  // doba vykonavani sluzby
        print_file(ctl, file, "U %u: service finished\n", cislo);
        
    } while (1);


    logv("jsem urednik cislo %d a koncim", cislo);

    return 0;
}


int zakaznik(control_t *ctl, unsigned int tz, FILE *file) {

    // cislo ktere bude jednoznacne identifikovat zakaznika
    unsigned int cislo;
    char otevreno = 0;
    char aktivita;
    queue_ele_t *q_ele;
    queue_t *q;


    // ocislovat se
    check_semaphore(sem_wait(&(ctl->zakaznici_sem)));  // lock
    ctl->z++;
    cislo = ctl->z;
    print_file(ctl, file, "Z %u: started\n", cislo);
    check_semaphore(sem_post(&(ctl->zakaznici_sem)));  // unlock
    db_sleep_rand(0.1, 0.2);

    sleep_rand_ms(0, tz);
    
    // zjisti jestli je otevreno
    switch (je_otevreno(ctl))
    {
    case 0:
        otevreno = 0;
        break;
    case 1:
        otevreno = 1;
        break;
    case 2:
        return 2;
    
    default:
        log("nastal stav co by nemel nastavat");
        return 2;
    }
    

    // je-li otevreno, zvolit aktivitu, jinak jit domu
    if (otevreno) {
        aktivita = (rand() % 3) + 1;
        logv("zakaznik %u si vybral frontu %hhd", cislo, aktivita);
    } else {
        print_file(ctl, file, "Z %u: going home\n", cislo);
        return 0;
    }

    // vybrat si spravnou frontu podle zvolene aktivity
    switch (aktivita) {
        case 1:
            q = ctl->listovni_sluzby;
            break;
        case 2:
            q = ctl->baliky;
            break;
        case 3:
            q = ctl->penezni_sluzby;
            break;
        default:
            log("nastal stav co by nemel nikdy nastat");
            q = ctl->listovni_sluzby;
            break;
    }
    print_file(ctl, file, "Z %u: entering office for a service %hhd\n", cislo, 
               aktivita);

    // zaradit se do vybrane fronty a cekat na vyvolani
    q_ele = queue_add(q, cislo);
    if (q_ele == NULL) {
        return 1;
    }
    
    check_semaphore(sem_init(&(q_ele->qele_sem), 1, 0));
    check_semaphore(sem_wait(&(q_ele->qele_sem)));
    print_file(ctl, file, "Z %u: called by office worker\n", cislo);

    sleep_rand_ms(0, 10);

    print_file(ctl, file, "Z %u: going home\n", cislo);

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
    
    // inicializace semaforu v ctl pro zakazniky i uredniky i otevreni posty
    // nonzero pshared -> semafor je sdilen mezi procesy
    if (sem_init(&(ctl->zakaznici_sem), 1, 1) == -1) {
        perror("sem_init failed");
        return NULL;
    }
    if (sem_init(&(ctl->urednici_sem), 1, 1) == -1) {
        perror("sem_init failed");
        return NULL;
    }
    if (sem_init(&(ctl->posta_otevrena_sem), 1, 1) == -1) {
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
    ctl->baliky = queue_init(pocet_zakazniku);
    if (ctl->baliky == NULL) {
        free_shm(control_p);
        return NULL;
    }
    ctl->penezni_sluzby = queue_init(pocet_zakazniku);
    if (ctl->penezni_sluzby == NULL) {
        free_shm(control_p);
        return NULL;
    }

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
        error = 1;
    }
    if (sscanf(argv[1], "%u", &(arg_struct->nu)) != 1) {
        error = 1;
    }
    if (sscanf(argv[1], "%u", &(arg_struct->tz)) != 1) {
        error = 1;
    }
    if (sscanf(argv[1], "%u", &(arg_struct->tu)) != 1) {
        error = 1;
    }
    if (sscanf(argv[1], "%u", &(arg_struct->f)) != 1) {
        error = 1;
    }
    if (
        !(arg_struct->tz <= 0 && arg_struct->tz <= 10000) ||
        !(arg_struct->tu <= 0 && arg_struct->tu <= 100) ||
        !(arg_struct->f <= 0 && arg_struct->f <= 10000)
    ) {
        error = 1;
    }
    if (error) {
        return 1;
    } else {
        fprintf(stderr, "Invalid args\n");
        return 0;
    }

}


int main(int argc, char **argv) {

    // toto aby to bylo jakz takz nahodne
    srand(time(NULL));

    log ("zacinam program");

    // parsnuti argumentu
    args_t args;
    if (!parse_args(argc, argv, &args)) {
        return 1;
    }
    log ("argumenty parsovany");

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

    // vytvoreni zakazniku
    int pid;
    for (unsigned int i = 0; i < args.nz; i++) {
        pid = fork();

        // pokud jsem dite
        if (pid == 0) {
            return zakaznik(ctl, args.tz, fd);  
        }
    }

    // vytvoreni uredniku
    for (unsigned int i = 0; i < args.nu; i++) {
        pid = fork();

        // pokud jsem dite
        if (pid == 0) {
            return urednik(ctl, args.tu, fd);
        }
    }

    // spanek a pak uzavreni posty
    sleep_rand_ms(0, args.f);
    check_semaphore(sem_wait(&(ctl->posta_otevrena_sem)));
    ctl->posta_otevrena = 0;
    print_file(ctl, fd, "closing%s\n", "");
    check_semaphore(sem_post(&(ctl->posta_otevrena_sem)));

    // pockani na vsechny zakazniky a uredniky nez skonci
    for (unsigned int i = 0; i < args.nu + args.nz; i++) {
        wait(NULL);
    }

    logv("jsem rodic a nez skoncim, vezte ze bylo %d zakazniku a %d uredniku", 
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
