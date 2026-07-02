// TankStorm — clone de Rocket Bot Royale en C + raylib
#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>
#include <math.h>

// ---- Dimensions ----
#define SCREEN_W 1280
#define SCREEN_H 720
#define DEF_WORLD_W 1600      // taille par défaut, chaque map porte la sienne
#define DEF_WORLD_H 960
#define WATER_MARGIN 55       // niveau initial de l'eau = worldH - WATER_MARGIN

#define MAX_PARTICLES 1024
#define MAX_CRATES    6
#define MAX_FEED      5
#define MAX_MAPS      64

// ---- Constantes physiques fixes ----
#define GRAVITY      640.0f
#define TANK_HALF_W  12.0f
#define TANK_HALF_H  7.0f
#define ROCKET_GRAV  430.0f
#define SHOT_DELAY   0.30f    // délai minimal entre deux tirs d'une rafale

typedef enum { W_ROCKET = 0, W_TRIPLE, W_BIG } WeaponType;
typedef enum { ST_MENU = 0, ST_MAPSELECT, ST_EDITOR, ST_COUNTDOWN, ST_PLAY, ST_OVER } GameState;
typedef enum { CR_HEAL = 0, CR_TRIPLE, CR_BIG } CrateType;
typedef enum { SFX_FIRE = 0, SFX_BOOM, SFX_PICK, SFX_SPLASH, SFX_DIE, SFX_COUNT } SfxId;

// Règles de la partie : embarquées dans chaque map (tous les champs font 4 octets).
// Aucune limite de gameplay : seuls des garde-fous techniques (mémoire, division
// par zéro) sont appliqués au chargement — voir ClampRules dans map.c.
typedef struct MapRules {
    int   numBots;        // nombre d'adversaires IA
    float tankHp;         // points de vie
    float tankSpeed;      // px/s
    int   rocketSlots;    // emplacements de roquettes (rafale)
    float reloadTime;     // s par roquette rechargée
    float rocketSpeed;    // px/s
    float rocketDmg;      // dégâts par roquette
    float rocketRadius;   // rayon d'explosion en px
    float waterGrace;     // s avant la montée des eaux
    float waterSpeed;     // px/s (premier palier)
    float recoil;         // impulsion de recul du tir (px/s) — moteur du vol
    int   worldW, worldH; // dimensions du terrain en px
} MapRules;

typedef struct Tank {
    bool  alive, isPlayer, grounded;   // grounded = accroché à une surface
    float x, y;             // centre du corps
    float vx, vy;           // vitesse monde (vol balistique)
    float nx, ny;           // normale de la surface d'accroche (vers l'extérieur)
    float surfSpeed;        // vitesse le long de la surface (signée)
    float bodyAngle;        // inclinaison du châssis (rendu)
    float aimAngle;         // angle de la tourelle
    float hp, cooldown, hitFlash;
    int   shots;            // roquettes disponibles dans les emplacements
    float reload;           // progression du rechargement de l'emplacement suivant
    WeaponType weapon;
    int   ammo;             // munitions de l'arme spéciale
    Color color;
    char  name[16];
    int   kills, placement;
    // IA
    int   target;
    float aiMoveDir, aiMoveTimer, aiFireTimer, aiAimErr, aiStuck;
} Tank;

typedef struct Rocket {
    bool  active;
    float x, y, vx, vy;
    float life, trailT;
    int   owner;
    WeaponType type;
} Rocket;

typedef struct Particle {
    bool  active;
    float x, y, vx, vy;
    float life, maxLife, size, grav; // grav = multiplicateur de gravité
    Color color;
} Particle;

typedef struct Crate {
    bool  active, landed;
    float x, y, vy;
    CrateType type;
} Crate;

typedef struct Feed { char text[64]; float t; } Feed;

