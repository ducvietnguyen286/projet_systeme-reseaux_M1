/*
PROJET SYSTEME-RESEAU 2020
Théo Gayant - Olivier Buhendwa
Master 1 Informatique 
Shell du projet
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glob.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>


#define ERR -1
#define FAILEDEXEC 	127

#define syerror(x)		perror(errormsg[x])
#define fatalsyerror(x)	syerror(x),exit(x)
#define nouveau(type) (type*)malloc(sizeof(type));

#define ROUGE(m)	"\033[01;31m"m"\033[0m"
#define VERT(m)		"\033[01;32m"m"\033[0m"
#define JAUNE(m)	"\033[1;33m"m"\033[0m"
#define BLEU(m)		"\033[1;34m"m"\033[0m"
#define VIOLET(m)	"\033[1;35m"m"\033[0m"
#define CYAN(m)		"\033[1;36m"m"\033[0m"
#define GRIS(m)		"\033[1;37m"m"\033[0m"

#define TAILLE_BUFFER 4096
#define TAILLE_LISTE_ARGU 1024
#define TAILLE 1024

char * errormsg[]= {
	"No error", //0
	ROUGE("Impossible to fork process"), //1
	ROUGE("Exec failed"),//2
	ROUGE("Open file failed"),//3
	"Too many arguments",//4
	"Aucun dossier de ce type",//5
	ROUGE("SIGKILL error"),//6
	ROUGE("background exec failed"),//7
	ROUGE("SIGSTOP error"),//8
	ROUGE("Need one argument")
};


//VARIABLE
typedef struct variable
{
	char nomVar[TAILLE_BUFFER];
	char valeurVar[TAILLE_BUFFER];
	struct variable * suivant;
} variable;

typedef variable * listeVariable;
//VARIABLE

//PROCESSUS BACKGROUND
typedef struct process {
	char cmd[TAILLE][TAILLE];
	int n;
	pid_t pid;
	int jobs;
	int en_cours;
	struct process *suiv;
} processus;

typedef processus* listeProcessus;
//PROCESSUS BACKGROUND

//VARIABLE GLOBAL
char buf[TAILLE_BUFFER], chemin[TAILLE_BUFFER];
int dernier_proc = 0, code_retour = -1, jobs = 1;
listeVariable lv = NULL;
listeProcessus l;
pid_t proc_back;//Processus en background
pid_t proc_fore;//Processus en foreground
int foreground = 0;//Indique si la commande est lancé en foreground
int ignore = 0;
int erreurCTRLZ = 0;
//VARIABLE GLOBAL

/*
	Permet de créer/modifier une variable
*/
void setVar(char * nomVar, char * valeurVar) {
	variable * v, * v_pere;
	if (lv == NULL) {//Si la liste est vide
		v = malloc(sizeof(variable));//On creer une variable
		strcpy(v->nomVar,nomVar);
		strcpy(v->valeurVar,valeurVar);
		v->suivant = NULL;
		lv = v;//On l'ajoute a la tete de liste
		//printf("VIDE\n");
	} else {//Si pas vide, on verifie si elle n'est pas deja creer
		v = lv;
		int new = 1;
		while(v != NULL) {
			if(strcmp(nomVar,v->nomVar) == 0) {//Si deja creer on la modifie juste
				strcpy(v->valeurVar,valeurVar);
				new = 0;
				break;
			}
			v_pere = v;
			v = v->suivant;
		}
		if (new) {//Sinon on la créer
			v = malloc(sizeof(variable));
			strcpy(v->nomVar,nomVar);
			strcpy(v->valeurVar,valeurVar);
			v->suivant = NULL;
			v_pere->suivant = v;
		}
	}
	v = lv;/*
	while(v != NULL) {
		printf("$%s ==> '%s'\n",v->nomVar,v->valeurVar);
		v = v->suivant;
	}*/
}

/*
	Permet de supprimer une variable
*/
void unsetVar(char * nomVar) {
	if (lv == NULL) return;//Si liste vide on sort
	variable *v, * v_pere;
	v_pere = NULL;
	v = lv;
	while (v != NULL) {//Sinon on parcourt la liste et chercher la variable
		if (strcmp(v->nomVar,nomVar) == 0) {
			if (v_pere == NULL) {//Si on la trouve on doit la supprimer
				lv = v->suivant;
			}
			else {
				v_pere->suivant = v->suivant;
			}
			break;
		}
		v_pere = v;
		v = v->suivant;
	}
	//Si elle n'existe pas on fait rien
	free(v);
	v = lv;/*
	while(v != NULL) {
		printf("$%s ==> '%s'\n",v->nomVar,v->valeurVar);
		v = v->suivant;
	}*/
}

/*
	Chercher une variable local
*/
char * findValeur(char *nomVar) {
	char * valeurVar = (char*) malloc(sizeof(char)*100);
	valeurVar[0] = '\0';
	if (lv == NULL) return valeurVar;

	variable *v;
	v = lv;
	while (v != NULL) {//Sinon on parcourt la liste et chercher la variable
		if (strcmp(v->nomVar,nomVar) == 0) {
			strcpy(valeurVar,v->valeurVar);
			break;
		}
		v = v->suivant;
	}
	return valeurVar;
}

/*
	Initialiser la liste des processus en background
*/
listeProcessus genererListe() {
	//On crée la nouvelle liste
	listeProcessus l = nouveau(processus);
	l = NULL;
	return l;
}

/*
	Ajouter un processus a la liste des processus en background
*/
void ajouterProcessus(char* tabcmd[TAILLE], pid_t pid, int en_cours) {
	listeProcessus new = nouveau(processus);//On crée un nouveau processus
	listeProcessus courant = l; 
	char **ps;
	
	//On crée le nouveau noeud de la chaine
	new->n = 0;
	for (ps = tabcmd; *ps; ps++) {
		strcpy(new->cmd[(new->n)++], *ps);
	}
	new->pid = pid;
	new->jobs = jobs++;
	new->en_cours = en_cours;
	new->suiv = NULL;
	
	//On ajoute le nouveau processus à la liste
	if (courant) {
		//On se place a la fin de la liste
		while (courant->suiv) {
			courant = courant->suiv;
		}
		courant->suiv = new; //On ajoute le nouveau processus
	} else {
		l = new; //Si liste vide on l'ajoute en tete de liste
	}
}

