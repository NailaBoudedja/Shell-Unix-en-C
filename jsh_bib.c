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
#include <linux/limits.h>

#define MAX_PROMPT_SIZE 30   //définition de la taille maximale du prompt
#define NORMAL_COLOR "\033[34m"   //définition de la couleur par défaut du prompt

#define MAX_ARGS 20


struct Prompt jsh;    //déclaration du shell jsh


char* pwd() {
    char* rep = (char *)malloc(PATH_MAX); //pour stocker le chemin du rep courrant
    if(getcwd(rep,PATH_MAX) == NULL)   //reccuperer le rep courrant
    {
        perror("error de getcwd");
        exit(1);
    } 
    else return rep;  
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

    jsh.ret.newret=0 ;    //initialiser les valeurs de returns
    jsh.ret.oldret= 0;
}


//afficher le prompt
void afficherJsh()
{
    jsh.currentDir.currentDir=pwd(); //obtenir le rep courrant
    // strcpy(jsh.currentDir.currentDir,pwd());
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



int retCommande(){ // fonciton ? 
    char oldret[2];
    oldret[0] = '0' + jsh.ret.oldret;
    oldret[1] = '\0';
    int val = write(STDOUT_FILENO,oldret,stringLength(oldret));
    if (val > 0) return 0; 
    else return 1;
}

int exitCommande (){ // fonction exit
    exit(0);
}


int  cd(char* newRep) {
    int ret = 0;
    if(newRep==NULL){
        ret =  chdir(getenv("HOME"));
        if (ret != 0) {
        perror("erreur de la cmd chdir dans cd");
        return 1;
        }
        return 0;

    }
    if (chdir(newRep) != 0) {
        perror("erreur de la cmd chdir dans cd avec argt");
        return 1;
    }
    return 0;
}

   
   
int executerCommand(char *command) {
    int ret = 0;

    // Organiser la commande entrée dans un tableau sous forme {command, arguments, NULL}
    char **cmd = stringToWords(command);

    if (strcmp(cmd[0], "exit") == 0) {
        ret = exitCommande();
    }
    else if (cmd[0] != NULL) {
        if (strcmp(cmd[0], "cd") == 0) {
            ret = cd(cmd[1]);
            if (ret == 0) {
                write(STDOUT_FILENO, "cd reussi\n", 10);
            } else {
                write(STDOUT_FILENO, "error cd\n", 9);
            }
        } else {
            pid_t pid = fork(); // Créer un processus fils

            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Code du fils
                if (strcmp(cmd[0], "pwd") == 0) {
                    char *currentDir = pwd();
                    write(STDOUT_FILENO, currentDir, stringLength(currentDir));
                    write(STDOUT_FILENO, "\n", 1);
                    
                    exit(0);
                } else if (strcmp(cmd[0], "?") == 0) {
                    ret = retCommande();
                }
                else { 
                    // Commande externe
                    //reccuperer les arguments de la commande 
                    char *args[MAX_ARGS];
                    int i = 0;  
                    char *token = strtok(command, " \t\n");


                    while (token != NULL && i < MAX_ARGS - 1) {
                         args[i++] = token;
                         token = strtok(NULL, " \t\n");
                    }
                    args[i] = NULL;
                    ret = execvp(args[0], args);

                    if (ret == -1) {
                        perror("execvp");
                    }
                }
                exit(ret);
            } else {
                // Code du père
                // Attente active de la terminaison du fils
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    ret = WEXITSTATUS(status); // Terminaison normale
                } else {
                    // Terminaison non normale
                    write(STDOUT_FILENO, "Child process did not terminate normally\n", 41);
                    ret = 1;
                }
            }
        }
    } else {
        // Commande vide
        printf("Commande vide\n");
    }
    return ret;
}









    
   



   


