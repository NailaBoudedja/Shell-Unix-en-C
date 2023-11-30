_Premier Jalon 
 Notre programme est organisé sous forme d'une bibliothèque bib_jsh.c qui contient les implémentations des différentes fonctions nécessaires pour le projet.

Elle contient des fonctions qui implémentent les commandes internes de notre shell [cd,pwd,exit et ?] en utilisant que des appels systèmes. Et une fonction dédiée à l'affichage du prompt, en formant la ligne du shell structuré comme suit : [0] + rep courrant + "$ .

Ensuiten, il y a la fonction executerCommande qui se charge de l'execution de la commande saisie; si une commande interne est entrée, elle fait appel à la fonction correspondante, sinon elle crée un processus fils qui l'exécute avec la fonction exec.

Le fichier jsh.c est le fichier main de notre programme, il gére l'affichage du prompt et la lecture de la commande saisie à chaque itéreation.COMPILATION ET EXECUTION:make