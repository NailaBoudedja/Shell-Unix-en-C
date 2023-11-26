
#include "jsh_bib.h" //include l'entete de la bibliothèque des fonctions de jsh
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

int main() {

    

    initializeJsh();  //initialiser les parametres du prompt
    while (1) { 
    
        rl_initialize();   //initialiser readline
        afficherJsh();    //afficher jsh

        char *input = readline("");  //lire la commande saisite par l'utilisateur 

        if (input && *input) {   //si la commande entrée n'est pas vide
            add_history(input);  //ajout de la commande à l'historique du shell 
            executerCommand(input); //execution de la commande 
        }
        free(input); //libérer l'espace memeoire alloué  
    }
    return 0;
}
