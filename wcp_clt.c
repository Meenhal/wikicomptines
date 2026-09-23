/* fichiers de la bibliothèque standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
/* bibliothèque standard unix */
#include <unistd.h> /* close, read, write */
#include <sys/types.h>
#include <sys/socket.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
/* spécifique aux comptines */
#include "comptine_utils.h"

#define PORT_WCP 4321
#define BUF_SIZE 256

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s addr_ipv4\n"
			"client pour WCP (Wikicomptine Protocol)\n"
			"Exemple: %s 208.97.177.124\n", nom_prog, nom_prog);
}

/** Retourne (en cas de succès) le descripteur de fichier d'une socket
 *  TCP/IPv4 connectée au processus écoutant sur port sur la machine d'adresse
 *  addr_ipv4 */
int creer_connecter_sock(char *addr_ipv4, uint16_t port);

/** Lit la liste numérotée des comptines dans le descripteur fd et les affiche
 *  sur le terminal.
 *  retourne : le nombre de comptines disponibles */
uint16_t recevoir_liste_comptines(int fd);

/** Demande à l'utilisateur un nombre entre 0 (compris) et nc (non compris)
 *  et retourne la valeur saisie. */
uint16_t saisir_num_comptine(uint16_t nb_comptines);

/** Écrit l'entier ic dans le fichier de descripteur fd en network byte order */
void envoyer_num_comptine(int fd, uint16_t nc);

/** Affiche la comptine arrivant dans fd sur le terminal */
void afficher_comptine(int fd);

int main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}

	// Crée une socket connectée au serveur
	int sock = creer_connecter_sock(argv[1], PORT_WCP);
	
	// Lit la liste des comptines et les affiche
	uint16_t nb = recevoir_liste_comptines(sock);
	printf("Il y a %" PRIu16 " comptines disponibles\n", nb);

	// Demande à l'utilisateur de choisir une comptine à afficher
	uint16_t n = saisir_num_comptine(nb);

	// Envoie le numéro de la comptine choisie au serveur
	envoyer_num_comptine(sock, n);

	// Affiche la comptine choisie
	afficher_comptine(sock);

	close(sock);
	return 0;
}

int creer_connecter_sock(char *addr_ipv4, uint16_t port)
{
	// Crée une socket TCP/IPv4
	int sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Erreur création socket");
		return -1;
	}

	// Création de la sockaddr distante
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_port = htons(port)
	};
	
	// Convertit l'adresse IPv4 en binaire
	if (inet_pton(AF_INET, addr_ipv4, &(sa.sin_addr)) <= 0) {
		fprintf(stderr, "adresse ipv4 non valable\n");
		close(sock);
		return -1;
	}

	// Demande de connexion TCP 
	socklen_t sl = sizeof(sa);
	if (connect(sock, (struct sockaddr *)&sa, sl) < 0) {
		perror("Erreur connexion serveur");
		close(sock);
		return -1;
	}

	// Retourne le descripteur de fichier de la socket connectée
	return sock;
}

uint16_t recevoir_liste_comptines(int fd)
{
	// Liste les comptines disponibles
	uint16_t nb_comptines = 0;
	char buf[BUF_SIZE];
	int n;

	// Tant qu'on peut lire une ligne, on l'affiche et on incrémente le nombre de comptines
	while ((n = read_until_nl(fd, buf)) > 0) {
		buf[n] = '\n';
		buf[n + 1] = '\0';
		printf("%s", buf);
		nb_comptines++;
	}
	return nb_comptines;
}

uint16_t saisir_num_comptine(uint16_t nb_comptines)
{
	uint16_t n;
	do
	{
		// Demande à l'utilisateur de choisir une comptine
		printf("Quelle comptine voulez-vous ? (Entrer un entier entre 1 et %" PRIu16 ") : ", nb_comptines);
		scanf("%" SCNu16, &n);
		if (n <= 0 || n > nb_comptines) {
			printf("Nombre invalide\n");
		}
	} while (n <= 0 || n > nb_comptines);
	return n;
}

void envoyer_num_comptine(int fd, uint16_t nc)
{	
	// On envoie un entier sur 2 octets écrit en network byte order
	nc = htons(nc);
	write(fd, &nc, sizeof(uint16_t));
}

void afficher_comptine(int fd)
{
	// On lit la comptine et on l'affiche
	char buf[BUF_SIZE];
	int n = read(fd, buf, BUF_SIZE - 1);
	if (n < 0) {
		n = 0;
	}
	// On termine la chaîne pour que printf s'arrête à la fin de la comptine
	buf[n] = '\0';
	printf("%s", buf);
}
