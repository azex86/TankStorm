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
static float sCamY = WORLD_H * 0.5f;

static void ResetMatch(unsigned int seed)
{
    G.seed = seed;
    memset(G.rockets, 0, sizeof(G.rockets));
    memset(G.particles, 0, sizeof(G.particles));
    memset(G.crates, 0, sizeof(G.crates));
    memset(G.feed, 0, sizeof(G.feed));
    G.time = 0;
    G.shake = 0;
    G.overTimer = 0;
    G.winner = -1;
    G.paused = false;
    G.waterLevel = WATER_START;
    G.waterSpeed = 0;
    G.waterPhase = 0;
    G.waterTimer = WATER_GRACE;
    G.waterWarn = 0;
    G.crateTimer = 6;
    TerrainGen();
    TankSpawnAll();
    sCamY = G.tanks[0].y;
    G.state = ST_COUNTDOWN;
    G.stateTime = 0;
}

static void WaterUpdate(float dt)
{
    G.waterTimer -= dt;
    if (G.waterTimer <= 0) {
        G.waterPhase++;
        G.waterSpeed = (G.waterPhase == 1) ? 6.0f : fminf(G.waterSpeed + 5.0f, 30.0f);
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
    p->aimAngle = atan2f(mw.y - (p->y - 7), mw.x - p->x);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) FireWeapon(0);
}

// ---------- Rendu ----------
static void DrawSky(void)
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

    DrawRectangle(-200, (int)wl + 3, WORLD_W + 400, WORLD_H - (int)wl + 400, (Color){ 30, 92, 172, 150 });
    DrawRectangle(-200, (int)wl + 70, WORLD_W + 400, WORLD_H - (int)wl + 400, (Color){ 18, 58, 130, 110 });
    DrawRectangle(0, (int)wl + 3, WORLD_W, 5, (Color){ 120, 190, 235, 120 });

    for (int x = 0; x < WORLD_W; x += 8) {
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
    float vx = cosf(p->aimAngle) * ROCKET_SPEED;
    float vy = sinf(p->aimAngle) * ROCKET_SPEED;

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
    default:       return "Roquettes";
    }
}

static void DrawHUD(void)
{
    const Tank *p = &G.tanks[0];

    // Vie + arme (haut gauche)
    DrawRectangle(16, 14, 204, 22, Fade(BLACK, 0.45f));
    Color hpCol = (p->hp > 50) ? LIME : (p->hp > 25) ? ORANGE : RED;
    DrawRectangle(18, 16, (int)(200 * Clampf(p->hp, 0, TANK_MAX_HP) / TANK_MAX_HP), 18, hpCol);
    DrawRectangleLinesEx((Rectangle){ 16, 14, 204, 22 }, 2, Fade(BLACK, 0.6f));
    DrawText(TextFormat("PV %d", (int)Clampf(p->hp, 0, 999)), 24, 17, 16, WHITE);

    DrawRectangle(16, 42, 10, 10, (p->weapon == W_ROCKET) ? GRAY
                  : (p->weapon == W_TRIPLE) ? (Color){ 255, 230, 90, 255 } : (Color){ 255, 120, 60, 255 });
    DrawText(WeaponLabel(p), 32, 40, 16, RAYWHITE);

    // Compteur de survivants + chrono (haut centre)
    DrawTextCentered(TextFormat("%d/%d tanks", G.aliveCount, MAX_TANKS), SCREEN_W / 2, 14, 24, RAYWHITE);
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
        DrawTextCentered(TextFormat("Éliminé - %d%s place · Entrée : rejouer · Échap : menu",
                                    p->placement, (p->placement == 1) ? "re" : "e"),
                         SCREEN_W / 2, SCREEN_H - 62, 20, RAYWHITE);
    }
}