/*
	Rechercher un processus dans la liste des processus en background
*/
listeProcessus rechercheProcessus(int pid) {
	listeProcessus courant = l;
	listeProcessus trouve = NULL;
	
	//On parcourt la liste des processus
	while (courant) {
	
		//Si on a trouvé le processus 
		if (pid == courant->pid) {
			trouve = courant;
			break;
		}
		courant = courant->suiv;
	}
	return trouve;
}

/*
	Vider la liste des processus en les tuants
*/
void viderListeProcessus() {
	listeProcessus courant = l;
	
	//On détruit les noeuds de la liste des processus
	while (courant) {
		
		//On arrête le processus en le tuant
		if (kill(courant->pid, SIGKILL) == ERR) {
			fatalsyerror(6);
		}
		
		waitpid(courant->pid, NULL, WNOHANG);
		
		//On enlève le processus
		l = l->suiv;
		free(courant);
		courant = l;
	}
}

/*
	Affiche l'état de tous les processus en background
*/
void myjobs() {
	listeProcessus courant = l;
	int i;
	
	//On parcourt la liste des processus en background
	while (courant != NULL) {
		printf("[%d] %d ", courant->jobs, courant->pid);
		
		//On affiche l'état du processus
		if (courant->en_cours) {//Si en cours
			printf("En cours d'exécution  ");
		} else {//Sinon stop
			printf("Stoppé  ");
		}
		
		//On affiche la commande qui a lancé le processus
		for (i = 0; i < courant->n; i++) {
			printf("%s", courant->cmd[i]);
		}
		printf("\n");
		courant = courant->suiv;
	}
	exit(0);
}

//SIGNAL
void rien() {
	return;
}

/*
	Vider le buffe stdin
*/
void viderBuffer()
{
    int c = 0;
    while (c != '\n' && c != EOF)
    {
        c = getchar();
        printf("c : '%c'\n",c);
    }
}

/*Création de la liste des arguments, on récupère chaque arguments un à un*/
char * liste_argu[TAILLE_LISTE_ARGU];
void creer_liste_argu(char * buffer) {
	char *buf_copie = strdup(buffer);//Copie du buffer
	char *arg = strtok(buf_copie," ");//On récupère les arguments un par un.
	int i = 0;
	while(arg != NULL) {
		liste_argu[i] = strdup(arg);
		arg = strtok(NULL," ");//on passe a l'argument suivant
		i++;
	}
	while (i < TAILLE_LISTE_ARGU) {//On met le reste a NULL
		liste_argu[i] = NULL;
		i++;
	}
	free(buf_copie);
}

/*
	Signal du controle C
*/
void controleC() {
	int reponse;
	ignore = 1;
	//printf("foreground : %d\n",foreground);
	
	//Un processus était présent en premier plan 
	if (foreground) {
		//printf("Proc _ fore : %d\n",proc_fore);
		if (proc_fore) {
			
			//On arrête ce processus
			if (kill(proc_fore, SIGKILL) == ERR) {
				fatalsyerror(6);
			}
			wait(NULL);
			puts("\nProcessus arrêté");
			ignore = 0;
			code_retour = -1;
		} 
	} else {//Sinon si aucun processus en foregroud
		
		//On demande une confirmation de l'utilisateur
		printf("\nQuitter le terminal (0 : non, 1 : oui) : ");
		do {
			while(!scanf("%d", &reponse)) scanf("%*[^\n]");
		} while (reponse != 0 && reponse != 1);
		if (reponse) {
			
			//On vide la liste des processus en arrière plan
			viderListeProcessus();
			exit(0);
		}
		printf(ROUGE("ERREUR non résolu après un controle C --> Appuyer 2 fois sur ENTRER\n"));
	}
}

/*
	Signal du controle Z
*/
void controleZ() {
	char *tab[TAILLE], **ps;
	int i;
	
	//Si un processus est présent au premier plan
	if (foreground) {
		if (proc_fore) {
			printf("proc fore : %d\n",proc_fore);
			//On arrête ce processus
			if (kill(proc_fore, SIGSTOP) == ERR) {
				fatalsyerror(8);
			}
			
			ajouterProcessus(liste_argu, proc_fore, 0);
			printf("\n[%d] Stoppé ", jobs - 1 );
			for (ps = liste_argu; *ps; ps++) printf("%s ", *ps);
			printf("\n");
			erreurCTRLZ = 1;
		}
	}
}
//SIGNAL

/*
	Affiche le status du dernier processus execute en premier plan
*/
void status(void) {
	//Si aucun processus n'a été lancé en premier plan
	if (dernier_proc == 0) {
		printf("Aucun processus n'a été lancé en premier plan pour l'instant\n");
		return;
	}
	//Si le processus s'est terminé normalement
	if (code_retour != -1) {
		printf("%d terminé avec comme code de retour %d\n",dernier_proc,code_retour);
	} 
	else {
		printf("%d terminé anormalement\n",dernier_proc);
	}
}


/*Sert a récupérer la commande entrée sous forme de chaine*/
void recupere_buffer() {
	fgets(buf,TAILLE_BUFFER-1,stdin);/*récupération commande*/
	//printf("stdin : '%s'\n",buf);
	if (strlen(buf) > 0) {
		buf[strlen(buf)-1] = '\0';//on remplace le \n par \0
	} else {
		buf[0] = '\0';
	}
}

/* 
	Retourne la taille de la liste_argu
*/
int taille_liste_argu(void) {
	int i = 0;
	while(liste_argu[i] != NULL) {
		i++;
	}
	return i;
}

/*
	Permet la gestion des variables set/unset
*/
void gestion_variable(void) {
	if (liste_argu[1] == NULL) {
		fatalsyerror(9);
	}
	if (liste_argu[2] != NULL) {
		syerror(4);
		exit(4);
	}
	char var[TAILLE_BUFFER];
	strcpy(var,liste_argu[1]);
	if (strcmp(liste_argu[0],"set") == 0) {//SET
		char * nomVar;
		char * valeurVar;

		nomVar = strtok(var,"=");
		valeurVar = strtok(NULL,"=");
		setVar(nomVar,valeurVar);
	}
	else {//UNSET
		unsetVar(var);
	}
}

