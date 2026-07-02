// TankStorm : boucle principale, états de jeu, caméra, HUD, menu
#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

Game G = { 0 };

// ---------- Utilitaires partagés ----------
float Clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
float Lerpf(float a, float b, float t)  { return a + (b - a) * t; }

float LerpAngle(float a, float b, float t)
{
    float d = atan2f(sinf(b - a), cosf(b - a));
    return a + d * t;
}

unsigned int RndU(void) { G.seed = G.seed * 1664525u + 1013904223u; return G.seed >> 8; }
float Rndf(float a, float b) { return a + (RndU() % 65536u) / 65535.0f * (b - a); }
int   RndI(int a, int b)     { return a + (int)(RndU() % (unsigned int)(b - a + 1)); }

static unsigned int NextSeed(void) { return (unsigned int)time(NULL) ^ (RndU() << 1) ^ 0x9E3779B9u; }

static void DrawTextCentered(const char *txt, int x, int y, int size, Color c)
{
    DrawText(txt, x - MeasureText(txt, size) / 2, y, size, c);
}

// ---------- Partie ----------
static float sCamX = DEF_WORLD_W * 0.5f;
static float sCamY = DEF_WORLD_H * 0.5f;
static bool  sMapSelForEditor = false;
static char  sMapNames[MAX_MAPS][48];
static int   sMapCount = 0;
static bool  sMapListDirty = true;
static int   sMapScroll = 0;

// Tanks et roquettes suivent le nombre de bots de la map : aucune limite fixe
static void EnsurePools(void)
{
    if (G.tankCount > G.tankCap) {
        G.tankCap = G.tankCount;
        G.tanks = (Tank *)realloc(G.tanks, sizeof(Tank) * G.tankCap);
    }
    int wantRockets = 64 + 24 * G.tankCount;
    if (wantRockets > G.rocketCap) {
        G.rocketCap = wantRockets;
        G.rockets = (Rocket *)realloc(G.rockets, sizeof(Rocket) * G.rocketCap);
    }
}

static void ResetMatch(unsigned int seed)
{
    G.seed = seed;
    ClampRules(&G.rules);
    G.tankCount = (G.rules.numBots + 1 < 1) ? 1 : G.rules.numBots + 1;
    EnsurePools();
    memset(G.rockets, 0, sizeof(Rocket) * G.rocketCap);
    memset(G.particles, 0, sizeof(G.particles));
    memset(G.crates, 0, sizeof(G.crates));
    memset(G.feed, 0, sizeof(G.feed));
    G.time = 0;
    G.shake = 0;
    G.overTimer = 0;
    G.winner = -1;
    G.paused = false;
    G.waterSpeed = 0;
    G.waterPhase = 0;
    G.waterTimer = G.rules.waterGrace;
    G.waterWarn = 0;
    G.crateTimer = 6;
    if (G.customTerrain) {   // map chargée / éditeur : repartir de la copie intacte
        memcpy(G.solid, G.solidBackup, (size_t)G.worldW * G.worldH);
        TerrainRefresh();
    } else {
        TerrainSetSize(G.rules.worldW, G.rules.worldH, false);
        TerrainGen();
    }
    G.waterLevel = WaterStartY();
    TankSpawnAll();
    sCamX = G.tanks[0].x;
    sCamY = G.tanks[0].y;
    G.state = ST_COUNTDOWN;
    G.stateTime = 0;
}

static void StartRandomMatch(void)
{
    G.rules = DefaultRules();
    G.customTerrain = false;
    G.fromEditor = false;
    ResetMatch(NextSeed());
}

static void BackToEditor(void)
{
    memcpy(G.solid, G.solidBackup, (size_t)G.worldW * G.worldH);
    TerrainRefresh();
    G.paused = false;
    G.state = ST_EDITOR;
}

static void WaterUpdate(float dt)
{
    G.waterTimer -= dt;
    if (G.waterTimer <= 0) {
        G.waterPhase++;
        G.waterSpeed = (G.waterPhase == 1) ? G.rules.waterSpeed
                                           : fminf(G.waterSpeed + 5.0f, G.rules.waterSpeed + 24.0f);
        G.waterTimer = 22.0f;
        G.waterWarn = 3.0f;
    }
    G.waterWarn -= dt;
    G.waterLevel -= G.waterSpeed * dt;
    if (G.waterLevel < 140) G.waterLevel = 140;
}

