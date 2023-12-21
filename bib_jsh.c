#include "bib_jsh.h"
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
#include <fcntl.h>
#include <stdbool.h>
//définition des couleurs du prompt
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m" 
//définition de la taille maximale du prompt
#define MAX_PROMPT_SIZE 30 
//définition du nombre maximum des arguments d'une commande 
#define MAX_ARGS 20

char * currentDir1 = NULL;
char * oldpath = NULL;


struct Prompt jsh;    //déclaration du shell jsh


//fonction qui renvoie le chemin du rep courrant
char *pwd() {

    if (getcwd(currentDir1, PATH_MAX) == NULL) {
        perror("erreur lors de la récupération du chemin courant");
        exit(1);
    } else {
        return currentDir1;
    }
}


int retCmd()
{
    return jsh.ret;
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
   
   if (ref == NULL) { // cd tout seul
        // Changer vers le répertoire home
        const char *home_dir = getenv("HOME");

        if (home_dir == NULL) {
           // free(home_dir);
            fprintf(stderr, "Impossible de récupérer la variable d'environnement $HOME.\n");
            return 1;
        }
        free(oldpath);
        oldpath = strdup(currentDir1); //sauvegarde de mon ancien rep avant de le mettre a jour

        if (chdir(home_dir) != 0) {
            perror("chdir home");
            return 1;
        }
        pwd(); // mise a jour du rep courant 
    }
    else{ // cas du cd -
        if (strcmp(ref, "-") == 0) {
            //printf("oldpath que je devai affecter %s\n",oldpath);
            const char *prev_dir = oldpath;
            if (prev_dir == NULL) {
                return 1;
            }

            if (chdir(prev_dir) != 0) {
                perror("chdir old path");
                return 1;
            }
            currentDir1 = pwd();
        }
        else{
            //cd avec ref
            if(isReferenceValid(ref) == 0)
            {
        
            free(oldpath);
            oldpath = strdup(currentDir1);
           
    
            if (chdir(ref) != 0) {
                perror("chdir");
                return 1;
            }
            pwd();
           
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
   
    //free(currentDir);  // Libérer la mémoire allouée par pwd()
    return aretourner;
}


char **extraireMots(char *commande, char *delimiteur) {
    // Vérifier si la chaîne contient uniquement des espaces
    char * phrase = strdup(commande); 
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
            //  free(mots);
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
    free(phrase);
    return mots;
}



int executerCommande(char * commande)
{
    //organiser commande
    //printf("ma commande %s \n", commande);
    char ** cmd = extraireMots(commande, " ");
    if (strcmp(cmd[0], " ") == 0)
    {
         for (int i = 0; cmd[i] != NULL; i++) {
                free(cmd[i]);
            }
            free(cmd);
        return retCmd();
    }
    else
    {
        if (strcmp(cmd[0], "exit") == 0) {
            int result = exitCmd(cmd[1]);

            for (int i = 0; cmd[i] != NULL; i++) {
                free(cmd[i]);
            }
            free(cmd);
            return result;
        }
        else if (strcmp(cmd[0], "cd") == 0) {
            int result = cd(cmd[1]);
            for (int i = 0; cmd[i] != NULL; i++) {
                free(cmd[i]);
            }
            free(cmd);
            return result;
        }
        else if (strcmp(cmd[0], "pwd") == 0) {
            char *currentDir = pwd();
            if (currentDir == NULL) {
                // En cas d'erreur
                for (int i = 0; cmd[i] != NULL; i++) {
                    free(cmd[i]);
                }
                free(cmd);
                return 1;
            }
            else {
                write(STDOUT_FILENO, currentDir, strlen(currentDir));
                write(STDOUT_FILENO, "\n", 1);
                for (int i = 0; cmd[i] != NULL; i++) {
                    free(cmd[i]);
                }
                free(cmd);
                return 0;
            }
        }
        else if (strcmp(cmd[0], "?") == 0) {
            int old_ret = retCmd();
            char old_ret_s[5];
            sprintf(old_ret_s, "%d", old_ret);
            write(STDOUT_FILENO, old_ret_s, strlen(old_ret_s));
            write(STDOUT_FILENO, "\n", 1);
            for (int i = 0; cmd[i] != NULL; i++) {
                free(cmd[i]);
            }
            free(cmd);
            return 0;
        }
        else {
            //commande externe
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0) {
                //printf("je suis dans avant exec \n");
                //printf("premier argument %s \n", cmd[0]);
               // printf("premier argument %s \n", cmd[1]);
                execvp(cmd[0], cmd);
                perror(cmd[0]);
                exit(EXIT_FAILURE);
            }
            else {

                for (int i = 0; cmd[i] != NULL; i++) {
                    free(cmd[i]);
                }
                free(cmd);

                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status);
                } else {
                    return 1;
                }
            }
        }
    }
    
    return 1; 
}


