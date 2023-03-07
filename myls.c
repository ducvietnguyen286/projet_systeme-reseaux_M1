

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_DROITS 10
#define MAX_CHAR 4096
#define ERR -1

#define syerror(x)		perror(errormsg[x])
#define fatalsyerror(x)	syerror(x),exit(x)

#define nouveau(type) (type*)malloc(sizeof(type));
#define ROUGE(m)	"\033[01;31m"m"\033[0m"
#define VERT(m)	"\033[01;32m"m"\033[0m"
#define JAUNE(m)	"\033[1;33m"m"\033[0m"
#define BLEU(m)	"\033[1;34m"m"\033[0m"
#define VIOLET(m)	"\033[1;35m"m"\033[0m"
#define CYAN(m)	"\033[1;36m"m"\033[0m"
#define GRIS(m)	"\033[1;37m"m"\033[0m"

char * errormsg[]= {
	"No error", //0
	ROUGE("Impossible to fork process"), //1
	ROUGE("Exec failed"),//2
	ROUGE("Open file failed") //3
	ROUGE("Wait failed") //4
};

int optionA = 0;
int optionR = 0;
int nbFichiers = 0;
int nbDocs = 0;

typedef struct _doc {
	char nom[MAX_CHAR];
	char longChemin[MAX_CHAR];
	char courtChemin[MAX_CHAR];
	struct _doc *suivant;
} doc;

typedef doc* ListeDoc;

ListeDoc fichiers;
ListeDoc fichiers2;
ListeDoc rep;
ListeDoc nouvRep;

char * formatAudio[6] = {".mp3",".mka",
					".acc",".au",
					".ogg",".flac"};


char * formatImg[13] = {".gif",".jpg",
				  ".jpeg",".png",
				  ".tif",".tiff",
				  ".bmp",".svg",
				  ".avi",".mpeg",
				  ".WebM",".mkv",".mp4"};

char * formatArch[8] = {".7z",".s7z",
				   ".rar",".zip",
				   ".jar",".z",
				   ".tar",".xz"};


/*-----------------------Comparaison de deux chaines-----------------------*/
int est_meme_chaine(char * chaine1, char * chaine2) {
	int ind1, ind2;
	
	//Cas des fichiers . et ..
	if (!strcmp(chaine1, ".") && !strcmp(chaine2, "..")) return -1;
	else if (!strcmp(chaine1, "..") && !strcmp(chaine2, ".")) return 1;
	else if (!strcmp(chaine1, ".") || (!strcmp(chaine1, ".."))) return -1;
	else if (!strcmp(chaine2, ".") || (!strcmp(chaine2, "..")))	return 1;
	
	
	//On passe les caractères qui ne sont ni des lettres ni des chiffres
	for (ind1 = 0; (chaine1[ind1] != '\0') && (!isalpha(chaine1[ind1])) && (!isdigit(chaine1[ind1])); ind1++);
	for (ind2 = 0; (chaine2[ind2] != '\0') && (!isalpha(chaine2[ind2])) && (!isdigit(chaine2[ind2])); ind2++);
	
	//On passe les caractères qui sont identiques dans les deux chaînes
	while((chaine1[ind1] != '\0') && (chaine2[ind2] != '\0') && (tolower(chaine1[ind1]) == tolower(chaine2[ind2]))){
		ind1++;
		ind2++;
	}
	
	//Les deux chaines sont identiques
	if (chaine1[ind1] == '\0' && chaine2[ind2] == '\0') return 0;
	
	//On est arrivé à la fin de la première chaine
	else if (chaine1[ind1] == '\0') return -1;
	
	//On est arrivé à la fin de la deuxième chaine
	else if (chaine2[ind2] == '\0') return 1;
	else return tolower(chaine1[ind1]) - tolower(chaine2[ind2]); //On compare les caractères sur lesquels on est arrivé
}