static void PlayerInput(Camera2D cam, bool allowMove)
{
    Tank *p = &G.tanks[0];
    if (!p->alive) return;

    float dir = 0;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dir -= 1;    // KEY_A = touche Q en AZERTY
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dir += 1;
    p->aiMoveDir = allowMove ? dir : 0;

    Vector2 mw = GetScreenToWorld2D(GetMousePosition(), cam);
    Vector2 pv = TankPivot(p);
    p->aimAngle = atan2f(mw.y - pv.y, mw.x - pv.x);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) FireWeapon(0);
}

// ---------- Rendu ----------
void DrawSky(void)
{
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H,
                           (Color){ 118, 182, 234, 255 }, (Color){ 190, 224, 244, 255 });
    DrawCircleGradient(1080, 110, 64, Fade(YELLOW, 0.55f), BLANK);
    DrawCircleV((Vector2){ 1080, 110 }, 28, (Color){ 255, 240, 185, 255 });

    float t = (float)GetTime();
    for (int i = 0; i < 4; i++) {
        float cx = fmodf(i * 420.0f + t * 9.0f, (float)SCREEN_W + 300.0f) - 150.0f;
        float cy = 60.0f + i * 52.0f;
        DrawEllipse((int)cx, (int)cy, 56, 17, Fade(WHITE, 0.85f));
        DrawEllipse((int)cx + 36, (int)cy + 8, 40, 13, Fade(WHITE, 0.75f));
        DrawEllipse((int)cx - 38, (int)cy + 7, 37, 12, Fade(WHITE, 0.75f));
    }
}

static void DrawWater(void)
{
    float wl = G.waterLevel;
    float t = (float)GetTime();

    // Larges marges : la caméra centrée sur le joueur peut voir au-delà des bords
    DrawRectangle(-2000, (int)wl + 3, G.worldW + 4000, G.worldH - (int)wl + 2400, (Color){ 30, 92, 172, 150 });
    DrawRectangle(-2000, (int)wl + 70, G.worldW + 4000, G.worldH - (int)wl + 2400, (Color){ 18, 58, 130, 110 });
    DrawRectangle(-700, (int)wl + 3, G.worldW + 1400, 5, (Color){ 120, 190, 235, 120 });

    for (int x = -700; x < G.worldW + 700; x += 8) {
        float y1 = wl + sinf(t * 2.2f + x * 0.018f) * 3.5f;
        float y2 = wl + sinf(t * 2.2f + (x + 8) * 0.018f) * 3.5f;
        DrawLineEx((Vector2){ (float)x, y1 }, (Vector2){ (float)(x + 8), y2 }, 2.5f,
                   (Color){ 150, 205, 245, 200 });
    }
}

static void DrawAimHelp(Camera2D cam)
{
    Tank *p = &G.tanks[0];
    Vector2 m = TankMuzzle(p);
    float vx = cosf(p->aimAngle) * G.rules.rocketSpeed;
    float vy = sinf(p->aimAngle) * G.rules.rocketSpeed;

    for (int i = 1; i <= 11; i++) {
        float t = i * 0.055f;
        float px = m.x + vx * t;
        float py = m.y + vy * t + 0.5f * ROCKET_GRAV * t * t;
        DrawCircleV((Vector2){ px, py }, 2.4f - i * 0.12f, Fade(WHITE, 0.55f - i * 0.04f));
        if (SolidAt((int)px, (int)py) || py > G.waterLevel) break;
    }

    Vector2 mw = GetScreenToWorld2D(GetMousePosition(), cam);
    DrawCircleLines((int)mw.x, (int)mw.y, 7, Fade(WHITE, 0.8f));
    DrawLineEx((Vector2){ mw.x - 11, mw.y }, (Vector2){ mw.x - 4, mw.y }, 1.5f, Fade(WHITE, 0.8f));
    DrawLineEx((Vector2){ mw.x + 4, mw.y }, (Vector2){ mw.x + 11, mw.y }, 1.5f, Fade(WHITE, 0.8f));
    DrawLineEx((Vector2){ mw.x, mw.y - 11 }, (Vector2){ mw.x, mw.y - 4 }, 1.5f, Fade(WHITE, 0.8f));
    DrawLineEx((Vector2){ mw.x, mw.y + 4 }, (Vector2){ mw.x, mw.y + 11 }, 1.5f, Fade(WHITE, 0.8f));
}

