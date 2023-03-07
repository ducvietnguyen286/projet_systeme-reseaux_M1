
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <math.h>

#define ERR -1
#define MAX_SIZE_CHEMIN 4096 //Taille de path 
#define MAX_SIZE_USER 64//Taille d'un nom d'utilisateur
#define MAX_SIZE_PID 8//Taille max d'un pid
#define MAX_SIZE_MEM 10//Taille max memoire
#define MAX_SIZE_RSS 1024//Taile max RSS
#define MAX_SIZE_FILE 10000//Taille pour lire un fichier
#define MAX_SIZE_COMMAND 1024//Taile max Command
#define MAX_SIZE_ETAT 5//Taille max d'un etat

//RECUPERER LE CHAMP 'USER'
//GRACE AU UID CONTENUE DANS LA STRUCT STAT
void recuperer_user(char * user, uid_t uid_user) {
	struct passwd * info_user;

	info_user = getpwuid(uid_user);//Recuperation des infos user uid
	strcpy(user,info_user->pw_name);//On copie son nom dans la variable associé
}

//RECUPERER LE CHAMP 'RSS'
//SE SITUE DANS LE DOSSIER /statm 2e champ
long int recuperer_rss(char * chemin) {
	FILE * fd;//File descriptor du fichier
	char buffer[MAX_SIZE_FILE];
	char * champ;

	//CHEMIN /proc/i/STATS
	char chemin_rss[MAX_SIZE_CHEMIN];//Contient le chemin pour accéder proc/i/statm
	strcpy(chemin_rss,chemin);
	strcat(chemin_rss,"/statm");
	//printf("Voici le dossier :%s\n",chemin_rss);

	//OUVRIR LE FICHIER
	fd = fopen(chemin_rss,"r");//Ouverture en lecture
	if (fd == NULL) perror("Open file failed"),exit(2);

	fgets(buffer,MAX_SIZE_FILE,fd);//Récupération de la ligne du fichier
	champ = strtok(buffer," ");//Les champs sont délimité par des espaces
	champ = strtok(NULL," ");//Récupération du 2e champs
	fclose(fd);

	return atol(champ)*4;
}


//RECUPERER LE CHAMP 'VSZ'
//SE SITUE DANS LE DOSSIER /statm 1e champ
long int recuperer_vsz(char * chemin) {
	FILE * fd;//File descriptor du fichier
	char buffer[MAX_SIZE_FILE];
	char * champ;

	//CHEMIN /proc/i/STATS
	char chemin_rss[MAX_SIZE_CHEMIN];//Contient le chemin pour accéder proc/i/statm
	strcpy(chemin_rss,chemin);
	strcat(chemin_rss,"/statm");
	//printf("Voici le dossier :%s\n",chemin_rss);

	//OUVRIR LE FICHIER
	fd = fopen(chemin_rss,"r");//Ouverture en lecture
	if (fd == NULL) perror("Open file failed"),exit(2);

	fgets(buffer,MAX_SIZE_FILE,fd);//Récupération de la ligne du fichier
	champ = strtok(buffer," ");//Les champs sont délimité par des espaces
	fclose(fd);

	return atol(champ)*4;
}

//RECUPERER LE CHAMP %MEM
float recuperer_mem(long rss) {
	FILE * fd;
	char buffer[MAX_SIZE_FILE];
	char * champ;
	long int memoire_max;

	//CHEMIN /proc/meminfo
	fd = fopen("/proc/meminfo","r");//Ouverture en lecture
	if (fd == NULL) perror("Open file failed"),exit(2);

	fgets(buffer,MAX_SIZE_FILE,fd);//Recupération de la 1ere ligne
	fclose(fd);
	champ = strtok(buffer," ");
	champ = strtok(NULL," ");

	memoire_max = atol(champ);

	float mem = (float)rss / (float)memoire_max * (float)100;
	mem = mem * 10;
	mem = floorf(mem);
	mem = mem / 10;
	
	return mem;
}

//RECUPERER ETAT
void recuperer_etat(char * etat, char * chemin) {
	FILE * fd;//File descriptor du fichier
	char buffer[MAX_SIZE_FILE];
	char * champ;

	//Chemin /proc/pid/STAT
	char chemin_etat[MAX_SIZE_CHEMIN];//Contient le chemin pour accéder proc/i/stat
	strcpy(chemin_etat,chemin);
	strcat(chemin_etat,"/stat");
	//printf("Voici le dossier :%s\n",chemin_etat);

	//OUVRIR LE FICHIER
	fd = fopen(chemin_etat,"r");//Ouverture en lecture
	if (fd == NULL) perror("Open file failed"),exit(2);

	fgets(buffer,MAX_SIZE_FILE,fd);//Recupération de la ligne du fichier
	// Ligne recup : iiiii (command) etat iii iii ii iii 
	fclose(fd);

	champ = strtok(buffer," ");//Recupération du 1er champ
	champ = strtok(NULL," ");//Recupération 2e champ
	champ = strtok(NULL," ");//Recuperation 3e champ

	strcpy(etat,champ);
}

