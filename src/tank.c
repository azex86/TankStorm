// Tanks : adhérence magnétique au terrain (pentes, murs, plafonds), vol
// balistique quand décroché (recul du tir, souffle d'explosion), dégâts, rendu
#include "game.h"
#include <string.h>
#include <stdio.h>

static const char *BOT_NAMES[] = { "Rusty", "Boum", "Turbo", "Sarge", "Piksel", "Vortex", "Kaboom" };
#define NUM_BOT_NAMES 7
static const Color TANK_COLORS[8] = {
    { 66, 135, 245, 255 },   // joueur (bleu)
    { 230,  70,  70, 255 },
    { 240, 150,  50, 255 },
    { 160,  90, 220, 255 },
    {  90, 190,  90, 255 },
    { 230, 210,  70, 255 },
    { 235, 110, 180, 255 },
    {  70, 200, 200, 255 },
};

// Palette fixe pour les 8 premiers, teintes générées au-delà
static Color TankColor(int i)
{
    if (i < 8) return TANK_COLORS[i];
    return ColorFromHSV(fmodf(i * 47.0f, 360.0f), 0.65f, 0.92f);
}

// ---- Adhérence : le tank rampe le long du contour du terrain, quelle que soit
// son orientation. Le corps est approximé par un disque ; la surface locale est
// caractérisée par sa normale (moyenne des directions opposées aux pixels solides).
#define BODY_R    6.5f               // rayon de collision du châssis
#define CONTACT_R (BODY_R + 2.5f)    // distance de « prise » : au-delà, décroché

static bool DiscSolid(float cx, float cy, float r)
{
    int x0 = (int)(cx - r), x1 = (int)(cx + r);
    int y0 = (int)(cy - r), y1 = (int)(cy + r);
    float r2 = r * r;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            if (dx * dx + dy * dy <= r2 && SolidAt(x, y)) return true;
        }
    return false;
}

static bool SurfNormal(float cx, float cy, float r, float *onx, float *ony)
{
    float sx = 0, sy = 0;
    int x0 = (int)(cx - r), x1 = (int)(cx + r);
    int y0 = (int)(cy - r), y1 = (int)(cy + r);
    float r2 = r * r;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            if (dx * dx + dy * dy <= r2 && SolidAt(x, y)) { sx -= dx; sy -= dy; }
        }
    float len = sqrtf(sx * sx + sy * sy);
    if (len < 0.01f) return false;
    *onx = sx / len;
    *ony = sy / len;
    return true;
}

// Accroche le tank à la surface la plus proche ; la vitesse balistique est
// convertie en vitesse tangentielle (la composante d'impact est absorbée)
static void Attach(Tank *t)
{
    float nx, ny;
    if (!SurfNormal(t->x, t->y, BODY_R + 4.0f, &nx, &ny)) return;
    t->nx = nx;
    t->ny = ny;
    t->grounded = true;
    t->surfSpeed = t->vx * -ny + t->vy * nx;
}

// Avance de `len` px le long de la surface. Essaie d'abord tout droit (tangente),
// puis en tournant vers la surface (contourner un sommet, une corniche) ou vers
// l'extérieur (grimper un mur de face). Renvoie false si complètement bloqué.
static bool CrawlStep(Tank *t, float dir, float len)
{
    float tx = -t->ny * dir, ty = t->nx * dir;
    static const float ANG[] = { 0.26f, 0.0f, 0.52f, -0.26f, 0.79f, -0.52f, 1.05f,
                                 -0.79f, 1.31f, -1.05f, 1.57f, -1.31f, -1.57f };
    for (int i = 0; i < 13; i++) {
        float a = ANG[i] * dir;   // >0 : vers la surface, <0 : s'en éloigner
        float ca = cosf(a), sa = sinf(a);
        float px = t->x + (tx * ca - ty * sa) * len;
        float py = t->y + (tx * sa + ty * ca) * len;
        if (DiscSolid(px, py, BODY_R)) continue;        // pénétrerait le terrain
        if (!DiscSolid(px, py, CONTACT_R)) continue;    // perdrait le contact
        t->x = px;
        t->y = py;
        float nx, ny;
        if (SurfNormal(px, py, BODY_R + 4.0f, &nx, &ny)) { t->nx = nx; t->ny = ny; }
        return true;
    }
    return false;
}

