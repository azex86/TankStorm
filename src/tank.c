// Tanks : physique pixel (pente, chute), dégâts, rendu
#include "game.h"
#include <string.h>
#include <stdio.h>

static const char *BOT_NAMES[] = { "Rusty", "Boum", "Turbo", "Sarge", "Piksel", "Vortex", "Kaboom" };
static const Color TANK_COLORS[MAX_TANKS] = {
    { 66, 135, 245, 255 },   // joueur (bleu)
    { 230,  70,  70, 255 },
    { 240, 150,  50, 255 },
    { 160,  90, 220, 255 },
    {  90, 190,  90, 255 },
    { 230, 210,  70, 255 },
    { 235, 110, 180, 255 },
    {  70, 200, 200, 255 },
};

static bool TankBoxFree(float cx, float cy)
{
    return !BoxSolid(cx - TANK_HALF_W + 1, cy - TANK_HALF_H,
                     cx + TANK_HALF_W - 1, cy + TANK_HALF_H);
}

static bool OnGround(const Tank *t)
{
    return BoxSolid(t->x - TANK_HALF_W + 2, t->y + TANK_HALF_H,
                    t->x + TANK_HALF_W - 2, t->y + TANK_HALF_H + 2);
}

void TankSpawnAll(void)
{
    memset(G.tanks, 0, sizeof(G.tanks));
    for (int i = 0; i < MAX_TANKS; i++) {
        Tank *t = &G.tanks[i];
        t->alive = true;
        t->isPlayer = (i == 0);
        t->hp = TANK_MAX_HP;
        t->weapon = W_ROCKET;
        t->color = TANK_COLORS[i];
        t->target = -1;
        t->aiAimErr = Rndf(0.05f, 0.16f);
        t->aiFireTimer = Rndf(1.5f, 3.5f);
        t->aimAngle = (i % 2) ? -2.5f : -0.6f;
        if (i == 0) snprintf(t->name, sizeof(t->name), "Vous");
        else        snprintf(t->name, sizeof(t->name), "%s", BOT_NAMES[i - 1]);

        // Position : créneaux répartis sur la largeur, posés sur le sol
        float slotW = (WORLD_W - 360.0f) / (MAX_TANKS - 1);
        float x = 180.0f + i * slotW + Rndf(-35, 35);
        float y = 0;
        for (int attempt = 0; attempt < 20; attempt++) {
            float sy = SurfaceYAt(x, 0);
            y = sy - TANK_HALF_H - 2;
            if (sy < G.waterLevel - 50 && TankBoxFree(x, y)) break;
            x = Clampf(x + Rndf(-80, 80), 180, WORLD_W - 180);
        }
        t->x = x;
        t->y = y;
    }
    G.aliveCount = MAX_TANKS;
}

void TankImpulse(Tank *t, float ix, float iy)
{
    t->vx += ix;
    t->vy += iy;
    if (iy < 0) t->grounded = false;
}

Vector2 TankMuzzle(const Tank *t)
{
    return (Vector2){ t->x + cosf(t->aimAngle) * 20.0f,
                      (t->y - 7.0f) + sinf(t->aimAngle) * 20.0f };
}

// Déplacement pixel par pixel : franchit les pentes (2 px de montée par px horizontal)
static void MoveTank(Tank *t, float dx, float dy)
{
    bool wasGrounded = t->grounded;

    int steps = (int)ceilf(fabsf(dx));
    if (steps > 0) {
        float sx = dx / steps;
        for (int i = 0; i < steps; i++) {
            float nx = t->x + sx;
            bool moved = false;
            for (int climb = 0; climb <= 2; climb++) {
                if (TankBoxFree(nx, t->y - climb)) {
                    t->x = nx;
                    t->y -= climb;
                    moved = true;
                    break;
                }
            }
            if (!moved) { t->vx = 0; break; }
        }
    }

    // Coller au sol en descente pour éviter les micro-rebonds
    if (wasGrounded && t->vy >= 0) {
        int k = 0;
        while (k < 4 && TankBoxFree(t->x, t->y + 1) && !OnGround(t)) { t->y += 1; k++; }
    }

    steps = (int)ceilf(fabsf(dy));
    if (steps > 0) {
        float sy = dy / steps;
        for (int i = 0; i < steps; i++) {
            float ny = t->y + sy;
            if (TankBoxFree(t->x, ny)) t->y = ny;
            else { t->vy = 0; break; }
        }
    }
}

