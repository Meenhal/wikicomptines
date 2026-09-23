# Wikicomptines

Un client et un serveur TCP en C qui s'échangent des comptines grâce à un petit
protocole maison, **WCP** (*Wikicomptine Protocol*).

Projet de programmation réseau réalisé en mai 2024 pendant ma licence
d'informatique. Le code est conservé tel qu'il a été rendu.

## Principe

Le serveur garde un répertoire de comptines (des fichiers texte `.cpt`). Quand
un client se connecte, le serveur lui envoie la liste des titres disponibles.
L'utilisateur en choisit une, le client transmet son numéro, et le serveur
renvoie le texte de la comptine avant de fermer la connexion.

```text
client                                   serveur (port 4321)
  |  ------------- connexion TCP ------------->  |
  |  <------ liste numérotée des titres -------  |
  |  ---- numéro choisi (2 octets, réseau) --->  |
  |  <---------- texte de la comptine ---------  |
  |                                  fermeture   |
```

## Aperçu

```text
$ ./wcp_clt 127.0.0.1
     1 Les petits poissons dans l'eau
     2 La famille tortue
     3 Dans sa maison, un grand cerf
     4 G comme Gaston
     5 Petit escargot
     6 Mon petit lapin
Il y a 6 comptines disponibles
Quelle comptine voulez-vous ? (Entrer un entier entre 1 et 6) : 3
Dans sa maison, un grand cerf

Dans sa maison, un grand cerf
Regardait par la fenêtre
Un lapin venir à lui
Et frapper ainsi

Cerf, cerf, ouvre-moi
Sinon le chasseur me tuera
Lapin, lapin, entre et viens
Me serrer la main
```

## Lancer le projet

Il faut un compilateur C et un système POSIX (Linux ou macOS).

```bash
make        # compile wcp_srv et wcp_clt
make clean  # supprime les fichiers compilés
```

Dans un premier terminal, démarrer le serveur en lui donnant le répertoire des
comptines :

```bash
./wcp_srv comptines/
```

Dans un second terminal, lancer le client avec l'adresse IPv4 du serveur :

```bash
./wcp_clt 127.0.0.1
```

Le serveur tourne en boucle et sert les clients les uns après les autres ;
Ctrl+C l'arrête.

## Le protocole WCP

| Étape | Sens               | Contenu                                                                                          |
| :---: | ------------------ | ------------------------------------------------------------------------------------------------ |
|   1   | serveur → client   | une ligne par comptine : son numéro en décimal sur 6 caractères, une espace, le titre et `'\n'` |
|   2   | serveur → client   | une ligne vide, qui marque la fin de la liste                                                    |
|   3   | client → serveur   | le numéro choisi, entier non signé sur 2 octets en *network byte order*                          |
|   4   | serveur → client   | le contenu du fichier de la comptine, titre compris                                              |

## Ajouter une comptine

Un fichier comptine porte l'extension `.cpt`. Sa première ligne est le titre,
qui apparaît dans la liste envoyée au client. Il suffit de déposer un nouveau
fichier dans [`comptines/`](comptines) puis de relancer le serveur.

## Organisation du code

| Fichier                                                            | Rôle                                                                            |
| ------------------------------------------------------------------ | ------------------------------------------------------------------------------- |
| [`wcp_srv.c`](wcp_srv.c)                                           | serveur : socket d'écoute, envoi de la liste, réception du choix, envoi du texte |
| [`wcp_clt.c`](wcp_clt.c)                                           | client : connexion, affichage de la liste, saisie du numéro, affichage du texte  |
| [`comptine_utils.c`](comptine_utils.c), [`.h`](comptine_utils.h)   | lecture des fichiers `.cpt` et construction du catalogue                         |
| [`comptines/`](comptines)                                          | les six comptines servies par défaut                                             |

## Licence

Distribué sous licence MIT, voir [`LICENSE`](LICENSE).
