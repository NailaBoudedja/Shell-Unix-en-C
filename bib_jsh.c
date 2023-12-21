#include "bib_jsh.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
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



char * currentDir1 = NULL;
char * oldpath = NULL;
char * tmpExtraire = NULL ;
int nb_jobs = 0;


struct Prompt jsh;    //déclaration du shell jsh


Job jobs[MAX_JOBS];



//fonction qui renvoie le chemin du rep courrant
char *pwd() {

    if (getcwd(currentDir1, PATH_MAX) == NULL) {
        perror("erreur lors de la récupération du chemin courant");
        exit(1);
    } else {
        return currentDir1;
    }
}

//fonction qui renvoie la dernière valeur de retour
int retCmd()
{
    return jsh.ret;
}

//tester si une reférence donnée est valide ou non
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
    //vérifier si la taille est valide
    if (size <= 0) {
        return NULL;
    }

    int strLength = strlen(str);

    //vérifier si la taille demandée est plus grande que la longueur de la chaîne
    if (size > strLength) {
        fprintf(stderr, "Taille demandée plus grande que la longueur de la chaîne.\n");
        return NULL;
    }

    //valculer l'indice de début du sous-string
    int startIndex = strLength - size;

    //allouer dynamiquement la mémoire pour le sous-string
    char *subStr = (char *)malloc((size + 1) * sizeof(char));

    //copier le sous-string
    strncpy(subStr, str + startIndex, size);

    //ajouter le caractère de fin de chaîne
    subStr[size] = '\0';

    return subStr;
}

//fonction qui découpe une chaine en un tableau de mots selon un délimiteur
char **extraireMots(char *phrase1, char *delimiteur) {
    
    char * phrase = strdup(phrase1);
    
    //vérifier si la chaîne contient uniquement des espaces
    int estToutEspaces = 1;
    for (int i = 0; i < strlen(phrase); i++) {
        if (phrase[i] != ' ') {
            estToutEspaces = 0;
            break;
        }
    }

    //si la chaîne est tout espaces, retourner un tableau avec un seul mot : l'espace
    if (estToutEspaces) {
        char **mots = (char **)malloc(2 * sizeof(char *));
        if (mots == NULL) {
            //  free(mots);
            perror("Erreur d'allocation de mémoire \n");
            exit(EXIT_FAILURE);
        }

        //alloucation de la mémoire pour le mot et le copier
        mots[0] = strdup(" ");
        if (mots[0] == NULL) {
            perror("Erreur d'allocation de mémoire \n");
            exit(EXIT_FAILURE);
        }

        //ajout de NULL a la fin du tableau
        mots[1] = NULL;
        return mots;
    }

    //sinon, traiter la chaîne normalement
    char **mots = (char **)malloc(MAX_ARGS * sizeof(char *));
    if (mots == NULL) {
        perror("Erreur d'allocation de mémoire \n");
        exit(EXIT_FAILURE);
    }

    //utilisation de strtok pour extraire le premier mot
    char *mot = strtok(phrase, delimiteur);
    int index = 0;  //indice du tableau

    //boucle pour extraire les mots suivants
    while (mot != NULL && index < MAX_ARGS) {
        //allouer de la mémoire pour le mot et le copier
        mots[index] = strdup(mot);

        //vérifier l'allocation de mémoire
        if (mots[index] == NULL) {
            perror("Erreur d'allocation de mémoire \n");
            exit(EXIT_FAILURE);
        }

        //incrémentation de l'indice
        index++;

        //utilisation de strtok pour extraire le prochain mot
        mot = strtok(NULL, delimiteur);
    }
    mots[index] = NULL;
    free(phrase);
    return mots;
}

int exitAvecArgument (int n){ 
    exit(n);
}

int exitSansArgument()
{
    int k= retCmd();
    exit(k);
}



