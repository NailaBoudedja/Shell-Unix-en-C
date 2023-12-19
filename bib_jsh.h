// jsh_bib.h
#ifndef BIB_JSH_H
#define BIB_JSH_H


extern char* currentDir1; // stock le rep courant
extern char* oldpath; // stock l'ancien rep courant


//définition de la structure du Prompt
struct Prompt { 
    int ret;  //pour stocker la valeur de retour
};

extern struct Prompt jsh;  //déclaration d'une variable externe du shell jsh 


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
char **extraireMots2(char *ligne);
int redirEntre(char *cmd, char *file);
int redirSortie (char *cmd , char * file, int r);
int redirErreur(char *cmd , char *file, int r);
int executerRedirection(char *commande);


#endif