void TankSpawnAll(void)
{
    int n = G.tankCount;
    memset(G.tanks, 0, sizeof(Tank) * G.tankCap);
    for (int i = 0; i < n; i++) {
        Tank *t = &G.tanks[i];
        t->alive = true;
        t->isPlayer = (i == 0);
        t->hp = G.rules.tankHp;
        t->shots = G.rules.rocketSlots;
        t->weapon = W_ROCKET;
        t->color = TankColor(i);
        t->target = -1;
        t->aiAimErr = Rndf(0.05f, 0.16f);
        t->aiFireTimer = Rndf(1.5f, 3.5f);
        t->aimAngle = (i % 2) ? -2.5f : -0.6f;
        if (i == 0) snprintf(t->name, sizeof(t->name), "Vous");
        else if (i - 1 < NUM_BOT_NAMES) snprintf(t->name, sizeof(t->name), "%s", BOT_NAMES[i - 1]);
        else snprintf(t->name, sizeof(t->name), "Bot %d", i);

        // Position : créneaux répartis sur la largeur, posés sur le sol
        float margin = fminf(180.0f, G.worldW * 0.12f);
        float slotW = (G.worldW - 2 * margin) / ((n > 1) ? (n - 1) : 1);
        float x = margin + i * slotW + Rndf(-35, 35);
        float y = 0;
        for (int attempt = 0; attempt < 20; attempt++) {
            float sy = SurfaceYAt(x, 0);
            y = sy - TANK_HALF_H - 2;
            if (sy < G.waterLevel - 50 && !DiscSolid(x, y, BODY_R)) break;
            x = Clampf(x + Rndf(-80, 80), margin, G.worldW - margin);
        }
        t->x = x;
        t->y = y;
        t->nx = 0;
        t->ny = -1;
        t->grounded = true;
        t->surfSpeed = 0;
    }
    G.aliveCount = n;
}

void TankImpulse(Tank *t, float ix, float iy)
{
    if (t->grounded) {
        if (ix * ix + iy * iy < 30.0f * 30.0f) return;   // petit souffle : l'adhérence tient
        t->grounded = false;   // vx/vy portent déjà la vitesse tangentielle
    }
    t->vx += ix;
    t->vy += iy;
}

Vector2 TankPivot(const Tank *t)
{
    return (Vector2){ t->x + sinf(t->bodyAngle) * 7.0f, t->y - cosf(t->bodyAngle) * 7.0f };
}

Vector2 TankMuzzle(const Tank *t)
{
    Vector2 p = TankPivot(t);
    return (Vector2){ p.x + cosf(t->aimAngle) * 20.0f, p.y + sinf(t->aimAngle) * 20.0f };
}

