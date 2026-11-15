# Odin pour Android

Portage Android d'[Odin](https://github.com/Tovarichtch/Odin), l'émulateur SEGA
Hikaru et Gaelco ATV Track de Réda Chérif-Touil (GPL 3). Ce dépôt ne contient
pas le code d'Odin : le workflow `.github/workflows/build.yml` récupère
Odin au commit de `odin.version`, applique `patches/0001-android.patch` et
construit l'APK, publié dans la release **`odin-apk`** de ce dépôt :
https://github.com/Laurent-67370/odin-android/releases/download/odin-apk/odin.apk

## Installer

1. Télécharger `odin.apk` depuis la release `odin-apk`, l'ouvrir sur le
   téléphone et autoriser l'installation depuis cette source.
2. Au premier lancement, autoriser Odin à accéder à tous les fichiers. Il crée
   le dossier `Odin/` du téléphone :
   - `Odin/roms` : les jeux (archives MAME `.zip` / `.7z`), avec `hikaru.zip` et
     `mie.zip` pour les jeux Hikaru ;
   - `Odin/artwork` : les images d'`Odin-Artwork.zip` (facultatif) ;
   - `odin.ini` et `odin.log` y sont écrits.
   Sans l'autorisation, Odin utilise `Android/data/org.odinport.android/files`.

Il faut un téléphone ARM64. Odin dessine par défaut avec OpenGL ES 3.2 ;
Vulkan reste au choix, page Display (option « API »). L'émulation
Hikaru est lourde : un téléphone haut de gamme récent est conseillé.

## Commandes

- **Manette tactile** par-dessus le jeu : stick, A B X Y, LB RB, LT RT
  (gâchettes, pédales), COIN, START et MENU (menu d'Odin). Elle se masque
  quand une vraie manette (Bluetooth / USB) est branchée.
- **Portrait ou paysage**, comme on tient le téléphone (le verrouillage de la
  rotation est respecté) : la manette se place autour de l'image dans les deux.
- La touche **Retour** d'Android ouvre / ferme le menu (Échap).
- Dans les menus, on touche les éléments comme avec une souris.

## Ce que fait le patch

- CMake : `libmain.so` (au lieu d'un exécutable), SDL3 en `libSDL3.so`,
  OpenGL ES (`GLESv3`, `EGL`) et Vulkan, pages de 16 Ko.
- Dossiers : `ODIN_ANDROID_FOLDER` (dossier `Odin/`) et `ODIN_ANDROID_DATA`
  (données de l'APK) donnés par l'activité ; polices de `/system/fonts`.
- Pas de X11 ni d'evdev ; sélecteurs de dossiers remplacés par `Odin/roms`.
- `touch_pad.cpp` : la manette tactile, une manette virtuelle SDL dessinée avec
  Dear ImGui (testable sur PC avec `ODIN_TOUCH_PAD=1`).
- Vulkan par défaut, comme sur macOS ; le recompilateur ARM64 d'Odin (fait
  pour les Mac Apple Silicon) tourne tel quel.

## Construire soi-même

```sh
sudo apt install glslang-tools      # glslangValidator
./fetch-odin.sh                      # Odin + sous-modules + patch dans odin-src/
echo "sdk.dir=$ANDROID_HOME" > local.properties
./gradlew assembleRelease            # app/build/outputs/apk/release/app-release.apk
```

Pour mettre à jour Odin : changer `odin.version`, relancer `./fetch-odin.sh` et
corriger le patch s'il ne s'applique plus.

## Signature (mises à jour sans désinstaller)

Android n'installe un APK par-dessus l'ancien que s'il est signé avec la même
clé. Le workflow la lit dans deux secrets du dépôt (Settings → Secrets and
variables → Actions) :

- `ODIN_KEYSTORE_B64` : le fichier de clé PKCS12 (alias `odin`), en base64 ;
- `ODIN_KEYSTORE_PASSWORD` : son mot de passe.

Sans eux, chaque APK est signé avec une clé de test nouvelle à chaque
compilation, et il faut désinstaller avant d'installer.