/*-----------------------Ajout d'un fichier-----------------------*/
int sub_ajout_fichier(char * nom, ListeDoc *nouvDoc, ListeDoc *courant, ListeDoc *precDoc, ListeDoc *doc){
	if (!(*doc)) {
		*doc = *nouvDoc;
		return 0;
	}
	
	*courant = *doc;
	
	//Un seul élément dans la liste
	if (*courant && !(*courant)->suivant) {
		if (est_meme_chaine(nom, (*courant)->nom) >= 1) {
			(*courant)->suivant = *nouvDoc;
		} else {
			(*nouvDoc)->suivant = *courant;
			*doc = *nouvDoc;
		}
		return 0;
	}
	
	//On passe tous les éléments de la liste qui sont inférieurs au nouveau fichier
	while ((*courant)->suivant && est_meme_chaine(nom, (*courant)->nom) >= 1) {
		*precDoc = *courant;
		*courant = (*courant)->suivant;
	}
	
	//Placer le nouveau fichier en tête de liste
	if (*courant == *doc) {
		(*nouvDoc)->suivant = *courant;
		*doc = *nouvDoc;
		return 0;
	}

	if ((*courant)->suivant) {
		//Placer le nouveau fichier dans la liste
		(*nouvDoc)->suivant = *courant;
		(*precDoc)->suivant = *nouvDoc;
	} else {
		//Placer le nouveau fichier en fin de liste
		(*courant)->suivant = *nouvDoc;
	}
	return 1;
} 


void ajout_fichier(char * nom, char * lChemin, char * cChemin, char * liste) {
	ListeDoc courant, precDoc, nouvDoc;
	
	//On crée le nouveau fichier
	nouvDoc = nouveau(doc);
	strcpy(nouvDoc->nom, nom);
	strcpy(nouvDoc->longChemin, lChemin);
	strcpy(nouvDoc->courtChemin, cChemin);
	nouvDoc->suivant = NULL;
	
	int retour = 1;
	//On choisit la liste en fonction de l'entrée
	if (!strcmp(liste, "fichiers")) {
		retour = sub_ajout_fichier(nom,&nouvDoc,&courant,&precDoc,&fichiers);
		if (!retour) return;
	}
	else if (!strcmp(liste, "fichiers2")) {
		retour = sub_ajout_fichier(nom,&nouvDoc,&courant,&precDoc,&fichiers2);
		if (!retour) return;
	}
	else if (!strcmp(liste, "rep")) {
		retour = sub_ajout_fichier(nom,&nouvDoc,&courant,&precDoc,&rep);
		if (!retour) return;
	}
	else if (!strcmp(liste, "nouvRep")) {
		retour = sub_ajout_fichier(nom,&nouvDoc,&courant,&precDoc,&nouvRep);
		if (!retour) return;
	}
}


/*-----------------------Récupération du premier fichier-----------------------*/
int sub_recup(char * nom, char * lChemin, char * cChemin, ListeDoc *courant, ListeDoc *doc){
	if (!(*doc))	return 0;
	*courant = *doc;
	strcpy(nom, (*courant)->nom);
	strcpy(lChemin, (*courant)->longChemin);
	strcpy(cChemin, (*courant)->courtChemin);
	return 1;

}

int recup_premier_fichier(char * nom, char * lChemin, char * cChemin, char * liste) {
	ListeDoc courant;
	
	int retour = 1;
	//On récupère la liste en fonction de l'entrée
	if (!strcmp(liste, "fichiers")) {
		retour = sub_recup(nom,lChemin,cChemin,&courant,&fichiers);
	}
	else if (!strcmp(liste, "fichiers2")) {
		retour = sub_recup(nom,lChemin,cChemin,&courant,&fichiers2);
	}
	else if (!strcmp(liste, "rep")) {
		retour = sub_recup(nom,lChemin,cChemin,&courant,&rep);
	}
	else if (!strcmp(liste, "nouvRep")) {
		retour = sub_recup(nom,lChemin,cChemin,&courant,&nouvRep);
	}
	return retour;
}

/*-----------------------Enleve le premier fichier traité-----------------------*/
int sub_enleve(ListeDoc *courant, ListeDoc *doc){
	//Liste vide
	if (!(*doc)) return 0;
	
	//On enlève le premier noeud de la liste
	*courant = *doc;
	*doc = (*courant)->suivant;
	return 1;
}