/*
	Commande interne cd
*/
void mycd(void) {
	char new_chemin[TAILLE_BUFFER];//Le nouveau chemin où l'on va de deplacer
	strcpy(new_chemin,chemin);

	if (liste_argu[2] != NULL) {//Il ne faut qu'un seul argument
		printf("argu en trop : '%s'\n",liste_argu[2]);
		fatalsyerror(4);
	}

	
	char * un_chemin = strtok(liste_argu[1],"/");
	while (un_chemin != NULL) {
		if (strcmp(un_chemin,".") == 0) {
		} 
		else if (strcmp(un_chemin,"..") == 0) {//On va dans le dossier parent
			int l = strlen(new_chemin);
			while(new_chemin[l] != '/') {
				new_chemin[l--] = '\0';
			}
			new_chemin[l] = '\0';
		} else {//On ajoute ce chemin
			strcat(new_chemin,"/");
			strcat(new_chemin,un_chemin);
		}
		un_chemin = strtok(NULL,"/");
	}

	//printf("===> '%s'\n",new_chemin);
	//On change de répertoire
	if (chdir(new_chemin) == ERR) {
		fatalsyerror(5);
	}
	strcpy(chemin,new_chemin);
	return;
}

/*Modifie la liste et remplace les arguments joker si il y en a*/
void remplace_joker() {
	char * liste_argu_copie[TAILLE_LISTE_ARGU];//On crée une liste temporaire;
	char * arg, *joker, *joker2, *joker3, *joker3bis;
	int i = 0,j = 0, isJoker;
	
	while(liste_argu[i] != NULL) {//On regarde parmis nos arguments si il possède un caractère joker
		arg = liste_argu[i];
		joker = strstr(arg,"*");//on verifie si on croise un '*'
		joker2 = strstr(arg,"?");//on verifie si on croise un '?'
		joker3 = strstr(arg,"[");
		joker3bis = strstr(arg,"]");
		isJoker = (joker != NULL) || (joker2 != NULL) || (joker3 && joker3bis);
		
		if (!isJoker) {//Si il n'y a pas de caractère joker
			liste_argu_copie[j] = arg;
			j++;
		}
		else {//Si il y a un caractère joker
			glob_t globbuf;//Va permet de remplacer le car joker
			int res = glob(arg,0,NULL,&globbuf);//Recherche tous les chemins correspondant au motif 
			if (!res) {//Si il reussi 
				int k;
				for(k=0; k < globbuf.gl_pathc; k++) {
					liste_argu_copie[j]=strdup(globbuf.gl_pathv[k]);
					j++;
				}
				free(liste_argu[i]);//On le free car on ne l'utilisera plus il a été remplacé 
			} else {
				liste_argu_copie[j]=arg;
				j++;
			}
			globfree(&globbuf);
		}
		i++;//On passe a l'argument suivant
	}
	liste_argu_copie[j] = NULL;
	i = 0;
	while (liste_argu_copie[i] != NULL) {//On recopie tous les arguments obtenue dans la vrai liste d'argument
		liste_argu[i] = liste_argu_copie[i];
		i++;
	}
	liste_argu[i] = NULL;
}

void affiche_liste_argu() {
	int i = 0;
	while (liste_argu[i] != NULL) printf("%s\n",liste_argu[i]),i++;
}

int nb_occurence(char * buffer, char * ch) {
	int cpt = 0;//Nombre d'occurence de la chaine ch
	long int len_ch = strlen(ch);//Longueur de la chaine ch

	char * p = buffer;
	char * uneOccu = strstr(p,ch);//On recherche la premiere occurence de ch
	//Tant qu'on a une occurence
	while(uneOccu != NULL) {
		cpt++;//On increment le nombre d'occurence
		p = uneOccu + len_ch;//On déplace le pointeur juste après l'occurence
		uneOccu = strstr(p,ch);//On recherche la prochaine occurence de ch
	}
	return cpt;
}

/*
	Remplacer une variable dans un echo
*/
void remplace_variable(void) {
	int i = 1;
	char * buffer = liste_argu[i];
	while(buffer != NULL) {
		if (nb_occurence(buffer,"$") > 0) {
			//printf("Variable : '%s'\n",buffer);
			char *ch = buffer;
			while(ch[0] != '$') {
				ch++;
			}
			ch++;
			liste_argu[i] = findValeur(ch);
		}
		buffer = liste_argu[++i];
	}
}

/* On remplace dans la chaine buffer les occurences de init par res
*/
void remplace_sous_chaine(char * buffer, char * init, char * res) {
	int cpt = nb_occurence(buffer,init); // Nombre d'occurence de la chaine init
	long int len_res = strlen(res);//longueur de la sous chaine res
	long int len_init = strlen(init);//Longueur de la sous chaine init

	char * p = buffer;//Pointeur sur le buffer
	int taille_new_buffer = strlen(buffer) + cpt * (len_res - len_init);//Calcul de la taille du nouveau buffer

	char * new_buffer = (char *) malloc(sizeof(char)*taille_new_buffer+1);//On alloue l'espace necessaire
	new_buffer[0] = '\0';//On initialise le nouveau buffer a une chaine vide;
	char * uneOccu = strstr(p,init);//Le pointeur pointe sur la premiere occurence
	//Tant qu'on a une occurence
	while(uneOccu!=NULL) {
		strncat(new_buffer,p,uneOccu-p);//On copie les caractère compris entre *p et l'occurence actuel
		strcat(new_buffer,res);//On copie la nouvelle occurence
		p = uneOccu + len_init;//On déplace le *p juste apres l'occurence
		uneOccu = strstr(p,init);//On se déplace a la prochaine occurence
	}
	strcat(new_buffer,p);//On copie le reste
	strcpy(buffer,new_buffer);//On copie new_buffer dans buffer afin de remplacer le buffer actuel
}