static const char *WeaponLabel(const Tank *t)
{
    switch (t->weapon) {
    case W_TRIPLE: return TextFormat("Tir triple ×%d", t->ammo);
    case W_BIG:    return TextFormat("Méga roquette ×%d", t->ammo);
    default:       return "";
    }
}

static void DrawHUD(void)
{
    const Tank *p = &G.tanks[0];

    // Vie (haut gauche)
    DrawRectangle(16, 14, 204, 22, Fade(BLACK, 0.45f));
    float hpf = Clampf(p->hp, 0, G.rules.tankHp) / G.rules.tankHp;
    Color hpCol = (hpf > 0.5f) ? LIME : (hpf > 0.25f) ? ORANGE : RED;
    DrawRectangle(18, 16, (int)(200 * hpf), 18, hpCol);
    DrawRectangleLinesEx((Rectangle){ 16, 14, 204, 22 }, 2, Fade(BLACK, 0.6f));
    DrawText(TextFormat("PV %d", (int)Clampf(p->hp, 0, 999999)), 24, 17, 16, WHITE);

    // Emplacements de roquettes (icônes, ou compteur si trop nombreux)
    int slots = G.rules.rocketSlots;
    int labelX = 16;
    if (slots <= 12) {
        for (int i = 0; i < slots; i++) {
            Rectangle rr = { 16 + i * 24.0f, 42, 20, 11 };
            DrawRectangleRec(rr, Fade(BLACK, 0.45f));
            if (i < p->shots) {
                DrawRectangleRec((Rectangle){ rr.x + 2, rr.y + 2, rr.width - 4, rr.height - 4 },
                                 (Color){ 255, 205, 60, 255 });
            } else if (i == p->shots) {   // emplacement en cours de rechargement
                float f = Clampf(p->reload / G.rules.reloadTime, 0, 1);
                DrawRectangleRec((Rectangle){ rr.x + 2, rr.y + 2, (rr.width - 4) * f, rr.height - 4 },
                                 Fade((Color){ 255, 205, 60, 255 }, 0.45f));
            }
            DrawRectangleLinesEx(rr, 1, Fade(BLACK, 0.6f));
        }
        labelX = 16 + slots * 24 + 10;
    } else {
        DrawText(TextFormat("Roquettes : %d/%d", p->shots, slots), 16, 40, 16,
                 (Color){ 255, 205, 60, 255 });
        if (p->shots < slots) {
            float f = Clampf(p->reload / G.rules.reloadTime, 0, 1);
            DrawRectangle(16, 58, (int)(140 * f), 4, Fade((Color){ 255, 205, 60, 255 }, 0.6f));
        }
        labelX = 190;
    }
    if (p->weapon != W_ROCKET)
        DrawText(WeaponLabel(p), labelX, 40, 16, RAYWHITE);

    // Compteur de survivants + chrono (haut centre)
    DrawTextCentered(TextFormat("%d/%d tanks", G.aliveCount, G.tankCount), SCREEN_W / 2, 14, 24, RAYWHITE);
    int sec = (int)G.time;
    DrawTextCentered(TextFormat("%d:%02d", sec / 60, sec % 60), SCREEN_W / 2, 40, 16, Fade(RAYWHITE, 0.8f));

    // Éliminations (haut droite)
    DrawText(TextFormat("Éliminations : %d", p->kills), SCREEN_W - 176, 17, 16, RAYWHITE);

    // Fil des événements
    for (int i = 0; i < MAX_FEED; i++) {
        if (G.feed[i].t <= 0) continue;
        float a = Clampf(G.feed[i].t / 1.5f, 0, 1);
        DrawTextCentered(G.feed[i].text, SCREEN_W / 2, 66 + i * 18, 14, Fade(RAYWHITE, 0.85f * a));
    }

    // Alertes montée des eaux
    if (G.waterWarn > 0 && G.waterPhase > 0) {
        float a = 0.55f + 0.45f * sinf((float)GetTime() * 9.0f);
        DrawTextCentered("L'EAU MONTE !", SCREEN_W / 2, 150, 34, Fade((Color){ 90, 190, 255, 255 }, a));
    } else if (G.waterPhase == 0 && G.waterTimer < 5.0f) {
        DrawTextCentered("L'eau va bientôt monter...", SCREEN_W / 2, 150, 20,
                         Fade((Color){ 90, 190, 255, 255 }, 0.8f));
    }

    // Joueur éliminé : bandeau
    if (!p->alive && G.state == ST_PLAY && !G.playerAuto) {
        DrawRectangle(0, SCREEN_H - 74, SCREEN_W, 44, Fade(BLACK, 0.55f));
        DrawTextCentered(TextFormat("Éliminé - %d%s place · Entrée : rejouer · Échap : pause",
                                    p->placement, (p->placement == 1) ? "re" : "e"),
                         SCREEN_W / 2, SCREEN_H - 62, 20, RAYWHITE);
    }
}

