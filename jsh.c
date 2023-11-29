#include "bibv2.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    rl_outstream = stderr; //derriger la sortie vers la sortie d'erreur
    char *input;
    jsh.newret = 0 ;
    jsh.oldret = 0;
    jsh.oldPath= pwd();

    while (1) {

        input  =  readline(afficherJsh());  //affichage du prompt  + lecture de la commande entrée
        if(input && *input) { 
              //si la commande entrée n'est pas vide
              jsh.oldret = jsh.newret;
              add_history(input);  //ajout de la commande à l'historique du shell
              jsh.newret = executerCommande(input); //execution de la commande 
                 
        }
        free(input);
    }

    free(jsh.oldPath); 
    return 0;


}
