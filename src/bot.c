// IA des bots : choix de cible, tir balistique bruité, fuite de l'eau
#include "game.h"

// Angle de tir pour atteindre `to` avec une vitesse fixe (trajectoire tendue).
// Convention y vers le bas ; renvoie false si la cible est hors de portée.
bool SolveBallistic(Vector2 from, Vector2 to, float speed, float grav, float *outAngle)
{
    float dx = to.x - from.x;
    float dyUp = -(to.y - from.y);        // repère mathématique (y vers le haut)
    float adx = fabsf(dx);

    if (adx < 8.0f) { *outAngle = (dyUp > 0) ? -PI / 2 : PI / 2; return true; }

    float v2 = speed * speed;
    float disc = v2 * v2 - grav * (grav * adx * adx + 2.0f * dyUp * v2);
    if (disc < 0) return false;

    float tanTheta = (v2 - sqrtf(disc)) / (grav * adx);
    float theta = atanf(tanTheta);
    float vxW = speed * cosf(theta) * ((dx >= 0) ? 1.0f : -1.0f);
    float vyW = -speed * sinf(theta);
    *outAngle = atan2f(vyW, vxW);
    return true;
}

static int PickTarget(int self)
{
    float best = 1e12f;
    int idx = -1;
    for (int i = 0; i < MAX_TANKS; i++) {
        if (i == self || !G.tanks[i].alive) continue;
        float dx = G.tanks[i].x - G.tanks[self].x;
        float dy = G.tanks[i].y - G.tanks[self].y;
        float d = dx * dx + dy * dy;
        if (d < best) { best = d; idx = i; }
    }
    return idx;
}

// Vrai si le canon tire droit dans un mur tout proche
static bool MuzzleBlocked(const Tank *t)
{
    Vector2 m = TankMuzzle(t);
    for (float d = 4; d <= 42; d += 6) {
        int x = (int)(m.x + cosf(t->aimAngle) * d);
        int y = (int)(m.y + sinf(t->aimAngle) * d);
        if (SolidAt(x, y)) return true;
    }
    return false;
}

void BotUpdate(int idx, float dt)
{
    Tank *t = &G.tanks[idx];
    if (!t->alive) return;

    t->aiMoveTimer -= dt;
    t->aiFireTimer -= dt;

    if (t->target < 0 || !G.tanks[t->target].alive)
        t->target = PickTarget(idx);

    // ---- Déplacement ----
    if (t->aiMoveTimer <= 0) {
        t->aiMoveTimer = Rndf(0.5f, 1.4f);
        float danger = G.waterLevel - 130.0f;

        if (t->y > danger) {
            // L'eau approche : chercher le point le plus haut aux alentours
            float bestDir = 0, bestY = 1e9f;
            static const float CAND[] = { -220, -130, -60, 60, 130, 220 };
            for (int c = 0; c < 6; c++) {
                float cx = Clampf(t->x + CAND[c], 20, WORLD_W - 20);
                float sy = SurfaceYAt(cx, fmaxf(0, t->y - 220));
                if (sy < bestY) { bestY = sy; bestDir = (CAND[c] > 0) ? 1.0f : -1.0f; }
            }
            t->aiMoveDir = bestDir;
        } else if (t->target >= 0) {
            float d = G.tanks[t->target].x - t->x;
            float ad = fabsf(d);
            if (ad > 480)      t->aiMoveDir = (d > 0) ? 1.0f : -1.0f;
            else if (ad < 170) t->aiMoveDir = (d > 0) ? -1.0f : 1.0f;
            else               t->aiMoveDir = (float)(RndI(-1, 1));
        }
    }

    // Ne jamais s'aventurer sur les pentes abruptes des bords de la carte
    if (t->x < 260) t->aiMoveDir = 1;
    else if (t->x > WORLD_W - 260) t->aiMoveDir = -1;

    // Anti-blocage : bloqué contre une paroi trop raide -> demi-tour
    if (t->grounded && fabsf(t->aiMoveDir) > 0.5f && fabsf(t->vx) < 8.0f) {
        t->aiStuck += dt;
        if (t->aiStuck > 0.7f) {
            t->aiMoveDir = -t->aiMoveDir;
            t->aiMoveTimer = Rndf(0.6f, 1.2f);
            t->aiStuck = 0;
        }
    } else {
        t->aiStuck = 0;
    }

    // ---- Visée et tir ----
    if (t->target >= 0) {
        Tank *e = &G.tanks[t->target];
        float desired;
        if (!SolveBallistic(TankMuzzle(t), (Vector2){ e->x, e->y - 4 },
                            ROCKET_SPEED, ROCKET_GRAV, &desired))
            desired = (e->x > t->x) ? -PI / 4 : -3 * PI / 4;   // lob à 45° vers la cible

        desired += t->aiAimErr * sinf(G.time * 1.3f + idx * 2.1f);
        t->aimAngle = LerpAngle(t->aimAngle, desired, Clampf(4.0f * dt, 0, 1));

        float dx = e->x - t->x, dy = e->y - t->y;
        if (t->aiFireTimer <= 0 && dx * dx + dy * dy < 650.0f * 650.0f) {
            t->aiFireTimer = Rndf(1.8f, 3.4f);
            if (!MuzzleBlocked(t)) FireWeapon(idx);
        }
    }
}