static void DrawMenu(void)
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.35f));

    DrawTextCentered("TANKSTORM", SCREEN_W / 2 + 4, 104, 76, Fade(BLACK, 0.6f));
    DrawTextCentered("TANKSTORM", SCREEN_W / 2, 100, 76, (Color){ 255, 205, 60, 255 });
    DrawTextCentered("Un clone de Rocket Bot Royale · 100 % C + raylib", SCREEN_W / 2, 186, 20, RAYWHITE);

    Rectangle panel = { SCREEN_W / 2 - 330, 250, 660, 250 };
    DrawRectangleRec(panel, Fade(BLACK, 0.5f));
    DrawRectangleLinesEx(panel, 2, Fade(RAYWHITE, 0.4f));

    int x = (int)panel.x + 30, y = (int)panel.y + 24;
    DrawText("Q / D ou Flèches : se déplacer", x, y, 20, RAYWHITE);            y += 32;
    DrawText("Souris : viser · Clic gauche : tirer", x, y, 20, RAYWHITE);      y += 32;
    DrawText("Rocket jump : tirez au sol sous votre tank !", x, y, 20,
             (Color){ 255, 205, 60, 255 });                                    y += 32;
    DrawText("L'eau monte : le dernier tank en vie gagne.", x, y, 20, RAYWHITE); y += 32;
    DrawText("Caisses parachutées : soins et armes spéciales.", x, y, 20, RAYWHITE); y += 32;
    DrawText("Échap : pause / quitter", x, y, 20, Fade(RAYWHITE, 0.7f));

    float a = 0.6f + 0.4f * sinf((float)GetTime() * 4.0f);
    DrawTextCentered("[ Clic gauche ou Entrée pour jouer ]", SCREEN_W / 2, 550, 26,
                     Fade((Color){ 255, 205, 60, 255 }, a));
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
    DrawTextCentered(TextFormat("Votre place : %d/%d · Éliminations : %d", place, MAX_TANKS, p->kills),
                     SCREEN_W / 2, 300, 24, RAYWHITE);
    DrawTextCentered("Entrée / clic : rejouer · Échap : menu", SCREEN_W / 2, 380, 20,
                     Fade(RAYWHITE, 0.8f));
}

