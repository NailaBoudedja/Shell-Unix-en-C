// jsh_bib.h
#ifndef JSH_BIB_H
#define JSH_BIB_H


//Définition de la structure Prompt
struct Prompt {
    int newret;
    int oldret;
     
};

extern struct Prompt jsh;  //Déclaration de la variable globale jsh shell 




char *pwd();
int exitCommande (int n);
int cd(char* newRep);
int retCommande();
int stringLength(char *chaine);
char **stringToWords(char *input);
char *tronkString(const char *str, int size);
char *afficherJsh();
int executerCommande(char* commande);

#endif
