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

#include <signal.h>   // Pour sigaction et les signaux





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


int saved_stdin =0;
int saved_stdout = 0;
int saved_stderr = 0;

int estPipe = 0;

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

int exitCmd(char * val)
{

    int still_running = 0;  //verifier si il y a des jobs en cours d'execution ou suspendus
    for (int i = 0; i < nb_jobs; i++) {
       if ((jobs[i].etat == STOPPED)||(jobs[i].etat == RUNNING))
       {
        still_running = 1;
        break;
       }
    }
    if(still_running)
    {
        write(STDERR_FILENO,"There are stopped jobs\n", 24);
        return 1;
    }
    else{
        //pas de jobs running ou suspendus
        if(val != NULL)
        {
           return exitAvecArgument (atoi(val)); 
        }
       else
        {
            return exitSansArgument();
        }
    } 
}




 //fonction pour ignorer un ensemble des signaux par jsh
void ignoreSignals() {

    //la liste des signaux à ignorer
    int signals[] = {SIGTERM, SIGTTIN, SIGQUIT, SIGTTOU, SIGTSTP};  //SIGINT,
    
    struct sigaction sig_action;
    sig_action.sa_handler = SIG_IGN; 
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    for (int i = 0; i < 5; ++i) {
        if (sigaction(signals[i], &sig_action, NULL) == -1) {
            perror("erreur lors de l'ignorance des signaux \n");
            exit(EXIT_FAILURE);
        }
    }
}

