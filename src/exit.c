// exit.c
#include "jsh_bib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    // Convertir l'argument en descripteur de fichier
    int tubeFd = atoi(argv[1]);

    // Mettre à jour la valeur de stop
    stop = 1;

    // Écrire la nouvelle valeur de stop dans le tube
    write(tubeFd, &stop, sizeof(int));

    // Fermer l'extrémité en écriture du tube dans le processus enfant
    close(tubeFd);

    printf("Exiting shell...\n");

    exit(0);
}


/*
// exit.c
#include "jsh_bib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char *argv[]) {

    
    // Convertir l'argument en descripteur de fichier
    int tubeFd = atoi(argv[1]);

    // Lire depuis le tube
    read(tubeFd, &stop, sizeof(int));

    // Mettre à jour la valeur de stop
    stop = 1;   

    // Mettre à jour la valeur de stop
    write(tubeFd, &stop, sizeof(int));

    // Fermer l'extrémité en écriture du tube dans le processus enfant
    close(tubeFd);

    printf("Exiting shell...\n");

    exit(0);
}

*/