void TankUpdate(int idx, float dt)
{
    Tank *t = &G.tanks[idx];
    if (!t->alive) return;

    t->cooldown -= dt;
    t->hitFlash -= dt;

    bool inWater = (t->y > G.waterLevel);

    // Accélération horizontale
    float target = t->aiMoveDir * TANK_SPEED * (inWater ? 0.55f : 1.0f);
    float accel = t->grounded ? 1000.0f : 260.0f;
    if (fabsf(t->aiMoveDir) < 0.01f && t->grounded)
        t->vx = Lerpf(t->vx, 0, Clampf(12.0f * dt, 0, 1));
    else
        t->vx += Clampf(target - t->vx, -accel * dt, accel * dt);

    // Gravité (amortie sous l'eau)
    t->vy += GRAVITY * (inWater ? 0.35f : 1.0f) * dt;
    if (inWater) {
        t->vy = Lerpf(t->vy, 0, Clampf(2.5f * dt, 0, 1));
        if (t->vy > 90) t->vy = 90;
    }
    t->vx = Clampf(t->vx, -420, 420);
    t->vy = Clampf(t->vy, -640, 700);

    MoveTank(t, t->vx * dt, t->vy * dt);

    // Limites du monde
    if (t->x < TANK_HALF_W) { t->x = TANK_HALF_W; if (t->vx < 0) t->vx = 0; }
    if (t->x > WORLD_W - TANK_HALF_W) { t->x = WORLD_W - TANK_HALF_W; if (t->vx > 0) t->vx = 0; }
    if (t->y < -300) { t->y = -300; if (t->vy < 0) t->vy = 0; }

    t->grounded = OnGround(t);

    // Inclinaison du châssis selon la pente
    if (t->grounded) {
        float ly = SurfaceYAt(t->x - 9, t->y - 14);
        float ry = SurfaceYAt(t->x + 9, t->y - 14);
        if (ly < WORLD_H && ry < WORLD_H && fabsf(ry - ly) < 22) {
            float a = atan2f(ry - ly, 18.0f);
            t->bodyAngle = LerpAngle(t->bodyAngle, a, Clampf(10.0f * dt, 0, 1));
        }
    } else {
        t->bodyAngle = LerpAngle(t->bodyAngle, 0, Clampf(3.0f * dt, 0, 1));
    }

    // Noyade
    if (inWater) {
        t->hp -= 42.0f * dt;
        if (RndU() % 100 < 12)
            ParticleAdd(t->x + Rndf(-8, 8), t->y - 6, Rndf(-6, 6), Rndf(-45, -25),
                        Rndf(0.5f, 1.1f), Rndf(1.5f, 3.0f), -0.25f,
                        (Color){ 200, 230, 255, 180 });
        if (t->hp <= 0) { TankKill(idx, -1, true); return; }
    }
    if (t->y > WORLD_H + 80) { TankKill(idx, -1, true); return; }
}

void TankDamage(int idx, float dmg, int attacker)
{
    Tank *t = &G.tanks[idx];
    if (!t->alive || dmg <= 0) return;
    t->hp -= dmg;
    t->hitFlash = 0.15f;
    if (t->hp <= 0) TankKill(idx, attacker, false);
}

void TankKill(int idx, int attacker, bool drowned)
{
    Tank *t = &G.tanks[idx];
    if (!t->alive) return;
    t->alive = false;
    t->hp = 0;
    G.aliveCount--;
    t->placement = G.aliveCount + 1;

    FxExplosion(t->x, t->y, 30);
    for (int i = 0; i < 14; i++)   // débris aux couleurs du tank
        ParticleAdd(t->x, t->y, Rndf(-220, 220), Rndf(-330, -60),
                    Rndf(0.6f, 1.3f), Rndf(2, 4), 1.0f, t->color);
    SfxPlay(SFX_DIE, 0.9f, 0.12f);
    G.shake = fmaxf(G.shake, 10.0f);

    if (drowned)
        AddFeed("%s s'est noyé", t->name);
    else if (attacker >= 0 && attacker != idx) {
        G.tanks[attacker].kills++;
        AddFeed("%s a détruit %s", G.tanks[attacker].name, t->name);
    } else
        AddFeed("%s a explosé", t->name);
}

void TankDraw(const Tank *t)
{
    if (!t->alive) return;
    float deg = t->bodyAngle * RAD2DEG;
    Color body = (t->hitFlash > 0) ? WHITE : t->color;
    Color darkBody = ColorBrightness(body, -0.25f);

    // Chenilles
    DrawRectanglePro((Rectangle){ t->x, t->y + 3, 30, 9 }, (Vector2){ 15, 4.5f }, deg,
                     (Color){ 55, 55, 60, 255 });
    // Châssis
    DrawRectanglePro((Rectangle){ t->x, t->y - 2, 26, 10 }, (Vector2){ 13, 5 }, deg, body);
    // Tourelle + canon
    Vector2 pivot = { t->x, t->y - 7 };
    Vector2 muzzle = { pivot.x + cosf(t->aimAngle) * 18.0f, pivot.y + sinf(t->aimAngle) * 18.0f };
    DrawLineEx(pivot, muzzle, 4.0f, ColorBrightness(body, -0.4f));
    DrawCircleV(pivot, 5.5f, darkBody);

    // Barre de vie + nom
    float w = 30;
    DrawRectangle((int)(t->x - w / 2), (int)(t->y - 27), (int)w, 4, Fade(BLACK, 0.5f));
    Color hpCol = (t->hp > 50) ? LIME : (t->hp > 25) ? ORANGE : RED;
    DrawRectangle((int)(t->x - w / 2), (int)(t->y - 27), (int)(w * t->hp / TANK_MAX_HP), 4, hpCol);

    int fs = 12;
    int tw = MeasureText(t->name, fs);
    DrawText(t->name, (int)(t->x - tw / 2), (int)(t->y - 42), fs,
             t->isPlayer ? (Color){ 255, 235, 120, 255 } : Fade(RAYWHITE, 0.8f));
    if (t->isPlayer) {
        // petit repère au-dessus du joueur
        DrawTriangle((Vector2){ t->x, t->y - 48 },
                     (Vector2){ t->x + 5, t->y - 56 },
                     (Vector2){ t->x - 5, t->y - 56 },
                     (Color){ 255, 235, 120, 220 });
    }
}
