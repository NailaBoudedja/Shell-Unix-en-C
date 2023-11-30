#include "bib_jsh.h"   //import bibliothèque de prompt
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>

int main() {
    rl_outstream = stderr; //redirection de la sortie standard vers la sortie erreur
    char *input;    //stocker la commande entrée
    jsh.ret = 0 ;  //initialiser la valeur de retour a 0 

    currentDir1 = (char *)malloc(PATH_MAX); 
    currentDir1= pwd(); // initialiser le rep courant 
    char* ligne;

    while (1) {

        ligne = afficherJsh();
        input  =  readline(ligne);  //affichage du prompt  + lecture de la commande entrée
        free(ligne);
       
        if (input == NULL)   //si la commande entrée est vide 
        {
          exit(retCmd());  //exit avec la derniere val de retour 
        }
        
        else if(input && *input) 
        {   
              add_history(input);  //ajout de la commande à l'historique du shell
              jsh.ret = executerCommande(input); //execution de la commande et stocker sa val de ret     
        }
        
        free(input);   //libération de la memoire allouée pours inzput
    }
    free(currentDir1);
    free(oldpath);
    free(input);
    return 0;
}
