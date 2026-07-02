// TankStorm — clone de Rocket Bot Royale en C + raylib
#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>
#include <math.h>

// ---- Dimensions ----
#define SCREEN_W 1280
#define SCREEN_H 720
#define WORLD_W  1600
#define WORLD_H  960

#define MAX_TANKS     8
#define MAX_ROCKETS   96
#define MAX_PARTICLES 1024
#define MAX_CRATES    6
#define MAX_FEED      5

// ---- Physique / équilibrage ----
#define GRAVITY        640.0f
#define TANK_HALF_W    12.0f
#define TANK_HALF_H    7.0f
#define TANK_SPEED     115.0f
#define TANK_MAX_HP    100.0f
#define FIRE_COOLDOWN  0.85f
#define ROCKET_SPEED   540.0f
#define ROCKET_GRAV    430.0f
#define WATER_START    905.0f
#define WATER_GRACE    18.0f

typedef enum { W_ROCKET = 0, W_TRIPLE, W_BIG } WeaponType;
typedef enum { ST_MENU = 0, ST_COUNTDOWN, ST_PLAY, ST_OVER } GameState;
typedef enum { CR_HEAL = 0, CR_TRIPLE, CR_BIG } CrateType;
typedef enum { SFX_FIRE = 0, SFX_BOOM, SFX_PICK, SFX_SPLASH, SFX_DIE, SFX_COUNT } SfxId;

typedef struct Tank {
    bool  alive, isPlayer, grounded;
    float x, y;             // centre du corps
    float vx, vy;
    float bodyAngle;        // inclinaison du châssis (rendu)
    float aimAngle;         // angle de la tourelle
    float hp, cooldown, hitFlash;
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
    // terrain
    unsigned char *solid;   // WORLD_W * WORLD_H : 0 = air, 1 = terre
    Color *pix;             // pixels du terrain (RGBA)
    Texture2D terrainTex;
    bool  texReady;
    // entités
    Tank     tanks[MAX_TANKS];
    Rocket   rockets[MAX_ROCKETS];
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
    bool  playerAuto;       // mode autotest : le joueur est piloté par l'IA
    bool  audio;
    unsigned int seed;
} Game;

extern Game G;

// terrain.c
void  TerrainGen(void);
void  TerrainCarve(float cx, float cy, float r);
bool  SolidAt(int x, int y);
bool  BoxSolid(float x0, float y0, float x1, float y1);
float SurfaceYAt(float x, float fromY);   // 1er pixel solide en descendant, sinon WORLD_H
void  TerrainDraw(void);

// tank.c
void    TankSpawnAll(void);
void    TankUpdate(int idx, float dt);
void    TankDraw(const Tank *t);
void    TankDamage(int idx, float dmg, int attacker);   // attacker -1 = environnement
void    TankKill(int idx, int attacker, bool drowned);
void    TankImpulse(Tank *t, float ix, float iy);
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

// main.c (utilitaires partagés)
float        Clampf(float v, float a, float b);
float        Lerpf(float a, float b, float t);
float        LerpAngle(float a, float b, float t);
unsigned int RndU(void);
float        Rndf(float a, float b);
int          RndI(int a, int b);   // [a, b] inclus

#endif