char * verifie_sortie(int * ecrase) {
	int i = 0;
	char * fichier = NULL;
	*ecrase=0;
	while (liste_argu[i] != NULL) {
		if ((strcmp(liste_argu[i],">") == 0) || (strcmp(liste_argu[i],">>") == 0)) {/*Si l'on a un argument redirection de sortie*/
			if(strcmp(liste_argu[i],">") == 0) *ecrase=1;
			fichier = liste_argu[i+1]; //Le fichier de redirection 
			free(liste_argu[i]);//On n'en a plus besoin
			//On décale tout les arguments sinon on aura un NULL entre les arguments ex : [argu1,argu2,NULL,fichier,argu3,..] --> [argu1,argu2,argu3,...]
			while(liste_argu[i+2]!=NULL){
				liste_argu[i] = liste_argu[i+2];
				i++;
			}
			liste_argu[i] = NULL;
		}
		i++;
	}
	return fichier;
}

char * verifie_sortie_err(int * ecrase) {
	int i = 0;
	char * fichier = NULL;
	*ecrase=0;
	while (liste_argu[i] != NULL) {
		if ((strcmp(liste_argu[i],"2>") == 0) || (strcmp(liste_argu[i],"2>>") == 0)) {/*Si l'on a un argument redirection de sortie*/
			if(strcmp(liste_argu[i],"2>") == 0) *ecrase=1;
			fichier = liste_argu[i+1]; //Le fichier de redirection 
			free(liste_argu[i]);//On n'en a plus besoin
			//On décale tout les arguments sinon on aura un NULL entre les arguments ex : [argu1,argu2,NULL,fichier,argu3,..] --> [argu1,argu2,argu3,...]
			while(liste_argu[i+2]!=NULL){
				liste_argu[i] = liste_argu[i+2];
				i++;
			}
			liste_argu[i] = NULL;
		}
		i++;
	}
	return fichier;
}

char * verifie_double_sortie(int * ecrase) {
	int i = 0;
	char * fichier = NULL;
	*ecrase=0;
	while (liste_argu[i] != NULL) {
		if ((strcmp(liste_argu[i],">&") == 0) || (strcmp(liste_argu[i],">>&") == 0)) {/*Si l'on a un argument redirection de sortie*/
			if(strcmp(liste_argu[i],">&") == 0) *ecrase=1;
			fichier = liste_argu[i+1]; //Le fichier de redirection 
			free(liste_argu[i]);//On n'en a plus besoin
			//On décale tout les arguments sinon on aura un NULL entre les arguments ex : [argu1,argu2,NULL,fichier,argu3,..] --> [argu1,argu2,argu3,...]
			while(liste_argu[i+2]!=NULL){
				liste_argu[i] = liste_argu[i+2];
				i++;
			}
			liste_argu[i] = NULL;
		}
		i++;
	}
	return fichier;
}

char * verifie_entree() {
	int i = 0;
	char * fichier = NULL;
	while (liste_argu[i] != NULL) {
		if ((strcmp(liste_argu[i],"<") == 0)) {/*Si l'on a un argument redirection d'entree*/
			fichier = liste_argu[i+1]; //Le fichier de redirection 
			free(liste_argu[i]);//On n'en a plus besoin
			//On décale tout les arguments sinon on aura un NULL entre les arguments ex : [argu1,argu2,NULL,fichier,argu3,..] --> [argu1,argu2,argu3,...]
			while(liste_argu[i+2]!=NULL){
				liste_argu[i] = liste_argu[i+2];
				i++;
			}
			liste_argu[i] = NULL;
		}
		i++;
	}
	return fichier;
}

/*Permet de formatter le buffer, afin de bien pouvoir séparer
les caractères de redirections du reste.
Exemple :
'ls>sortie' devient 'ls > sortie'
'ls >sortie' devient 'ls > sortie'
'ls|sort -r' devient 'ls | sort -r'
*/
void maj_buf_redirection(char * buffer) {
	remplace_sous_chaine(buffer,">&",">& ");//On met un espace entre le fichier en sortie
	remplace_sous_chaine(buffer," >",">");//On retire l'espace avant '>' si il y en a un
	remplace_sous_chaine(buffer,"> ",">");//On retire l'espace après '>' si il y en a un
	remplace_sous_chaine(buffer,">"," > ");//On ajoute un espace de chaque coté de '>' vu qu'il n'y en a pas
	remplace_sous_chaine(buffer," >  > "," >> ");//Si on avait une redirection avec 2 '>' il faut les réassemblé
	remplace_sous_chaine(buffer," 2 > "," 2> ");
	remplace_sous_chaine(buffer," 2 >> "," 2>> ");
	remplace_sous_chaine(buffer," > & "," >& ");
	remplace_sous_chaine(buffer," >> & "," >>& ");
	remplace_sous_chaine(buffer," <","<");//On retire l'espace avant '<' si il y en a un
	remplace_sous_chaine(buffer,"< ","<");//On retire l'espace apres '<' si il y en au un
	remplace_sous_chaine(buffer,"<"," < ");
	remplace_sous_chaine(buffer,"| ","|");
	remplace_sous_chaine(buffer," |","|");
	remplace_sous_chaine(buffer,"|"," | ");
}

/*Permet de formatter le buffer, afin de bien pouvoir séparer
les caractères de séquencement du reste.
Exemple :
'ls *.h&&ls *.c' --> 'ls *.h && ls *.c'
*/
void maj_buf_sequencement(char * buffer) {
	remplace_sous_chaine(buffer," ;",";");
	remplace_sous_chaine(buffer,"; ",";");
	remplace_sous_chaine(buffer,";"," ; ");

	remplace_sous_chaine(buffer," &&","&&");
	remplace_sous_chaine(buffer,"&& ","&&");
	remplace_sous_chaine(buffer,"&&"," && ");

	remplace_sous_chaine(buffer," ||","||");
	remplace_sous_chaine(buffer,"|| ","||");
	remplace_sous_chaine(buffer,"||"," || ");
}


