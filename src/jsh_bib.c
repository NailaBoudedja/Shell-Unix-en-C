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

    jsh.ret.newret=0 ;
    jsh.ret.oldret= 0 ;
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
   
    int i = 0; //parcourir input
    int j = 0; //parcourir les mots du result
    int k = 0; //parcourir les char du mot
    
    char **result = (char **)malloc(3 * sizeof(char *)); //contient au max 3 mots

    for (j = 0; j < 3; j++) {
        result[j] = (char *)malloc(100 * sizeof(char)); //allocation d'espace memoire pour chaque mot
    }

    j = 0; //réinitialiser j

    while (input[i] != '\0') { //tq on est pas arrivé a la fin de input
        if (input[i] != ' ' && input[i] != '\t' && input[i] != '\n') {
            result[j][k] = input[i]; 
            k++;
        } else {
            result[j][k] = '\0'; //fin du mot
            j++;
            k = 0;
        }
        i++;
    }

    result[j][k] = '\0'; // Terminer le dernier mot
    return result;
}




int executerCommand(char *command) {
    int ret = 0;
    pid_t pid = fork();  //creer un processus fils pour se charger de l'execution de la commande entree
   
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { 
        //prog du fils
        
        //organiser la commande entrée dans un tableau sous forme {command, arguments, NULL}
        char **cmd = stringToWords(command);

        if (cmd[0] != NULL) {   //detecter la commande saisie

            if (cmd[0][0] == 'c' && cmd[0][1] == 'd' && cmd[0][2] == '\0') {
                ret = execvp("./cd.c", cmd);
                
            } else if (cmd[0][0] == 'p' && cmd[0][1] == 'w' && cmd[0][2] == 'd' && cmd[0][3] == '\0') {
                ret = execlp("./pwd", "./pwd", (char *)NULL);
            } else if (cmd[0][0] == '?' && cmd[0][1] == '\0') {
               //reccuperer la derniere valeur retournée
               char oldret[2];
               oldret[0] = '0' + jsh.ret.oldret;  //stocker la valeur dans une chaine 
               oldret[1] = '\0';
               //passer la valeur de retour en parametre pour ?
               ret = execlp("./retCmd", "./retCmd", oldret, (char *)NULL);

            } else if (cmd[0][0] == 'e' && cmd[0][1] == 'x' && cmd[0][2] == 'i' && cmd[0][3] == 't' && cmd[0][4] == '\0') {
                ret = execlp("./exit", "./exit", (char *)NULL);
            } else {
                write(STDOUT_FILENO, "Command not known\n", 18);
                ret = 1;
            }

            //erreur lors de l'execution
            if (ret == -1) {
                perror("exec");
                exit(EXIT_FAILURE);
            }
        } else {
            //la commande saisie est vide
            write(STDOUT_FILENO, "Empty command\n", 15);
            ret = 1;
        }

        exit(ret);
    } else { 
        //prg du pere
        
        //attente active de la terminaison du fils
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            ret = WEXITSTATUS(status);   //terminaison normale
        } else {
            //terminaison non normale
            write(STDOUT_FILENO, "Child process did not terminate normally\n", 41);
            ret = 1;
        }
    }

    return ret; //valeur de retour
}
