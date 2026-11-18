# Homebrew pour la SEGA Hikaru (sous Odin)

Un jeu fait maison qui tourne sur la borne **SEGA Hikaru** telle qu'Odin
l'émule : un vrai programme pour son processeur SH-4, lancé par le BIOS
SEGA comme les jeux d'origine.

Le jeu : **NOVA SIEGE**, un jeu de tir à écran fixe, original. Une
formation d'aliens avance de côté et descend à chaque bord, de plus en plus
vite à mesure qu'elle se vide ; le chasseur en bas tire (deux tirs à la fois ;
un appui fait pendant que les deux volent part dès qu'un tir se libère) depuis
l'abri de quatre boucliers que les tirs et les bombes rongent. Un pillard
traverse le haut de l'écran de temps en temps : 50 à 300 points. Trois
chasseurs.

Les vagues changent :

- **la forme de la formation**, quatre à tour de rôle : le mur, le coin, les
  pinces, le damier (son nom s'affiche au début de la vague) ;
- **des blindés** (orange, 40 points) qui encaissent deux tirs : la rangée du
  haut dès la vague 3, deux rangées dès la 7, trois dès la 11 ;
- **des plongeurs** dès la vague 2 : un alien quitte la formation et fond sur
  le chasseur en zigzaguant, puis revient à sa place par le haut ; abattu en
  vol, il rapporte le double ;
- **un vaisseau-mère toutes les 5 vagues**, annoncé par une alarme : une
  barre de vie, des tirs en éventail (trois, puis cinq quand il est à moitié
  détruit), plus rapide à mesure qu'il est touché ; 1000 points par cycle.

**Le son** : tirs, explosions, les quatre notes du pas de la formation (qui
accélèrent avec elle), la sirène du pillard, le tintement des coups sur les
blindés et le vaisseau-mère, la plongée, les jingles de départ, de vague
nettoyée et de fin de partie. Tout est synthétisé par `tools/sounds.py`.

**La musique** : trois morceaux composés pour le jeu, qui bouclent : un
calme en la mineur pour l'écran titre, un entraînant en mi mineur pour les
vagues, un rapide en ré mineur pour le vaisseau-mère. Basse, mélodie,
arpèges ou nappe, et batterie. **B (bouton 2) sur l'écran titre la coupe ou
la remet** ; le choix est gardé.

**Le record** est gardé dans la RAM sauvegardée de la borne : il reste d'une
partie et d'un lancement à l'autre.

| Commande | Action |
|---|---|
| Stick (ou flèches) gauche / droite | déplacer le chasseur |
| A / bouton 1 (ou B / bouton 2) | tirer |
| START | commencer une partie |
| B / bouton 2 (écran titre) | musique on / off |

| Ennemi | Points |
|---|---|
| Blindé (orange, deux tirs) | 40 |
| Guetteur (rose, rangée du haut) | 30 |
| Méduse (bleue, 2 rangées) | 20 |
| Scarabée (vert, 2 rangées du bas) | 10 |
| Pillard (rouge, de passage) | 50, 100, 150 ou 300 |
| Plongeur abattu en vol | le double de son espèce |
| Vaisseau-mère | 10 par coup, 1000 × cycle à la fin |

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

### Le son : l'AICA

La carte son est sur le bus externe, page `0C` : le contrôleur mémoire ouvre
la fenêtre `02000000` sur elle (octet `1A` de ses registres, à `04000000`).
Ses registres sont à `02700000`, sa mémoire d'échantillons à `02800000`. Le
jeu tient l'ARM7 de la carte en reset (registre `2C00`) et joue lui-même
les voix : ce sont celles de l'AICA de la Dreamcast (`src/sound.c`).
`KYONEX` déclenche ou coupe toutes les voix selon leur `KYONB` : une voix
qui a fini garde son `KYONB` effacé un peu avant sa fin, pour ne pas
repartir au prochain déclenchement.

### La musique : un séquenceur

`tools/music.py` écrit chaque morceau en lignes de notes (`NOTE:TEMPS`, un
temps étant une double croche ; `-` un silence ; `K`, `H`, `S` la grosse
caisse, le charleston, la caisse claire). Les instruments sont des cycles
d'onde bouclés par l'AICA (carré, dent de scie, triangle), avec l'enveloppe
de l'AICA ; `src/music.c` lit les notes à chaque image, sur les voix 16 et
suivantes.

### Le record : la RAM sauvegardée

64 Ko à `00C00000`, sur pile sur la borne, qu'Odin garde dans le fichier
`nvram` du jeu. Le record y est à `F000` (une signature, le score, un
contrôle). Odin l'écrit à la fermeture et, sur Android, quand l'application
passe en arrière-plan.

### Les commandes : le MIE et le JVS

Les commandes passent par le **MIE**, un Z80 qui parle à la carte
d'entrées-sorties en JVS. Au lancement, le jeu y charge le programme JVS du
BIOS de la borne (copié depuis le BIOS, pas inclus dans la ROM), puis
dialogue par une boîte aux lettres en mémoire du Z80 :

- `C004 = 3` : le programme est prêt (après une dizaine de secondes : il
  initialise la carte JVS) ;
- `C001 = 0` : il est libre ; on écrit alors la commande 1 dans `C000` et 1
  dans `C001` ;
- quand `C001` revient à 0 (en 4 ms environ : le jeu l'attend, pour lire
  les commandes de l'instant et non celles de l'image d'avant), les boutons
  sont en `E003` (la borne : TEST =
  bit 7) et `E004`–`E005` (joueur 1, ordre JVS : START, SERVICE, haut, bas,
  gauche, droite, bouton 1, bouton 2, puis boutons 3 à 8).

## Les fichiers

| Fichier | Rôle |
|---|---|
| `src/hikaru.h`, `src/hikaru.c` | la borne : couche 2D et double tampon, image suivante, commandes |
| `src/gfx.h`, `src/gfx.c` | le dessin : effacer, rectangles, sprites, texte, nombres |
| `src/sound.h`, `src/sound.c` | le son : les voix de l'AICA |
| `tools/sounds.py` | les sons et les instruments, synthétisés |
| `src/music.h`, `src/music.c` | le séquenceur de la musique |
| `tools/music.py` | la musique, écrite en notes (`E4:2`, `-:4`, `K:2`…) |
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
- Pas encore de 3D.

## Tester

`build.py --start-wave N` fabrique une ROM qui commence à la vague N (5 : le
vaisseau-mère) ; `--define RECORD_TEST=1234` écrit ce record au lancement.
Dans Odin sur PC, `ODIN_WAV=son.wav` enregistre le son de la borne.
