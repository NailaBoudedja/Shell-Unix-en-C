// jsh_bib.h
#ifndef BIBV2_H
#define BIBV2_H

//Définition de la structure Prompt
struct Prompt {
    int newret;   
    int oldret;
    char* oldPath;
     

};

extern struct Prompt jsh;  //Déclaration de la variable globale jsh shell 

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