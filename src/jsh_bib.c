#include "jsh_bib.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_PROMPT_SIZE 30   //définition de la taille maximale du prompt
#define NORMAL_COLOR "\033[34m"   //définition de la couleur par défaut du prompt


struct Prompt jsh;    //déclaration du shell jsh





//a supprimer apres 
char* getCurrentDirectory() {
    char *currentDir = (char *)malloc(PATH_MAX);
    if (getcwd(currentDir, PATH_MAX) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    return currentDir;
}


//renvoie la taille d'une chaine de caracteres
int stringLength(char *chaine)
{
    int length = 0;
    int i = 0;  //pour parcourir la chaine
    while (chaine[i] != '\0')  //fin de la chaine
    {
        length++;
        i++;
    }
    return length;
}


//initailiser les champs du prompt
void initializeJsh()
{
    jsh.jobs.nb_jobs = "0"; //initialiser le nombre de jobs a 0
    jsh.jobs.jobs_color = "\033[91m"; //rouge

    jsh.currentDir.currentDir = "/home/";   //initialiser le rep courrant
    jsh.currentDir.dir_color = "\033[32m";   //vert

    jsh.dollar.val = "$ ";  
    jsh.dollar.dollar_color = "\033[34m";  //couleur standard (bleu)
}


//afficher le prompt
void afficherJsh()
{
    jsh.currentDir.currentDir=getCurrentDirectory(); //obtenir le rep courrant
    //jsh.jobs.nb_jobs = getnbjobs();  //a implementer par la suite

    //calculer la taille du prompt = taille du rep courant  + taille de nb_jobs + taille de [] +  taille de "$ "
    int prompt_length = stringLength(jsh.currentDir.currentDir) + stringLength(jsh.jobs.nb_jobs) + 4;  
    
    //si la taille du prompt a depassé 30
    if (prompt_length > MAX_PROMPT_SIZE)
    {
        int new_length = prompt_length - MAX_PROMPT_SIZE + 3;  //3points
        //affichage du jobs
        write(STDOUT_FILENO, jsh.jobs.jobs_color, stringLength(jsh.jobs.jobs_color));
        write(STDOUT_FILENO, "[", 1);
        write(STDOUT_FILENO, jsh.jobs.nb_jobs, stringLength(jsh.jobs.nb_jobs));
        write(STDOUT_FILENO, "]", 1);
        //affichage du rep courant tranqué
        write(STDOUT_FILENO, jsh.currentDir.dir_color, stringLength(jsh.currentDir.dir_color));
        write(STDOUT_FILENO, "...", 3);
        write(STDOUT_FILENO, jsh.currentDir.currentDir + new_length, MAX_PROMPT_SIZE - 5 );
        //affichage du dollar
        write(STDOUT_FILENO, jsh.dollar.dollar_color, stringLength(jsh.dollar.dollar_color));
        write(STDOUT_FILENO, jsh.dollar.val, stringLength(jsh.dollar.val));
    }
    else
    {   //affichage du jobs 
        write(STDOUT_FILENO, jsh.jobs.jobs_color, stringLength(jsh.jobs.jobs_color));
        write(STDOUT_FILENO, "[", 1);
        write(STDOUT_FILENO, jsh.jobs.nb_jobs, stringLength(jsh.jobs.nb_jobs));
        write(STDOUT_FILENO, "]", 1);
        //affichage du rep courant complet
        write(STDOUT_FILENO, jsh.currentDir.dir_color, stringLength(jsh.currentDir.dir_color));
        write(STDOUT_FILENO, jsh.currentDir.currentDir, stringLength(jsh.currentDir.currentDir));
         //affichage du dollar
        write(STDOUT_FILENO, jsh.dollar.dollar_color, stringLength(jsh.dollar.dollar_color));
        write(STDOUT_FILENO, jsh.dollar.val, stringLength(jsh.dollar.val));
    }

    fflush(stdout); //vider le tampon pour afficher
}




//convertir une chaine de char en tableau de mots
char **stringToWords(char *input) {
   //
}


//executer une commande donnée
void executerCommand(char *command)
{
  //
}