typedef struct Game {
    GameState state;
    float stateTime;
    bool  paused;
    // règles / map
    MapRules rules;
    int   tankCount;          // joueur + bots
    bool  customTerrain;      // le terrain vient d'une map (solidBackup) et non du générateur
    bool  fromEditor;         // partie lancée depuis l'éditeur -> y retourner ensuite
    // terrain (dimensions de la map courante, alloué par TerrainSetSize)
    int   worldW, worldH;
    unsigned char *solid;     // worldW * worldH : 0 = air, 1 = terre
    unsigned char *solidBackup; // copie intacte de la map (rejouer / retour éditeur)
    Color *pix;               // pixels du terrain (RGBA)
    Texture2D terrainTex;
    bool  texReady;
    // entités (tanks et roquettes dimensionnés selon les règles)
    Tank     *tanks;
    int       tankCap;
    Rocket   *rockets;
    int       rocketCap;
    Particle particles[MAX_PARTICLES];
    Crate    crates[MAX_CRATES];
    Feed     feed[MAX_FEED];
    // partie
    float time;
    float waterLevel, waterSpeed, waterTimer, waterWarn;
    int   waterPhase;
    float crateTimer;
    float shake;
    float overTimer;
    int   aliveCount, winner;
    bool  playerAuto;         // mode autotest : le joueur est piloté par l'IA
    bool  audio;
    unsigned int seed;
} Game;

extern Game G;

static inline float WaterStartY(void) { return (float)G.worldH - WATER_MARGIN; }

// terrain.c
void  TerrainSetSize(int w, int h, bool preserve);       // (ré)alloue le terrain ; preserve = garder le dessin
void  TerrainGen(void);
void  TerrainRefresh(void);                              // recalcule couleurs + texture depuis G.solid
void  TerrainCarve(float cx, float cy, float r);         // explosion (bords roussis)
void  TerrainPaint(float cx, float cy, float r, bool add); // pinceau de l'éditeur (recolore proprement)
void  TerrainClear(void);
bool  SolidAt(int x, int y);
bool  BoxSolid(float x0, float y0, float x1, float y1);
float SurfaceYAt(float x, float fromY);   // 1er pixel solide en descendant, sinon worldH
void  TerrainDraw(void);

// tank.c
void    TankSpawnAll(void);
void    TankUpdate(int idx, float dt);
void    TankDraw(const Tank *t);
void    TankDamage(int idx, float dmg, int attacker);   // attacker -1 = environnement
void    TankKill(int idx, int attacker, bool drowned);
void    TankImpulse(Tank *t, float ix, float iy);
Vector2 TankPivot(const Tank *t);    // base de la tourelle (suit l'orientation)
Vector2 TankMuzzle(const Tank *t);

// rocket.c
void FireWeapon(int owner);
void RocketsUpdate(float dt);
void RocketsDraw(void);
void Explode(float x, float y, float radius, float dmg, float knock, int owner, bool carve);

// bot.c
void BotUpdate(int idx, float dt);
bool SolveBallistic(Vector2 from, Vector2 to, float speed, float grav, float *outAngle);

// crate.c
void CratesUpdate(float dt);
void CratesDraw(void);

// fx.c
void FxInitSounds(void);
void FxUnloadSounds(void);
void SfxPlay(SfxId id, float vol, float pitchVar);
void ParticleAdd(float x, float y, float vx, float vy, float life, float size, float grav, Color c);
void FxExplosion(float x, float y, float r);
void FxSplash(float x, float y);
void FxMuzzle(float x, float y, float angle);
void ParticlesUpdate(float dt);
void ParticlesDraw(void);
void AddFeed(const char *fmt, ...);

// map.c : une map = règles + terrain (RLE), fichier maps/<nom>.tsm
MapRules DefaultRules(void);
void ClampRules(MapRules *r);                            // garde-fous techniques uniquement
bool MapSave(const char *name);
bool MapLoad(const char *path);                          // remplit G.rules + G.solid (redimensionne)
int  MapListNames(char names[][48], int max);

// editor.c : éditeur de maps + widgets d'interface
void EditorInit(bool keepTerrain, const char *name);
int  EditorFrame(float dt);   // 0 = rien, 1 = tester la map, 2 = menu, 3 = charger une map
bool  UiButton(Rectangle r, const char *label);
float UiSlider(Rectangle r, const char *label, float v, float vmin, float vmax, float step, const char *fmt);
void  UiTextBox(Rectangle r, char *buf, int cap, bool *focus);

// main.c (utilitaires partagés)
void         DrawSky(void);
float        Clampf(float v, float a, float b);
float        Lerpf(float a, float b, float t);
float        LerpAngle(float a, float b, float t);
unsigned int RndU(void);
float        Rndf(float a, float b);
int          RndI(int a, int b);   // [a, b] inclus

#endif
