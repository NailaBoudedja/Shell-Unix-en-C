#include "bibv2.h"
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
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
        free(rep);
        exit(1);
    } else
        return rep;
}


int retCmd()
{
    return jsh.oldret;
}
 //fonction pour exiter le programme avec le code n
int exitAvecArgument (int n){ 
    exit(n);
}
int exitSansArgument()
{
    int k= retCmd();
    exit(k);
}
int exitCmd(char * val)
{
   if(val != NULL)
   {
    return exitAvecArgument (atoi(val)); 
   }
   else
   {
    return exitSansArgument();
   }
}


int isReferenceValid(char *ref) {
    struct stat st;

    // stat pour obtenir les infos du fichier ref
    if (lstat(ref, &st) == 0) {
        // Le fichier existe, donc la référence est valide.  
        return 0;
    } else {
        //perror("la référence n'est pas valide");  //afficher le message d'erreur 
        return 1;
    }
}

int cd( char * ref)
{  
   
    //strcm avec vide
   if (ref == NULL) {
        // Changer vers le répertoire home
        const char *home_dir = getenv("HOME");

        if (home_dir == NULL) {
           // free(home_dir);
            fprintf(stderr, "Impossible de récupérer la variable d'environnement $HOME.\n");
            return 1;
        }
        jsh.oldPath = pwd();
        if (chdir(home_dir) != 0) {
            // free(home_dir);
            perror("chdir home");
            return 1;
        }
    }
    else{
        //cd vers pere
        if (strcmp(ref, "-") == 0) {
            const char *prev_dir = jsh.oldPath;
            if (prev_dir == NULL) {
               // free(prev_dir);
                fprintf(stderr, "Impossible de récupérer la variable d'environnement $OLDPWD.\n");
                return 1;
            }

            if (chdir(prev_dir) != 0) {
                // free(prev_dir);
                perror("chdir old path");
                return 1;
            }
        }
        else{
            //cd ref
            if(isReferenceValid(ref) == 0)
            {
            jsh.oldPath = pwd();
            if (chdir(ref) != 0) {
                perror("chdir");
                return 1;
            }
            }
            else{
                //ref non valide
                fprintf(stderr, "bash: cd: %s: No such file or directory\n", ref);
                return 1;
            }
        } 
    }



  return 0;


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

char *afficherJsh() {
    char result[MAX_PROMPT_SIZE * 2] = ""; // Initialisez result avec une chaîne nulle
    char *currentDir = pwd();  // Récupérer le chemin du répertoire courant

    // Mettre le nombre de jobs dans result avec la couleur rouge
    strcat(result, COLOR_RED "[0]");

    // Calculer la taille du prompt
    int prompt_length = strlen(currentDir) + 5;

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


char **extraireMots(char *phrase, char *delimiteur) {
    // Vérifier si la chaîne contient uniquement des espaces
    int estToutEspaces = 1;
    for (int i = 0; i < strlen(phrase); i++) {
        if (phrase[i] != ' ') {
            estToutEspaces = 0;
            break;
        }
    }

    // Si la chaîne est tout espaces, retourner un tableau avec un seul mot : l'espace
    if (estToutEspaces) {
        char **mots = (char **)malloc(2 * sizeof(char *));
        if (mots == NULL) {
            free(mots);
            perror("Erreur d'allocation de mémoire");
            exit(EXIT_FAILURE);
        }

        // Allouer de la mémoire pour le mot et le copier
        mots[0] = strdup(" ");
        if (mots[0] == NULL) {
            perror("Erreur d'allocation de mémoire");
            exit(EXIT_FAILURE);
        }

        // Terminer le tableau
        mots[1] = NULL;

        return mots;
    }

    // Sinon, traiter la chaîne normalement
    char **mots = (char **)malloc(MAX_ARGS * sizeof(char *));
    if (mots == NULL) {
        perror("Erreur d'allocation de mémoire");
        exit(EXIT_FAILURE);
    }

    // Utilisation de strtok pour extraire le premier mot
    char *mot = strtok(phrase, delimiteur);
    int index = 0;  // Indice du tableau

    // Boucle pour extraire les mots suivants
    while (mot != NULL && index < MAX_ARGS) {
        // Allouer de la mémoire pour le mot et le copier
        mots[index] = strdup(mot);

        // Vérifier l'allocation de mémoire
        if (mots[index] == NULL) {
            perror("Erreur d'allocation de mémoire");
            exit(EXIT_FAILURE);
        }

        // Incrémentation de l'indice
        index++;

        // Utilisation de strtok pour extraire le prochain mot
        mot = strtok(NULL, delimiteur);
    }
    mots[index] = NULL;

    return mots;
}

int executerCommande(char * commande)
{
    //organiser commande
    char ** cmd = extraireMots(commande," ");
    if (strcmp(cmd[0]," ") == 0)
    {
        return 0;
    }
    else{
  


    if(strcmp(cmd[0], "exit") == 0){
        //
        //printf("dans exit \n");
        return exitCmd(cmd[1]);
    }
    else if(strcmp(cmd[0], "cd") == 0){
        //printf("dans cd \n");
        return cd(cmd[1]);    
    }
    else if(strcmp(cmd[0], "pwd") == 0) {
        //printf("dans pwd \n");
        char *currentDir = pwd(); //reccuperer le chemin du rep courrant
        if(currentDir == NULL)
        {
            free(currentDir);
            return 1;
        }
        else {
           write(STDOUT_FILENO, currentDir, strlen(currentDir));  //affichage du chemin courrant
           write(STDOUT_FILENO, "\n", 1);
           free(currentDir);
           return 0 ;
        } 
         
    }
    else if(strcmp(cmd[0], "?") == 0){    
        int old_ret = retCmd();
        char old_ret_s[5]; 
        sprintf(old_ret_s, "%d", old_ret);
        write(STDOUT_FILENO, old_ret_s , strlen(old_ret_s)); 
        write(STDOUT_FILENO, "\n", 1);
        return 0;
    }
    else
    { 
        //commande externe
        pid_t pid = fork(); 
        if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
        }
        else if(pid == 0 )
        {
            //pg du fils
            //printf("cmd externe  \n");
            execvp(cmd[0], cmd);
            perror(cmd[0]);
            exit(EXIT_FAILURE);
          //  return 1;
           // exit(EXIT_FAILURE);

        }
        else{
            //pg du pere
             for (int i = 0; cmd[i] != NULL; i++) {
                free(cmd[i]);
            }
            free(cmd);
           
            int status;
            waitpid(pid, &status, 0);
            if ( WIFEXITED(status)) {
                   return (WEXITSTATUS(status)); // Terminaison normale
            } else {
                   return 1;
            }
           

        }
    }
    }

   

    //return 0;
    
    
 // return 0;
}

