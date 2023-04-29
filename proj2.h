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

/* API pro nektere funkce pouzivane v main zdrojovem souboru proj2.c */

#ifndef __PROJ2_H_
#define __PROJ2_H_

#include <stdio.h>     // size_t
#include <sys/ipc.h>    // IPC ctl symboly
#include <sys/shm.h>    // shmget shmat shmctl 

/*----------------------------------------------------------------------------*/
/* definovano v souboru zvlast protoze struktura potrebuje typy z `proj2.h` 
   i `fronta.h, ale zaroven `proj2.h` (tento soubor) porebuje `control_t` do 
   hlavicek funkci urednika a zakaznika */

struct control_structure;
typedef struct control_structure control_t;
/*----------------------------------------------------------------------------*/

typedef struct {
   size_t size;
   char *shm;
   int shmid;
} shm_t;

/* vytvori + pripoji sdilenou pamet, ulozi ukazatel na ni a jeji ID a velikost
   do struktury `t`, na niz vrati ukazatel, pri neuspechu vraci NULL */
shm_t *get_shm(const size_t size, shm_t *t);

/* odpoji pamet od procesu a pak ji zrusi */
void free_shm(shm_t *t);

/* tento kod je kompletni kod zakaznika, ocekava se ze sdilena pamet je jiz 
   pripojena k procesu vytvarejicimu zakaznika, zakaznik sdilenou pamet 
   sam nepripojuje */
int zakaznik(control_t *ctl, unsigned int tz, FILE *file);

/* kompletni kod urednika, stejne nalezitosti jako pro `zakaznik()` */
int urednik(control_t *ctl, unsigned int tu, FILE *file);

/* ziska veskerou sdilenou pamet: struktura control_t a vsechny fronty v ni,
   vsechno inicializuje, pri neuspechu vraci NULL*/
control_t *ctl_init(unsigned int pocet_zakazniku, shm_t *control_p);

#endif  // ifndef __PROJ2_H_
