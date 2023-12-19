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
//définition des couleurs du prompt
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m" 
//définition de la taille maximale du prompt
#define MAX_PROMPT_SIZE 30 
//définition du nombre maximum des arguments d'une commande 
#define MAX_ARGS 20
//constante qui me permette de gerer mes redirections 
#define SIMPLE_REDIR 0
#define REDIR_WITH_TRUNC 1
#define REDIR_APPEND 2 
#define MAX_CMD_LEN 256

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

    return mots;
}
int executerCommande(char * commande)
{
    //organiser commande
    char ** cmd = extraireMots(commande, " ");
    if (strcmp(cmd[0], " ") == 0)
    {
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


// penser à faire un nouveau fichier pour les redirections 
// int saved_stdin; // Pour sauvegarder le descripteur de fichier de l'entrée standard
//int new_stdin;   // Pour le nouveau descripteur de fichier après la redirection
// penser à gerer si la redirection n'est pas gerer genre erreur
// char **extraireMots2(char *phrase, char *delimiteurs) {
//     char **mots = (char **)malloc(MAX_ARGS * sizeof(char *));
//     if (mots == NULL) {
//         perror("Erreur d'allocation de mémoire");
//         exit(EXIT_FAILURE);
//     }

//     int index = 0;
//     char *mot = phrase;

//     while (*mot != '\0' && index < MAX_ARGS) {
//         // Si le caractère actuel est un délimiteur, ajoutez-le comme un mot distinct
//         if (strchr(delimiteurs, *mot) != NULL) {
//             mots[index++] = strndup(mot, 1);
//             mot++;
//         } else {
//             // Sinon, trouvez la fin du mot actuel et ajoutez-le au tableau
//             char *finMot = mot;
//             while (*finMot != '\0' && strchr(delimiteurs, *finMot) == NULL) {
//                 finMot++;
//             }
//             mots[index++] = strndup(mot, finMot - mot);
//             mot = finMot;
//         }
//     }

//     mots[index] = NULL;

//     return mots;
// }
char **extraireMots2(char *ligne) {
    char **mots = malloc(MAX_ARGS * sizeof(char *));
    char *mot;
    int i = 0;

    mot = strtok(ligne, " ");
    while (mot != NULL) {
        // Ignorer les espaces
        if (strlen(mot) > 0) {
            // Inclure <, >, 2>, 2>>, 2>| comme des mots distincts
            if (strcmp(mot, "<") == 0 || strcmp(mot, ">") == 0 || strcmp(mot, "2>") == 0 || strcmp(mot, "2>>") == 0 || strcmp(mot, "2>|") == 0) {
                mots[i] = strdup(mot);
                i++;
            } else {
                // Inclure le mot tel quel
                mots[i] = strdup(mot);
                i++;
            }
        }
        mot = strtok(NULL, " ");
    }
    mots[i] = NULL; // Marquer la fin du tableau avec NULL

    return mots;
}




int redirEntre(char *cmd, char *file){
    // Diviser la commande en son nom de programme et ses arguments

    char **mots = extraireMots2(cmd);

    // le pere cree un fils 
    pid_t pid = fork();
    if (pid < 0){
        perror("erreur fork");
        exit(EXIT_FAILURE);
    }
    else if (pid ==0){
        // on est dans le fil qui execute cmd

        int fd = open(file,O_RDONLY);
        if (fd==-1){
            perror("erreur open");
            exit(EXIT_FAILURE);
        }
        // on redirige l'entrée standard vers le fichier
        dup2(fd, STDIN_FILENO);
        close(fd);
        // ensuite on execute la cmd
        execvp(mots[0], mots); // Utiliser execvp au lieu de execlp
        perror("exec error");
        exit(EXIT_FAILURE);
    }else{
        // on est dans le pere. On attend que le fil se termine
      int status;
        wait(&status);
        if (WIFEXITED(status)) {
            return 0;
        } else {
            return 1; // Ou une autre valeur d'erreur
        }
    }
}
int redirSortie (char *cmd , char * file, int r) {
    // Diviser la commande en son nom de programme et ses arguments
    char **mots = extraireMots2(cmd);

    pid_t pid = fork();
    if(pid < 0){
        perror("erreur fork");
        exit(EXIT_FAILURE);  
    }
    else if(pid==0){
        int fd ;
        if(r == REDIR_WITH_TRUNC){ // >|
            fd = open(file,O_WRONLY | O_CREAT | O_TRUNC,0644);
        }else if (r == REDIR_APPEND){ // >>
            fd = open(file,O_WRONLY | O_CREAT | O_APPEND,0644);
        }else{ // >
            fd = open(file,O_WRONLY | O_CREAT |O_EXCL,0644);
        }
        if(fd == -1){
            perror("erreur open");
            exit(EXIT_FAILURE);
        }
    // on redirige la sortie standard vers le fichier
     dup2(fd, STDOUT_FILENO);
     close(fd);
     // ensuite on execute la cmd  
    //debeugprintf("Executing: %s < %s\n", cmd, file);
     execvp(mots[0], mots); // Utiliser execvp au lieu de execlp
    perror("exec error");
    exit(EXIT_FAILURE);
    }else{
     // on est dans le pere. On attend que le fil se termine
         int status;
            wait(&status);
        if (WIFEXITED(status)) {
            return 0;
        } else {
            return 1; // Ou une autre valeur d'erreur
        }
    }
}
int redirErreur(char *cmd , char *file, int r){
    // Diviser la commande en son nom de programme et ses arguments
    char **mots = extraireMots2(cmd);

    pid_t pid = fork();
    if(pid < 0){
        perror("erreur fork");
        exit(EXIT_FAILURE);  
    }
    else if(pid==0){
        int fd ;
        if(r == REDIR_WITH_TRUNC){ // 2>|
            fd = open(file,O_WRONLY | O_CREAT | O_TRUNC,0644);
        }else if (r == REDIR_APPEND){ // 2>>
            fd = open(file,O_WRONLY | O_CREAT | O_APPEND,0644);
        }else{ // 2>
            fd = open(file,O_WRONLY | O_CREAT |O_EXCL,0644);
        }
        if(fd == -1){
            perror("erreur open");
            exit(EXIT_FAILURE);
        }
    // on redirige la sortie standard vers le fichier
     dup2(fd, STDERR_FILENO);
     close(fd);
     // ensuite on execute la cmd  
     execvp(mots[0], mots); // Utiliser execvp au lieu de execlp
    perror("exec error");
    exit(EXIT_FAILURE);
    }else{
     // on est dans le pere. On attend que le fil se termine
         int status;
             wait(&status);
        if (WIFEXITED(status)) {
            return 0;
        } else {
            return 1; // Ou une autre valeur d'erreur
        }
    }
}


int executerRedirection(char *commande) {
    char **mots = extraireMots2(commande);
    int indexRedirection = 0;

    // Trouver l'index du caractère de redirection
    while (mots[indexRedirection] != NULL) {
        if (strcmp(mots[indexRedirection], ">") == 0 || strcmp(mots[indexRedirection], ">>") == 0 ||
            strcmp(mots[indexRedirection], "<") == 0 || strcmp(mots[indexRedirection], "2>") == 0 ||
            strcmp(mots[indexRedirection], "2>>") == 0 || strcmp(mots[indexRedirection], "2>|") == 0) {
            break;
        }
        indexRedirection++;
    }

    // Si le caractère de redirection n'est pas trouvé, afficher une erreur
    if (mots[indexRedirection] == NULL) {
        fprintf(stderr, "Aucun caractère de redirection trouvé\n");
        return 1;
    }

    // Construire la commande avec ses options
    char cmd[MAX_CMD_LEN] = "";
    for (int i = 0; i < indexRedirection; i++) {
        strcat(cmd, mots[i]);
        strcat(cmd, " ");
    }

    // Exécuter la redirection en fonction du type de redirection
    if (strcmp(mots[indexRedirection], "<") == 0) {
        return redirEntre(cmd, mots[indexRedirection + 1]);
    } else if (strcmp(mots[indexRedirection], ">") == 0) {
        return redirSortie(cmd, mots[indexRedirection + 1], SIMPLE_REDIR);
    } else if (strcmp(mots[indexRedirection], ">>") == 0) {
        return redirSortie(cmd, mots[indexRedirection + 1], REDIR_APPEND);
    } else if (strcmp(mots[indexRedirection], ">|") == 0) {
        return redirSortie(cmd, mots[indexRedirection + 1], REDIR_WITH_TRUNC);
    } else if (strcmp(mots[indexRedirection], "2>") == 0) {
        return redirErreur(cmd, mots[indexRedirection + 1], SIMPLE_REDIR);
    } else if (strcmp(mots[indexRedirection], "2>>") == 0 || strcmp(mots[indexRedirection], "2>|") == 0) {
        return redirErreur(cmd, mots[indexRedirection + 1], REDIR_APPEND);
    } else {
        fprintf(stderr, "Type de redirection non reconnu : %s\n", mots[indexRedirection]);
        return 1;
    }
}

// int executerRedirection(char *commande){
//     char **mots = extraireMots(commande, " ");
//     printf(" mots contient  %s",mots);
//     char *cmd = strdup(mots[0]);
//      printf(" cmd contient  %s",cmd);
//     char *typeRedirection = strdup(mots[1]);
//     printf(" type redirection contient  %s",typeRedirection);
//     char *file = strdup(mots[2]);

//     if (strcmp(typeRedirection, "<") == 0) {
//         return redirEntre(cmd, file);
//     } else if (strcmp(typeRedirection, ">") == 0) {
//         return redirSortie(cmd, file, SIMPLE_REDIR);
//     } else if (strcmp(typeRedirection, ">>") == 0) {
//         return redirSortie(cmd, file, REDIR_APPEND);
//     } else if (strcmp(typeRedirection, ">|") == 0) {
//         return redirSortie(cmd, file, REDIR_WITH_TRUNC);
//     } else if (strcmp(typeRedirection, "2>") == 0) {
//         return redirErreur(cmd, file, SIMPLE_REDIR);
//     } else if (strcmp(typeRedirection, "2>>") == 0) {
//         return redirErreur(cmd, file, REDIR_APPEND);
//     }else if (strcmp(typeRedirection, "2>|") == 0) {
//         return redirErreur(cmd, file, REDIR_WITH_TRUNC);    
//     } else {
//         fprintf(stderr, "Type de redirection non reconnu : %s\n", typeRedirection);
//         return 1;
//     }
//}
// nouveelle fonction extraire mots qui prend en compte le cas des fonctions avec plusieurs 
// char **extraireMots2(char *phrase, char *delimiteurs) {
//     char **mots = (char **)malloc(MAX_ARGS * sizeof(char *));
//     if (mots == NULL) {
//         perror("Erreur d'allocation de mémoire");
//         exit(EXIT_FAILURE);
//     }

//     char *mot = strtok(phrase, delimiteurs);
//     int index = 0;

//     while (mot != NULL && index < MAX_ARGS) {
//         mots[index++] = strdup(mot);
//         mot = strtok(NULL, delimiteurs);
//     }

//     mots[index] = NULL;

//     return mots;
// }