//RECUPERER COMMAND
void recuperer_command(char * command, char * chemin) {
	FILE * fd;//File descriptor du fichier
	char buffer[MAX_SIZE_FILE];

	//CHEMIN /proc/i/CMDLINE
	char chemin_cmd[MAX_SIZE_CHEMIN];//Contient le chemin pour accéder proc/i/cmdline
	strcpy(chemin_cmd,chemin);
	strcat(chemin_cmd,"/cmdline");

	//OUVRIR LE FICHIER
	fd = fopen(chemin_cmd,"r");//Ouverture en lecture
	if (fd == NULL) perror("Open file failed"),exit(2);

	if (fgets(buffer,MAX_SIZE_FILE,fd) != NULL) {//Récupération de la ligne du fichier
		fclose(fd);
		strcpy(command,buffer);
		return;
	}
	fclose(fd);

	//Sinon il faut aller la chercher autre part
	chemin_cmd[0] = '\0';//On réinitialise le chemin du fichier
	strcpy(chemin_cmd,chemin);
	strcat(chemin_cmd,"/stat");// -->/proc/pid/stat

	//OUVRIR LE FICHIER
	fd = fopen(chemin_cmd,"r");//Ouverture en lecture
	if (fd == NULL) perror("Open file failed"),exit(2);

	fgets(buffer,MAX_SIZE_FILE,fd);//Recupération de la ligne du fichier
	// Ligne recup : iiiii (command) iiiiii iii iii ii iii 
	fclose(fd);
	char * champ = strtok(buffer,")");//On obtient 'iiiii (command'
	strcpy(buffer,champ);
	champ = strtok(buffer,"(");//Ici on a 'iiiii '
	champ = strtok(NULL,"(");// Ici on a 'command'
	strcpy(command,champ);//On le stocke
}

void getDebutCouleur(char c) {
	switch(c) {
		case 'S':
			printf("\033[01;31m");
			break;//ROUGE
		case 'R':
			printf("\033[1;36m");
			break;//CYAN
		case 'D':
			printf("\033[1;35m");
			break;//VIOLET
		case 'T':
			printf("\033[1;33m");
			break;//JAUNE
		case 'Z':
			printf("\033[01;32m");
			break;//VERT
		case 'I':
			printf("\033[1;34m");
			break;
	}
}

void getFinCouleur() {
	printf("\033[0m");
}

int main(void) {
	char chemin[MAX_SIZE_CHEMIN];//Contient le chemin pour accéder au processus i
	char user_name[MAX_SIZE_USER];//Stocker le nom du user
	char pid[MAX_SIZE_PID];//Stocker le PID du processus
	char memoire[MAX_SIZE_MEM];//Stocker le % de mémoire utilisé
	long int rss;//Stocker le RSS
	long int vsz;//Stocker le VSZ
	float mem;//Stocker le %mem
	char command[MAX_SIZE_COMMAND];//stocker la commande utilise
	char etat[MAX_SIZE_ETAT];//stocker l'etat du proc
	DIR * dossier_proc;//Contient le dossier '/proc'
	struct dirent * dossier_suivant;//Permet de naviguer entre les dossier d'un dossier
	struct stat stat_dossier;//Recuperer des informations sur un fichier

	//ACCEDER A /PROC
	dossier_proc = opendir("/proc");//On accède au dossier où sont stocké les processus
	if (dossier_proc == NULL) perror("Error to access /proc"),exit(1);

	//AFFICHAGE EN-TETE
	printf("%-10s %6s %4s %4s %10s %10s %-3s %-4s %-6s %-6s %s\n","USER","PID","%CPU","%MEM","VSZ","RSS","TTY","STAT","START","TIME","COMMAND");	
	
	//LIRE TOUS LES PROCESSUS
	while(dossier_suivant = readdir(dossier_proc)) {//On accède au info de chaque fichier du dossier proc
		int i;
		if ((i=atoi(dossier_suivant->d_name)) > 0) { //Si c'est un dossier contenant un processus (car chaque processus a un dosssier qui a comme nom son numéro de pid)
			//printf("I:%d\n",i);
			//if(i != 275) continue;
			//RECUPERER CHEMIN DU PROCESSUS i
			strcpy(chemin,"/proc/");//On va stocker le chemin pour accéder au processus i -> ici on a juste '/proc/'
			strcat(chemin,dossier_suivant->d_name);//Puis on ajoute le nom du dossier du processus i -> '/proc/i'

			//INFO DU FICHIER
			stat(chemin,&stat_dossier);//Recuperer infos du dossier

			//printf("Voici mon dossier : '%s'\n",chemin);
			//printf("UID User : %d\n",(int)stat_dossier.st_uid);

			//RECUPERER ETAT
			recuperer_etat(etat,chemin);
			getDebutCouleur(etat[0]);

			//printf("ETAT : '%s'\n",etat);

			//RECUPERER LE NOM DU USER
			recuperer_user(user_name,stat_dossier.st_uid);
			//printf("Nom User : %s\n",user_name);
			printf("%-10s ",user_name);

			//RECUPERER LE PID DU PROCESSUS
			strcpy(pid,dossier_suivant->d_name);
			//printf("PID : %s\n",pid);
			printf("%6s ",pid);

			//RECUPERER %CPU
			//PAS REUSSI
			printf("%4s ","0.0");

			//RECUPERER VSZ
			vsz = recuperer_vsz(chemin);
			//printf("VSZ : %ld\n",vsz);

			//RECUPERER RSS
			rss = recuperer_rss(chemin);
			//printf("RSS : %ld\n",rss);

			//RECUPERER %MEM
			mem=recuperer_mem(rss);
			//printf("%%MEM : %.1f\n",mem);
			printf("%4.1f ",mem);
			printf("%10ld ",vsz);
			printf("%10ld ",rss);

			//RECUPERER TTY
			//PAS REUSSI
			printf("%-3s ","?");

			printf("%-4s ",etat);

			//RECUPERER START
			//PAS REUSSI
			printf("%-6s ","00:00");

			//RECUPERER TIME
			//PAS REUSSI
			printf("%-6s ","0:00");

			//RECUPERER COMMAND
			recuperer_command(command,chemin);
			//printf("Command : %s\n",command);
			printf("%s\n",command);

			getFinCouleur();
			//printf("INI\n");
			//break;
		}
	}
	exit(0);
}
