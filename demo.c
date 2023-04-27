/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2023-04-24  **
**              **
** Last edited: **
**  2023-04-25  **
*****************/
// Fakulta: FIT VUT
// Vyvijeno s gcc 10.2.1 na Debian GNU/Linux 11

/* demo pro vytvoreni segmentu sdilene pameti pres SYSTEM V cally 
   a pak fork a data race */

#include "makra.h"

#include <stdio.h>
#include <errno.h>  // errno

#include <unistd.h>  // fork
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#define SLEEP_TIME 0.3

void child(char *shm, size_t velikost) {
    for (unsigned int i = 0; i < velikost; i++) {
        printf("Child: %u. prvek je %c\n", i, shm[i]);
        db_sleep(SLEEP_TIME);
    }
}


void parent(char *shm, size_t velikost) {
    for (unsigned int i = 0; i < velikost; i++) {
        printf("Parent: pokusim se zmenit %u. prvek z %c na %c\n", 
                i, shm[i], shm[i] + ('A' - 'a'));
        shm[i] += 'A' - 'a';
        db_sleep(SLEEP_TIME);
    }
}


int main() {

    key_t key;             // sem nam da ftok IPC klic
    size_t velikost = 64;  // velikost shared memory segmentu
    int shmid;             // id vytvoreneho shm segmentu
    char *shm;             // pointer na shared memory segment
    
    /*
    DESCRIPTION
    The  ftok()  function uses the identity of the file named by the given 
    pathname (which must refer to an existing, accessible file) and the least 
    significant 8 bits of proj_id (which must be nonzero) to generate a  key_t
    type System V IPC key, suitable for use with msgget(2), semget(2), or 
    shmget(2).

    The  resulting  value is the same for all pathnames that name the same 
    file, when the same value of proj_id is used.  The value returned should 
    be different when the (simultaneously existing) files or the project 
    IDs differ.
    */
    if ((key = ftok(".", IPC_CREAT | 0777)) == -1) {
        perror("ftok error");
        return 1;
    }

    /*
    DESCRIPTION
    shmget()  returns  the identifier of the System V shared memory segment 
    associated with the value of the argument key.  It may be used either 
    to obtain the identifier of a previously created shared memory segment  
    (when shmflg is zero and key does not have the value IPC_PRIVATE), 
    or to create a new set.
    */
    if ((shmid = shmget(key, velikost, IPC_CREAT | 0777)) == -1) {
        perror("shmget failed");
        return 1;
    }

    /* 
    DESCRIPTION
    shmat()
       shmat()  attaches  the  System V shared memory segment identified by 
       shmid to the address space of the calling process.  The attaching 
       address is specified by shmaddr with one of the following criteria:

       - If shmaddr is NULL, the system chooses a suitable (unused) 
       page-aligned address to attach the segment.

       - If shmaddr isn't NULL and SHM_RND is specified in shmflg, the attach
       occurs at the address equal to  shmaddr rounded down to the 
       nearest multiple of SHMLBA.

       - Otherwise, shmaddr must be a page-aligned address at which the attach 
       occurs.

       On success, shmat() returns the address of the attached shared memory 
       segment; on error,  (void *) -1  is  returned, and errno is set to 
       indicate the cause of the error.
    */
    if ((shm = shmat(shmid, NULL, 0)) == (void *) -1) {
        perror("shmat failed");        
        return 1;
    }

    // inicializace bytu shm segmentu
    for (unsigned int i = 0; i < velikost; i++) {
        shm[i] = 'a' + i % ('z' - 'a');
        logv("nastavuji %u. prvek na %c", i, 'a' + i % ('z' - 'a'));
    }
    
    // vytvoreni jednoho potomka
    pid_t pid = fork();

    if (pid == 0) {
        child(shm, velikost);
    } else {
        parent(shm, velikost);
    }

    


    return 0;
}
