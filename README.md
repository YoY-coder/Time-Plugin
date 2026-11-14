# Time — chronomètre du temps de travail réel (plugin VST3)

**Time** ne modifie ni le son ni le MIDI. On le place sur le **master** d'un projet. Il compte le temps de travail **réel** passé sur ce projet : le chrono avance seulement quand vous travaillez vraiment, et s'arrête quand vous êtes inactif.

- JUCE 8, CMake, format **VST3** uniquement
- Windows (Visual Studio 2022) et macOS (Xcode), pour tous les DAW qui chargent du VST3 (testé pour Ableton Live)
- L'audio passe sans aucune modification, **latence nulle**, mono ou stéréo, **aucun paramètre automatisable**

---

## Sommaire

1. [Comment le chrono compte](#1-comment-le-chrono-compte)
2. [L'interface](#2-linterface)
3. [Récupérer le code sur votre ordinateur](#3-récupérer-le-code-sur-votre-ordinateur)
4. [Windows : installer, compiler, installer le plugin](#4-windows--installer-compiler-installer-le-plugin)
5. [Mac : installer, compiler, signer, installer le plugin](#5-mac--installer-compiler-signer-installer-le-plugin)
6. [Faire apparaître Time dans Ableton](#6-faire-apparaître-time-dans-ableton)
7. [Tests à faire pour vérifier le chrono](#7-tests-à-faire-pour-vérifier-le-chrono)
8. [En cas de problème](#8-en-cas-de-problème)
9. [Organisation du code (pour un développeur)](#9-organisation-du-code-pour-un-développeur)

---

## 1. Comment le chrono compte

| Situation | Effet sur le chrono |
|---|---|
| Vous cliquez ou tapez une touche **dans le DAW** (le DAW est l'application au premier plan) | **Activité.** Le temps compte (état vert « Actif »). |
| Le transport du DAW est **en lecture** | Le temps compte **toujours**, sans limite, même si vous ne touchez à rien ou si vous êtes dans une autre application (état bleu « Lecture »). Un **export audio** compte aussi, pour sa durée réelle. |
| Aucune activité, pas de lecture | Le temps passé depuis la dernière action est **en attente** (état orange). S'il y a une nouvelle action dans les **5 minutes**, ce temps en attente est validé. |
| **Plus de 5 minutes** sans activité ni lecture | Le chrono **s'arrête** et ces 5 minutes sont **retirées** : l'affichage redescend (état gris « Arrêté »). Le chrono repart à la prochaine action. |
| **Pause** activée | Rien ne compte, même avec de l'activité ou de la lecture (état gris « En pause »). |

Fonctionnement technique :

- **Sonde toutes les 250 ms.** Windows : `GetForegroundWindow` + `GetWindowThreadProcessId`, comparés au processus du DAW, et `GetLastInputInfo` pour détecter les actions. Mac : `NSWorkspace.frontmostApplication` comparé au processus du DAW, et `CGEventSourceSecondsSinceLastEventType` (clics souris et touches). Cette fonction ne demande **aucune autorisation d'accessibilité**.
- Certains DAW (par exemple Bitwig) font tourner les plugins dans un processus séparé. Time considère donc aussi le **processus parent** comme « le DAW ».
- La lecture est détectée par le transport du DAW. Si l'audio n'est plus traité depuis plus d'1 seconde, Time considère que la lecture est arrêtée.
- Le temps est mesuré avec une **horloge monotone**, pas en comptant des « ticks ». Il reste donc précis même si l'ordinateur ralentit.
- Le comptage tourne dans le **processeur audio du plugin**, pas dans la fenêtre. Il continue donc quand la fenêtre de Time est **fermée**.

> **Petite différence Windows / Mac :** sous Windows, `GetLastInputInfo` compte aussi les **mouvements de souris** comme une activité. Sur Mac, seuls les **clics** et les **touches** comptent. Dans les deux cas, il faut que le DAW soit au premier plan.

### Sessions et historique

- Une **session** commence à l'ouverture du projet (chargement de Time) et se termine à sa fermeture. Une session sans aucune seconde active n'est pas enregistrée.
- Chaque session indique sa date, son heure de début, son heure de fin (le dernier moment compté) et son temps actif.
- **Total affiché = somme des sessions + ajustements manuels + temps antérieur.**

### Sauvegarde

Tout est enregistré **dans le projet du DAW** : sessions, ajustements, temps antérieur, état de pause et taille de la fenêtre. Le format porte un numéro de version, ce qui permettra de le faire évoluer.

- ⚠️ **La session en cours n'est conservée que si vous enregistrez le projet**, comme n'importe quel réglage de plugin. Si vous fermez sans enregistrer, cette session est perdue.
- Un **« Enregistrer sous »** (v2 d'un projet) conserve tout le temps cumulé.

---

## 2. L'interface

- **Chrono total** en HH:MM:SS au centre. Les chiffres sont à chasse fixe : ils ne bougent pas en défilant.
- **Indicateur d'état** sous le chrono :
  - 🟢 **Actif**
  - 🔵 **Lecture**
  - 🟠 **Inactif** : temps en attente, avec le délai avant qu'il soit retiré
  - ⚪ **Arrêté** (en attente d'activité) ou **En pause**
- **Boutons :**
  - **Pause / Reprendre** : gèle le chrono.
  - **Corriger ±** : ajoute ou retire des minutes. Chaque correction apparaît comme une ligne « Ajustement » datée dans l'historique.
  - **Temps antérieur** : le temps déjà passé sur un projet commencé avant d'avoir Time. Il apparaît comme une ligne distincte. Pour le modifier, rouvrez ce bouton ou cliquez sur la ligne dans l'historique. Mettez 0 pour le supprimer.
  - **Reset** : remet tout à zéro, après une fenêtre de confirmation.
- **Sessions ▸** : déplie l'historique (sessions, ajustements, temps antérieur), le plus récent en haut, avec défilement. La fenêtre s'agrandit vers le bas, puis reprend sa taille quand vous repliez la liste. L'historique est replié par défaut.
- **Fenêtre redimensionnable** par le coin en bas à droite. Tout s'adapte proportionnellement, et la taille est mémorisée dans le projet.
- Les champs de saisie ont aussi des boutons **− / +**. Si votre DAW intercepte le clavier, vous pouvez régler les valeurs à la souris.

---

## 3. Récupérer le code sur votre ordinateur

Le code se trouve sur GitHub, dans le dépôt `yoy-coder/time-plugin`, branche `ccr-c08becc6-71lnzr`. Vous pouvez aussi utiliser la branche principale si le code y a été fusionné.

**Méthode simple (sans ligne de commande) :**

1. Ouvrez la page du dépôt sur github.com.
2. En haut à gauche, sélectionnez la bonne **branche** dans le menu déroulant.
3. Cliquez sur le bouton vert **Code**, puis sur **Download ZIP**.
4. Décompressez le ZIP à un endroit simple, **sans espaces ni accents dans le chemin** :
   - Windows : `C:\Dev\Time-Plugin`
   - Mac : dans votre dossier personnel, `Dev/Time-Plugin` (le chemin complet est `/Users/VOTRE_NOM/Dev/Time-Plugin`)

   Si le dossier décompressé a un nom plus long (par exemple `time-plugin-ccr-c08becc6-71lnzr`), renommez-le en `Time-Plugin`.

Le dossier doit contenir un fichier `CMakeLists.txt`, un dossier `Source` et ce `README.md`.

---

## 4. Windows : installer, compiler, installer le plugin

### 4.1 Installer les outils (une seule fois)

1. **Visual Studio 2022 Community** (gratuit)
   - Téléchargez-le sur https://visualstudio.microsoft.com/fr/vs/community/
   - Lancez l'installateur. Dans l'onglet **Charges de travail**, cochez **« Développement Desktop en C++ »**.
   - Dans la colonne de droite, vérifiez que **« Outils CMake C++ pour Windows »** est coché (c'est le cas par défaut).
   - Cliquez sur **Installer**. C'est long : plusieurs Go à télécharger.
2. **Git pour Windows**
   - Téléchargez-le sur https://git-scm.com/download/win
   - Installez avec toutes les options par défaut (cliquez sur « Next » jusqu'au bout).
   - Git sert à télécharger automatiquement JUCE lors de la première compilation.
3. Redémarrez l'ordinateur.

### 4.2 Compiler

1. Ouvrez le menu Démarrer et tapez **« Developer PowerShell for VS 2022 »**. Ouvrez ce programme (pas le PowerShell normal).
2. Placez-vous dans le dossier du projet. Tapez cette commande puis appuyez sur Entrée :
   ```
   cd C:\Dev\Time-Plugin
   ```
3. Préparez la compilation. Cette étape télécharge JUCE et prend 1 à 3 minutes la première fois :
   ```
   cmake -B build -G "Visual Studio 17 2022" -A x64
   ```
   Elle doit se terminer par `-- Build files have been written to: ...`
4. Compilez :
   ```
   cmake --build build --config Release
   ```
   La première compilation prend de 5 à 15 minutes. Elle doit se terminer sans ligne contenant `error`.

### 4.3 Trouver le plugin

Le plugin se trouve ici :

```
C:\Dev\Time-Plugin\build\Time_artefacts\Release\VST3\Time.vst3
```

`Time.vst3` est un **dossier**. C'est normal : c'est le format VST3.

### 4.4 Copier le plugin dans le dossier VST3 de Windows

**Méthode A, avec l'Explorateur :**

1. Ouvrez `C:\Dev\Time-Plugin\build\Time_artefacts\Release\VST3\`
2. Faites un clic droit sur `Time.vst3`, puis **Copier**.
3. Ouvrez `C:\Program Files\Common Files\VST3\`. Créez le dossier `VST3` s'il n'existe pas.
4. Faites un clic droit puis **Coller**. Windows demande une autorisation d'administrateur : acceptez.

**Méthode B, en ligne de commande :**

1. Ouvrez le menu Démarrer et tapez **PowerShell**.
2. Faites un clic droit sur PowerShell, puis **Exécuter en tant qu'administrateur**.
3. Tapez cette commande :
   ```
   xcopy /E /I /Y "C:\Dev\Time-Plugin\build\Time_artefacts\Release\VST3\Time.vst3" "C:\Program Files\Common Files\VST3\Time.vst3"
   ```

> Pour mettre à jour le plugin plus tard : **fermez Ableton**, recompilez (étape 4.2, commande 4 seulement), puis recopiez `Time.vst3` en remplaçant l'ancien.

---

## 5. Mac : installer, compiler, signer, installer le plugin

### 5.1 Installer les outils (une seule fois)

1. **Xcode**
   - Installez Xcode depuis l'**App Store**. Il est gratuit mais volumineux.
   - Ouvrez Xcode une fois, acceptez la licence et laissez-le installer ses composants, puis quittez-le.
   - Ouvrez le **Terminal** (Applications > Utilitaires > Terminal) et tapez :
     ```
     sudo xcodebuild -license accept
     ```
     Votre mot de passe de session vous est demandé. Rien ne s'affiche pendant la saisie, c'est normal.
   - Git est fourni avec Xcode.
2. **CMake**
   - Téléchargez le fichier `.dmg` pour macOS sur https://cmake.org/download/ (section « Binary distributions », « macOS 10.13 or later »).
   - Ouvrez le `.dmg` et glissez **CMake** dans **Applications**.
   - Pour pouvoir utiliser CMake dans le Terminal, tapez :
     ```
     sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install
     ```
   - Fermez puis rouvrez le Terminal. Vérifiez l'installation avec `cmake --version` : un numéro de version doit s'afficher (3.22 ou plus).

### 5.2 Compiler

Dans le Terminal :

```
cd ~/Dev/Time-Plugin
cmake -B build -G Xcode
cmake --build build --config Release
```

- La 2ᵉ commande télécharge JUCE (1 à 3 minutes la première fois).
- La 3ᵉ compile le plugin (5 à 15 minutes la première fois). Elle produit un plugin **universel** (Mac Intel et Apple Silicon) et le **signe automatiquement en ad-hoc**.

### 5.3 Trouver le plugin

```
~/Dev/Time-Plugin/build/Time_artefacts/Release/VST3/Time.vst3
```

Dans le Finder, `Time.vst3` apparaît comme un seul fichier (c'est un « paquet »).

### 5.4 Copier, signer et débloquer le plugin

Dans le Terminal, tapez ces commandes une par une :

```
mkdir -p ~/Library/Audio/Plug-Ins/VST3
rm -rf ~/Library/Audio/Plug-Ins/VST3/Time.vst3
cp -R ~/Dev/Time-Plugin/build/Time_artefacts/Release/VST3/Time.vst3 ~/Library/Audio/Plug-Ins/VST3/
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/Time.vst3
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Time.vst3
```

Ce que font ces commandes :

- `mkdir -p` crée le dossier VST3 de votre compte s'il n'existe pas.
- `rm -rf` supprime une ancienne version de Time, s'il y en a une.
- `cp -R` copie le plugin dans ce dossier.
- `codesign --sign -` fait une **signature ad-hoc**. Sans elle, macOS refuse de charger le plugin.
- `xattr -dr com.apple.quarantine` retire le marquage « téléchargé d'Internet ». La commande ne dit rien s'il n'y avait rien à retirer.

Vérification (facultative) :

```
codesign -dv ~/Library/Audio/Plug-Ins/VST3/Time.vst3
```

La réponse doit contenir `Signature=adhoc`.

> Le dossier `~/Library` est caché dans le Finder. Pour l'ouvrir : menu **Aller** > **Aller au dossier…**, puis tapez `~/Library/Audio/Plug-Ins/VST3`.

> Pour mettre à jour le plugin plus tard : **quittez Ableton**, relancez `cmake --build build --config Release` depuis `~/Dev/Time-Plugin`, puis refaites les commandes de la section 5.4.

---

## 6. Faire apparaître Time dans Ableton

1. Ouvrez Ableton Live.
2. Ouvrez les préférences :
   - Windows : **Options > Préférences**
   - Mac : **Live > Réglages** ou **Live > Préférences**
3. Allez dans l'onglet **Plug-ins** (« Plug-Ins » ou « File/Folder » selon la version).
4. Activez **« Utiliser les dossiers système VST3 »** (Use VST3 Plug-in System Folders).
5. Cliquez sur **Rescan** (Re-scanner). Pour forcer une analyse complète, maintenez **Alt** (Windows) ou **Option** (Mac) en cliquant.
6. Dans le navigateur de gauche, ouvrez **Plug-ins > VST3 > YoY > Time**.
7. **Glissez Time sur la piste Master.** Une seule instance par projet.
8. Cliquez sur l'icône 🔧 ou sur la petite flèche du périphérique pour ouvrir la fenêtre de Time.
9. **Enregistrez le projet**, puis enregistrez-le régulièrement pour conserver les sessions.

> Astuce : enregistrez un projet vide contenant Time sur le master, puis définissez-le comme **modèle par défaut** (Préférences > File/Folder > Save Current Set as Default). Time sera alors présent dans chaque nouveau projet.

---

## 7. Tests à faire pour vérifier le chrono

Avant de commencer, ajoutez Time sur le master d'un projet de test et ouvrez sa fenêtre.

| # | Test | Résultat attendu |
|---|---|---|
| 1 | **Activité** : cliquez quelques fois dans Ableton (dans l'arrangement, par exemple) pendant 1 minute. | L'état passe en **vert « Actif »** et le chrono avance d'environ 1 minute. |
| 2 | **Autre application** : sans lecture, passez dans votre navigateur web et cliquez dedans pendant 1 minute. | Les clics hors du DAW ne comptent pas. Après environ 10 s, l'état passe en **orange « Inactif »** et le temps en attente augmente. |
| 3 | **Retour avant 5 min** : revenez dans Ableton et cliquez, moins de 5 minutes après votre dernière action. | Le temps en attente est **validé** et le chrono ne redescend pas. L'état redevient vert. |
| 4 | **Inactivité de 5 min** : ne touchez plus à rien (sans lecture) pendant un peu plus de 5 minutes. | Le chrono monte jusqu'à environ +5 min, puis **redescend de 5 min** d'un coup. L'état passe en gris **« Arrêté »** et le chrono reste figé. |
| 5 | **Lecture** : lancez la lecture (en boucle), passez dans une autre application et attendez 6 à 7 minutes. | L'état est **bleu « Lecture »** et le chrono avance **sans jamais redescendre**. |
| 6 | **Arrêt de la lecture** : stoppez la lecture et attendez. | L'état passe en vert, puis orange. Le comportement est celui des tests 2 à 4. |
| 7 | **Pause** : cliquez sur **Pause**, puis cliquez dans Ableton et lancez la lecture. | Le chrono est **gelé** (gris, « En pause »). Il repart quand vous cliquez sur **Reprendre**. |
| 8 | **Fenêtre fermée** : fermez la fenêtre de Time, travaillez 2 minutes dans Ableton, puis rouvrez-la. | Les 2 minutes ont été comptées. |
| 9 | **Correction** : cliquez sur Corriger ±, saisissez 15, puis « + Ajouter ». Ensuite, saisissez 5 et cliquez sur « − Retirer ». | Le total augmente de 15 min, puis baisse de 5 min. L'historique montre deux lignes « Ajustement » datées. |
| 10 | **Temps antérieur** : saisissez 2 h 30, puis Enregistrer. | Le total augmente de 2:30:00. L'historique montre une ligne « Temps antérieur ». Un clic sur cette ligne permet de la modifier. |
| 11 | **Sauvegarde / réouverture** : enregistrez le projet, fermez-le, puis rouvrez-le. | Le **total est identique**. L'historique contient la session précédente (date, début → fin, durée) et une nouvelle « Session en cours ». La taille de la fenêtre est conservée. |
| 12 | **Enregistrer sous** : faites « Enregistrer sous » un v2, puis ouvrez ce v2. | Tout l'historique et le total sont présents. |
| 13 | **Export** : exportez l'audio du morceau (Fichier > Exporter audio/vidéo). | La durée de l'export s'ajoute au chrono. |
| 14 | **Reset** : cliquez sur Reset, puis sur Annuler. Recommencez et cliquez sur « Tout effacer ». | Annuler ne change rien. « Tout effacer » remet le chrono à 00:00:00 et vide l'historique. |
| 15 | **Son** : comparez le master avec Time activé et désactivé. | Le son et la latence sont strictement identiques. |

---

## 8. En cas de problème

- **`cmake` n'est pas reconnu (Windows)** : utilisez bien **Developer PowerShell for VS 2022**, pas le PowerShell normal. Vérifiez aussi que « Outils CMake C++ pour Windows » est installé.
- **`cmake` n'est pas reconnu (Mac)** : refaites la commande `sudo ".../cmake-gui" --install` de la section 5.1, puis rouvrez le Terminal.
- **Erreur au téléchargement de JUCE** : vérifiez votre connexion Internet et que Git est installé (`git --version`). Supprimez ensuite le dossier `build` et recommencez la préparation.
- **Une étape a échoué et vous voulez repartir de zéro** : supprimez le dossier `build` du projet et refaites la section « Compiler ».
- **Time n'apparaît pas dans Ableton** :
  - Vérifiez que `Time.vst3` est bien dans le dossier VST3 (section 4.4 ou 5.4).
  - Vérifiez que l'option « dossiers système VST3 » est activée, puis faites un Rescan avec Alt ou Option.
  - Sur Mac, refaites les commandes `codesign` et `xattr` de la section 5.4.
- **Mac : « Time.vst3 ne peut pas être ouvert »** : refaites la section 5.4. Si le message persiste, ouvrez **Réglages Système > Confidentialité et sécurité** et cliquez sur **Autoriser quand même**.
- **Le chrono ne réagit pas aux clics sous Bitwig ou un autre DAW** : ces DAW font tourner les plugins dans un processus à part. Time prend en compte le processus parent. Si votre DAW utilise une organisation différente, seule la lecture fera avancer le chrono. Signalez-le pour qu'on adapte la détection.

---

## 9. Organisation du code (pour un développeur)

```
CMakeLists.txt              Projet CMake (télécharge JUCE 8 automatiquement)
Source/
  LiveClock.h/.cpp          Cœur du chronométrage (sans JUCE, testable) :
                            temps validé / en attente, 5 min, lecture, pause
  ActivityProbe.h           Interface de détection d'activité
  ActivityProbe_win.cpp     Windows : GetForegroundWindow, GetLastInputInfo, PID parent
  ActivityProbe_mac.mm      macOS : NSWorkspace, CGEventSourceSecondsSinceLastEventType, getppid
  ActivityProbe_other.cpp   Autres systèmes : pas de détection (lecture uniquement)
  PluginProcessor.h/.cpp    Audio (passe-plat), détection de lecture, sonde 250 ms,
                            sessions / ajustements / temps antérieur, sauvegarde XML
  PluginEditor.h/.cpp       Interface : chrono, état, boutons, panneaux, historique
  TimeLookAndFeel.h/.cpp    Couleurs, polices, dessin des boutons
Tests/
  LiveClockTests.cpp        Tests automatiques de la logique du chrono
```

Pour lancer les tests de logique :

```
cmake -B build-tests -DTIME_BUILD_TESTS=ON
cmake --build build-tests --config Release --target TimeTests
```

Lancez ensuite l'exécutable `TimeTests` généré.

Format de sauvegarde : un élément XML `TIME`, avec `formatVersion="1"`, les attributs `paused`, `priorSeconds`, `editorWidth` et `editorHeight`, et les enfants `SESSIONS/SESSION` (`id`, `start`, `end`, `seconds`) et `ADJUSTMENTS/ADJUSTMENT` (`id`, `time`, `seconds`). Les heures sont en millisecondes depuis 1970.