int enleve_premier_fichier(char * liste) {
	ListeDoc courant;
	int retour = 1;
	//On récupère la liste en fonction de l'entrée
	if (!strcmp(liste, "fichiers")){
		retour = sub_enleve(&courant,&fichiers);
	}
	else if (!strcmp(liste, "fichiers2")){
		retour = sub_enleve(&courant,&fichiers2);
	}
	else if (!strcmp(liste, "rep")){
		retour = sub_enleve(&courant,&rep);
	}
	else if (!strcmp(liste, "nouvRep")){
		retour = sub_enleve(&courant,&nouvRep);
	}
	
	//On libère le noeud de la liste
	free(courant);
	return retour;
}

/*-----------------------Libération de la mémoire-----------------------*/
void detruire_listes() {
	
	free(fichiers);
	free(fichiers2);
	free(rep);
	free(nouvRep);
}

/*-----------------------Enlève répertoire-----------------------*/
void enleve_repertoire(char * rep) {
	int taille;
	
	//On enlève les caractères de l'adresse jusqu'à retrouver un '/'
	for (taille = strlen(rep) - 1; rep[taille] != '/'; taille--) {
		rep[taille] = '\0';
	}
	rep[taille] = '\0';
}

/*-----------------------Récupérations-----------------------*/
void recup_droits(char * droits, struct stat s) {
	
	memset(droits, 0, MAX_DROITS);
	
	droits[0] = ((S_ISDIR(s.st_mode)) ? ('d') : ((S_ISLNK(s.st_mode)) ? ('l') : ('-')));
	
	//Droits utilisateur
	((s.st_mode & S_IRUSR) ? (strcat(droits, "r")) : (strcat(droits, "-")));
	((s.st_mode & S_IWUSR) ? (strcat(droits, "w")) : (strcat(droits, "-")));
	((s.st_mode & S_IXUSR) ? (strcat(droits, "x")) : (strcat(droits, "-")));
	
	//Droits groupe
	((s.st_mode & S_IRGRP) ? (strcat(droits, "r")) : (strcat(droits, "-")));
	((s.st_mode & S_IWGRP) ? (strcat(droits, "w")) : (strcat(droits, "-")));
	((s.st_mode & S_IXGRP) ? (strcat(droits, "x")) : (strcat(droits, "-")));
	
	//Droits autres
	((s.st_mode & S_IROTH) ? (strcat(droits, "r")) : (strcat(droits, "-")));
	((s.st_mode & S_IWOTH) ? (strcat(droits, "w")) : (strcat(droits, "-")));
	((s.st_mode & S_IXOTH) ? (strcat(droits, "x")) : (strcat(droits, "-")));
}

void recup_date(char * date, struct stat s) {
	time_t t, t_local;
	struct tm *tmp, *tmp_local;
	char d[MAX_CHAR], d_local[MAX_CHAR];
	char* mois[13] = {"", "janv.", "févr.", "mars", "avril", "mai", "juin", "juil", "août", "sept.","oct.", "nov.", "déc."};
	int taille,ind;

	t_local = time(NULL);
	tmp_local = localtime(&t_local);
	strftime(d_local, sizeof(d_local), "%Y", tmp_local);
	
	t = s.st_mtime;
	tmp = localtime(&t);
	
	//Récupération du mois
	strftime(d, sizeof(d), "%m", tmp);
	ind = atoi(d);
	strcpy(date, mois[ind]);

	if(ind != 1 && ind != 2 && ind != 4){
		if(ind == 5){
			strcat(date, "  ");
		}
		else strcat(date, " ");
	}

	
	//Récupération du jour
	strftime(d, sizeof(d), "%d", tmp);
	strcat(date, " ");
	if (d[0] == '0') {
		d[0] = ' ';
	}
	strcat(date, d);
	
	//Récupération de l'année ou de l'heure
	strftime(d, sizeof(d), "%Y", tmp);
	strcat(date, " ");
	if (strcmp(d, d_local)) {
		strcat(date, " ");
		strcat(date, d);
	} else {
		strftime(d, sizeof(d),"%H", tmp);
		strcat(date, d);
		strcat(date, ":");
		strftime(d, sizeof(d),"%M", tmp);
		strcat(date, d);
	}
}