void TankUpdate(int idx, float dt)
{
    Tank *t = &G.tanks[idx];
    if (!t->alive) return;

    t->cooldown -= dt;
    t->hitFlash -= dt;

    // Rechargement des emplacements de roquettes
    if (t->shots < G.rules.rocketSlots) {
        t->reload += dt;
        if (t->reload >= G.rules.reloadTime) { t->reload = 0; t->shots++; }
    } else {
        t->reload = 0;
    }

    bool inWater = (t->y > G.waterLevel);
    // Plafond de sécurité : suit les règles (vitesse et recul très élevés permis)
    float vmax = fmaxf(1000.0f, G.rules.tankSpeed * 3.0f + G.rules.recoil * 4.0f);

    if (t->grounded) {
        // ---- Accroché : roule le long du contour (murs et plafonds compris),
        // la gravité n'a pas de prise tant que l'adhérence tient
        float target = t->aiMoveDir * G.rules.tankSpeed * (inWater ? 0.55f : 1.0f);
        if (fabsf(t->aiMoveDir) < 0.01f)
            t->surfSpeed = Lerpf(t->surfSpeed, 0, Clampf(12.0f * dt, 0, 1));
        else
            t->surfSpeed += Clampf(target - t->surfSpeed, -1000.0f * dt, 1000.0f * dt);
        t->surfSpeed = Clampf(t->surfSpeed, -vmax, vmax);

        float dir = (t->surfSpeed >= 0) ? 1.0f : -1.0f;
        float remain = fabsf(t->surfSpeed) * dt;
        while (remain > 0.001f) {
            float step = (remain > 1.0f) ? 1.0f : remain;
            if (!CrawlStep(t, dir, step)) { t->surfSpeed = 0; break; }
            remain -= step;
        }

        // Vitesse monde équivalente (héritée si on décroche)
        t->vx = -t->ny * t->surfSpeed;
        t->vy =  t->nx * t->surfSpeed;

        // Le terrain d'accroche a pu être soufflé par une explosion
        if (!DiscSolid(t->x, t->y, CONTACT_R)) t->grounded = false;
    } else {
        // ---- Vol balistique
        float target = t->aiMoveDir * G.rules.tankSpeed * (inWater ? 0.55f : 1.0f);
        if (fabsf(t->aiMoveDir) >= 0.01f)
            t->vx += Clampf(target - t->vx, -260.0f * dt, 260.0f * dt);
        t->vy += GRAVITY * (inWater ? 0.35f : 1.0f) * dt;
        if (inWater) {
            t->vy = Lerpf(t->vy, 0, Clampf(2.5f * dt, 0, 1));
            if (t->vy > 90) t->vy = 90;
        }
        t->vx = Clampf(t->vx, -vmax, vmax);
        t->vy = Clampf(t->vy, -vmax, fmaxf(900.0f, vmax * 0.6f));

        // Déplacement pixel par pixel : s'accroche à la première surface touchée
        float dist = sqrtf(t->vx * t->vx + t->vy * t->vy) * dt;
        int steps = (int)dist + 1;
        float sdt = dt / steps;
        for (int s = 0; s < steps; s++) {
            float nx2 = t->x + t->vx * sdt, ny2 = t->y + t->vy * sdt;
            if (DiscSolid(nx2, ny2, BODY_R)) { Attach(t); break; }
            t->x = nx2;
            t->y = ny2;
        }
    }

    // Limites du monde
    if (t->x < TANK_HALF_W) { t->x = TANK_HALF_W; if (t->vx < 0) t->vx = 0; t->surfSpeed = 0; }
    if (t->x > G.worldW - TANK_HALF_W) { t->x = G.worldW - TANK_HALF_W; if (t->vx > 0) t->vx = 0; t->surfSpeed = 0; }
    if (t->y < -300) { t->y = -300; if (t->vy < 0) t->vy = 0; }

    // Orientation du châssis : suit la surface d'accroche, se redresse en vol
    if (t->grounded) {
        float a = atan2f(t->nx, -t->ny);
        t->bodyAngle = LerpAngle(t->bodyAngle, a, Clampf(14.0f * dt, 0, 1));
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
    if (t->y > G.worldH + 80) { TankKill(idx, -1, true); return; }
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
    float upx = sinf(t->bodyAngle), upy = -cosf(t->bodyAngle);
    Color body = (t->hitFlash > 0) ? WHITE : t->color;
    Color darkBody = ColorBrightness(body, -0.25f);

    // Chenilles (côté surface) et châssis : décalés le long de l'axe du tank
    DrawRectanglePro((Rectangle){ t->x - upx * 3, t->y - upy * 3, 30, 9 }, (Vector2){ 15, 4.5f }, deg,
                     (Color){ 55, 55, 60, 255 });
    DrawRectanglePro((Rectangle){ t->x + upx * 2, t->y + upy * 2, 26, 10 }, (Vector2){ 13, 5 }, deg, body);
    // Tourelle + canon
    Vector2 pivot = TankPivot(t);
    Vector2 muzzle = { pivot.x + cosf(t->aimAngle) * 18.0f, pivot.y + sinf(t->aimAngle) * 18.0f };
    DrawLineEx(pivot, muzzle, 4.0f, ColorBrightness(body, -0.4f));
    DrawCircleV(pivot, 5.5f, darkBody);

    // Barre de vie + nom (toujours à l'horizontale au-dessus du tank)
    float w = 30;
    DrawRectangle((int)(t->x - w / 2), (int)(t->y - 27), (int)w, 4, Fade(BLACK, 0.5f));
    float hpf = t->hp / G.rules.tankHp;
    Color hpCol = (hpf > 0.5f) ? LIME : (hpf > 0.25f) ? ORANGE : RED;
    DrawRectangle((int)(t->x - w / 2), (int)(t->y - 27), (int)(w * hpf), 4, hpCol);

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
