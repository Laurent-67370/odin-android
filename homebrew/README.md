# Homebrew pour la SEGA Hikaru (sous Odin)

Un jeu fait maison qui tourne sur la borne **SEGA Hikaru** telle qu'Odin
l'émule : un vrai programme pour son processeur SH-4, lancé par le BIOS
SEGA comme les jeux d'origine.

Pour l'instant : **une démo**. Un carré se déplace au stick (ou aux flèches)
et change de couleur avec les boutons 1, 2 et 3 ; START change la couleur du
fond.

![La démo : le carré poussé à droite, puis rouge avec le bouton 1](docs/demo.png)

## Construire

```sh
sudo apt install gcc-sh4-linux-gnu      # le compilateur C pour SH-4
python3 build.py                        # → build/odinhb.zip et build/odinhb.json
python3 build.py --games chemin/vers/odin-games.json   # ajoute aussi le jeu à la liste d'Odin
```

`build/odinhb.zip` contient les deux EPROM du jeu (`odinhb.ic29` et
`odinhb.ic30`). Il se met dans le dossier des ROM d'Odin, à côté de
`hikaru.zip` et `mie.zip`. Le jeu s'appelle **Odin Homebrew** dans la liste.

La ROM ne contient **aucun code de SEGA**. Il faut le BIOS de la borne
(`hikaru.zip`, version 0.96) et `mie.zip`, comme pour les autres jeux Hikaru.

## Comment ça marche

### Le démarrage (le BIOS SEGA)

Le BIOS cherche la carte ROM et la lance s'il y trouve :

| Où (dans les EPROM) | Quoi |
|---|---|
| `0x000` | `SAMURAI\0` |
| `0x13C` | 20 entrées de 10 octets décrivant les puces : la taille d'une paire (log2 en Mo), puis des sommes ; `FFFF` = absente |
| `0x390` | la table de chargement : des triplets (adresse sur le bus, adresse en mémoire, longueur), terminée par un triplet dont le premier mot n'est pas positif ; son deuxième mot est le point d'entrée |

Le BIOS copie le programme (ici à `0x8C100000`) et y saute. La carte de
Brave Firefighters, sans puce de chiffrement, sert de modèle (`eprom_mb: 2`).

### L'image : la couche 2D

La Hikaru a deux couches 2D par-dessus (ou sous) la 3D. La couche 1 montre
une zone de la mémoire d'image (`1B000000`, 2048×2048 pixels de 16 bits) :
chaque pixel s'y écrit directement, en ABGR1555, deux pixels par mot de 32
bits, échangés. Les registres de la puce d'affichage (`1A000190` et
suivants) disent où est la couche, son format, si elle est visible et
au-dessus de la 3D.

### Les commandes : le MIE et le JVS

Les commandes passent par le **MIE**, un Z80 qui parle à la carte
d'entrées-sorties en JVS. Au lancement, le jeu y charge le programme JVS du
BIOS de la borne (copié depuis le BIOS, pas inclus dans la ROM), puis
dialogue par une boîte aux lettres en mémoire du Z80 :

- `C004 = 3` : le programme est prêt (après une dizaine de secondes : il
  initialise la carte JVS) ;
- `C001 = 0` : il est libre ; on écrit alors la commande 1 dans `C000` et 1
  dans `C001` ;
- quand `C001` revient à 0, les boutons sont en `E003` (la borne : TEST =
  bit 7) et `E004`–`E005` (joueur 1, ordre JVS : START, SERVICE, haut, bas,
  gauche, droite, bouton 1, bouton 2, puis boutons 3 à 8).

## Les fichiers

| Fichier | Rôle |
|---|---|
| `src/hikaru.h`, `src/hikaru.c` | la borne : couche 2D, image suivante, commandes |
| `src/main.c` | la démo |
| `src/crt0.S`, `src/link.ld` | le démarrage du programme et son placement en mémoire |
| `build.py` | compile, fabrique les EPROM et l'entrée de la liste des jeux |

## Limites connues

- Le programme JVS est pris à l'adresse qu'il a dans le BIOS **0.96**. Les
  BIOS 0.92 et 0.84 ne sont pas encore pris en charge.
- Les commandes ne répondent qu'une dizaine de secondes après le lancement
  (l'initialisation de la carte JVS).
- Pas encore de texte, de son, ni de 3D.
