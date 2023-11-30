#include "bib_jsh.h" 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/limits.h>


//définition des couleurs du prompt
#define COLOR_RED "\033[31m" 
#define COLOR_GREEN "\033[32m"
#define COLOR_BLUE "\033[34m"

//définition de la taille maximale du prompt
#define MAX_PROMPT_SIZE 30 

//définition du nombre maximum des arguments d'une commande 
#define MAX_ARGS 20

//déclaration du shell jsh
struct Prompt jsh;   


/****************************** FONCTIONS DES COMMANDES ************************************/

//fonction qui renvoie le chemin courrant 
char *pwd() { 
    char *rep = (char *)malloc(PATH_MAX); //allocation d'espace memoire pour stocker le chemin courrant
    if (getcwd(rep, PATH_MAX) == NULL)   //récupérer le répertoire courant
    {
        perror("erreur lorsq de réccuperation du chemin courrant");
        free(rep);   //libérer l'espace memoire alloué pour rep
        exit(1);   
    }
     else
        return rep;  //renvoyer le chemin courrant 
}

//fonction qui renvoie la valeur de retour de la dernière commande executée
int retCmd()
{
    return jsh.ret;
}

//fonction pour quitter le programme avec le code n
int exitAvecArgument (int n){ 
    exit(n);
}

//fonction pour quitter le programme avec la valeur de la dernière commande executée
int exitSansArgument()
{
    int k= retCmd();  //reccupérer la dernière valeur de retour
    exit(k);
}

//fonction exit complete
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

//fonction qui teste si une reference est valide ou non
int isReferenceValid(char *ref) {
    struct stat st;
    if (lstat(ref, &st) == 0) {
        // Le fichier existe, donc la référence est valide.  
        return 0;
    } 
    else {
        //le fichier n'existe pas
        return 1;
    }
}

//fonction pour changer le rep courrant
int cd( char * ref)
{  
    //si y a pas de ref entrée ---> cd vers Home
   if (ref == NULL) {
       
        const char *home = getenv("HOME");  //récuperer le chemin de Home
        if (home == NULL) {
            //free(home);
            perror("erreur lors de récupération de la variable $HOME");
            return 1;
        }
        jsh.oldPath = pwd(); //stocker le chemin du rep courrant avant le changer
        if (chdir(home) != 0) {  //cd vers Home
            //free(home);
            perror("erreur lors de changement de repertoir vers Home");
            return 1;
        }
    }
    //ref != NULL 
    else{  
        if (strcmp(ref, "-") == 0) {  //cd vers le dernier rep courrant
            if (jsh.oldPath == NULL) {
                perror("erreur lors de changement de repertoir vers -");
                return 1;
            }

            if (chdir(jsh.oldPath) != 0) {
                perror("erreur lors de changement de repertoir vers -");
                return 1;
            }
        }
        else{   //cd vers ref
            if(isReferenceValid(ref) == 0)
            {  //si la ref entrée est valide  
            jsh.oldPath = pwd(); //stocker le chemin du rep courrant avant le changer
            if (chdir(ref) != 0) {  
                perror("erreur lors de changement de repertoir courrant");
                return 1;
            }
            }
            else{
                //si la ref n'est pas valide
                perror(ref);
                return 1;
            }
        } 
    }



  return 0;


}



/*****************************  FONCTIONS D'AFFICHAGE DU PROMPT  ********************************/

//fonction pour tronker une chaine de char selon la taille entrée
char *tronkString(const char *chaine, int taille) {
    int chaineLength = strlen(chaine);  //calculer la taille de la chaine entrée
    int debut = chaineLength - taille;  //calculer le debut de la sous chaine a retourner
    char *resultat = (char *)malloc((taille + 1) * sizeof(char)); //stocker la sosu chaine a retourner
    strncpy(resultat, chaine + debut, taille);  //copier la sous chaine vers resultat
    resultat[taille] = '\0';  //fin chaine
    return resultat;
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



/******************************FONCTIONS D'EXECUTIONS**********************************/
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
        return retCmd();
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

