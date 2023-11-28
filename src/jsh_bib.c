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

int stop ;
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

int taille_chaine(char *chaine)
{
    int length = 0;
    int i = 0;
    while (chaine[i] != '\0')
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



//executer une commande donnée
void executerCommand(char *command)
{
    /*
    int tube[2];
    if (pipe(tube) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    */

    // Convertir le descripteur de fichier du tube en chaîne
    char tubeFdStr[10];

    pid_t pid = fork(); //créer un processus fils pour l'execution de la commande

    if (pid < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid == 0) //programme du fils
    {
        //organiser la commande dans un tableau sous la forme {command,arguments,NULL}
        char **cmd = stringToWords(command);

        if (cmd[0][0] == 'c' && cmd[0][1] == 'd' && cmd[0][2] == '\0')
        {
            printf("%s \n","je suis dans cd");
        }
        else if (cmd[0][0] == 'p' && cmd[0][1] == 'w' && cmd[0][2] == 'd' && cmd[0][3] == '\0')
        {
            printf("%s \n","je suis dans pwd");
        }
        else if (cmd[0][0] == '?' && cmd[0][1] == '\0')
        {
            printf("%s \n","je suis dans ?");
        }
        else if (cmd[0][0] == 'e' && cmd[0][1] == 'x' && cmd[0][2] == 'i' && cmd[0][3] == 't' && cmd[0][4] == '\0')
        {

            /*
            close(tube[0]); // Fermer l'extrémité en lecture du tube dans le processus fils
            snprintf(tubeFdStr, sizeof(tubeFdStr), "%d", tube[1]);
            write(tube[1], &stop, sizeof(int)); // Écrire la valeur de stop dans le tube
            execl("./exit", "./exit", tubeFdStr, (char *)NULL);
            perror("execl");
            exit(EXIT_FAILURE);  */
            exit(1);
        }
        else
        {
            write(STDOUT_FILENO, "command not known\n", 18);
        }

        //gestion d'erreur de execlp
        perror("execlp");
        exit(1);
    }
    else
    {
        wait(NULL);


        /*
      
        //processus parent
        close(tube[1]); // Fermer l'extrémité en écriture du tube dans le processus parent
        // Lire depuis le tube
        read(tube[0], &stop, sizeof(int));
        printf("la valeur de stop apres exit %d \n", stop);  */
    
    }
}