static bool MenuFrame(void)   // renvoie true si l'utilisateur quitte
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.35f));

    DrawTextCentered("TANKSTORM", SCREEN_W / 2 + 4, 84, 76, Fade(BLACK, 0.6f));
    DrawTextCentered("TANKSTORM", SCREEN_W / 2, 80, 76, (Color){ 255, 205, 60, 255 });
    DrawTextCentered("Un clone de Rocket Bot Royale · 100 % C + raylib", SCREEN_W / 2, 166, 20, RAYWHITE);

    Rectangle b = { SCREEN_W / 2 - 170, 250, 340, 46 };
    if (UiButton(b, "Jouer : île aléatoire") || IsKeyPressed(KEY_ENTER)) StartRandomMatch();
    b.y += 58;
    if (UiButton(b, "Choisir une map")) {
        G.state = ST_MAPSELECT;
        sMapSelForEditor = false;
        sMapListDirty = true;
    }
    b.y += 58;
    if (UiButton(b, "Éditeur de maps")) {
        G.rules = DefaultRules();
        EditorInit(false, NULL);
        G.state = ST_EDITOR;
    }
    b.y += 58;
    bool quit = UiButton(b, "Quitter") || IsKeyPressed(KEY_ESCAPE);

    DrawTextCentered("Q / D ou Flèches : bouger · Souris : viser · Clic gauche : tirer",
                     SCREEN_W / 2, 600, 18, RAYWHITE);
    DrawTextCentered("Rocket jump : tirez au sol sous votre tank ! · Rafale de 3 roquettes, rechargement auto",
                     SCREEN_W / 2, 626, 18, (Color){ 255, 205, 60, 255 });
    DrawTextCentered("L'eau monte : le dernier tank en vie gagne · Caisses : soins et armes spéciales",
                     SCREEN_W / 2, 652, 18, RAYWHITE);
    return quit;
}

