# Versions

Le numéro d'une version est `majeur.mineur.N` : `majeur.mineur` vient de
[`version.txt`](version.txt), changé à la main quand une version apporte
quelque chose de notable ; `N` est le nombre de modifications du dépôt, qui
augmente à chaque mise à jour publiée. Odin l'affiche dans **MENU → About**
(`android-1.0.N-odin-xxxxxxx`, `xxxxxxx` étant le commit d'Odin utilisé), et
Android dans les informations de l'application.

## 1.0 — 22 novembre 2026

- **Ajouter des jeux depuis le téléphone** : MENU → File → Add games…, le
  sélecteur de fichiers d'Android ; chaque archive est copiée dans
  `Odin/roms` sous le bon nom (reconnue à son contenu), puis vérifiée :
  « ready to play », BIOS manquant, ancienne version.
- **Jeux maison sans nouvel APK** : leur zip porte leur description.
- **Télécharger les jeux maison** : MENU → File → Get homebrew games…, la
  liste des jeux publiés, chacun nouveau, installé ou à mettre à jour.
- **Portrait lisible** : menus agrandis ×2,5, fiche du jeu sous les
  vignettes ; menus espacés pour le doigt, en portrait comme en paysage.
- Deux jeux maison de plus : **VOLT VIPER** et **GLOWBUG GARDEN**.
- Les PR vers Odin sont vérifiées pour Android (la première : Odin#58).
- Ce numéro de version, et ce fichier.
- Les jeux maison dans une rubrique **HOMEBREW** de la liste, après les jeux
  des bornes.

## 0.3 — 17 et 18 novembre 2026

- Jeux maison : **NOVA SIEGE** (son, musique, record, vagues variées) et
  **SHARD STORM** (raquette au doigt).
- Manette tactile : stick plus vif dans les jeux à directions, chaque tape
  comptée même très brève.
- Records et réglages des bornes gardés quand Android met l'app en
  arrière-plan.

## 0.2 — 16 novembre 2026

- Manette tactile : visée au doigt (Brave Firefighters), second stick
  (Gaelco Football), deux leviers (Virtual-On), en paysage dans deux colonnes.
- Menus en Roboto, nets à la taille d'un téléphone.
- Le premier programme fait maison pour la Hikaru.

## 0.1 — 15 novembre 2026

- Le portage : Odin construit pour Android ARM64, Vulkan par défaut,
  60 images/s sur un Snapdragon 8 Gen 2, cache des shaders, plein écran,
  retour dans l'app sans écran noir.
- La manette tactile : stick flottant, boutons, gâchettes, vibration,
  panneau de raccourcis.
- Le format de l'image (4:3, 16:9, plein écran), la résolution interne.