// ---------- Programme ----------
int main(int argc, char **argv)
{
    bool selftest = false;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--selftest") == 0) selftest = true;

    SetConfigFlags(selftest ? FLAG_MSAA_4X_HINT : (FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT));
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_W, SCREEN_H, "TankStorm - Rocket Bot Royale en C");
    SetExitKey(KEY_NULL);
    if (!selftest) SetTargetFPS(60);

    InitAudioDevice();
    G.audio = IsAudioDeviceReady();
    FxInitSounds();

    G.solid = (unsigned char *)malloc(WORLD_W * WORLD_H);
    G.pix = (Color *)malloc(WORLD_W * WORLD_H * sizeof(Color));
    G.seed = (unsigned int)time(NULL);
    G.waterLevel = WATER_START;
    TerrainGen();               // décor derrière le menu
    G.state = ST_MENU;

    if (selftest) G.playerAuto = true;   // le menu défile 1,5 s puis la partie démarre seule

    long frames = 0, endFrame = -1;
    bool quit = false, shotMid = false;

    while (!WindowShouldClose() && !quit) {
        float dt = selftest ? (1.0f / 60.0f) : Clampf(GetFrameTime(), 0.0f, 1.0f / 30.0f);
        frames++;

        // ---- Caméra (suit le joueur, ou un survivant en mode spectateur) ----
        float zoom = (float)SCREEN_W / WORLD_W;
        float viewH = SCREEN_H / zoom;
        float focusY = WORLD_H * 0.55f;
        if (G.state != ST_MENU) {
            const Tank *f = &G.tanks[0];
            if (!f->alive)
                for (int i = 1; i < MAX_TANKS; i++)
                    if (G.tanks[i].alive) { f = &G.tanks[i]; break; }
            if (f->alive) focusY = f->y;
        }
        sCamY = Lerpf(sCamY, Clampf(focusY, viewH / 2, WORLD_H - viewH / 2), Clampf(4 * dt, 0, 1));
        G.shake *= expf(-7 * dt);
        if (G.shake < 0.1f) G.shake = 0;
        Camera2D cam = { 0 };
        cam.offset = (Vector2){ SCREEN_W / 2.0f, SCREEN_H / 2.0f };
        cam.target = (Vector2){ WORLD_W / 2.0f + Rndf(-1, 1) * G.shake, sCamY + Rndf(-1, 1) * G.shake };
        cam.zoom = zoom;

        // ---- Logique ----
        switch (G.state) {
        case ST_MENU:
            if (IsKeyPressed(KEY_ESCAPE)) quit = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER))
                ResetMatch(NextSeed());
            break;

        case ST_COUNTDOWN:
            G.stateTime += dt;
            if (!G.playerAuto) PlayerInput(cam, false);
            for (int i = 0; i < MAX_TANKS; i++) TankUpdate(i, dt);   // les tanks se posent
            ParticlesUpdate(dt);
            if (G.stateTime >= 3.0f) { G.state = ST_PLAY; G.time = 0; }
            break;

        case ST_PLAY:
            if (IsKeyPressed(KEY_ESCAPE)) G.paused = !G.paused;
            if (G.paused) {
                if (IsKeyPressed(KEY_M)) G.state = ST_MENU;
                break;
            }
            G.time += dt;
            WaterUpdate(dt);

            if (G.playerAuto) BotUpdate(0, dt);
            else PlayerInput(cam, true);
            for (int i = 1; i < MAX_TANKS; i++) BotUpdate(i, dt);
            for (int i = 0; i < MAX_TANKS; i++) TankUpdate(i, dt);
            RocketsUpdate(dt);
            CratesUpdate(dt);
            ParticlesUpdate(dt);
            for (int i = 0; i < MAX_FEED; i++) G.feed[i].t -= dt;

            if (!G.tanks[0].alive && !G.playerAuto && IsKeyPressed(KEY_ENTER))
                ResetMatch(NextSeed());

            if (G.aliveCount <= 1) {
                G.overTimer += dt;
                if (G.overTimer > 1.4f) {
                    for (int i = 0; i < MAX_TANKS; i++)
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
            if (IsKeyPressed(KEY_ESCAPE)) G.state = ST_MENU;
            break;
        }

        // ---- Rendu ----
        BeginDrawing();
        DrawSky();
        BeginMode2D(cam);
        TerrainDraw();
        CratesDraw();
        for (int i = 0; i < MAX_TANKS; i++) TankDraw(&G.tanks[i]);
        RocketsDraw();
        ParticlesDraw();
        DrawWater();
        if ((G.state == ST_PLAY || G.state == ST_COUNTDOWN) && G.tanks[0].alive && !G.playerAuto)
            DrawAimHelp(cam);
        EndMode2D();

        if (G.state == ST_MENU) {
            DrawMenu();
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
                DrawTextCentered("PAUSE", SCREEN_W / 2, 280, 60, RAYWHITE);
                DrawTextCentered("Échap : reprendre · M : menu", SCREEN_W / 2, 370, 22, Fade(RAYWHITE, 0.85f));
            }
            if (G.state == ST_OVER) DrawOverlayOver();
        }
        DrawFPS(SCREEN_W - 90, SCREEN_H - 24);
        EndDrawing();

        // ---- Autotest : screenshots + arrêt automatique ----
        if (selftest) {
            if (G.state == ST_MENU && frames >= 90) {
                TakeScreenshot("selftest_menu.png");
                ResetMatch(42);
            }
            if (G.state == ST_PLAY && !shotMid && G.time > 8.0f) {
                TakeScreenshot("selftest_mid.png");
                shotMid = true;
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
    free(G.pix);
    CloseWindow();
    return 0;
}