static void MapSelectFrame(void)
{
    if (sMapListDirty) {
        sMapCount = MapListNames(sMapNames, MAX_MAPS);
        sMapListDirty = false;
        sMapScroll = 0;
    }

    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.55f));
    DrawTextCentered(sMapSelForEditor ? "Charger une map dans l'éditeur" : "Choisir une map",
                     SCREEN_W / 2, 90, 36, (Color){ 255, 205, 60, 255 });

    Rectangle b = { SCREEN_W / 2 - 210, 160, 420, 38 };
    if (!sMapSelForEditor) {
        if (UiButton(b, "Île aléatoire (règles par défaut)")) { StartRandomMatch(); return; }
        b.y += 46;
    }

    // Liste déroulante à la molette si trop de maps
    const int perPage = 9;
    int maxScroll = (sMapCount > perPage) ? sMapCount - perPage : 0;
    sMapScroll -= (int)GetMouseWheelMove();
    sMapScroll = (int)Clampf((float)sMapScroll, 0, (float)maxScroll);

    int shown = (sMapCount < perPage) ? sMapCount : perPage;
    for (int k = 0; k < shown; k++) {
        int i = sMapScroll + k;
        if (UiButton(b, sMapNames[i])) {
            char path[300];
            snprintf(path, sizeof(path), "maps/%s.tsm", sMapNames[i]);
            if (MapLoad(path)) {
                if (sMapSelForEditor) {
                    EditorInit(true, sMapNames[i]);
                    G.state = ST_EDITOR;
                } else {
                    memcpy(G.solidBackup, G.solid, (size_t)G.worldW * G.worldH);
                    G.customTerrain = true;
                    G.fromEditor = false;
                    ResetMatch(NextSeed());
                }
            }
            return;
        }
        b.y += 46;
    }
    if (maxScroll > 0)
        DrawTextCentered(TextFormat("Molette : défiler (%d-%d / %d)", sMapScroll + 1,
                                    sMapScroll + shown, sMapCount),
                         SCREEN_W / 2, (int)b.y + 4, 14, Fade(RAYWHITE, 0.6f));
    if (sMapCount == 0)
        DrawTextCentered("Aucune map sauvegardée : créez-en une dans l'éditeur !",
                         SCREEN_W / 2, (int)b.y + 10, 20, Fade(RAYWHITE, 0.8f));

    if (UiButton((Rectangle){ SCREEN_W / 2 - 100, SCREEN_H - 70, 200, 40 }, "Retour") ||
        IsKeyPressed(KEY_ESCAPE))
        G.state = sMapSelForEditor ? ST_EDITOR : ST_MENU;
}

static void DrawOverlayOver(void)
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.45f));
    const Tank *p = &G.tanks[0];

    if (G.winner == 0 && !G.playerAuto) {
        DrawTextCentered("VICTOIRE ROYALE !", SCREEN_W / 2, 200, 60, (Color){ 255, 205, 60, 255 });
    } else if (G.winner >= 0) {
        DrawTextCentered(TextFormat("%s remporte la partie !", G.tanks[G.winner].name),
                         SCREEN_W / 2, 210, 44, RAYWHITE);
    } else {
        DrawTextCentered("Égalité : tout le monde a coulé !", SCREEN_W / 2, 210, 44, RAYWHITE);
    }

    int place = (G.winner == 0) ? 1 : p->placement;
    DrawTextCentered(TextFormat("Votre place : %d/%d · Éliminations : %d", place, G.tankCount, p->kills),
                     SCREEN_W / 2, 300, 24, RAYWHITE);
    DrawTextCentered(G.fromEditor ? "Entrée / clic : rejouer · Échap : éditeur"
                                  : "Entrée / clic : rejouer · Échap : menu",
                     SCREEN_W / 2, 380, 20, Fade(RAYWHITE, 0.8f));
}

