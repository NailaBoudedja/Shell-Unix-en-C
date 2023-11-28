#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "jsh_bib.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <linux/limits.h>

#define MAX_PROMPT_SIZE 30
#define NORMAL_COLOR "\033[34m"







int main()
{
    initializeJsh();
    stop = 0;
    while (stop != 1)
    {
        // Initialiser Readline
        rl_initialize();

        // Afficher le prompt pour tester
        afficherJsh();

        // Lire la ligne de commande avec Readline
        char *input = readline(">");

        if (input && *input)
        {
            add_history(input);
            
        }

        // Traiter la ligne de commande
        executerCommand(input);

        // Libérer la mémoire allouée par Readline
        free(input);

      
    }
    return 0;
}
