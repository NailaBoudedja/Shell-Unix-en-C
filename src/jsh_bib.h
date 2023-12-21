// jsh_bib.h
#ifndef JSH_BIB_H
#define JSH_BIB_H



//Définition de la structure Prompt
struct Prompt {
    struct Jobs { //structure de jobs
        char* nb_jobs;
        char *jobs_color; 
    } jobs;
    struct CurrentDir {  //structure du repertoir courrant
        char *currentDir;
        char *dir_color; 
    } currentDir;
    struct Dollar {   //structure de  "$ "
        char *val;
        char *dollar_color;
    } dollar;
};
extern int stop;
extern struct Prompt jsh;  //Déclaration de la variable globale jsh shell 



char* getCurrentDirectory(); //a supprimer apres (car elle est implementée par des fonctions standards) 
int stringLength(char *chaine);  //renvoie la taille d'une chaine de caracteres
void initializeJsh(); //initialiser les champs du prompt jsh
void afficherJsh();  //afficher le prompt 
char **stringToWords(char *input); //convertir une chaine de caracteres en un tableau de mots
void executerCommand(char *command);  //executer une commande
int taille_chaine(char *chaine);
#endif