/*
	Commande interne myfg
*/
void myfg(int numJob) {
	int status, i, j, en_cours;
	char liste[TAILLE][TAILLE];
	listeProcessus un_proc, prec = NULL;
	
	//On recherche le processus dans la liste en Background
	un_proc = l;
	while (un_proc && un_proc->jobs != numJob) {
		prec = un_proc;
		un_proc = un_proc->suiv;
	}
	
	//Si on a trouvé le proc
	if (un_proc) {
		
		//On récupère les informations
		proc_fore = un_proc->pid;
		
		for (i = 0; i < un_proc->n; i++) {
			strcpy(liste[i], un_proc->cmd[i]);
		}
		
		en_cours = un_proc->en_cours;
		
		//On enlève le processus de la liste 
		if (un_proc == l) {
			l = l->suiv;
			free(un_proc);
		} else {
			prec->suiv = un_proc->suiv;
			free(un_proc);
		}
		
		//On remet les jobs à 1 si il n'y a plus aucun processus stoppé ou en background
		if (!l) {
			jobs = 1;
		}
		
		if (proc_fore) {
			
			//Si il est arrété on le relance
			if (!en_cours) {
				if (kill(proc_fore, SIGCONT) == ERR) {
					fatalsyerror(9);
				}
			}
						
			foreground = 1;
			
			//Récupération signaux
			signal(SIGINT, controleC);
			//signal(SIGTSTP, controleZ);
			
			//On attend qu'il se termine et on affiche son status de retour
			waitpid(proc_fore, &status, 0);
			if (WIFEXITED(status)) {
				if ((status = WEXITSTATUS(status)) != FAILEDEXEC) {
					
				}
			} else {
				//Quelque chose s'est mal passé
				puts(ROUGE("Abnormal exit"));
			}
			dernier_proc = proc_fore;
			code_retour = status;
			foreground = 0;
		}
	}
}

/*
	Commande mybg
*/
void mybg(int numJob) {
	listeProcessus l_proc;
	
	//On chercher le processus associé a ce numJob
	l_proc = l;
	while (l_proc && l_proc->jobs != numJob) {
		l_proc = l_proc->suiv;
	}
	
	//Si on l'a trouvé
	if (l_proc) {
		//Si il est déja en cours rien a faire
		if (l_proc->en_cours) {
			printf(ROUGE("Already in background\n"));
		} else {//Sinon il faut changé l'etat
			
			if (l_proc->pid > 0) {
				//On le marque en cours d'execution
				l_proc->en_cours = 1;
							
				//On relance le processus	
				if (kill(l_proc->pid, SIGCONT) == ERR) {
					fatalsyerror(9);
				}
			}
		}
	}
}

/*
	Permet de traiter la fin de l'execution d'un processus en background
*/
void finBack() {
	int status, i;
	pid_t pid;
	listeProcessus courant, prec = NULL;
	if (!foreground) {
		//Récupération du pid du processus fils
		pid = wait(&status);
		
		//Recherche dans la liste des processus
		courant = l;
		while (courant) {
			//Le processus a été trouvé
			if (courant->pid == pid) {
			
				//On affiche son status de retour
				printf("\n");
				if (WIFEXITED(status)) {
					if ((status = WEXITSTATUS(status)) != FAILEDEXEC) {
						for(i = 0; i < courant->n; i++) printf("%s ", courant->cmd[i]);
						printf("(jobs = [%d], pid = %d) terminée avec status = %d\n", courant->jobs, courant->pid, status);
						ignore = 1;
					}
				}
			
				//On l'enlève de la liste des processus
				if (courant == l) {
					l = l->suiv;
					free(courant);
					courant = l;
				} else {
					prec->suiv = courant->suiv;
					free(courant);
					courant = prec->suiv;
				}
				
				//Si il n'y a plus aucun processus, les jobs sont remis à 1
				if (l == NULL) {
					jobs = 1;
				}
				return;
			}
			prec = courant;
			courant = courant->suiv;
		}
	}
}

