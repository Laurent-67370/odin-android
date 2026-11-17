# Homebrew pour la SEGA Hikaru (sous Odin)

Un jeu fait maison qui tourne sur la borne **SEGA Hikaru** telle qu'Odin
l'émule : un vrai programme pour son processeur SH-4, lancé par le BIOS
SEGA comme les jeux d'origine.

Le jeu : **NOVA SIEGE**, un jeu de tir à écran fixe, original. Une
formation de 50 aliens avance de côté et descend à chaque bord, de plus en
plus vite à mesure qu'elle se vide ; le chasseur en bas tire (un tir à la
fois) depuis l'abri de quatre boucliers que les tirs et les bombes rongent.
Un pillard traverse le haut de l'écran de temps en temps : 50 à 300 points.
Trois chasseurs ; chaque vague nettoyée revient un peu plus bas. Record de la
session en haut de l'écran.

| Commande | Action |
|---|---|
| Stick (ou flèches) gauche / droite | déplacer le chasseur |
| A / bouton 1 (ou B / bouton 2) | tirer |
| START | commencer une partie |

| Ennemi | Points |
|---|---|
| Guetteur (rose, rangée du haut) | 30 |
| Méduse (bleue, 2 rangées) | 20 |
| Scarabée (vert, 2 rangées du bas) | 10 |
| Pillard (rouge, de passage) | 50, 100, 150 ou 300 |

Pendant la dizaine de secondes où la carte d'entrées-sorties s'initialise,
l'écran titre affiche « WAITING FOR THE I/O BOARD... ».

![NOVA SIEGE dans Odin : l'écran titre et une partie](docs/demo.png)

## Jouer sur le téléphone

L'APK connaît le jeu, et `odinhb.zip` est publié avec lui à chaque version :

1. Télécharger
   [odinhb.zip](https://github.com/Laurent-67370/odin-android/releases/download/odin-apk/odinhb.zip).
2. Le mettre dans `Odin/roms`, à côté de `hikaru.zip` et `mie.zip`.
3. Lancer **Nova Siege** dans la liste des jeux.

Manette tactile : le stick déplace le chasseur, A tire, START lance la
partie. Après une mise à jour de l'APK qui change le homebrew, retélécharger
`odinhb.zip` : Odin vérifie que les EPROM sont celles que sa liste attend.

## Construire

```sh
sudo apt install gcc-sh4-linux-gnu python3-pil   # le compilateur C pour SH-4, Pillow pour les PNG
python3 build.py                        # → build/odinhb.zip et build/odinhb.json
python3 build.py --games chemin/vers/odin-games.json   # ajoute aussi le jeu à la liste d'Odin
```

`build/odinhb.zip` contient les deux EPROM du jeu (`odinhb.ic29` et
`odinhb.ic30`). Il se met dans le dossier des ROM d'Odin, à côté de
`hikaru.zip` et `mie.zip`. Le jeu s'appelle **Nova Siege** dans la liste.

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

### Double tampon, texte et sprites

Deux images de la couche 1 sont dans la mémoire d'image (lignes 1024 et
1536) : l'une est affichée pendant que le programme dessine l'autre, puis
`screen_flip()` les échange au début de l'image suivante. Rien ne scintille.

- **Texte** : une police 5×7 dessinée pour ce projet (`tools/font5x7.txt`,
  modifiable à la main), agrandissable (`draw_text`, `draw_number`).
- **Sprites** : les images PNG de `assets/` deviennent des sprites
  (`spr_<nom>`) à la construction ; les pixels transparents ne sont pas
  dessinés (`draw_sprite`, `draw_sprite_scaled`).

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
| `src/hikaru.h`, `src/hikaru.c` | la borne : couche 2D et double tampon, image suivante, commandes |
| `src/gfx.h`, `src/gfx.c` | le dessin : effacer, rectangles, sprites, texte, nombres |
| `src/main.c` | le jeu, NOVA SIEGE |
| `assets/*.png` | les sprites (aliens en deux poses, chasseur, pillard, explosions) |
| `tools/sprites.py` | les sprites dessinés en texte : il écrit `assets/*.png` |
| `tools/font5x7.txt` | la police |
| `src/crt0.S`, `src/link.ld` | le démarrage du programme et son placement en mémoire |
| `build.py` | génère la police et les sprites, compile, fabrique les EPROM et l'entrée de la liste des jeux |

## Limites connues

- Le programme JVS est pris à l'adresse qu'il a dans le BIOS **0.96**. Les
  BIOS 0.92 et 0.84 ne sont pas encore pris en charge.
- Les commandes ne répondent qu'une dizaine de secondes après le lancement
  (l'initialisation de la carte JVS).
- Pas encore de son ni de 3D.