//fonction qui update l'etat des jobs
void UpdateJobs()
{
    int result; int signal; int status;
    //parcourir tous les jobs
    for (int i = 0; i < nb_jobs; i++) 
    {
        
        int cpt_done = 0 ;    
        int cpt_killed = 0;
        int cpt_stopped = 0;
        int is_dettached = 0;
        int cpt_countinue = 0;
        
        if ((jobs[i].etat != DONE) && (jobs[i].etat != DETACHED) && (jobs[i].etat != KILLED)) //que les jobs running ou stopped qui peuvent changer d'etat
        {
            for (int j = 0; j <jobs[i].nb_processus ; j++){
                
                //reccuperer l'etat du processus 
                result = waitpid(jobs[i].tableau_processus[j], &status, WNOHANG | WUNTRACED | WCONTINUED);
                if(result == 0)
                {
                    //le processus est en cours d'execution ==> le job est running
                    break;
                }
                else {
                    //le processus a terminé
                    if(WIFEXITED(status)){
                        //terminaison normale
                        cpt_done ++ ;
                        //verifier ses fils 
                        int child =  waitpid(-(jobs[i].tableau_processus[0]),&status,WNOHANG);
                        if (child == 0){
                        //il y a un sous processus qui n'est pas encore terminé
                        is_dettached = 1; 
                        }
                    }
                    else if(WIFSTOPPED(status))
                    {
                        //le processus est suspendu
                        cpt_stopped ++ ;
                    }
                    if (WIFCONTINUED(status)) {
                        //le processus a recu un signal cont
                        cpt_countinue ++ ;
                    }
                    else if(WIFSIGNALED(status))
                    { 
                        //le processus a terminé avec un signal
                        signal = WTERMSIG(status); //reccuperer le numero du signal
                        if ( (signal == 19) || (signal == 20)) {
                            //signal stop
                            cpt_stopped ++ ;
                        }
                        else if(signal == 18)
                        { 
                            //signal cont
                            cpt_countinue ++ ;
                        }
                        else {
                            //autre signal
                            cpt_killed ++ ;
                        }
                    }
                }
            
            }

            //update les etats selon les cpt
            if((cpt_done == jobs[i].nb_processus) && (is_dettached == 0))
            {
                if(jobs[i].etat != DONE)
                {
                    jobs[i].etat = DONE;
                    jobs[i].a_afficher = 1;  //pour l'afficher
                }
            
            }
            else if((cpt_done == jobs[i].nb_processus) && (is_dettached == 1))
            {
                if(jobs[i].etat != DETACHED)
                {
                    jobs[i].etat = DETACHED;
                    jobs[i].a_afficher = 1;
                } 
                jobs[i].est_surveille = 0;// faux
            }
            else if(cpt_stopped == jobs[i].nb_processus )
            {
                if(jobs[i].etat != STOPPED)
                {
                    jobs[i].etat =STOPPED;
                    jobs[i].a_afficher = 1;
                }
            }
            else if((cpt_killed + cpt_done) == jobs[i].nb_processus) 
            {
                if(jobs[i].etat != KILLED)
                {
                    jobs[i].etat = KILLED;
                    jobs[i].a_afficher = 1;
                }
            }
            else if( cpt_countinue  == jobs[i].nb_processus ) 
            {
                 if(jobs[i].etat == STOPPED)
                {
                    jobs[i].etat = RUNNING;
                    jobs[i].a_afficher = 1;
                }
            }
           
        }
        else {
            //sinon le job sort de la surveillence de jsh
            jobs[i].a_afficher = 0;
            jobs[i].est_surveille = 0;
        }
    }
}

//affichage du prompt
char *afficherJsh() {
   
    //maj des jobs
    UpdateJobs();

    char result[MAX_PROMPT_SIZE * 2] = ""; //initialisez result avec une chaîne nulle
    char *currentDir = pwd();  //récupérer le chemin du répertoire courant

    //reccuperer le nb de jobs surveillés
    int nb = 0; 

    for(int i = 0 ; i < nb_jobs ; i ++ )
    {
        if ((jobs[i].etat == RUNNING) || (jobs[i].etat == STOPPED) )
        {
            nb ++ ;
        }
    }
    //convertir nb jobs en un string
    
    char nb_string[3]; 
    sprintf(nb_string, "%d", nb);
    strcpy(result, COLOR_RED "[");
    strcat(result, nb_string);
    strcat(result, "]");


    //calculer la taille du prompt
    int prompt_length = strlen(currentDir) + 4 + strlen(nb_string);

    //vérifier la taille du prompt
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

    //allouer de la mémoire pour le résultat et le retourner
    char *aretourner = strdup(result);
   
    return aretourner;
}

