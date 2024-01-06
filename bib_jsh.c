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
    int signals[] = {SIGINT,SIGTERM, SIGTTIN, SIGQUIT, SIGTTOU, SIGTSTP}; 
    
    struct sigaction sig_action;
    sig_action.sa_handler = SIG_IGN; 
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    for (int i = 0; i < 6; ++i) {
        if (sigaction(signals[i], &sig_action, NULL) == -1) {
            perror("erreur lors de l'ignorance des signaux \n");
            exit(EXIT_FAILURE);
        }
    }
}

//fonction pour reprendre le traitement par défaut des signaux
void restoreSignals() {

    int signals[] = {SIGINT,SIGTERM, SIGTTIN, SIGQUIT, SIGTTOU, SIGTSTP};   
    
    struct sigaction sig_action;
    sig_action.sa_handler = SIG_DFL;   //action par défaut
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    for (int i = 0; i < 6 ; ++i) {
        if (sigaction(signals[i], &sig_action, NULL) == -1) {
            perror("erreur lors de l'ignorance des signaux \n");
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
void detecterNumJob(char *chaine) {
    if (chaine != NULL && chaine[0] == '%') {
        int len = strlen(chaine);
        for (int i = 0; i < len; ++i) {
            chaine[i] = chaine[i + 1];   // decalage des positions
        }
    }
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
          detecterNumJob(cmd[1]);
          int id_job = atoi(cmd[1]);
          for (int i = 0; cmd[i] != NULL; i++)
            {
                free(cmd[i]);
            }
          free(cmd);
          return  killJob(NULL,id_job-1);
        }
        else {
            //kill sig %job
            detecterNumJob(cmd[2]);
            int id_job = atoi(cmd[2]);
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





int lancerProcessus(int in, int out, char **cmd) {
    pid_t pid;

    if ((pid = fork()) == 0) {
        if (in != 0) { //Si in n'est pas 0, elle redirige l'entrée standard du processus depuis in
            dup2(in, 0);
            close(in);
        }

        if (out != 1) { //i out n'est pas 1, elle redirige la sortie standard du processus vers out.
             dup2(out, 1);
            close(out);
        }
      
        if (execvp(cmd[0], cmd) < 0) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    } else {
        return 1;
    }
}


int executeCmdAvecPipe(char ***cmds, int nbCmds) { // liste de commandes et le nbr de cmmandes
    int i;
    int in = 0;
    int fd[2];

    for (i = 0; i < nbCmds - 1; ++i) { // pour chaque cmd on cree un tube et lance un processus pour executer cette cmd à l'exception de la derniere
        pipe(fd);
         if (lancerProcessus(in, fd[1], cmds[i]) != 0) {//in est l'extrémité de lecture du tube précédent, ecrit dans fd[1]
           return 1;
        }
        close(fd[1]);//fd[1]qui est l'extrémité d'écriture du tube actuel
        in = fd[0];//Pour la dernière commande, elle redirige  son entrée standard depuis in et l'exécute
    }

    if (in != 0){
        dup2(in, 0);
        close(in); 
    }
  
        return lancerProcessus(in, 1, cmds[i]);

}



int executerCommandeGeneral(char *commande) {

// Extraire les mots de la commande
char ** mots = extraireMots(commande, " ");

////deebeugggggggggggggggggggggggg
int i = 0;
while(mots[i] != NULL) {
    printf("%s\n", mots[i]);
    i++;
}
////deebeugggggggggggggggggggggggg

 int pipe_found = 0;
 int nbPipes = 0;
    for (int i = 0; mots[i] != NULL; i++) {
        if (strcmp(mots[i], "|") == 0) {
            nbPipes++;
            pipe_found = 1;

        }
    }
     // Si le caractère pipe est trouvé, exécuter les commandes avec pipe
    if (pipe_found) {
    // Tableau pour stocker les commandes entre les pipes
    char ***cmds = (char ***)malloc((nbPipes + 1) * sizeof(char **));
    if (cmds == NULL) {
        perror("Erreur d'allocation de mémoire");
        exit(EXIT_FAILURE);
    }

    int cmdIndex = 0;
    int startIndex = 0;

 // vu q'on modifie le tab de mots enmettant certains el à null, donc on fait une copie avant de le modiifer car i pointera plus sur le mot complet
int nbMots = 0;
while (mots[nbMots] != NULL) nbMots++;
char ** mots_copy = malloc((nbMots + 1) * sizeof(char *));
for (int i = 0; i < nbMots; i++) {
    mots_copy[i] = strdup(mots[i]);
}
mots_copy[nbMots] = NULL;
       // Découper la commande en parties entre les pipes
    for (int i = 0; mots_copy[i] != NULL; i++) {
        if (strcmp(mots_copy[i], "|") == 0) {
            mots_copy[i] = NULL; // pour terminer la sous commande
            cmds[cmdIndex] = &mots_copy[startIndex]; 
            cmdIndex++;
            startIndex = i + 1;
        }
    }
    // Dernière commande après le dernier pipe
    cmds[cmdIndex] = &mots_copy[startIndex];
    cmds[cmdIndex + 1] = NULL;
// Afficher le contenu de cmds debeuhggggggggggg
    for (int i = 0; i <= nbPipes; i++) {
        printf("Commande %d :\n", i + 1);
        for (int j = 0; cmds[i][j] != NULL; j++) {
            printf("  Argument %d : %s\n", j + 1, cmds[i][j]);
        }
        printf("\n");}
     ////// pour debeugggggggggg

        // Exécuter les commandes avec les pipes
    executeCmdAvecPipe(cmds, nbPipes + 1);
 // Libérer la mémoire
    for (int i = 0; i <= nbPipes; i++) {
        free(cmds[i]);
        
        }
    free(cmds);
    //free(mots);
    free(mots_copy);

    }

       // return 0;  // Indiquer que la commande avec pipeline a été exécutée
    
// Parcourir le tableau de mots pour trouver le symbole de redirection
int k = 0;
int cpt [3];
int result ;
int estExterne = 1; // pour tester si on a une cmd avec ou sans redirection 
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
int result = executerCmdGlobal(commande);
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
fd[l] = open(mots[cpt[l]+1], O_WRONLY | O_CREAT | O_EXCL, 0664);
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
if (dup2(fd[l], STDOUT_FILENO) == -1) {
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


result = executerCmdGlobal(cmd);
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
goto cleanup; 
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
