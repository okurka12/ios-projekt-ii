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

/* definice ridici struktury pro druhy projekt predmetu IOS LS 2022/2023 */

/* tato ridici struktura bude umistena ve sdilene pameti a vsechny procesi se 
   pres ni budou ridit a synchronizovat */

#include "makra.h"
#include "proj2.h"
#include "fronta.h"

#ifndef __CONTROL_STRUCT_
#define __CONTROL_STRUCT_

/* pres tuto strukturu se zakaznici a urednici ocisluji a zjisti jestli je 
posta otevrena, a taky si najdou ukazatel na frontu, do ktere chteji jit */
struct control_structure {

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

};

#endif  // ifndef __CONTROL_STRUCT_