// ---------- Programme ----------
int main(int argc, char **argv)
{
    bool selftest = false;
    unsigned int matchSeed = 42;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--selftest") == 0) selftest = true;
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
            matchSeed = (unsigned int)atoi(argv[++i]);
    }

    SetConfigFlags(selftest ? FLAG_MSAA_4X_HINT : (FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT));
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_W, SCREEN_H, "TankStorm - Rocket Bot Royale en C");
    SetExitKey(KEY_NULL);
    if (!selftest) SetTargetFPS(60);

    InitAudioDevice();
    G.audio = IsAudioDeviceReady();
    FxInitSounds();

    G.seed = (unsigned int)time(NULL);
    G.rules = DefaultRules();
    TerrainSetSize(G.rules.worldW, G.rules.worldH, false);
    G.waterLevel = WaterStartY();
    TerrainGen();               // décor derrière le menu
    G.state = ST_MENU;

    if (selftest) {
        G.playerAuto = true;   // le menu défile 1,5 s puis la partie démarre seule

        // Test aller-retour du format TSM2 : règles modifiées (dont recul) + taille + terrain
        MapRules ref = DefaultRules();
        ref.rocketSlots = 5; ref.tankSpeed = 200; ref.rocketDmg = 55;
        ref.recoil = 300; ref.worldW = 1200; ref.worldH = 800;
        G.rules = ref;
        TerrainSetSize(1200, 800, false);
        TerrainGen();
        TerrainCarve(600, 400, 120);   // une modification repérable
        int solidCount = 0;
        for (int i = 0; i < G.worldW * G.worldH; i++) solidCount += G.solid[i];
        bool okSave = MapSave("__selftest__");
        G.rules = DefaultRules();
        TerrainSetSize(DEF_WORLD_W, DEF_WORLD_H, false);   // force un redimensionnement au chargement
        bool okLoad = MapLoad("maps/__selftest__.tsm");
        int solidCount2 = 0;
        for (int i = 0; i < G.worldW * G.worldH; i++) solidCount2 += G.solid[i];
        bool okRules = (G.rules.rocketSlots == 5 && (int)G.rules.tankSpeed == 200 &&
                        (int)G.rules.rocketDmg == 55 && (int)G.rules.recoil == 300);
        bool okSize = (G.worldW == 1200 && G.worldH == 800);
        printf("SELFTEST map: save=%d load=%d rules=%d taille=%s terrain=%s (%d->%d)\n",
               okSave, okLoad, okRules, okSize ? "OK" : "DIFF",
               (solidCount == solidCount2) ? "OK" : "DIFF", solidCount, solidCount2);
        remove("maps/__selftest__.tsm");
        G.rules = DefaultRules();
        TerrainSetSize(DEF_WORLD_W, DEF_WORLD_H, false);
        TerrainGen();

        // Test d'adhérence : un tank roule d'un bord à l'autre de l'île en continu,
        // il doit rester au contact du sol (pas de rebonds dans les descentes)
        G.customTerrain = false;
        G.fromEditor = false;
        ResetMatch(1234);
        {
            Tank *t = &G.tanks[0];
            t->x = 300;
            t->y = SurfaceYAt(300, 0) - TANK_HALF_H - 2;
            t->vx = t->vy = 0;
            t->surfSpeed = 0;
            t->nx = 0;
            t->ny = -1;
            t->grounded = true;
            // « Parasite » = en l'air à moins de 12 px du sol : un rebond, pas une
            // vraie chute (rouler au-delà d'une falaise ou d'une île flottante est normal)
            int air = 0, total = 0, parasites = 0;
            for (int f = 0; f < 60 * 14 && t->alive; f++) {
                t->aiMoveDir = 1;   // roule vers la droite sans s'arrêter
                TankUpdate(0, 1.0f / 60.0f);
                if (t->x > G.worldW - 300) break;
                total++;
                if (!t->grounded) {
                    air++;
                    float gap = SurfaceYAt(t->x, t->y) - (t->y + TANK_HALF_H);
                    if (gap <= 12) parasites++;
                }
            }
            printf("SELFTEST adherence: sol=%d%% rebonds-parasites=%d (%d frames, x final=%d)\n",
                   (total > 0) ? 100 * (total - air) / total : -1, parasites, total, (int)t->x);
        }
        G.state = ST_MENU;
        TerrainGen();

        // Capture du rendu de l'éditeur (quelques frames pour laisser l'UI s'établir)
        EditorInit(true, "apercu");
        for (int f = 0; f < 3; f++) { BeginDrawing(); EditorFrame(1.0f / 60.0f); EndDrawing(); }
        TakeScreenshot("selftest_editor.png");
        TerrainGen();
    }

    long frames = 0, endFrame = -1;
    bool quit = false, shotMid = false, shotPause = false;

    while (!WindowShouldClose() && !quit) {
        float dt = selftest ? (1.0f / 60.0f) : Clampf(GetFrameTime(), 0.0f, 1.0f / 30.0f);
        frames++;

        // ---- Caméra : toujours centrée sur le joueur (ou un survivant en spectateur) ----
        float zoom = Clampf((float)SCREEN_W / G.worldW, 1.0f, 1.35f);
        float focusX = G.worldW * 0.5f, focusY = G.worldH * 0.55f;
        if (G.state == ST_COUNTDOWN || G.state == ST_PLAY || G.state == ST_OVER) {
            const Tank *f = &G.tanks[0];
            if (!f->alive)
                for (int i = 1; i < G.tankCount; i++)
                    if (G.tanks[i].alive) { f = &G.tanks[i]; break; }
            if (f->alive) { focusX = f->x; focusY = f->y; }
        }
        sCamX = Lerpf(sCamX, focusX, Clampf(4 * dt, 0, 1));
        sCamY = Lerpf(sCamY, focusY, Clampf(4 * dt, 0, 1));
        G.shake *= expf(-7 * dt);
        if (G.shake < 0.1f) G.shake = 0;
        Camera2D cam = { 0 };
        cam.offset = (Vector2){ SCREEN_W / 2.0f, SCREEN_H / 2.0f };
        cam.target = (Vector2){ sCamX + Rndf(-1, 1) * G.shake, sCamY + Rndf(-1, 1) * G.shake };
        cam.zoom = zoom;

        // ---- Logique (menu / sélection / éditeur : gérés dans le rendu, UI immédiate) ----
        switch (G.state) {
        case ST_COUNTDOWN:
            G.stateTime += dt;
            if (!G.playerAuto) PlayerInput(cam, false);
            for (int i = 0; i < G.tankCount; i++) TankUpdate(i, dt);   // les tanks se posent
            ParticlesUpdate(dt);
            if (G.stateTime >= 3.0f) { G.state = ST_PLAY; G.time = 0; }
            break;

        case ST_PLAY:
            if (IsKeyPressed(KEY_ESCAPE)) G.paused = !G.paused;
            if (G.paused) {
                // KEY_M = position AZERTY « , » ; KEY_SEMICOLON = touche physique « M » d'un AZERTY
                if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_SEMICOLON)) {
                    if (G.fromEditor) BackToEditor();
                    else { G.state = ST_MENU; G.paused = false; }
                }
                break;
            }
            G.time += dt;
            WaterUpdate(dt);

            if (G.playerAuto) BotUpdate(0, dt);
            else PlayerInput(cam, true);
            for (int i = 1; i < G.tankCount; i++) BotUpdate(i, dt);
            for (int i = 0; i < G.tankCount; i++) TankUpdate(i, dt);
            RocketsUpdate(dt);
            CratesUpdate(dt);
            ParticlesUpdate(dt);
            for (int i = 0; i < MAX_FEED; i++) G.feed[i].t -= dt;

            if (!G.tanks[0].alive && !G.playerAuto && IsKeyPressed(KEY_ENTER))
                ResetMatch(NextSeed());

            if (G.aliveCount <= 1) {
                G.overTimer += dt;
                if (G.overTimer > 1.4f) {
                    for (int i = 0; i < G.tankCount; i++)
                        if (G.tanks[i].alive) { G.winner = i; G.tanks[i].placement = 1; }
                    G.state = ST_OVER;
                    G.stateTime = 0;
                }
            }
            break;

        case ST_OVER:
            G.stateTime += dt;
            ParticlesUpdate(dt);
            for (int i = 0; i < MAX_FEED; i++) G.feed[i].t -= dt;
            if (G.stateTime > 0.6f &&
                (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
                ResetMatch(NextSeed());
            else if (IsKeyPressed(KEY_ESCAPE)) {
                if (G.fromEditor) BackToEditor();
                else G.state = ST_MENU;
            }
            break;

        default: break;
        }

        // ---- Rendu ----
        BeginDrawing();
        if (G.state == ST_EDITOR) {
            int act = EditorFrame(dt);
            if (act == 1) {          // tester la map
                memcpy(G.solidBackup, G.solid, (size_t)G.worldW * G.worldH);
                G.customTerrain = true;
                G.fromEditor = true;
                ResetMatch(NextSeed());
            } else if (act == 2) {   // retour menu
                G.state = ST_MENU;
                G.fromEditor = false;
            } else if (act == 3) {   // charger une map
                G.state = ST_MAPSELECT;
                sMapSelForEditor = true;
                sMapListDirty = true;
            }
        } else {
            DrawSky();
            BeginMode2D(cam);
            TerrainDraw();
            CratesDraw();
            for (int i = 0; i < G.tankCount; i++) TankDraw(&G.tanks[i]);
            RocketsDraw();
            ParticlesDraw();
            DrawWater();
            if ((G.state == ST_PLAY || G.state == ST_COUNTDOWN) && G.tanks && G.tanks[0].alive &&
                !G.playerAuto)
                DrawAimHelp(cam);
            EndMode2D();

            if (G.state == ST_MENU) {
                if (MenuFrame()) quit = true;
            } else if (G.state == ST_MAPSELECT) {
                MapSelectFrame();
            } else {
                DrawHUD();
                if (G.state == ST_COUNTDOWN) {
                    int n = 3 - (int)G.stateTime;
                    DrawTextCentered(TextFormat("%d", n), SCREEN_W / 2, 240, 90, (Color){ 255, 205, 60, 255 });
                    DrawTextCentered("Préparez-vous !", SCREEN_W / 2, 340, 24, RAYWHITE);
                }
                if (G.state == ST_PLAY && G.time < 0.7f)
                    DrawTextCentered("GO !", SCREEN_W / 2, 240, 90, (Color){ 120, 230, 90, 255 });
                if (G.state == ST_PLAY && G.paused) {
                    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.5f));
                    DrawTextCentered("PAUSE", SCREEN_W / 2, 220, 60, RAYWHITE);
                    Rectangle pb = { SCREEN_W / 2 - 150, 320, 300, 46 };
                    if (UiButton(pb, "Reprendre")) G.paused = false;
                    pb.y += 58;
                    if (UiButton(pb, G.fromEditor ? "Retour à l'éditeur" : "Menu principal")) {
                        if (G.fromEditor) BackToEditor();
                        else { G.state = ST_MENU; G.paused = false; }
                    }
                    DrawTextCentered("Échap : reprendre", SCREEN_W / 2, 440, 18, Fade(RAYWHITE, 0.7f));
                }
                if (G.state == ST_OVER) DrawOverlayOver();
            }
        }
        DrawFPS(SCREEN_W - 90, SCREEN_H - 24);
        EndDrawing();

        // ---- Autotest : screenshots + arrêt automatique ----
        if (selftest) {
            if (G.state == ST_MENU && frames >= 90) {
                TakeScreenshot("selftest_menu.png");
                G.rules = DefaultRules();
                G.rules.worldW = 2400;   // taille non standard : exerce le suivi
                G.rules.worldH = 720;    // horizontal de la caméra et les marges
                G.customTerrain = false;
                G.fromEditor = false;
                ResetMatch(matchSeed);   // partie déterministe
            }
            if (G.state == ST_PLAY && !shotMid && G.time > 8.0f) {
                TakeScreenshot("selftest_mid.png");
                shotMid = true;
            }
            // Vérification de l'écran de pause. TakeScreenshot (après EndDrawing) lit le
            // buffer arrière = frame précédente, donc on reste en pause quelques frames.
            if (shotMid && !shotPause && G.time > 9.0f) {
                G.paused = true;
                static int ph = 0;
                if (++ph >= 3) { TakeScreenshot("selftest_pause.png"); shotPause = true; G.paused = false; }
            }
            if (G.state == ST_OVER && endFrame < 0) endFrame = frames;
            if (endFrame > 0 && frames == endFrame + 20) TakeScreenshot("selftest_end.png");
            if (endFrame > 0 && frames > endFrame + 30) break;
            if (frames > 60 * 60 * 12) { printf("SELFTEST: TIMEOUT\n"); break; }
        }
    }

    if (selftest) {
        printf("SELFTEST: frames=%ld temps=%.1fs vivants=%d gagnant=%s\n",
               frames, G.time, G.aliveCount,
               (G.winner >= 0) ? G.tanks[G.winner].name : "aucun");
        fflush(stdout);
    }

    FxUnloadSounds();
    CloseAudioDevice();
    if (G.texReady) UnloadTexture(G.terrainTex);
    free(G.solid);
    free(G.solidBackup);
    free(G.pix);
    free(G.tanks);
    free(G.rockets);
    CloseWindow();
    return 0;
}
