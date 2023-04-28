/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-28  **
**              **
** Last edited: **
**  2023-04-28  **
*****************/
// Fakulta: FIT VUT
// Vyvijeno s gcc 10.2.1 na Debian GNU/Linux 11

/* implementace procesu zakaznik pro druhy projekt predmetu IOS LS 2022/2023 */

#include "makra.h"
#include "proj2.h"
#include "fronta.h"
#include "control_struct.h"

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