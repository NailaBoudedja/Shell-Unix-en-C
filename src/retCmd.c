#include "jsh_bib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Argument non valide\n", 20);
        exit(EXIT_FAILURE);
    }

    
    write(STDOUT_FILENO, "la valeur de retour de la dernière commande:  ", 46);
    write(STDOUT_FILENO, argv[1], stringLength(argv[1]));
    write(STDOUT_FILENO, "\n", 1);

    return EXIT_SUCCESS;
}