void recup_utilisateur(char * user, struct stat s) {
	struct passwd *p;
	
	//Récupération del'utilisateur
	p = getpwuid(s.st_uid);
	strcpy(user, p->pw_name);
}

void recup_groupe(char * groupe, struct stat s) {
	struct group *grp;
	
	//Récupération de l'utilisateur
	grp = getgrgid(s.st_gid);
	strcpy(groupe, grp->gr_name);
}

/*-----------------------Analyse du dossier fils-----------------------*/
void analyse_dossier_fils(char * cChemin) {
	pid_t pid;
	int status;
	
	//On crée un processus fils
	if ((pid = fork()) == ERR) syerror(1);
	if (pid) {
		//Dans le père, on attend la fin du fils
		if (wait(&status) == ERR) syerror(4);
	} else {
		//Dans le fils, on relance le programme vec les options et le nouveau dossier
		if (optionA) {
			execlp("./myls", "myls", "-a", "-R", cChemin, NULL);
		} else {
			execlp("./myls", "myls", "-R", cChemin, NULL);
		}
		fatalsyerror(2);
	}
}

/*-----------------------Extension = Audio-----------------------*/
int est_audio(char * nom){
	for (int i = 0; i < 6; i++){
		if(!strcmp(nom,formatAudio[i])) return 1;
	}
	return 0;
}

/*-----------------------Extension = Image-----------------------*/
int est_img(char * nom){
	int i;
	for (i = 0; i < 13; i++){
		if(!strcmp(nom,formatImg[i])) return 1;
	}
	return 0;
}

/*-----------------------Extension = Archive -----------------------*/
int est_arch(char * nom){
	for (int i = 0; i < 8; i++){
		if(!strcmp(nom,formatArch[i])) return 1;
	}
	return 0;
}

/*-----------------------Récupération de l'extension-----------------------*/
char * recup_ext(char * nom){
    char *point = strrchr(nom, '.');
    if(!point || point == nom) return "";
    return point + 1;
}

/*-----------------------Type de format-----------------------*/
int quel_format(char * nom){
	char extension[10] = ".";
	strcat(extension,recup_ext(nom));


	if(est_audio(extension)) return 1;

	if(est_img(extension)) return 2;

	if(est_arch(extension)) return 3;

	return 0;
}