/*Execute la commande dans le buffer et retourne la valeur du status*/
int lance_commande(char * buffer) {
	pid_t fils;
	char * fichier_sortie, * fichier_sortie_err, * fichier_entree;
	int res_exec = 0;
	int t_la = 0;//TAILLE DE LA LISTE D'ARGU

	maj_buf_redirection(buffer);
	creer_liste_argu(buffer);
	t_la = taille_liste_argu();
	//EXIT ?
	if (liste_argu[0] == NULL) return(0);
	if (liste_argu[0] != NULL && strcmp(liste_argu[0],"exit") == 0) {
		printf("exec:exit\n");
		return(0);
	}
	//affiche_liste_argu();
	if (t_la >= 2 && liste_argu[t_la-1][0] == '&') {//Si on lance en background
		foreground = 0;
		liste_argu[t_la-1] = NULL;
	}
	remplace_joker();

	//FICHIER SORTIE ET SORTIE ERREUR
	int ecrase_sortie;
	int ecrase_sortie_err;
	int double_sortie = 1;//Double sortie par défaut
	fichier_sortie = verifie_double_sortie(&ecrase_sortie);//On verifie si on a un redirection >& ou >>&
	if(fichier_sortie == NULL) {//Si on n'a pas de double redirection
		double_sortie = 0;
		fichier_sortie = verifie_sortie(&ecrase_sortie);
		fichier_sortie_err = verifie_sortie_err(&ecrase_sortie_err);
	} else {//Sinon on recopie la sortie dans la sortie des erreurs
		fichier_sortie_err = fichier_sortie;
		ecrase_sortie_err = ecrase_sortie;
	}
	//FICHIER ENTREE
	fichier_entree = verifie_entree();//On verifie si on a une redirection <
	
	//EXECUTION DE LA COMMANDE-----------------------
	if((fils=fork()) == ERR) syerror(1);//Si ERREUR lors du fork
	if(fils && foreground) {
		proc_fore = fils;
		signal(SIGINT, controleC);
		//signal(SIGTSTP, controleZ);
	}
	else if (fils && !foreground) {
		printf("[%d] %d\n", jobs , fils);
		ajouterProcessus(liste_argu, fils, 1);
	}
	if(!fils) {//Si on est le fils

		//Signaux par défaut
		signal(SIGCONT, SIG_DFL);
		signal(SIGSTOP, SIG_DFL);
		signal(SIGKILL, SIG_DFL);
		
		//On ignore certains signaux
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		

		//FICHIER ENTREE
		if (fichier_entree != NULL) {
			int fd_entree;
			if (fd_entree == -1) fatalsyerror(3);
			close(0);//On ferme l'entree standard
			dup(fd_entree);//On copie le fichier a la place de l'entree standard
			close(fd_entree);//On ferme l'ancien fichier
		}

		//FICHIER SORTIE
		if (fichier_sortie != NULL) {//Si on a indiqué un fichier en sortie
			int fd;
			//En cas de meme sortie avec err : On n'ecrase pas le fichier de sortie de la commande
			//(car c'est le fichier des erreurs qui a ecrasé le fichier avant et affiché les errs)
			if (!double_sortie && ecrase_sortie) {//Si on veut ecraser le fichier en sortie
				fd = open(fichier_sortie,O_WRONLY | O_TRUNC | O_CREAT,S_IRWXU | S_IRWXG | S_IRWXO);
			} else {//Sinon on ecrit a la suite du fichier
				fd = open(fichier_sortie,O_WRONLY | O_APPEND | O_CREAT,S_IRWXU | S_IRWXG | S_IRWXO);
			}
			if (fd == -1) fatalsyerror(3);
			close(1);//On ferme la sortie standard
			dup(fd);//On copie le fichier a la place de la sortie standard
			close(fd);//On ferme l'ancien fichier
		}

		//FICHIER SORTIE ERREUR
		if (fichier_sortie_err != NULL) {//Si on a indiqué un fichier en sortie pour les erreurs
			int fd_err;
			if (ecrase_sortie_err) {//Si on veut ecraser le fichier des erreurs en sortie
				fd_err = open(fichier_sortie_err,O_WRONLY | O_TRUNC | O_CREAT,S_IRWXU | S_IRWXG | S_IRWXO);
			} else {//Sinon on ecrit a la suite du fichier
				fd_err = open(fichier_sortie_err,O_WRONLY | O_APPEND | O_CREAT,S_IRWXU | S_IRWXG | S_IRWXO);
			}
			if (fd_err == -1) fatalsyerror(3);
			close(2);//On ferme la sortie des erreurs
			dup(fd_err);//On copie le fichier a la place de la sortie des erreurs
			close(fd_err);//On ferme l'ancien fichier
		}

		//EXEC
		if(strcmp(liste_argu[0],"status") == 0) {
			status();
			exit(0);
		}
		else if (strcmp(liste_argu[0],"cd") == 0) {
			mycd();
			exit(0);
		}else if (strcmp(liste_argu[0],"set") == 0) {
			gestion_variable();
			exit(0);
		}
		else if (strcmp(liste_argu[0],"unset") == 0) {
			gestion_variable();
			exit(0);
		}
		else if (strcmp(liste_argu[0],"echo") == 0) {
			remplace_variable();
		}
		else if (strcmp(liste_argu[0],"myjobs") == 0) {
			myjobs();
		}
		else if (strcmp(liste_argu[0],"myfg") == 0) {
			if (liste_argu[2] != NULL) {
				fatalsyerror(4);
			}
			exit(0);
		}
		res_exec = execvp(*liste_argu,liste_argu);
		syerror(2);
		exit(FAILEDEXEC);
	}
	int status;
	if(foreground) {
		waitpid(proc_fore, &status, 0);;//Si on est le pere on attend le fils
		signal(SIGINT, controleC);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGCHLD, finBack);
	}
	if (WIFEXITED(status)) {
		res_exec = WEXITSTATUS(status);
	}
	else {
		res_exec = -1;
	/*
		if (erreurCTRLZ) {
			erreurCTRLZ = 0;
		} else {
			puts(ROUGE("Anormal exit foreground"));
		}*/
	}
	dernier_proc = fils;
	code_retour = res_exec;

	if ((strcmp(liste_argu[0],"cd") == 0) && res_exec == 0) {
		mycd();
	}
	else if (strcmp(liste_argu[0],"set") == 0 && res_exec == 0) {
		gestion_variable();
	}
	else if (strcmp(liste_argu[0],"unset") == 0 && res_exec == 0) {
		gestion_variable();
	}
	else if (strcmp(liste_argu[0],"myfg") == 0 && res_exec == 0) {
		if(liste_argu[1] != NULL) {
			myfg(atoi(liste_argu[1]));
		} else {
			int numJob = -1;
			listeProcessus courant = l;

			//On va prend le dernier processus mis en background
			while(courant) {
				numJob = courant->jobs;
				courant = courant->suiv;
			}
			myfg(numJob);
		}
	}

	//FREE--------------------------------
	int i = 0;
	while (liste_argu[i] != NULL) {
		free(liste_argu[i]);
		i++;
	}
	return res_exec;
}