//fonction qui excute un commande entrée
int executerCommande(char *commande)
{
    //découper la commande selon l'espace en un tableau de mots 
    char **cmd = extraireMots(commande, " ");

    //la commande vide
    if (strcmp(cmd[0], " ") == 0)
    {
        for (int i = 0; cmd[i] != NULL; i++)
        {
            free(cmd[i]);
        }
        free(cmd);
        return retCmd();
    }
    else
    {
        if (strcmp(cmd[0], "exit") == 0)
        {
            int result = exitCmd(cmd[1]); //executer la commande exit 
            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);
            return result;
        }
        else if (strcmp(cmd[0], "jobs") == 0)
        {

            Jobs();   //executer la commande jobs
            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);
            return 0;
        }
        else if (strcmp(cmd[0], "kill") == 0)
        {
   
            int ret = Kill(commande);  //executer la commande kill
            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);
            return ret;
        }
        else if (strcmp(cmd[0], "cd") == 0)
        {
            int result = cd(cmd[1]);
            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);
            return result;
        }
        else if (strcmp(cmd[0], "pwd") == 0)
        {

            char *currentDir = pwd();
            if (currentDir == NULL)
            {
                //en cas d'erreur
                for (int i = 0; cmd[i] != NULL; i++)
                {
                    free(cmd[i]);
                }
                free(cmd);
                return 1;
            }
            else
            {
                write(STDOUT_FILENO, currentDir, strlen(currentDir));  //affichage du chemin courrant
                write(STDOUT_FILENO, "\n", 1);
                for (int i = 0; cmd[i] != NULL; i++)
                {
                    free(cmd[i]);
                }
                free(cmd);
                return 0;
            }
        }
        else if (strcmp(cmd[0], "?") == 0)
        {
            int old_ret = retCmd();  //reccuperer la derniere valeur de retour 
            char old_ret_s[5];
            sprintf(old_ret_s, "%d", old_ret);
            write(STDOUT_FILENO, old_ret_s, strlen(old_ret_s));
            write(STDOUT_FILENO, "\n", 1);
            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);
            return 0;
        }
        else
        { 
            //commande externe 
            pid_t pid = fork();  //creation d'un processus fils 
            if (pid < 0)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {  
                //code fils 
                setpgid(getpid(),getpid());   //affecter son grouppid 
                restoreSignals(); //restorer les signaux ignorés
                execvp(cmd[0], cmd);  //executer la commande 
                perror(cmd[0]);
                exit(EXIT_FAILURE);
            }
            else
            {
                //code du pere
                int r , status;
                for (int i = 0; cmd[i] != NULL; i++)
                {
                    free(cmd[i]);
                }
                free(cmd);


                do{
                    //reccuperer l'etat du fils
                    r = waitpid(pid, &status, WUNTRACED | WCONTINUED);
                        
                }while( !WIFEXITED(status) && !WIFSTOPPED(status) && !WIFSIGNALED(status));
               
                
                    
               if(WIFSTOPPED(status))
                {
                    //si le processus fils a capté un signal stop == création d'un job suspendu en arrière plan
                    jobs[nb_jobs].job_id = nb_jobs + 1;
                    jobs[nb_jobs].cmd = strdup(commande);
                    jobs[nb_jobs].etat = STOPPED;
                    jobs[nb_jobs].a_afficher = 1;
                    jobs[nb_jobs].est_surveille = 1;
                    jobs[nb_jobs].nb_processus = 1;
                    jobs[nb_jobs].tableau_processus[0] = pid ;
                    nb_jobs ++ ;
                    return 0 ;
                }
                else if(WIFSIGNALED(status))
                { 
                     //si le processus fils a capté un signal autre que stop == création d'un job killed en arrière plan
                    jobs[nb_jobs].job_id = nb_jobs + 1;
                    jobs[nb_jobs].cmd = strdup(commande);
                    jobs[nb_jobs].etat = KILLED;
                    jobs[nb_jobs].a_afficher = 1;
                    jobs[nb_jobs].est_surveille = 1;
                    jobs[nb_jobs].nb_processus = 1;
                    jobs[nb_jobs].tableau_processus[0] = pid ;
                    nb_jobs ++ ;
                    return 0 ;
                }else if (WIFEXITED(status))   //terminaison normale du fils
                {
                    return (WEXITSTATUS(status));  //renvoyer le code de retour du fils
                }
            }
        }
    }
    return 1;
}

