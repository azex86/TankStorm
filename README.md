# TankStorm

Un clone de **Rocket Bot Royale** écrit en **C** avec [raylib](https://www.raylib.com/).

Battle royale de tanks sur une île entièrement destructible : 8 tanks (vous + 7 bots),
des roquettes qui creusent le terrain, une eau qui monte inexorablement… le dernier
tank en vie gagne.

## Fonctionnalités

- Terrain destructible au pixel près, généré procéduralement (île + îles flottantes)
- Physique des tanks : pentes, chutes, souffle des explosions
- **Rocket jump** : tirez au sol sous votre tank pour vous propulser
- Montée des eaux par paliers, de plus en plus rapide
- 7 bots avec visée balistique, esquive de l'eau et recherche de hauteur
- Caisses parachutées : soins, tir triple, méga roquette
- Particules, secousses d'écran, sons synthétisés (aucun asset externe)

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

`tankstorm --selftest` lance une partie entre bots en accéléré, enregistre deux
captures d'écran (`selftest_mid.png`, `selftest_end.png`) et affiche le résultat.

## Crédits

Inspiré de *Rocket Bot Royale* (Winterpixel Games). Projet éducatif sans affiliation.