int executerCommandeAvecRedirection(char * commande) {

// Extraire les mots de la commande
char ** mots = extraireMots(commande, " ");

// Parcourir le tableau de mots pour trouver le symbole de redirection
int k = 0;
int cpt [3];
int result ;
int estExterne = 1; // nous permet de tester si on a une cmd avec ou sans redirection 
for (int i = 0; mots[i] != NULL; i++) {
if (strcmp(mots[i], ">") == 0 || strcmp(mots[i], "<") == 0 || strcmp(mots[i], ">>") == 0 || strcmp(mots[i], ">|") == 0 || strcmp(mots[i], "2>") == 0 || strcmp(mots[i], "2>>") == 0 || strcmp(mots[i], "2>|") == 0) {
    estExterne = 0;
cpt[k] = i;
k ++ ;
}
}

if (k != 0){
k--;
}

// Si le symbole de redirection n'est pas trouvé, appeler executerCommande
if (estExterne ) {
int result = executerCommande(commande);
for (int j = 0; mots[j] != NULL; j++) {
free(mots[j]);
}
free(mots);

return result;
}

// Sinon, diviser la commande en deux parties
char cmd[1024] = "";
for (int j = 0; j < cpt[0]; j++) {
strcat(cmd, mots[j]);
strcat(cmd, " ");
}
char * fichier = mots[cpt[0] + 1];


// Ouvrir le fichier de redirection
int fd[3] ;
for (int l = 0; l <= k;l++){
if (strcmp(mots[cpt[l]], "<") == 0) {
// Ouvrir le fichier en mode lecture pour la redirection d'entrée
fd[l] = open(mots[cpt[l]+1], O_RDONLY);
} else if (strcmp(mots[cpt[l]], ">") == 0 || strcmp(mots[cpt[l]], "2>") == 0) {
// printf("je suis llllllllllllllllllllllllh \n");
// Ouvrir le fichier en mode écriture pour la redirection de sortie
fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_EXCL, 0644);
} else if (strcmp(mots[cpt[l]], ">>") == 0 || strcmp(mots[cpt[l]], "2>>") == 0) {
// Ouvrir le fichier en mode append pour la redirection de sortie
fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_APPEND, 0664);
} else if (strcmp(mots[cpt[l]], ">|") == 0 || strcmp(mots[cpt[l]], "2>|") == 0) {
// Ouvrir le fichier en mode écriture avec troncature pour la redirection de sortie
fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
}
if (fd[l] == -1) {
result = 1;
perror("open");
fprintf(stderr, "Erreur lors de l'ouverture du fichier %s\n", fichier);
goto cleanup;
}

}



// Sauvegarder la sortie standard
int stdout_backup = dup(STDOUT_FILENO);
if (stdout_backup == -1) {
perror("dupOut");
exit(EXIT_FAILURE);
}
int stdin_backup = dup(STDIN_FILENO);
if (stdin_backup == -1) {
perror("dupIn");
exit(EXIT_FAILURE);
}

int stderr_backup = dup(STDERR_FILENO);
if (stderr_backup == -1) {
perror("dupErr");
exit(EXIT_FAILURE);
} 


for (int l = 0; l <= k; l++){

// Rediriger la sortie standard vers le fichier de redirection
if ((strcmp(mots[cpt[l]], ">") == 0) || (strcmp(mots[cpt[l]], ">>") == 0) || (strcmp(mots[cpt[l]], ">|") == 0)) {
// printf("je suis dans > \n");
if (dup2(fd[l], STDOUT_FILENO) == -1) {
// printf("je suis dans erreur \n");
perror("dup2");
exit(EXIT_FAILURE);
} 
}
// Rediriger l'entrée standard depuis le fichier de redirection
else if (strcmp(mots[cpt[l]], "<") == 0) {
if (dup2(fd[l], STDIN_FILENO) == -1) {
perror("dup2");
exit(EXIT_FAILURE);
}
}
// Rediriger la sortie d'erreur vers le fichier de redirection
else if (strcmp(mots[cpt[l]], "2>") == 0 || strcmp(mots[cpt[l]], "2>>") == 0 || strcmp(mots[cpt[l]], "2>|") == 0) {
if (dup2(fd[l], STDERR_FILENO) == -1) {
perror("dup2");
exit(EXIT_FAILURE);
}
}

}


result = executerCommande(cmd);
// avec cette ligne je quittte le jsh comment gerer ?
if (dup2(stdout_backup, STDOUT_FILENO) == -1) {
perror("dup2");
exit(EXIT_FAILURE);
}
if (dup2(stdin_backup, STDIN_FILENO) == -1) {
perror("dup2");
exit(EXIT_FAILURE);
}

if (dup2(stderr_backup, STDERR_FILENO) == -1) {
perror("dup2");
exit(EXIT_FAILURE);
}

if (result != 0) {
fprintf(stderr, "Erreur lors de l'exécution de la commande\n");
perror("executerCommande");
goto cleanup; // Utilisation d'une étiquette pour éviter la duplication de code
}

for (int l = 0 ; l <= k ; l++){
if (close(fd[l]) == -1) {
perror("close");
exit(EXIT_FAILURE);
}
}

// Restaurer la sortie standard

if (close(stdout_backup) == -1) {
perror("close");
exit(EXIT_FAILURE);
}
if (close(stdin_backup) == -1) {
perror("close");
exit(EXIT_FAILURE);
}


if (close(stderr_backup) == -1) {
perror("close");
exit(EXIT_FAILURE);
} 

// Libérer la mémoire et retourner le résultat
cleanup:
for (int j = 0; mots[j] != NULL; j++) {
free(mots[j]);
}
free(mots);

if (result != 0){
    return 1;
}
else {

return result;
}
}