//création d'un nouveau job
void creerJob(char* commande, pid_t tableau_des_processus[]) {
   
    if (nb_jobs < MAX_JOBS) {
        jobs[nb_jobs].job_id = nb_jobs+1;
        jobs[nb_jobs].cmd = strdup(commande);
        jobs[nb_jobs].etat = RUNNING;
        jobs[nb_jobs].a_afficher = 1; //vrai
        jobs[nb_jobs].est_surveille = 1; //vrai
        jobs[nb_jobs].nb_processus = 1;  //a changer 
        memcpy(jobs[nb_jobs].tableau_processus, tableau_des_processus, MAX_PROCESSUS * sizeof(pid_t)); 
        for (int j = 0 ; j< jobs[nb_jobs].nb_processus; j ++ )
        {
            setpgid(jobs[nb_jobs].tableau_processus[j], jobs[nb_jobs].tableau_processus[0]);  //affecter le pid du premier processus comme le pgroup de tous les processus du job
        
        }
        nb_jobs++;  
    }   
}



//fonction pour l'affichage des jobs survéillés
void Jobs() {
    
    //maj des etats des jobs
    UpdateJobs();
   
    char buffer[1024];
   
    for (int i = 0; i < nb_jobs; i++) {

        if(jobs[i].etat != KILLED && jobs[i].est_surveille == 1)   //affichage des jobs surveillés et non killed
        {
        int length = snprintf(buffer, sizeof(buffer), "[%d] %d ", jobs[i].job_id, jobs[i].tableau_processus[0]);

        switch (jobs[i].etat) {
            case RUNNING:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Running\t");
                break;
            case STOPPED:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Stopped\t");
                break;
            case DETACHED:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Detached\t");
                break;
            case KILLED:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Killed\t");
                break;
            case DONE:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Done\t");
                break;
            default:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Unknown\t");
        }

        length += snprintf(buffer + length, sizeof(buffer) - length, "%s\n", jobs[i].cmd);
        write(STDOUT_FILENO, buffer, length);
        
    }
    }
}

//supprimer le char : % d'une chaine pour reccuperer le id du job
void detecterNumJob(char *chaine) {
    if (chaine != NULL && chaine[0] == '%') {
        int len = strlen(chaine);
        for (int i = 0; i < len; ++i) {
            chaine[i] = chaine[i + 1];   // decalage des positions
        }
    }
}


//fonction pour afficher les jobs qui ont changé d'etat
void affichageJobsModifies()
{
    UpdateJobs();
   
    char buffer[1024];
   
    for (int i = 0; i < nb_jobs; i++) {
        if(jobs[i].a_afficher == 1)
        {
        int length = snprintf(buffer, sizeof(buffer), "[%d] %d ", jobs[i].job_id, jobs[i].tableau_processus[0]);

        switch (jobs[i].etat) {
            case RUNNING:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Running\t");
                break;
            case STOPPED:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Stopped\t");
                break;
            case DETACHED:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Detached\t");
                break;
            case KILLED:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Killed\t");
                break;
            case DONE:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Done\t");
                break;
            default:
                length += snprintf(buffer + length, sizeof(buffer) - length, "Unknown\t");
        }

        length += snprintf(buffer + length, sizeof(buffer) - length, "%s\n", jobs[i].cmd);

       //l'affichage sur la sortie err standard
       write(STDERR_FILENO, buffer, length);
       jobs[i].a_afficher = 0; 
    }
}
}