/*-----------------------Analyse du dossier-----------------------*/
void analyse_dossier(char lChemin[MAX_CHAR], char cChemin[MAX_CHAR]) {
	DIR *rep;
	struct dirent *ent;
	struct stat s;
	char droits[MAX_DROITS], date[MAX_CHAR], user[MAX_CHAR], groupe[MAX_CHAR];
	char longChemin[MAX_CHAR], courtChemin[MAX_CHAR], nom[MAX_CHAR];
	int total = 0, format = 0;
	
	//On récupère les chemins absolu et relatif
	strcpy(longChemin, lChemin);
	strcpy(courtChemin, cChemin);
	
	//On ouvre le dossier
	rep = opendir(courtChemin);
	if (rep) {
		
		//On affiche le nom du dossier si nécessaire
		if (nbFichiers + nbDocs > 1 || optionR) {
			printf("%s:\n", courtChemin);
		}
		
		//On parcourt le contenu du dossier
		while ((ent = readdir(rep))) {
			
			//Récupération du nom et des chemins du fichier
			strcpy(nom, ent->d_name);
			strcpy(longChemin, lChemin);
			strcpy(courtChemin, cChemin);
			if (longChemin[strlen(longChemin) - 1] != '/') {
				strcat(longChemin, "/");
			}
			strcat(longChemin, ent->d_name);
			if (courtChemin[strlen(courtChemin) - 1] != '/') {
				strcat(courtChemin, "/");
			}
			strcat(courtChemin, ent->d_name);
			
			//On vérifie que l'on peut accéder au fichier
			if (lstat(longChemin, &s) == ERR) {
				printf(ROUGE("ls: impossible to access to '%s': No such file or directory\n"), courtChemin);
				return;
			}
			
			//Récupération du total
			if ((!optionA && nom[0] != '.') || (optionA)) {
				total += (int)s.st_blocks;
			}
			
			//On ajoute le fichier à la liste des fichiers
			ajout_fichier(nom, longChemin, courtChemin, "fichiers2");
		}
		
		//On affiche le total
		printf("total %d\n", total>>1);
		
		//On parcourt la liste des fichiers trouvés
		for (; recup_premier_fichier(nom, longChemin, courtChemin, "fichiers2"); ) {
			
			//On vérifie que l'on prend en compte ce fichier
			if ((!optionA && nom[0] != '.') || (optionA)) {
				
				//On vérifie si l'on peut accéder au fichier
				if (lstat(longChemin, &s) == ERR) {
					printf(ROUGE("ls: impossible to access to '%s': No such file or directory\n"), courtChemin);
					return;
				}
				
				//On affiche les droits
				recup_droits(droits, s);
				printf("%s ", droits);
				
				//On affiche le nombre des liens physiques
				printf("%2d ", (int)s.st_nlink);
				
				//On affiche l'utilisateur
				recup_utilisateur(user, s);
				printf("%s ", user);
				
				//On affiche le groupe
				recup_groupe(groupe, s);
				printf("%s ", groupe);
				
				
				//On affiche la taille
				printf("%10d ", (int)s.st_size);
				
				//On affiche la date
				recup_date(date, s);
				printf("%s  ", date);
				
				//On affiche le nom du fichier, du dossier ou du lien
				if (droits[0] == '-') {

					if ((droits[3] == 'x') || (droits[6] == 'x') || (droits[9] == 'x')) {
						printf(VERT("%s\n"), nom);
					}
					else{
						format = quel_format(nom);
						if(format == 1){
							printf(CYAN("%s\n"), nom);
						}
						else if(format == 2){
							printf(VIOLET("%s\n"), nom);
						}
						else if(format == 3){
							printf(ROUGE("%s\n"), nom);
						}
						else{
						printf("%s\n", nom);
						}
					}
				}
				else if (droits[0] == 'd') {
					printf(BLEU("%s\n"), nom);
					
					//On a un dossier, on l'ajoute à la liste des dossiers
					if (optionR && strcmp(nom, ".") && strcmp(nom, "..")) {
						recup_premier_fichier(nom, longChemin, courtChemin, "fichiers2");
						ajout_fichier(nom, longChemin, courtChemin, "nouvRep");
						recup_premier_fichier(nom, longChemin, courtChemin, "fichiers2");
					}
				}
				else if (droits[0] == 'l') {
					printf(CYAN("%s\n"), nom);
					
				}
			}
			
			//On enlève le fichier de la liste des fichiers
			enleve_premier_fichier("fichiers2");
		}         
		
		//On parcourt la liste des dossiers trouvés et on les analyse dans un processus fils
		for (; recup_premier_fichier(nom, longChemin, courtChemin, "nouvRep"); ) {
			printf("\n");
			analyse_dossier_fils(courtChemin);
			enleve_premier_fichier("nouvRep");
		}
	}
	
	//On ferme le dossier ouvert
	closedir(rep);
}

