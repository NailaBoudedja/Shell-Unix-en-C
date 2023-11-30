#ifndef BIB_JSH_H
#define BIB_JSH_H

//définition de la structure du Prompt
struct Prompt { 
    int ret;  //pour stocker la valeur de retour
    char* oldPath;  //pour stocker le chemin du dernier rep du travail
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

#endif