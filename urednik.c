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

/* implementace procesu urednik pro druhy projekt predmetu IOS LS 2022/2023 */

#include "makra.h"
#include "proj2.h"
#include "fronta.h"
#include "control_struct.h"

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
        
        // uzamce vsechno aby si vybral frontu (aby se nezmenila mezitim)
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