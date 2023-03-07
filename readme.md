--- README PROJET SYTEME / RESEAU ---

Auteur : Théo Gayant / Duc Viet NGUYEN


> SYSTEME :

	myls (commande externe) --> (réalisé par Duc Viet NGUYEN 100%)
		Compilation : 
		$ gcc myls.c -o myls

		Execution :
		$ ./myls ou $ ./myls -aR ou $ ./myls -a ou $ ./myls -a ou $ ./myls -a -R ou $ ./myls -Ra ou 
		$ ./myls / ou $ ./myls -a /

	myps --> (réalisé par Duc Viet NGUYEN 100%)
		- Difficulté rencontré pour récupérer l'argument TIME et START.
		- On affiche chaque processus qui est dans /proc, avec une couleur différente selon l'état du processus. Et comme ci on avait tapé la commande : ps -aux
	
		Compilation : 
		$ gcc myps.c -o myps -lm
		
		Execution :
		$ ./myps
	
	mysh --> (réalisé par Théo 100%)
		- Mini-shell qui contient les fonctionnalités demandé dans le sujet pour le mysh.
		- On peut mettre autant de ';' pour lancer plusieurs commandes sur une ligne.
		- On peut mettre autant de '&&' et de '||' entre les commandes. (ex : ls *.c && ls *.h || ls ????; ls ???; exit)
		- Gère les wildcards.
		- On ne peut utiliser qu'UN SEUL pipe par commande
		- Commande cd intégré
		- Commande status intégré pour afficher le retour du dernier processus éxécuté.($>status)
		- Commande myjobs pour affiché tout les processus en arrière plan
		- Redirection vers ou depuis un fichier.
		- Gère les espaces entre les caractères de séquencement et de redirection (ex: ls *.c&&ls;ls ???? ==> ls *.c && ls ; ls ????)
		- Gestion des variables /!\ il faut bient que le nom de la variable et la valeur affecté soient collé au '=' /!\ (ex: set a=foo ==> echo salut $a ==> salut foo)
		
		Fonctionnement :
		Le programme va d'abord découpé la commande entré, il va la séparé en plusieurs commande si il y a des ';'. Ensuite pour chaque commande si il y a des '&&' ou '||' on va d'abord lancer la première partie de la commande et éxécuté la seconde selon le résultats de la première. Chaque commande est transformé sous une liste d'argument, on y stock tout les mots de la commande sans les espaces (ex: >ls *.c ???? ==> [ls,*.c,????]) ce qui permettra de vérifié si le nombre d'argument pour une commande interne est suffisant, ou pour lancé une commande avec exec... Pour utiliser les commandes externes il suffit de taper '>./myls' ou '>./myps' 
		
		Compilation : 
		$ gcc mysh -o mysh
		
		Execution
		$ mysh
	
	
	Bug connu :
	Quand l'on fait un CTRL+C sans avoir lancé de commande et que l'on répond négativement pour quitter le shell, il faut appuyer 2 fois sur entrer car il y a une erreur avec le buffer que nous avons pas réussi à résoudre. Si vous tapé une commande, il y aura une erreur.
	
	Non implémenté : 
	Le signal du CTRL+Z : on a essayé de l'implémenté (code de la fonction toujours disponible dans le fichier mysh.c) mais le processus ne se mettait pas en pause, on était bloqué sur la fonction wait (en attente de la fin du processus), alors que le signal était détecté.
	
	Il est préférable d'utiliser mysh directement si vous voulez pleinement exploiter nos fonctionnalité implémenté dans celui ci, nous n'avons pas forcément réussi à l'adapter à une utilisation en réseau.

> RESEAU :

	Authentification (réalisé par Duc Viet NGUYEN) :
	Difficultés rencontrés :
	- Multiclients
	- Redemander le nom et le mot de passe s'ils sont incorrects, donc si l'un d'entr'eux est incorrect, on sort carrément de la connexion.

	Nous avons mis en place un administrateur (nom:admin & mdp:admin), c'est lui seul qui peut ajouter les autres utilisateurs, et peut quitter la connexion avec la commande (exit). L'administrateur ne peut pas faire les autres commandes shell, cela est reservé seulement aux autres utilisateurs. Les autres utilisateurs peuvent aussi quitter la connexion avec la commande 'exit'.

	Pour tester : 
	nom d'utilisateur : viet
	mdp d'utilisateur : viet

		
	Envoie des commandes via Sockets (Viet & Théo) :
	Une fois connecté l'utilisateur tape la commande qu'il souhaite éxécuter sur son terminal, celle ci est récupéré est envoyé avec la socket au serveur, le serveur récupère la socket et envoie la commande au mysh. Tout ce qui doit être affiché au client est stocké dans un fichier, une fois que la commande est executé, on récupère tout ce qu'il y a à afficher au client, et l'envoie dans la socket destiné au client.

	Pour exécuter la partie reseau, il faut ouvrir deux terminals :

	1er terminal pour serveur :
	$ gcc serveur.c -o serveur -lcrypt
	$ ./serveur

	2e terminal pour client :
	$ gcc client.c -o client -lcrypt
	$ ./client

	L'adresse ip attendu est soit 0 soit 0.0.0.0 soit 127.0.0.1 comme on est localhost
	Une fois l'adresse ip renseignée, il faut taper le nom de l'utilisateur puis le mot de passe.
	
/!\ Nous avons eu beaucoup de difficultés à gérer la partie réseau du projet, nous avons fait notre possible pour faire quelques choses de fonctionnel /!\


/!\ JOYEUX ANNIVERSAIRE MR. THIBAULT FALQUE
ET BONNE ANNÉE 2021 ;-) /!\