/*-----------------------Main-----------------------*/
int main(int argc, char* argv[]){
	struct stat s;
	char droits[MAX_DROITS], date[MAX_CHAR], user[MAX_CHAR], groupe[MAX_CHAR];
	char longChemin[MAX_CHAR], courtChemin[MAX_CHAR], nom[MAX_CHAR];
	int i, j, taille, aucunFichier = 1;
	
	srand(time(NULL));
	
	//Générer les différentes listes
	fichiers = NULL;
	fichiers2 = NULL;
	rep = NULL;
	nouvRep = NULL;
	
	//Paramètres présents
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			
			// '-' repéré
			if (argv[i][0] == '-') {
				taille = strlen(argv[i]);
				
				//On vérifie les options passées en paramètres
				for (j = 1; j < taille; j++) {
					if (argv[i][j] == 'a') {
						optionA = 1;
					}
					else if (argv[i][j] == 'R') {
						optionR = 1;
					} else {
						printf(ROUGE("ls : invalid option --'%c'\n"), argv[i][j]);
						exit(1);
					}
				}
			} else {
			
				//On vérifie que l'on peut accéder au fichier
				if (lstat(argv[i], &s) == ERR) {
					printf(ROUGE("ls: impossible to access to '%s': No such file or directory\n"), argv[i]);
					continue;
				}
				
				aucunFichier = 0;
				if (S_ISDIR(s.st_mode)) {
					
					//On a trouvé un dossier
					nbFichiers++;
					
					//Récupération des chemins absolu et relatif
					if (!strcmp(argv[i], ".")) {
						getcwd(longChemin, MAX_CHAR);
						strcpy(courtChemin, ".");
					}
					else if (!strcmp(argv[i], "..")) {
						getcwd(longChemin, MAX_CHAR);
						enleve_repertoire(longChemin);
						strcpy(courtChemin, "..");
					} else {
						strcpy(longChemin, argv[i]);
						strcpy(courtChemin, argv[i]);
					}
					strcpy(nom, argv[i]);
					
					//On ajoute le dossier à la liste des dossiers
					ajout_fichier(nom, longChemin, courtChemin, "rep");
				} else {
					//On a trouvé un fichier
					nbDocs++;
					
					//Récupération des chemins absolu et relatif
					strcpy(longChemin, argv[i]);
					strcpy(courtChemin, argv[i]);
					strcpy(nom, argv[i]);
					
					//On ajoute le fichier à la liste des fichiers
					ajout_fichier(nom, longChemin, courtChemin, "fichiers");
				}
			}
		}
	} 
	
	//Aucun dossier ni fichier, on prend le répertoire courant
	if (argc == 1 || (!nbFichiers && !nbDocs && aucunFichier)) {
		getcwd(longChemin, MAX_CHAR);
		strcpy(courtChemin, ".");
		strcpy(nom, ".");
		ajout_fichier(nom, longChemin, courtChemin, "rep");
	}

	//On parcourt la liste des fichiers trouvés
	for (; recup_premier_fichier(nom, longChemin, courtChemin, "fichiers"); ) {
		if (lstat(longChemin, &s) == ERR) {
			printf(ROUGE("ls: impossible to access to '%s': No such file or directory\n"), argv[i]);
			continue;
		}
		
		//On affiche les droits					
		recup_droits(droits, s);
		printf("%s ", droits);
		
		//On affiche la deuxième information
		printf("%3d ", (int)s.st_nlink);
		
		//On affiche l'utilisateur
		recup_utilisateur(user, s);
		printf("%s ", user);
		
		//On affiche le groupe
		recup_groupe(groupe, s);
		printf("%s ", groupe);
		
		//On affiche la taille
		printf("%8d ", (int)s.st_size);
		
		//On affiche la date
		recup_date(date, s);
		printf("%s ", date);
		
		//On affiche le nom du fichier en fonction de son type
		if (droits[0] == '-') {
			printf("%s\n", nom);
		}
		else if (droits[0] == 'l') {
			printf(CYAN("%s\n"), nom);
		}
		
		//On enlève le fichier de la liste des fichiers
		enleve_premier_fichier("fichiers");
	}
	
	//On passe une ligne si besoin
	if (nbFichiers && nbDocs) {
		printf("\n");
	}
	
	//On parcourt la liste des répertoires trouvés
	for (; recup_premier_fichier(nom, longChemin, courtChemin, "rep"); ) {
		//On analyse ce dossier
		analyse_dossier(longChemin, courtChemin);
		if ((nbFichiers > 1)) {
			printf("\n");
		}
		
		//On enlève le dossier de la liste des dossiers
		enleve_premier_fichier("rep");
	}    
	
	//Détruire les listes
	detruire_listes();
	return 0;
}
