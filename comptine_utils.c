#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "comptine_utils.h"

int read_until_nl(int fd, char *buf)
{
	// Initialise le nombre d'octets lus à 0
	int nb_lus = 0;
	// Lit les octets dans le fichier de descripteur fd et les met dans buf jusqu'au prochain '\n' rencontré, y compris
	while (read(fd, buf + nb_lus, 1) > 0 && buf[nb_lus] != '\n') {
		nb_lus++;
	}
	return nb_lus;
}

int est_nom_fichier_comptine(char *nom_fich)
{
	// Initialise la taille du nom du fichier
	int taille = strlen(nom_fich);
	// Retourne 1 si ta la taille est supérieure à 4 et que les 4 derniers caractères sont ".cpt"
	if (taille > 4 && strcmp(nom_fich + taille - 4, ".cpt") == 0) {
		return 1;
	}
	return 0;
}

struct comptine *init_cpt_depuis_fichier(const char *dir_name, const char *base_name)
{
	// Allocation de la struct comptine
	struct comptine *comptine = malloc(sizeof(struct comptine));
	if (comptine == NULL) {
		perror("malloc comptine");
		return NULL;
	}
	// Allocation du titre
	char *titre = malloc(256);
	if (titre == NULL) {
		perror("malloc titre");
		free(comptine);
		return NULL;
	}
	// Allocation du nom de fichier
	char *nom_fichier = malloc(strlen(base_name) + 1);
	if (nom_fichier == NULL) {
		perror("malloc nom_fichier");
		free(comptine);
		free(titre);
		return NULL;
	}
	// Allocation du chemin
	char *path = malloc(strlen(dir_name) + strlen(base_name) + 2);
	if (path == NULL) {
		perror("malloc path");
		free(comptine);
		free(nom_fichier);
		free(titre);
		return NULL;
	}

	// Concaténation du chemin
	strcpy(path, dir_name);
	strcat(path, "/");
	strcat(path, base_name);

	// Ouverture du fichier
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		free(comptine);
		free(titre);
		free(nom_fichier);
		free(path);
		return NULL;
	}
	// Lecture du titre
	if (read_until_nl(fd, titre) < 0) {
		perror("read_until_nl");
		free(comptine);
		free(titre);
		free(nom_fichier);
		free(path);
		close(fd);
		return NULL;
	}

	// Initialisation de la struct comptine
	comptine->titre = titre;
	strcpy(nom_fichier, base_name);
	comptine->nom_fichier = nom_fichier;
	
	// Libération de la mémoire
	free(path);
	close(fd);
	return comptine;
}

void liberer_comptine(struct comptine *cpt)
{
	// Libère le titre et le nom de fichier
	free(cpt->titre);
	free(cpt->nom_fichier);
	// Libère la struct comptine
	free(cpt);
}


struct catalogue *creer_catalogue(const char *dir_name)
{	
	// Allocation du catalogue
	struct catalogue *c = malloc(sizeof(struct catalogue));
	if (c == NULL) {
		perror("malloc catalogue");
		return NULL;
	}

	// Ouverture du répertoire
	DIR *dir = opendir(dir_name);
	if (dir == NULL) {
		perror("opendir");
		free(c);
		return NULL;
	}

	// Compte le nombre de fichiers comptine
	struct dirent *ent;
	int cpt = 0;
	while ((ent = readdir(dir)) != NULL) {
		if (est_nom_fichier_comptine(ent->d_name)) {
			cpt++;
		}
	}
	c->nb = cpt;

	// Allocation du tableau de comptines
	c->tab = malloc(cpt * sizeof(struct comptine *));
	if (c->tab == NULL) {
		perror("malloc tab");
		free(c);
		closedir(dir);
		return NULL;
	}

	// Revient au début du répertoire
	rewinddir(dir);

	// Initialisation du tableau de comptines
	int i = 0;
	while ((ent = readdir(dir)) != NULL) {
		if (est_nom_fichier_comptine(ent->d_name)) {
			struct comptine *cpt = init_cpt_depuis_fichier(dir_name, ent->d_name);
			c->tab[i] = cpt;
			i++;
		}
	}
	// Fermeture du répertoire
	closedir(dir);
	return c;
}

void liberer_catalogue(struct catalogue *c)
{
	// Libère les cases du tableau de comptines
	for (int i = 0; i < c->nb; i++) {
		liberer_comptine(c->tab[i]);
	}
	free(c->tab);
	free(c);
}