//fonction pour reprendre le traitement par défaut des signaux
void restoreSignals() {
    int signals[] = {SIGINT, SIGTERM, SIGTTIN, SIGQUIT, SIGTTOU, SIGTSTP};   
    struct sigaction sig_action;
    sig_action.sa_handler = SIG_DFL;   // Action par défaut
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    for (int i = 0; i < 6; ++i) {
        if (sigaction(signals[i], &sig_action, NULL) == -1) {
            perror("Erreur lors de la restauration du signal par défaut");
            exit(EXIT_FAILURE);
        }
    }
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
            if(cmd[1] == NULL)
            {
                //la cmd est jobs
                Jobs();
            }
            else if(strcmp(cmd[1], "-t") == 0)
            {
                
                //avec l'arboressence des processus
                if(cmd[2] == NULL)
                {
                    //all jobs
                    afficherjobsAvecT();
                }
                else{
                    //un job précis
                    detecterNumJob(cmd[2]);
                    int id_job =  atoi(cmd[2]);
                    afficherUnjobAvecT(id_job);
                }
            }
            else {

                //un seul job sans t
                detecterNumJob(cmd[1]);
                int id_job =  atoi(cmd[1]);
                afficherUnjobSansT(id_job);

            }
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
          else if (strcmp(cmd[0], "bg") == 0)
        {

            char* result = detecterNumJob(cmd[1]);

            
            int ret  = relancerJobArrierePlan(result);
            free(result);
        
            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);
            return ret;
            
        }
        else if (strcmp(cmd[0], "fg") == 0)
        {
           
            char* result = detecterNumJob(cmd[1]);
            
            int ret  = relancerJobAvantPlan(result);

            free(result);
           

            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);

           
            
            return ret;
            
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
                restoreSignals();

                struct sigaction sa;

                sigemptyset(&sa.sa_mask);
                sa.sa_handler = SIG_IGN; 
                sa.sa_flags = 0; 

              
                if (sigaction(SIGTTOU, &sa, NULL) == -1) {
                    perror("Erreur sigaction SIGTTOU");
                    exit(EXIT_FAILURE);
                }

               
                if (sigaction(SIGTTIN, &sa, NULL) == -1) {
                    perror("Erreur sigaction SIGINT");
                    exit(EXIT_FAILURE);
                }

                
               
                  if (isatty(STDIN_FILENO))
                {
                    if (tcsetpgrp(STDIN_FILENO, getpid()) == -1) {
                    perror("tcsetpgrp du pere");
                    exit(EXIT_FAILURE);
                    }

                }
                
                
                // Restaurer le traitement par défaut pour SIGTTOU et SIGTTIN
                sa.sa_handler = SIG_DFL;

                if (sigaction(SIGTTOU, &sa, NULL) == -1) {
                    perror("Erreur sigaction SIGTTOU");
                    exit(EXIT_FAILURE);
                }

                if (sigaction(SIGTTIN, &sa, NULL) == -1) {
                    perror("Erreur sigaction SIGTTIN");
                    exit(EXIT_FAILURE);
                }



                
             
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


                struct sigaction sa;

                sigemptyset(&sa.sa_mask);
                sa.sa_handler = SIG_IGN; 
                sa.sa_flags = 0; 

              
                if (sigaction(SIGTTOU, &sa, NULL) == -1) {
                    perror("Erreur sigaction SIGTTOU");
                    exit(EXIT_FAILURE);
                }

               
                if (sigaction(SIGTTIN, &sa, NULL) == -1) {
                    perror("Erreur sigaction SIGINT");
                    exit(EXIT_FAILURE);
                }

                
                if (isatty(STDIN_FILENO))
                {
                    if (tcsetpgrp(STDIN_FILENO, getpid()) == -1) {
                    perror("tcsetpgrp du pere");
                    exit(EXIT_FAILURE);
                    }

                }
                
        
                // Restaurer le traitement par défaut pour SIGTTOU et SIGTTIN
                sa.sa_handler = SIG_DFL;

                if (sigaction(SIGTTOU, &sa, NULL) == -1) {
                    perror("Erreur sigaction SIGTTOU");
                    exit(EXIT_FAILURE);
                }

                if (sigaction(SIGTTIN, &sa, NULL) == -1) {
                    perror("Erreur sigaction SIGTTIN");
                    exit(EXIT_FAILURE);
                }

                    
                if (WIFEXITED(status))   //terminaison normale du fils
                {
                    jobs[nb_jobs].etat = DONE;
                    return (WEXITSTATUS(status));  //renvoyer le code de retour du fils
                    
                }else if(WIFSIGNALED(status))
                { 
                    //si le processus fils a capté un signal autre que stop == création d'un job killed en arrière plan
                    saved_stdin = dup(STDIN_FILENO);
                    saved_stderr = dup(STDERR_FILENO);
                    saved_stdout= dup(STDOUT_FILENO);



                    jobs[nb_jobs].job_id = nb_jobs + 1;
                    jobs[nb_jobs].cmd = strdup(commande);
                    jobs[nb_jobs].etat = KILLED;
                    jobs[nb_jobs].a_afficher = 1;
                    jobs[nb_jobs].est_surveille = 1;
                    jobs[nb_jobs].nb_processus = 1;
                    jobs[nb_jobs].tableau_processus[0] = pid ;
                    nb_jobs ++ ;
                    return 0 ;
                }
                else if(WIFSTOPPED(status))
               {
                   
                    
                    //printf("le signal recu %d \n", sigst)
                    //si le processus fils a capté un signal stop == création d'un job suspendu en arrière plan
                    saved_stdin = dup(STDIN_FILENO);
                    saved_stderr = dup(STDERR_FILENO);
                    saved_stdout= dup(STDOUT_FILENO);


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
            }
        }
    }
    return 1;
}

    int relancerJobArrierePlan(char* id_job)
    {
        if (id_job != NULL)
        {
            int job_id = atoi(id_job); 
            if (jobs[job_id - 1].etat == STOPPED)
            {
                char* cont = "-18";
                int ret = (killJob(cont,job_id -1));
                return ret ;
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

//fonction qui execute une commande en arrière plan
int executerCmdArrierePlan(char* commande) {
    
    
    char** cmd = extraireMots(commande, " ");
   
    if (strcmp(cmd[0], " ") == 0) {
        //libérer la mémoire
        for (int i = 0; cmd[i] != NULL; i++) {
            free(cmd[i]);
        }
        return retCmd();
    } else {
        
        pid_t pid = fork();  //cration d'un processus fils

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            //code fils
            
            restoreSignals();  //restoration des signaux ignorés
            int fnull = open("/dev/null", O_RDWR);  //ouverture du fichier /dev/null en mode lecture
            if (fnull == -1) {
                perror("erreur lors d'ouverture du fichier \n");
                exit(EXIT_FAILURE);
            }

            //sauvgarder les descripteurs


            saved_stdin = dup(STDIN_FILENO);
            saved_stderr = dup(STDERR_FILENO);
            saved_stdout= dup(STDOUT_FILENO);

           /* if (dup2(saved_stdin, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            if (dup2(saved_stdout, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
            }
            if (dup2(saved_stderr, STDERR_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
            }
            */
            close(saved_stderr);
            close(saved_stdin);
            close(saved_stdout);
        
            //rediriger la sortie et entrée et erreur standards vers null
            if (dup2(fnull, STDIN_FILENO) == -1  || dup2(fnull, STDOUT_FILENO) == -1  || dup2(fnull, STDERR_FILENO) == -1) {
                perror("erruer lors de la duplication \n");
                exit(EXIT_FAILURE);
            }
            execvp(cmd[0], cmd);  //executer la commande

            exit(1);
        
        } else {
            //création d'un job
           
            pid_t tableau_des_processus[1] = {pid};
            creerJob(commande, tableau_des_processus);
            
            //libérer la mémoire
            for (int i = 0; cmd[i] != NULL; i++) {
                free(cmd[i]);
            }
            free(cmd);
        }
    }
    return 1;
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
char* detecterNumJob(char *chaine) {
    char *result = NULL;
    if (chaine != NULL && chaine[0] == '%') {
        int len = strlen(chaine);
        // Allouer de la mémoire pour 'result' (len pour les caractères + 1 pour le caractère nul)
        result = malloc(len * sizeof(char));
        if (result != NULL) {
            for (int i = 0; i < len - 1; ++i) {
                result[i] = chaine[i + 1]; // Décalage des positions
            }
            result[len - 1] = '\0'; // Ajout du caractère nul à la fin
        }
    }
    return result;
}

//fonction pour envoyer un signal a un processus
int killProcessus(char* signal, pid_t pid)
{ 
    //tester si le processus avec pid existe ou non
    if (kill(pid, 0) == 0) {
        //processus exite
        if(signal == NULL)
        {
            //envoyer le sign par defaut
            return -(kill(pid,SIGTERM));
        }
        else
        {
            int sig = atoi(signal);
            return -(kill(pid,-sig));  
        }
    } else {
        //le processus n'existe pas
        write(STDERR_FILENO,"le processus n'existe pas\n",27);
        return 0; 
    }

}

//envoyer un signal a un job
int killJob(char * signal, int id_job)
{
    int ret ;
    //tester si le job existe ou non
    if((id_job >= 0) && (id_job <= nb_jobs ))
    {
        if(jobs[id_job].etat == DONE)
        {
            return  1 ;
        }
       
        if (signal == NULL){
        //envoyer le signal par défaut pour tous les processus du groupe du job
        ret = kill(-(jobs[id_job].tableau_processus[0]),SIGTERM);
        return (- ret);
         }
        else{
        int sig = atoi(signal);
        //envoyer le signal pour tous les processus du groupe du job
        ret = kill(-(jobs[id_job].tableau_processus[0]),-sig);
        return (- ret); 
    }
    }
    else{
        write(STDERR_FILENO,"le job n'existe pas\n",21);
        return 0;
    }
}

int Kill(char * commande)
{
    char c = '%';  
    char *result = strchr(commande,c);  //chercher le caractère % pour déterminer si le signal sera envoyé a un job ou a un processus

    char ** cmd = extraireMots(commande, " ");
    if (result != NULL) {  
        //envoyer un signal a un job
        
        if(cmd[2] == NULL)   //kill %job
        {
          char* result = detecterNumJob(cmd[1]);
          int id_job = atoi(result);
          free(result);
          for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
          free(cmd);
          return  killJob(NULL,id_job-1);
        }
        else {
            //kill sig %job
            char * result = detecterNumJob(cmd[2]);
            int id_job = atoi(result);
            free(result);
            int ret = killJob(cmd[1],id_job-1);
            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);
            return ret ;
        }

    } else {
        //kill processus 
        if(cmd[2] == NULL)
        {
            //kill pid
            pid_t pid= atoi(cmd[1]);

            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);

            return killProcessus(NULL,pid);
        }
        else{

            //kill sig pid
            pid_t pid= atoi(cmd[2]);
            int ret = killProcessus(cmd[1],pid); 

            for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
            free(cmd);
            return ret ; 
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


int executerCmdGlobal(char * commande)
{

   
    //UpdateJobs();
    char caractere = '&';
    char* caractere2 = " &";


    char * resultat = strstr(commande, caractere2);
    if (resultat == NULL)
    {
        
        resultat =strchr(commande, caractere) ;
       
    }
    if (resultat != NULL) {
      
            size_t longueur = resultat - commande;
            char newCommande[longueur];

           
             strncpy(newCommande, commande, longueur);
             newCommande[longueur] = '\0';  // Assurez-vous d'ajouter le caractère nul à la fin

            int ret = executerCmdArrierePlan(newCommande);


            affichageJobsModifies();

        
            return ret;
         
    } else {
        int rett = executerCommande(commande);
        char ** cmd = extraireMots(commande, " ");
        if (strcmp(cmd[0], "jobs") != 0)
        {
    
          affichageJobsModifies();
        }
        for (int i = 0; cmd[i] != NULL; i++) {
            free(cmd[i]);
        }
        free(cmd);

        return rett;
    }


}

// decoupe une commande contenant une substituion
char** extraireMotsAvecSubstitution(char *commande) {
    char **mots = malloc(MAX_ARGS * sizeof(char *));
    int index = 0;
    char *debut = commande;
    char *fin = commande;
    int dansSubstitution = 0;

    while (*fin != '\0') {
        if (*fin == ' ' && !dansSubstitution) {
            mots[index] = strndup(debut, fin - debut);
            index++;
            debut = fin + 1;
        } else if (*fin == '<' && *(fin + 1) == '(') {
            dansSubstitution = 1;
        } else if (*fin == ')' && dansSubstitution) {
            mots[index] = strndup(debut, fin - debut + 1);
            index++;
            debut = fin + 1;
            dansSubstitution = 0;
        }
        fin++;
    }

    if (debut != fin) {
        mots[index] = strndup(debut, fin - debut);
        index++;
    }

    mots[index] = NULL;

    return mots;
}


int executeCmdAvecSubstitution(char *commande) {
    // on extraie les mots de la commande
    char** mots = extraireMotsAvecSubstitution(commande);

    // Compte le nombre de substitutions de processus
    int nbCmds = 0;
    for (int i = 0; mots[i] != NULL; i++) {
        if (strstr(mots[i], "<(") != NULL) {
            nbCmds++;
        }
    }

    // on cree un tableau pour stocker les noms des tubes nommés
    char *pipes[nbCmds];

    // Crée un tube nommé pour chaque commande substituée
    for (int i = 0; i < nbCmds; i++) {
        char pipeName[20];
         sprintf(pipeName, "/tmp/pipe%d", i);
         if (access(pipeName, F_OK) != -1) {
            if (unlink(pipeName) == -1) {
                perror("unlink");
                exit(EXIT_FAILURE);
            }
        }
        // Créer le tube nommé
        if (mkfifo(pipeName, 0666) == -1) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
        pipes[i] = strdup(pipeName);
    }
    int cmdIndex = 0;
    for (int i = 0; mots[i] != NULL; i++) { 
    if (strstr(mots[i], "<(") != NULL) {
        char* cmdCopy = strdup(mots[i] + 2);  // Créer une copie de la chaîne, en excluant les deux premiers caractères ("<(")
        cmdCopy[strlen(cmdCopy) - 1] = '\0';  // Enlever le dernier caractère (")")
       // printf("-------------Commande a executer----- : %s\n", cmdCopy); /// ici on a bien echo 123

        char* cmd = strtok(cmdCopy, " \t");
     //   printf("-------------Commande apres strok : %s\n", cmd); // ici on a bien echo dans cmd 

        char *args[MAX_ARGS];
        int argCount = 0;
        while ((args[argCount] = strtok(NULL, " \t")) != NULL) {
          //  printf("--------------------Argument %d : %s\n", argCount, args[argCount]); // on a bien 123
            argCount++;
        }
        args[argCount] = NULL; // pour marquer la fin du tableau args 

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Dans le processus fils
                int fd = open(pipes[cmdIndex], O_WRONLY); // on ouvre le tube en ecrture 
              //  printf("-----------------pipe ouvert est %s\n",pipes[cmdIndex]);
                if (fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO); // on redirige sa sortie vers le tube 
                close(fd);
            for (int j = 0; j < argCount; j++) {
            }
                execvp(cmd, args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } // le fils 
            cmdIndex++;
            free(cmdCopy);
        }
    }
    
    // Lancer la commande principale avec l'entrée connectée à la sortie du dernier tube
        char *args[MAX_ARGS];
        int index = 0;
    for (int i = 0; i < nbCmds; i++) {
    args[index] = pipes[i];
   //     printf("---------------Ajout du tube %s comme argument\n", pipes[i]);  // Imprimer le nom du tube

    index++;
    }
    args[index] = NULL;  
   
  //  printf("--------------------la commmande a executer est %s\n",mots[0]);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Dans le processus fils
        int fd = open(pipes[nbCmds - 1], O_RDONLY);  // Ouvrir le tube nommé pour la lecture
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);

        execvp(mots[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // Attendre la fin de tous les processus fils
    for (int i = 0; i < nbCmds + 1; i++) {
        wait(NULL);
    }


    // Supprimer les tubes nommés dans le processus parent
    for (int i = 0; i < nbCmds; i++) {
        if (unlink(pipes[i]) == -1) {
            perror("unlink");
            exit(EXIT_FAILURE);
        }
        free(pipes[i]);
    }
   // free(cmdCopy);

    return 0;
}


//execution des commandes simple + commandes des redirection 

int executerCommandeGeneral(char * commande) {
    
    // Extraire les mots de la commande
    char ** mots = extraireMots(commande, " ");
    int k = 0, cpt[3], result, estExterne = 1; // Pour tester si on a une commande avec ou sans redirection
    int estSubstitution = 0; // pour tester si on a une substitution de processus
    // Parcourir le tableau de mots pour trouver le symbole de redirection
    for (int i = 0; mots[i] != NULL; i++) {
         if (strstr(mots[i], "<(") != NULL) {
        estSubstitution = 1;
        break;
    }

        if (strcmp(mots[i], ">") == 0 || strcmp(mots[i], "<") == 0 || strcmp(mots[i], ">>") == 0 || 
            strcmp(mots[i], ">|") == 0 || strcmp(mots[i], "2>") == 0 || strcmp(mots[i], "2>>") == 0 || 
            strcmp(mots[i], "2>|") == 0) {
            estExterne = 0;
            cpt[k] = i;
            k++;
        }
    }
    if (k != 0) k--;
    // Si la substitution de processus est trouvée, appeler executeCmdAvecSubstitution
    if (estSubstitution) {
    result = executeCmdAvecSubstitution(commande);
    for (int j = 0; mots[j] != NULL; j++) {
        free(mots[j]);
    }
    free(mots);

    return result;
}

    // Si le symbole de redirection n'est pas trouvé, appeler executerCmdGlobal
    if (estExterne) {
        result = executerCmdGlobal(commande);
        for (int j = 0; mots[j] != NULL; j++) free(mots[j]);
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
    int fd[3];
    for (int l = 0; l <= k; l++) {
        if (strcmp(mots[cpt[l]], "<") == 0) {
            fd[l] = open(mots[cpt[l]+1], O_RDONLY);
        } else if (strcmp(mots[cpt[l]], ">") == 0 || strcmp(mots[cpt[l]], "2>") == 0) {
            fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_EXCL, 0664);
        } else if (strcmp(mots[cpt[l]], ">>") == 0 || strcmp(mots[cpt[l]], "2>>") == 0) {
            fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_APPEND, 0664);
        } else if (strcmp(mots[cpt[l]], ">|") == 0 || strcmp(mots[cpt[l]], "2>|") == 0) {
            fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
        }
        if (fd[l] == -1) {
            result = 1;
            perror("open");
            fprintf(stderr, "Erreur lors de l'ouverture du fichier %s\n", fichier);
            goto cleanup;
        }
    }

    // Sauvegarder la sortie standard, l'entrée standard et la sortie d'erreur
    int stdout_backup = dup(STDOUT_FILENO);
    int stdin_backup = dup(STDIN_FILENO);
    int stderr_backup = dup(STDERR_FILENO);

    if (stdout_backup == -1 || stdin_backup == -1 || stderr_backup == -1) {
        perror("dup");
        exit(EXIT_FAILURE);
    }

    // Redirection des flux
    for (int l = 0; l <= k; l++) {
        if ((strcmp(mots[cpt[l]], ">") == 0) || (strcmp(mots[cpt[l]], ">>") == 0) || (strcmp(mots[cpt[l]], ">|") == 0)) {
            if (dup2(fd[l], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            } 
        } else if (strcmp(mots[cpt[l]], "<") == 0) {
            if (dup2(fd[l], STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(mots[cpt[l]], "2>") == 0 || strcmp(mots[cpt[l]], "2>>") == 0 || strcmp(mots[cpt[l]], "2>|") == 0) {
            if (dup2(fd[l], STDERR_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
        }
    }

    result = executerCmdGlobal(cmd);

    // Restaurer les flux standard
    if (dup2(stdout_backup, STDOUT_FILENO) == -1 || dup2(stdin_backup, STDIN_FILENO) == -1 ||
        dup2(stderr_backup, STDERR_FILENO) == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
    }

    // Fermeture des descripteurs de fichiers et des sauvegardes
    for (int l = 0 ; l <= k ; l++) {
        if (close(fd[l]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }
    if (close(stdout_backup) == -1 || close(stdin_backup) == -1 || close(stderr_backup) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    } 

    // Libérer la mémoire et retourner le résultat
    cleanup:
    for (int j = 0; mots[j] != NULL; j++) {
        free(mots[j]);
    }
    free(mots);

    return (result != 0) ? 1 : result;
}






int relancerJobAvantPlan(char* id_job)
{


  
   if(id_job != NULL)
   {


    int job_id  = atoi(id_job);

    

    if(jobs[job_id - 1].est_surveille == 1)
    {   
    //write(STDOUT_FILENO, jobs[job_id -1 ].cmd, strlen(jobs[job_id -1 ].cmd));
    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);

    close(saved_stdin);
    close(saved_stdout);
    close(saved_stderr);

    if(jobs[job_id - 1 ].etat == STOPPED)
    {
        char* cont = "-18";
        int ret = (killJob(cont,job_id -1));
        UpdateJobs();
       
    }

    int r, status ;
    do{
    
        r = waitpid(jobs[job_id - 1].tableau_processus[0], &status, WUNTRACED | WCONTINUED);
                        
    }while( !WIFEXITED(status) && !WIFSTOPPED(status) && !WIFSIGNALED(status));

    
    if(WIFSIGNALED(status))
    {
      
        jobs[job_id  - 1].etat = KILLED;
        return 0;
    }
    else if(WIFSTOPPED(status))
    {


        //si le processus fils a capté un signal stop
        jobs[job_id  - 1].etat = STOPPED;
        return 0;
    }
   
    else if (WIFEXITED(status))   //terminaison normale du fils
    {

       
        jobs[job_id  - 1].etat = DONE;
    
        return (WEXITSTATUS(status));  //renvoyer le code de retour du fils
    }
    
    }
   }
   return 1;

}


int compterOccurrences(char *chaine, char caractere) {
    int occurrences = 0;


    while (*chaine != '\0') {
        
        if (*chaine == caractere) {
            occurrences++;
        }
        chaine++;
    }

    return occurrences;
}


int pipeline(char *commande) {
    int n = compterOccurrences(commande, '|') + 1;
    char **tableauDesCommandes = extraireMots(commande, "|");

    int tubes[n - 1][2];
    for (int i = 0; i < n - 1; i++) {
        if (pipe(tubes[i]) == -1) {
            perror("Erreur lors de la création du tube");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Processus fils
            setpgid(getpid(), getpid());
            restoreSignals();
        
            for (int j = 0; j < n - 1; j++) {
                if (j != i - 1) {
                    close(tubes[j][0]);
                }
                if (j != i) {
                    close(tubes[j][1]);
                }
            }

            if (i > 0) {
                dup2(tubes[i - 1][0], STDIN_FILENO);
                close(tubes[i - 1][0]);
            }

            if (i < n - 1) {
                dup2(tubes[i][1], STDOUT_FILENO);
                close(tubes[i][1]);
            }


            //cas par cas
            if((i > 0 ) && (i < n - 1))
            {
                //milieu
                //chercher 2>
                char *resultat = strstr(tableauDesCommandes[i], "2>");
                if (resultat != NULL) {
                  

                    //tella redirection
                    char* commande; char *fichier ;
                    int c =0;

                    for(int k = 0;  k< strlen(resultat) - strlen(tableauDesCommandes[i]) ; k++)
                    {
                        commande[c]= tableauDesCommandes[i][k];
                        c ++ ;

                    }
                    c=0;
                    for(int k = strlen(resultat)-strlen(tableauDesCommandes[i]);  k<strlen(tableauDesCommandes[i])  ; k++)
                    {
                        fichier[c]= tableauDesCommandes[i][k];
                        c ++;

                    }



                    int fd = open(fichier, O_WRONLY | O_CREAT | O_EXCL, 0664);
                    if (fd == -1) {
            
                     perror("open");
                     exit(1);
            
                    }
                    if (dup2(fd, STDERR_FILENO) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    char **cmd = extraireMots(commande, " ");
                   // printf("redirection milieu  avant execution cmd 0   \n");
                    if(strcmp(cmd[0],"pwd")== 0){
                        write(STDOUT_FILENO,pwd(),sizeof(pwd()));
                    }else{
                         execvp(cmd[0], cmd);
                    }
                   
                    // En cas d'échec de l'exécution
                    perror("execvp");
                    exit(EXIT_FAILURE);
                  
                } else {
                 
                                   

                    char **cmd = extraireMots(tableauDesCommandes[i], " ");

                    if(strcmp(cmd[0],"pwd")== 0){
                        write(STDOUT_FILENO,pwd(),sizeof(pwd()));
                    }else{
                         execvp(cmd[0], cmd);
                    }
            
                    // En cas d'échec de l'exécution
                    perror("execvp");
                    exit(EXIT_FAILURE);

                }

            }
            if(i == n-1)
            {
               
                //dernier fils 
                //dup sortie  + dup err
                // Parcourir le tableau de mots pour trouver le symbole de redirection
                int cpt[2]; int k=0;
                char** mots = extraireMots(tableauDesCommandes[i]," ");

                for (int j = 0; mots[j] != NULL; j++) {

                    if (strcmp(mots[j], ">") == 0 || strcmp(mots[j], "2>") == 0)
                    {
                        cpt[k] = j;
                        k++;

                    }
                    
                 }
                
                if (k != 0) k--;

                if(k != 0){
               
                    
                

                char cmd[1024] = "";
                for (int j = 0; j < cpt[0]; j++) {
                    strcat(cmd, mots[j]);
                    strcat(cmd, " ");
                }
                 char * fichier = mots[cpt[0] + 1];

                 int fd[2];
                  for (int l = 0; l <= k; l++) {


                     if (strcmp(mots[cpt[l]], ">") == 0 || strcmp(mots[cpt[l]], "2>") == 0) {
                             fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_EXCL, 0664);
                    } 
                 }

                 // Redirection des flux
                for (int l = 0; l <= k; l++) {
                if ((strcmp(mots[cpt[l]], ">") == 0)) {
                if (dup2(fd[l], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
                } 
                 } else if (strcmp(mots[cpt[l]], "2>") == 0) {
                 if (dup2(fd[l], STDERR_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
                 }
                }
             }
               char **cmd1 = extraireMots(cmd, " ");
               
               
                
                  if(strcmp(cmd1[0],"pwd")== 0){
                        write(STDOUT_FILENO,pwd(),sizeof(pwd()));
                    }else{
                         execvp(cmd1[0], cmd1);
                    }
                // En cas d'échec de l'exécution
                perror("execvp");
                exit(EXIT_FAILURE);


             }else{
                
                 char **cmd = extraireMots(tableauDesCommandes[i], " ");
                  if(strcmp(cmd[0],"pwd")== 0){
                        write(STDOUT_FILENO,pwd(),sizeof(pwd()));
                    }else{
                          execvp(cmd[0], cmd);
                    }
               
                perror("execvp");
                exit(EXIT_FAILURE);

             }   
               
            }
            if( i == 0 )
            {
               
                
                int cpt[2]; int k=0;
                char** mots = extraireMots(tableauDesCommandes[i]," ");

                for (int j = 0;mots[j] != NULL; j++) {
                  
                    if (strcmp(mots[j], "<") == 0 || strcmp(mots[j], "2>") == 0)
                    {
                        
                        cpt[k] = j;
                        k++;

                    }
                   
                }
               
                
                if (k != 0) k--;
                
                if(k != 0){
                
               


                char cmd[1024] = "";
                for (int j = 0; j < cpt[0]; j++) {
                    strcat(cmd, mots[j]);
                    strcat(cmd, " ");
                }
                 char * fichier = mots[cpt[0] + 1];

                 int fd[2];
                  for (int l = 0; l <= k; l++) {
                     if (strcmp(mots[cpt[l]], "2>") == 0) {
                             fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_EXCL, 0664);
                    } else if (strcmp(mots[cpt[l]], "<") == 0){
                            fd[l] = open(mots[cpt[l]+1], O_RDONLY);
                    }
                 }

                 // Redirection des flux
                for (int l = 0; l <= k; l++) {
                if ((strcmp(mots[cpt[l]], "<") == 0)) {
                if (dup2(fd[l], STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
                } 
                 } else if (strcmp(mots[cpt[l]], "2>") == 0) {
                 if (dup2(fd[l], STDERR_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
                 }
                }
             }
              char **cmd1 = extraireMots(cmd, " ");
                execvp(cmd1[0], cmd1);
                // En cas d'échec de l'exécution
                perror("execvp");
                exit(EXIT_FAILURE);


             }else{
                
                 char **cmd = extraireMots(tableauDesCommandes[i], " ");
                   if(strcmp(cmd[0],"pwd")== 0){
                        write(STDOUT_FILENO,pwd(),sizeof(pwd()));
                    }else{
                         execvp(cmd[0], cmd);
                    }
               
                // En cas d'échec de l'exécution
                perror("execvp");
                exit(EXIT_FAILURE);

             } 
                
            }
            
        }
    }

    // Fermeture des descripteurs de tube dans le processus parent
    for (int i = 0; i < n - 1; i++) {
        close(tubes[i][0]);
        close(tubes[i][1]);
    }

    // Attente de la fin des processus fils
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    return 0;
}


int contientRedirection(const char *commande) {
    const char *redirections[] = { ">", ">>", "<", "2>", "2>>", "&>", "&>>", NULL };
    
    for (int i = 0; redirections[i] != NULL; ++i) {
        if (strstr(commande, redirections[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}


void afficherUnjobSansT(int id_job)
{

    UpdateJobs();
   
    char buffer[1024];
    
   
  
        if(jobs[id_job - 1].etat != KILLED && jobs[id_job - 1].est_surveille == 1)   //affichage des jobs surveillés et non killed
        {
        int length = snprintf(buffer, sizeof(buffer), "[%d] %d ", jobs[id_job - 1].job_id, jobs[id_job - 1].tableau_processus[0]);

        switch (jobs[id_job - 1].etat) {
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

        length += snprintf(buffer + length, sizeof(buffer) - length, "%s\n", jobs[id_job - 1].cmd);
        write(STDOUT_FILENO, buffer, length);

}
}



void afficherUnjobAvecT(int id_job)
{

    UpdateJobs();
   
    char buffer[1024];

    
    if(jobs[id_job - 1].etat != KILLED && jobs[id_job - 1].est_surveille == 1)   //affichage des jobs surveillés et non killed
    {

        //affichage des infos du job
        int length = snprintf(buffer, sizeof(buffer), "[%d] %d ", jobs[id_job - 1].job_id, jobs[id_job - 1].tableau_processus[0]);

        switch (jobs[id_job - 1].etat) {
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

        length += snprintf(buffer + length, sizeof(buffer) - length, "%s\n", jobs[id_job - 1].cmd);
        write(STDOUT_FILENO, buffer, length);
        

        
        //affichage des infos des processus
        //vider le buffer
        int r, status ; int length2;
        for (int i = 0 ; i < jobs[id_job - 1].nb_processus; i ++ )
        {
            length2 = snprintf(buffer, sizeof(buffer), "      |%d  ", jobs[id_job - 1].tableau_processus[i]);
            
            r= waitpid(jobs[id_job - 1].tableau_processus[i], &status, WNOHANG | WUNTRACED);
            
            if(r == 0)
            {
                length2 += snprintf(buffer + length2, sizeof(buffer) - length2, "Running\t");
            }
                
            else {
                if (WIFEXITED(status))
                {
                    length2 += snprintf(buffer + length2, sizeof(buffer) - length2, "Done\t");
                }
                else if (WIFSIGNALED(status))
                {
                    length2 += snprintf(buffer + length2, sizeof(buffer) - length2, "Killed\t");
                }
                else if (WIFSTOPPED(status))
                {
                    length2 += snprintf(buffer + length2, sizeof(buffer) - length2, "Stopped\t");
                }
             }
              
              length2 += snprintf(buffer + length2, sizeof(buffer) - length2, "%s\n", jobs[id_job - 1].cmd);
              write(STDOUT_FILENO, buffer, length2);
        
            


}
}
}


void afficherjobsAvecT()
{


    //maj des etats des jobs
    UpdateJobs();
   
    char buffer[1024];
   
    for (int i = 1; i <= nb_jobs; i++) {
        afficherUnjobAvecT(i);
    }
    
}