/* Permet de gerer les pipes d'une commande */
void gerer_pipe(char * buffer) {
	int nb_pipe = nb_occurence(buffer," | ");
	printf("il y a %d pipe\n",nb_pipe);
	char * buffer_cp = strdup(buffer);
	char *cmd, *cmd2;
	cmd = strtok(buffer_cp,"|");
	printf("Commande 1 : '%s'\n",cmd);
	cmd2 = strtok(NULL,"|");
	printf("Commande 2 : '%s'\n",cmd2);
	
	int fd[2];
    if(pipe(fd) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    if(fork() == 0){            
        close(STDOUT_FILENO);  
        dup2(fd[1], 1);        
        close(fd[0]);    
        close(fd[1]);

        lance_commande(cmd);
        exit(1);
    }

    if(fork() == 0) {           
        close(STDIN_FILENO); 
        dup2(fd[0], 0);      
        close(fd[1]);  
        close(fd[0]);

        lance_commande(cmd2);
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);
    wait(0);
    wait(0);
}

/*Lance la commande dans buffer, si celle-ci est séparé par '&&' ou '||'
alors il va séparer les différentes commandes pour les lancers une par une
*/
void pre_traitement_commande(char * buffer) {
	char * liste_commande[TAILLE_LISTE_ARGU];//On va stocker les commandes ici
	char * liste_commande_tmp[TAILLE_LISTE_ARGU];//Liste temporaire pour effectuer le traitement
	char * p = buffer;//Un pointeur pour se déplacer sur le buffer

	int i = 0;

	//SEPARATION AVEC SEPARATEUR &&
	char * uneOccu = strstr(p,"&&");//Pointe sur la premiere occurence de "&&"
	/* Exemple : 
	'ls *.c && ls *.h' --> ['ls *.c', '&&', 'ls *.h'];
	*/
	//On met tout dans une liste temporaire car après on doit traiter le ||
	while(uneOccu != NULL) {
		liste_commande_tmp[i++] = strndup(p,uneOccu-p);
		liste_commande_tmp[i] = strdup("&&");
		p = uneOccu + 2;
		uneOccu = strstr(p,"&&");
		i++;
	}
	liste_commande_tmp[i++] = strdup(p);
	while(i < TAILLE_LISTE_ARGU) {
		liste_commande_tmp[i] = NULL;
		i++;
	}

	//SEPARATION AVEC SEPARTEUR ||
	i = 0;
	int j = 0;
	while(liste_commande_tmp[i] != NULL) {
		p = liste_commande_tmp[i];
		uneOccu = strstr(p,"||");
		while(uneOccu != NULL) {
			liste_commande[j++] = strndup(p,uneOccu-p);
			liste_commande[j] = strdup("||");
			p = uneOccu + 2;
			uneOccu = strstr(p,"||");
			j++;
		}
		liste_commande[j++] = strdup(p);
		i++;
	}
	while(j < TAILLE_LISTE_ARGU) {
		liste_commande[j] = NULL;
		j++;
	}


	//LANCEMENT COMMANDE
	i = 0;
	int res_exec;
	while(liste_commande[i] != NULL) {
		//printf("Commmande2 : '%s'\n",liste_commande[i]);
		if (strstr(liste_commande[i]," | ") != NULL) {//Si il y a un pipe dans la commande
			//printf("PIPE\n");
			gerer_pipe(liste_commande[i]);
		} else {//Si pas de pipe on lance directement la commande
			res_exec = lance_commande(liste_commande[i++]);//On lance la commande et récupère la valeur de retour
		}
		if (liste_commande[i] != NULL) {//On verifie si il y a un caractère de sequencement && ou ||
			//Si il y a un && et que la commande d'avant a échoué on arrette
			if (strcmp(liste_commande[i],"&&") == 0 && (res_exec != 0)) {
				break;
			}//Si il y a une || et que la commande d'avant a reussi on arrette
			else if (strcmp(liste_commande[i],"||") == 0 && (res_exec == 0)) {
				break;
			}
		}
		i++;
	}

	//FREE
	i = 0;
	while(liste_commande_tmp[i] != NULL) free(liste_commande_tmp[i++]);
	i=0;
	while(liste_commande[i] != NULL) free(liste_commande[i++]);
}
int recoit_socket(char * msg)
{	
	char * uneCommande;

	//Initialisation des signaux
	signal(SIGINT, controleC);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGCHLD, finBack);

	buf[0] = '\0';
	ignore = 0;

	strcpy(buf,msg);//Récupere ce que l'utisateur a rentré

	maj_buf_sequencement(buf);
	//printf("Buf : '%s'\n",buf);

	char * liste_commande[TAILLE_LISTE_ARGU];//Stocke toute les commandes séparé par ';'
	int i = 0;

	//STOCKAGE DES COMMANDES
	uneCommande = strtok(buf,";");
	while (uneCommande != NULL) {
		liste_commande[i] = strdup(uneCommande);
		uneCommande = strtok(NULL,";");
		i++;
	}
	while (i < TAILLE_LISTE_ARGU) {//On met le reste a NULL
		liste_commande[i] = NULL;
		i++;
	}

	//TRAITEMENT DES COMMANDES
	i=0;
	while(liste_commande[i]!= NULL) {
		uneCommande = liste_commande[i];
		//printf("\nCommande : '%s'\n",uneCommande);
		foreground=1;
		pre_traitement_commande(uneCommande);
		i++;
		foreground=0;
	}

	//FREE
	i= 0;
	while(liste_commande[i] != NULL) free(liste_commande[i++]);

} 



// Compilation : gcc server.c -o server -lcrypt 
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <crypt.h>

#define MAX_CHAR 429496
#define MAX_CLIENTS 3
#define ERR -1

#define syerror2(x)	perror(errormsg2[x])
#define fatalsyerror2(x)	syerror2(x),exit(x)



char * errormsg2[]= {
	"No error", //0
	ROUGE("Impossible de créer une socket"), //1
	ROUGE("Erreur sur bind"),//2
	ROUGE("Erreur sur listen") //3
	ROUGE("Erreur sur accept") //4
};

