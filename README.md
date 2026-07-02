# TankStorm

Un clone de **Rocket Bot Royale** écrit en **C** avec [raylib](https://www.raylib.com/).

Battle royale de tanks sur une île entièrement destructible : 8 tanks (vous + 7 bots),
des roquettes qui creusent le terrain, une eau qui monte inexorablement… le dernier
tank en vie gagne.

## Fonctionnalités

- Terrain destructible au pixel près, généré procéduralement (île + îles flottantes)
- **Adhérence magnétique** : les tanks collent à la surface et roulent sur les
  pentes, les murs et même les plafonds ; seuls le recul du tir et les
  explosions les décrochent
- **Recul du tir** : chaque roquette propulse le tank à l'opposé du canon —
  c'est le moteur du déplacement aérien (réglable par map)
- **Emplacements de roquettes** : tir en rafale (3 par défaut) puis rechargement automatique
- Montée des eaux par paliers, de plus en plus rapide
- Bots avec visée balistique, esquive de l'eau et recherche de hauteur
- Caisses parachutées : soins, tir triple, méga roquette
- Particules, secousses d'écran, sons synthétisés (aucun asset externe)
- **Éditeur de maps intégré** : chaque map embarque ses propres règles

## Éditeur de maps

Depuis le menu, choisissez **Éditeur de maps**. Une map contient à la fois un
terrain dessiné à la main et ses **règles de jeu**, toutes saisies au clavier
dans des champs numériques — **aucune limite de gameplay** (seuls des
garde-fous techniques s'appliquent, comme la mémoire pour la taille du
terrain) :

| Règle | Valeur par défaut |
|-------|-------------------|
| Nombre de bots | 7 |
| PV des tanks | 100 |
| Vitesse des tanks | 115 px/s |
| Emplacements de roquettes | 3 |
| Temps de rechargement | 2 s |
| Vitesse des roquettes | 540 px/s |
| Dégâts des roquettes | 34 |
| Rayon d'explosion | 26 px |
| **Recul du tir** | 155 px/s |
| Délai avant la montée des eaux | 18 s |
| Vitesse de l'eau | 6 px/s |
| **Largeur de la map** | 1600 px |
| **Hauteur de la map** | 960 px |

Le recul du tir est la propulsion appliquée au tank à chaque roquette tirée :
c'est le moteur du déplacement aérien (0 = désactivé, valeurs hautes = jetpack).
Changer la taille de la map conserve le terrain déjà dessiné (ancré en bas).

Commandes de l'éditeur :

- **Clic gauche** : dessiner avec l'outil courant (*Terre* ou *Creuser*) ·
  **Maj** : outil inverse
- **Clic droit maintenu** : déplacer la vue · **Molette** : zoomer ·
  **Ctrl + Molette** : taille du pinceau
- **Tab** : masquer/afficher le panneau · **Échap** : annuler la saisie / menu
- Boutons : *Île aléatoire*, *Vider*, *Sauvegarder*, *Charger*, *Tester la map*

Les maps sont enregistrées dans le dossier `maps/` au format `.tsm`
(règles + terrain compressé en RLE, format « TSM2 » ; les anciennes maps
« TSM1 » restent lisibles). Depuis le menu, **Choisir une map** permet
de lancer une partie sur n'importe quelle map sauvegardée.

## Compiler

Prérequis : [CMake](https://cmake.org/) ≥ 3.16 et un compilateur C
(Visual Studio sous Windows, GCC/Clang sous Linux). raylib est téléchargé
et compilé automatiquement à la configuration.

```sh
cmake -S . -B build
cmake --build build --config Release
```

Puis lancez `build/Release/tankstorm.exe` (Windows) ou `build/tankstorm` (Linux).

## Contrôles

| Action       | Touche                    |
|--------------|---------------------------|
| Se déplacer  | `Q` / `D` ou `←` / `→`    |
| Viser        | Souris                    |
| Tirer        | Clic gauche               |
| Rocket jump  | Tirer au sol sous le tank |
| Pause        | `Échap`                   |

## Autotest

`tankstorm --selftest` vérifie le format de map (aller-retour), mesure
l'adhérence d'un tank roulant sur toute l'île, lance une partie entre bots en
accéléré (`--seed N` pour varier la partie), enregistre des captures d'écran
et affiche le résultat.

## Crédits

Inspiré de *Rocket Bot Royale* (Winterpixel Games). Projet éducatif sans affiliation.
