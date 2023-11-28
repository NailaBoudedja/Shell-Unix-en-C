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

//définition des couleurs du prompt
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m" 
//définition de la taille maximale du prompt
#define MAX_PROMPT_SIZE 30 
//définition du nombre maximum des arguments d'une commande 
#define MAX_ARGS 20 


struct Prompt jsh;    //déclaration du shell jsh

//fonction qui renvoie le chemin du rep courrant
char *pwd() {
    char *rep = (char *)malloc(PATH_MAX); //allocation d'espace memeoire pour stocker le chemin du répertoire courant
    if (getcwd(rep, PATH_MAX) == NULL)   //récupérer le répertoire courant
    {
        perror("erreur lorsq de réccuperation du chemin courrant");
        exit(1);
    } else
        return rep;
}

 //fonction pour exiter le programme avec le code n
int exitCommande (int n){ 
    exit(n);
}

//fonction pour changer le rep courrant vers newRep
int cd(char* newRep) {
    int ret = 0;  //valeur de retourne
    if(newRep==NULL){    //aucun rep entré 
        ret =  chdir(getenv("HOME"));  //cd vers home
        if (ret != 0) {
        perror("erreur lors de changement du rep");
        return 1;
        }
        return 0;
    }
    else
    {  //changer vers newRep
    if (chdir(newRep) != 0) {
        perror("erreur lors de changement du rep");
        return 1;
    }
    }
    return 0;
}

//fonciton pour la commande ? qui renvoie la valeur de retour de la dernière commande executée
int retCommande(){ 
   return jsh.oldret; 
}


//renvoie la taille d'une chaine de char
int stringLength(char *chaine) {
    int length = 0;
    int i = 0; //pour parcourir la chaîne
    while (chaine[i] != '\0') //fin de la chaine
    {
        length++;
        i++;
    }
    return length;
}

//convertir une chaine de char en tableau de mots
char **stringToWords(char *input) {
    int i = 0; //parcourir input
    int j = 0; //parcourir les mots du result
    int k = 0; //parcourir les caractères du mot

    char **result = (char **)malloc(MAX_ARGS * sizeof(char *)); //contient au max MAX_ARGS mots

    for (j = 0; j < MAX_ARGS; j++) {
        result[j] = (char *)malloc(100 * sizeof(char)); //allocation d'espace mémoire pour chaque mot
    }

    j = 0; //réinitialiser j

    while (input[i] != '\0') { //tq on n'est pas arrivé à la fin de input
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

    result[j][k] = '\0'; //char de fin pour le dernier mot
    return result;
}




char *tronkString(const char *str, int size) {
    // Vérifier si la taille est valide
    if (size <= 0) {
        return NULL;
    }

    int strLength = strlen(str);

    // Vérifier si la taille demandée est plus grande que la longueur de la chaîne
    if (size > strLength) {
        fprintf(stderr, "Taille demandée plus grande que la longueur de la chaîne.\n");
        return NULL;
    }

    // Calculer l'indice de début du sous-string
    int startIndex = strLength - size;

    // Allouer dynamiquement la mémoire pour le sous-string
    char *subStr = (char *)malloc((size + 1) * sizeof(char));

    // Copier le sous-string
    strncpy(subStr, str + startIndex, size);

    // Ajouter le caractère de fin de chaîne
    subStr[size] = '\0';

    return subStr;
}
//fonction d'affichage du prompt
char *afficherJsh() {
    char result[MAX_PROMPT_SIZE * 2] = ""; // Initialisez result avec une chaîne nulle
    char *currentDir = pwd();  // Récupérer le chemin du répertoire courant

    // Mettre le nombre de jobs dans result avec la couleur rouge
    strcat(result, COLOR_RED "[0]" COLOR_RESET);

    // Calculer la taille du prompt
    int prompt_length = stringLength(currentDir) + 5;

    // Vérifier la taille du prompt
    if (prompt_length > MAX_PROMPT_SIZE) {
        int new_length = MAX_PROMPT_SIZE - 8;
        strcat(result, COLOR_GREEN "...");
        char *newDir = tronkString(currentDir, new_length);
        strcat(result, newDir);
        free(newDir);
    } else {
        strcat(result, COLOR_GREEN);
        strcat(result, currentDir);
    }

    strcat(result, COLOR_BLUE "$ ");

    // Allouer de la mémoire pour le résultat et le retourner
    char *aretourner = strdup(result);
    free(currentDir);  // Libérer la mémoire allouée par pwd()
    return aretourner;
}

//la fonction qui se charge de l'execution de la commande entrée
int executerCommande(char* commande){
  
  int ret = 0; //la valeur de retour
  //organiser la commande entrée dans un tableau sous forme [commande,args];
  char **cmd = stringToWords(commande);
  if(cmd[0] != NULL)
  {
    if(strcmp(cmd[0], "exit") == 0){
        //la commande entrée est exit
        ret = exitCommande(atoi(cmd[1])) ;
    }
    else if(strcmp(cmd[0], "cd") == 0){
        //la commande entrée est cd
        ret = cd(cmd[1]);
        

    }
    else{
        //pour les autres commandes on doit créer un processus fils qui s'occupe de leurs execution
        pid_t pid = fork(); 
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if(pid == 0 )
            {
               //processus fils
               if (strcmp(cmd[0], "pwd") == 0) {
                //la commande entrée est pwd
                    char *currentDir = pwd(); //reccuperer le chemin du rep courrant
                    write(STDOUT_FILENO, currentDir, stringLength(currentDir));  //affichage du chemin courrant
                    write(STDOUT_FILENO, "\n", 1);
                    ret = 0;
                }

                else if(strcmp(cmd[0], "?") == 0){
                  //la commande entrée est ?
                   int old_ret = retCommande();
                   char old_ret_s[2]; 
                   sprintf(old_ret_s, "%d", old_ret);
                   write(STDOUT_FILENO, old_ret_s , stringLength(old_ret_s));  //affichage de la valeur ret
                   write(STDOUT_FILENO, "\n", 1);
                   ret = 0 ;
                }
                else{
                    //commande externe
                    //reccuperer les arguments et les parametres de la commande
                    char *args[MAX_ARGS];
                    int i = 0;
                    char *token = strtok(commande, " \t\n");

                    while (token != NULL && i < MAX_ARGS - 1) {
                        args[i++] = token;
                        token = strtok(NULL, " \t\n");
                    }
                    args[i] = NULL;
                    ret = execvp(args[0], args);  //executer la commande 
                }
                //terminaison du fils
                exit(ret);
            }
            else{
                //code du pere
                //attente active de la terminaison du fils
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
 
}
 return ret;
 }
