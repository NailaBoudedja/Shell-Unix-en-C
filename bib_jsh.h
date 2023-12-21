// jsh_bib.h

#ifndef BIB_JSH_H
#define BIB_JSH_H

#include <signal.h>
#include <sys/types.h>
#include <limits.h>



#define MAX_JOBS 100
#define MAX_PROCESSUS 100


extern char* currentDir1; // stock le rep courant
extern char* oldpath; // stock l'ancien rep courant
extern int nb_jobs ;  
extern char * tmpExtraire;
//extern int jobs_surv[]; //les jobs a afficher avant chaque affichage du prompt sur err
//extern int nb_jobssurv; ; 
//définition de la structure du Prompt
struct Prompt { 
    int ret;  //pour stocker la valeur de retour
};


extern struct Prompt jsh;  //déclaration d'une variable externe du shell jsh 



//définition d'un type enum pour l'etat de job
typedef enum {
    RUNNING,   // Job en cours d'exécution
    STOPPED,   // Job suspendu
    DETACHED,  // Job détaché
    KILLED,    // Job tué (au moins un processus terminé suite à la réception d'un signal)
    DONE       // Job terminé (tous les processus terminés correctement avec exit)
} JobEtat;

//définition de la structure d'un job
typedef struct{
    int job_id;
    JobEtat etat;
    char* cmd;
    pid_t tableau_processus[MAX_PROCESSUS];  //stocker les processus d'un job //le premier processus cad tab[0] est le representant de ce job cad son pid et le group pid des autres
    int nb_processus;
    int est_surveille; 
    int a_afficher ;
}Job;


extern Job jobs[MAX_JOBS];


char *pwd();
int exitAvecArgument (int n);
int exitSansArgument();
int exitCmd(char * val);
int retCmd();
int isReferenceValid(char *ref);
int cd( char * ref);
char *tronkString(const char *str, int size);
char *afficherJsh();
char **extraireMots(char *phrase, char *delimiteur);
int executerCommande(char * commande);
int executerCmdArrierePlan(char* commande);
void creerJob(char* commande, pid_t tableau_des_processus[]);
void Jobs();
int executerCmdGlobal(char * commande);
int Kill(char * commande);
int killJob(char * signal,int id_job);
int killProcessus(char * signal, pid_t pid);
void UpdateJobs();
void detecterNumJob(char *chaine);
void affichageJobsModifies();
void ignoreSignals();
void restoreSignals();


#endif