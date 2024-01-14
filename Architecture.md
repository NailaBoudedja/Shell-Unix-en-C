Structures : 

struct Prompt:
Contient un seul champ, ret, qui stocke la valeur de retour du shell. Cette valeur va être utilisée pour indiquer le succès ou l'échec des commandes exécutées.

enum JobEtat:
Une énumération qui définit différents états possibles pour un job : RUNNING, STOPPED, DETACHED, KILLED, DONE. Ces états permettent de suivre l'état d'exécution des processus et des jobs dans le shell.

struct Job:
Représente un job dans le shell. Chaque job peut contenir plusieurs processus.
Champs:
job_id: Identifiant unique du job.
etat: L'état actuel du job (basé sur JobEtat).
cmd: La commande associée au job.
tableau_processus: Un tableau stockant les PID des processus appartenant au job.
nb_processus: Le nombre de processus dans le job.
est_surveille: Indique si le job est surveillé (pour la gestion des jobs en arrière-plan).
a_afficher: Un drapeau pour déterminer si le job doit être affiché dans la liste des jobs.
Fonctions et leur rôle

Fonctions de gestion des répertoires:
pwd(): Retourne le répertoire courant. 

cd(ref): Change le répertoire courant.
Peut être appelée par executerCommandeGeneral lors de l'exécution de la commande cd.

Fonctions de gestion des commandes et des jobs:
executerCommandeGeneral(commande): Exécute une commande générale. Selon la nature de la commande, elle peut appeler d'autres fonctions comme executerCommande, pipeline,substitution ou des fonctions spécifiques comme cd.
pipeline(commande): Gère l'exécution des commandes avec des pipelines (utilisation de |). Découpe la commande en sous-commandes et les exécute séquentiellement en reliant leurs entrées/sorties.

executerCommande(commande): Exécute une commande simple sans pipeline.

executerCmdArrierePlan(commande): Exécute une commande en arrière-plan. Peut être appelée par executerCommande si la commande se termine par &.

creerJob(commande, tableau_des_processus): Crée un nouveau job et l'ajoute à la liste des jobs. Utilisée lors de l'exécution de commandes en arrière-plan.
Jobs(): Affiche la liste des jobs actifs.

UpdateJobs(): Met à jour l'état des jobs. Peut être appelée régulièrement pour surveiller l'état des processus.
Fonctions d'assistance et utilitaires:

extraireMots(phrase, delimiteur): Découpe une chaîne de caractères en mots basés sur un délimiteur. Utile pour analyser les commandes.

afficherJsh(): Génère et retourne le prompt du shell.

ignoreSignals(), restoreSignals(): Gèrent les signaux du système d'exploitation, les ignorants ou les restaurant selon le besoin.

tronkString(str, size), detecterNumJob(chaine): Fonctions utilitaires pour manipuler des chaînes de caractères.

retCmd(): Retourne la dernière valeur de retour stockée dans jsh.ret.
Fonctions de gestion des signaux et des processus:

Kill(commande), killJob(signal, id_job), killProcessus(signal, pid): Gèrent l'envoi de signaux aux processus ou aux jobs, permettant de les arrêter, de les reprendre, etc.
Architecture globale et interaction
des fonctions

Le programme commence dans main(), où le shell est initialisé (gestion des signaux, initialisation des variables globales, etc.).
Dans la boucle principale, main() lit les commandes entrées par l'utilisateur.
Pour chaque commande, main() vérifie si c'est un pipeline (avec strstr) et appelle pipeline(commande) ou executerCommandeGeneral(commande) en fonction.

executerCommandeGeneral(commande) est la fonction centrale pour traiter les commandes. Elle peut appeler d'autres fonctions comme executerCommande,executeCmdAvecSubstitution, executerCmdArrierePlan, ou des commandes spécifiques comme cd.

Les commandes peuvent entraîner la création de jobs (processus en arrière-plan) qui sont gérés via la structure Job et les fonctions associées (creerJob, Jobs, UpdateJobs).

Les fonctions d'assistance et utilitaires (extraireMots, afficherJsh, etc.) sont utilisées tout au long pour diverses opérations, comme l'analyse de commandes ou la gestion de l'affichage.
Les fonctions de gestion des signaux (ignoreSignals, restoreSignals) et des processus (Kill, killJob, killProcessus) sont utilisées pour gérer l'interaction avec les processus du système d'exploitation, en envoyant des signaux ou en modifiant l'état des jobs.
