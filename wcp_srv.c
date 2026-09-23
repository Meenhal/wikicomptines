/* fichiers de la bibliothèque standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
/* bibliothèque standard unix */
#include <unistd.h> /* close, read, write */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <errno.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
/* spécifique aux comptines */
#include "comptine_utils.h"

#define PORT_WCP 4321
#define BUF_SIZE 256

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s repertoire_comptines\n"
			"serveur pour WCP (Wikicomptine Protocol)\n"
			"Exemple: %s comptines\n", nom_prog, nom_prog);
}
/** Retourne en cas de succès le descripteur de fichier d'une socket d'écoute
 *  attachée au port port et à toutes les adresses locales. */
int creer_configurer_sock_ecoute(uint16_t port);

/** Écrit dans le fichier de desripteur fd la liste des comptines présents dans
 *  le catalogue c comme spécifié par le protocole WCP, c'est-à-dire sous la
 *  forme de plusieurs lignes terminées par '\n' :
 *  chaque ligne commence par le numéro de la comptine (son indice dans le
 *  catalogue) commençant à 0, écrit en décimal, sur 6 caractères
 *  suivi d'un espace
 *  puis du titre de la comptine
 *  une ligne vide termine le message */
void envoyer_liste(int fd, struct catalogue *c);

/** Lit dans fd un entier sur 2 octets écrit en network byte order
 *  retourne : cet entier en boutisme machine. */
uint16_t recevoir_num_comptine(int fd);

/** Écrit dans fd la comptine numéro ic du catalogue c dont le fichier est situé
 *  dans le répertoire dirname comme spécifié par le protocole WCP, c'est-à-dire :
 *  chaque ligne du fichier de comptine est écrite avec son '\n' final, y
 *  compris son titre, deux lignes vides terminent le message */
void envoyer_comptine(int fd, const char *dirname, struct catalogue *c, uint16_t ic);

int main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}

	// On crée le catalogue
	struct catalogue *cat = creer_catalogue(argv[1]);
	if (cat == NULL) {
		printf("Erreur lors de la création du catalogue.\n");
		return 1;
	}
	
	// On crée et configure la socket d'écoute
	int sock = creer_configurer_sock_ecoute(PORT_WCP);

	// On attend les connexions
	for(;;) {
		int s = accept(sock, NULL, NULL);
		if (s < 0) {
			perror("Erreur accept");
			close(sock);
			return 1;
		}
		// On envoie la liste des comptines dès qu'il y a une connexion
		envoyer_liste(s, cat);
		// On reçoit le numéro de la comptine
		uint16_t ic = recevoir_num_comptine(s);
		// On envoie la comptine correspondante
		envoyer_comptine(s, argv[1], cat, ic);
		// On ferme la connexion
		close(s);
	}
	close(sock);
	return 0;
}

int creer_configurer_sock_ecoute(uint16_t port)
{
	// Crée une socket TCP/IPv4
	int sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Erreur création socket");
		return -1;
	}

	// Création de la sockaddr locale
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY
	};

	// Réutilisation de l'adresse locale
	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
	
	// Attache la socket à l'adresse
	if (bind(sock, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("Erreur bind");
		close(sock);
		return -1;
	}

	// Met la socket en mode écoute
	if (listen(sock, 128) < 0) {
		perror("Erreur listen");
		close(sock);
		return -1;
	}
	return sock;
}

void envoyer_liste(int fd, struct catalogue *c)
{	
	for (int i = 0; i < c->nb; i++) {
		// Le numéro de la comptine
		dprintf(fd, "% 6d ", i + 1);
		// Le titre de la comptine
		write(fd, c->tab[i]->titre, strlen(c->tab[i]->titre));
	}
	// On envoie une ligne vide
	write(fd, "\n", 1);
}

uint16_t recevoir_num_comptine(int fd)
{
	uint16_t nc;
	// On lit un entier sur 2 octets écrit en network byte order
	read(fd, &nc, sizeof(nc));
	nc = ntohs(nc);
	return nc;
}

void envoyer_comptine(int fd, const char *dirname, struct catalogue *c, uint16_t ic)
{
	// On ouvre le répertoire dirname
	DIR *dir = opendir(dirname);
	if (dir == NULL) {
		perror("opendir");
		exit(1);
	}

	struct dirent *ent;
	char * comptine = c->tab[ic - 1]->nom_fichier;

	// On alloue de la mémoire pour le chemin du fichier
	char * path = malloc(strlen(dirname) + strlen(comptine) + 2);
	if (path == NULL) {
		perror("malloc");
		exit(1);
	}
	strcpy(path, dirname);
	strcat(path, "/");

	// On cherche le fichier de la comptine
	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, comptine) == 0) {
			int f = open(strcat(path, ent->d_name), O_RDONLY);
			if (f < 0) {
				perror("open");
				exit(1);
			}
			// On lit le fichier et on l'envoie
			char buf[BUF_SIZE];
			int n = read(f, buf, BUF_SIZE);
			write(fd, buf, n);
			close(f);
			break;
		}
	}
	free(path);
	closedir(dir);
}