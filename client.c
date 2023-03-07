

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <crypt.h>
#include <unistd.h>

#define MAX_CHAR 4294967
#define ERR -1

#define syerror(x)	perror(errormsg[x])
#define fatalsyerror(x)	syerror(x),exit(x)

#define ROUGE(m)	"\033[01;31m"m"\033[0m"
#define VERT(m)		"\033[01;32m"m"\033[0m"

char * errormsg[]= {
	"No error", //0
	ROUGE("Impossible de créer une socket"), //1
	ROUGE("Erreur sur bind"),//2
	ROUGE("Erreur sur listen") //3
	ROUGE("Erreur sur accept") //4
};

int main(){
	int sockClient,c,s,bytes_r,m;
	int interrupt = 1;
	char fname[50],ipServeur[25],donnee_recue[1024],nom_user[MAX_CHAR],op[MAX_CHAR],donnee_envoyee[1024],*op1;
	char* mdp_user = (char*)malloc(sizeof(char)*MAX_CHAR);
	struct sockaddr_in addrClient;
	struct hostent *he;

	FILE *fp;
	
	//Soit 0, soit 0.0.0.0 soit 127.0.0.1
	printf("Entrer l'adresse IP du serveur : ");
	scanf("%s",ipServeur);
	
	he = gethostbyname(ipServeur);
	
	if((sockClient = socket(AF_INET, SOCK_STREAM, 0)) == ERR){
		fatalsyerror(1);		
	}

	addrClient.sin_family = AF_INET;     
    addrClient.sin_port = htons(10165);    // Port pour se connecter au serveur
    addrClient.sin_addr = *((struct in_addr *)he->h_addr);

	if((connect(sockClient,(struct sockaddr *)&addrClient,sizeof(struct sockaddr))) == ERR){
		fprintf(stderr, "Impossible de connecter la socket\n");
		exit(5);
	}
		
	while(interrupt){
		printf("\n~>myssh ");//Prompt pour taper le nom d'utilisateur
		scanf("%s",nom_user);
		send(sockClient,nom_user,strlen(nom_user), 0);//Envoie du nom de l'utilisateur
		mdp_user = getpass("Mot de passe: ");
		op1 = crypt(mdp_user,nom_user);
		strcpy(mdp_user,op1);
		send(sockClient,mdp_user,strlen(mdp_user),0);//Envoie du mot de passe haché de l'utilisateur

		bytes_r = recv(sockClient,donnee_recue,1024,0);//Reception de la reponse du serveur (Connexion reussie ou echouée)
		donnee_recue[bytes_r] = '\0';
		int first = 1;
		char* p;
		if(donnee_recue[0] == 'B'){ //B de Bienvenue si le mot de passe est correct et la connexion est établie
			printf("%s",donnee_recue);
			getchar();
			while(1){
				fflush(stdout);
				if (first) {
					printf("\n%s@%s:~> ",nom_user,ipServeur); //Prompt pour taper les commandes
				}
				if(strcmp(nom_user,"admin") == 0){ //Si c'est l'administarteur du réseau
					scanf("%s",donnee_envoyee);
					send(sockClient,donnee_envoyee,strlen(donnee_envoyee), 0);
					if(strcmp(donnee_envoyee,"exit")==0) //L'administrateur quitte la connexion
						break;
					else if(strcmp(donnee_envoyee,"ajoutuser") == 0){//Ajout d'un nouvel utilisateur par l'administrateur
						printf("\nTaper le nom du nouvel utilisateur \n%s@%s: ",nom_user,ipServeur);	
						scanf("%s",donnee_envoyee);
						send(sockClient,donnee_envoyee,strlen(donnee_envoyee), 0);
					
						bytes_r = recv(sockClient,donnee_recue,1024,0);	
						donnee_recue[bytes_r] = '\0';
					
						if(strcmp(donnee_recue,"L'utilisateur existe déjà!") != 0){
							printf(" \n%s@%s: ",nom_user,ipServeur);	
							mdp_user = getpass("Taper le mot de passe du nouvel utilisateur\n");
							op1 = crypt(mdp_user,donnee_envoyee);
							strcpy(donnee_envoyee,op1);						
							send(sockClient,donnee_envoyee,strlen(donnee_envoyee), 0);
							printf("%s\n",VERT("Ajout réussi"));
						}
						else printf("%s\n",donnee_recue);				
					}
					else printf("%s\n",ROUGE("Commande invalide"));			
				}
				else{		//non-administrateur
					bytes_r = recv(sockClient,donnee_recue,1024,0);
					donnee_recue[bytes_r] = '\0';
					if (strcmp(donnee_recue,"null") != 0) {
						printf("%s",donnee_recue);
						if (strncmp(donnee_recue,"exec:",5) == 0) {
							printf("\nEXECUTION\n");
							p = donnee_recue;
							p += 5;
							if (strncmp(p,"exit",4) == 0) {
								printf("AUREVOIR ET A BIENTOT\n");
								break;
							}
						}
					}
					if (first) {
						strcpy(donnee_envoyee,nom_user);
						strcat(donnee_envoyee,"@");
						strcat(donnee_envoyee,ipServeur);
						strcat(donnee_envoyee,":~");
						send(sockClient,donnee_envoyee,strlen(donnee_envoyee), 0);
						sleep(1);
						strcpy(donnee_envoyee,"null");
						first = 0;
					} else {
						fgets(donnee_envoyee,MAX_CHAR-1,stdin); /*récupération commande*/
						if (strlen(donnee_envoyee) > 1) {
							donnee_envoyee[strlen(donnee_envoyee)-1] = '\0'; //on remplace le \n par \0
						} else {
							strcpy(donnee_envoyee,"null");
						}
					}
					send(sockClient,donnee_envoyee,strlen(donnee_envoyee), 0);//sending cmd
				}		
			}	
		}
		else{ // Si le mot de passe est incorrect
			printf("%s",donnee_recue);
		}
		interrupt = 0;
	}	
	close(sockClient);
	return 0;
}

		