int main(void){	
	int sockServeur, sockClient;
	int bytes_r, tailleClient;
	int interrupt = 1, interrupt2 = 1;
	int first = 1;

	int b,f_open,m,pid,i,k,st;
	
	char fname[50],donnee[MAX_CHAR],nom_user[MAX_CHAR];
	char* mdp_user = (char*)malloc(sizeof(char)*MAX_CHAR);
	char chemin_f_tmp[1024];
	
	struct sockaddr_in addrClient, addrServeur;
	
	FILE *fp;

	if((sockServeur = socket(AF_INET,SOCK_STREAM,0)) == ERR){
		fatalsyerror2(1);
	}

	addrServeur.sin_family = AF_INET;
	addrServeur.sin_port = htons(10165);
	addrServeur.sin_addr.s_addr = INADDR_ANY;
	
	if((bind(sockServeur,(struct sockaddr*)&addrServeur,sizeof(addrServeur))) == ERR){
		fatalsyerror2(2);
	}
	else{
		printf("Bind reussit\n");
	}

	if(listen(sockServeur,5) == ERR){
		fatalsyerror2(3);
	}

	printf("\nLe Serveur SSH attend les clients\n"); 

	tailleClient = sizeof(addrClient);
	if((sockClient = accept(sockServeur, (struct sockaddr *)&addrClient,&tailleClient)) == ERR){
		fatalsyerror2(4);
	}

	printf("\n Connexion de (%s , %d)\n",inet_ntoa(addrClient.sin_addr),ntohs(addrClient.sin_port));
		
	while(interrupt){
		interrupt2 = 1;
		while(interrupt2){
			bytes_r = recv(sockClient,nom_user,MAX_CHAR,0);
			nom_user[bytes_r] = '\0';
			printf("Utilisateur : '%s'\n",nom_user);
		
			bytes_r = recv(sockClient,mdp_user,MAX_CHAR,0);
			mdp_user[bytes_r] = '\0';
			printf("Mot de passe : '%s'\n",mdp_user);
		
			char fi[100],fpass[100];
			
			//Verification si le nom et mot de passe figure dans le fichier nom_mdp.txt
			int y = 0;
			fp = fopen("nom_mdp.txt","r");
			while(!feof(fp)){
				fscanf(fp,"%s",fname);
				fscanf(fp,"%s",fpass);
				strcpy(fi,fpass);
				if((strcmp(fi,mdp_user) == 0) && (strcmp(fname,nom_user) == 0)) y++;
			}
			fclose(fp);
			if(y > 0){ //mot de passe correct
				strcat(donnee,"Bienvenue ");
				strcat(donnee,nom_user);
				strcat(donnee,"\n");				
				send(sockClient, donnee,strlen(donnee), 0);
				if(strcmp(nom_user,"admin") == 0){
					printf("Bienvenue admin!\n");	
					while(1){
						bytes_r = recv(sockClient,donnee,strlen(donnee),0);
						donnee[bytes_r] = '\0';
						printf("Cmd recue de l'admin : '%s'\n",donnee);	
						if(strcmp(donnee,"exit") == 0)
							break;
						else if(strcmp(donnee,"ajoutuser") == 0){
							bytes_r = recv(sockClient,donnee,strlen(donnee),0);
							donnee[bytes_r]='\0';
							k = 0;		
							fp = fopen("nom_mdp.txt","r");
							while(!feof(fp)){
								fscanf(fp,"%s",fname);
								fscanf(fp,"%s",fpass);
								if(strcmp(fname,donnee) == 0)	k=1;
							}

							fclose(fp);		
							if(k == 1){
								printf("L'utilisateur existe déjà!\n");
								strcpy(donnee,"L'utilisateur existe déjà!");				
								send(sockClient,donnee,strlen(donnee),0);				
							}
							else{
								send(sockClient,donnee,strlen(donnee),0);			
								// Creation d'un repertoire pour le nouvel utilisateur
								chdir("/home/olivier-buh/Documents/'Mes Cours'/'M1 INFO'/SE/PROJET_RES_SE/projet-partie-res/'new client'/users/");
								strcpy(nom_user,donnee);
								mkdir(donnee,0);
								strcpy(donnee,"chmod 770 ");
								strcat(donnee,nom_user);
								system(donnee);
								//got the password
								bytes_r = recv(sockClient,donnee,1024,0);
								donnee[bytes_r]='\0';
								strcat(nom_user,"\t");
								strcat(nom_user,donnee);
								strcat(nom_user,"\n");
								//storing the password in the file		
								fp = fopen("nom_mdp.txt","a");	
								fprintf(fp,"%s",nom_user);
								fclose(fp);
							}			
						}							
						else	printf("Commande invalide!\n");
					}				
				}
				else{		//non-ADMIN
					chdir(nom_user);
					system("pwd");
					char addrHote[200];
					if (first) {
						getcwd(chemin,TAILLE_BUFFER);
						strcpy(chemin_f_tmp,chemin);
						strcat(donnee,"\033[1;36m");
						strcat(donnee,chemin);
						strcat(donnee,"~>\033[0m");
						send(sockClient,donnee,strlen(donnee),0);
						bytes_r = recv(sockClient,donnee,1024,0);
						donnee[bytes_r] = '\0';
						strcpy(addrHote,donnee);
						printf("Voici addrHote: '%s'\n",addrHote);
					}
					int fd, new_stdout, new_tmp, new_tmp2,new_stderr;
					FILE * f;
					char unbuffer[TAILLE_BUFFER];
					while(1){
						bytes_r = recv(sockClient,donnee,1024,0);
						donnee[bytes_r] = '\0';
						printf("Commande recue ~> '%s'\n",donnee);
						if (first) {
							first = 0;
							strcpy(donnee," ");

							//ENVOIE SOCKET
							send(sockClient,donnee,strlen(donnee),0);

							strcat(chemin_f_tmp,"/.tempfile");
						}
						else {
							//ON CHANGE STDOUT
							fd = open(chemin_f_tmp,O_WRONLY | O_TRUNC | O_CREAT,S_IRWXU | S_IRWXG | S_IRWXO);
							
							//STDOUT
							new_stdout = dup(1);
							close(1);
							new_tmp = dup(fd);
							close(fd);

							//STDERR
							new_stderr = dup(2);
							close(2);
							new_tmp2 = dup(new_tmp);

							
							recoit_socket(donnee);
							donnee[0] = '\0';

							sleep(1);
							close(new_tmp);
							//ON CHANGE STDOUT
							dup(new_stdout);

							f = fopen(chemin_f_tmp,"r");
							while (fgets( unbuffer, TAILLE_BUFFER, f ) != NULL) {  
						        strcat(donnee,unbuffer);       
						    }
						    fclose(f);



							//ENVOIE DU CHEMIN
							strcat(donnee,addrHote);
							strcat(donnee,"\033[1;36m");
							strcat(donnee,chemin);
							strcat(donnee,"~> \033[0m");

							//ENVOIE SOCKET
							send(sockClient,donnee,strlen(donnee),0);

							//ON CHANGE STDERR
							close(new_tmp2);
							dup(new_stderr);

							printf("Envoyé : '%s'\n",donnee);
							//printf("FINI\n");
						}
					}	
				}
			}
			else{	// mauvais mot de passe
				strcpy(donnee,"Nom de l'utilisateur ou Mot de passe incorrect\nEchec de la connexion!\n");
				send(sockClient,donnee,strlen(donnee),0);
				interrupt = 0;
			}
			interrupt2 = 0;
			y = 0;
		}
		interrupt = 0;
	}
	close(sockClient);
	close(sockServeur);
	return 0;
}