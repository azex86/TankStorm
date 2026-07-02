// Roquettes : balistique, collisions, explosions (creusent le terrain, dégâts + souffle)
#include "game.h"

typedef struct { float radius, dmg, knock; } WeaponDef;

// Paramètres dérivés des règles de la map
static WeaponDef WeaponParams(WeaponType w)
{
    float r = G.rules.rocketRadius;
    float d = G.rules.rocketDmg;
    float k = d * 7.6f;   // le souffle suit les dégâts
    switch (w) {
    case W_TRIPLE: return (WeaponDef){ r * 0.73f, d * 0.60f, k * 0.73f };
    case W_BIG:    return (WeaponDef){ r * 1.77f, d * 1.80f, k * 1.46f };
    default:       return (WeaponDef){ r, d, k };
    }
}

static void SpawnRocket(int owner, float x, float y, float angle, WeaponType type)
{
    for (int i = 0; i < G.rocketCap; i++) {
        Rocket *r = &G.rockets[i];
        if (r->active) continue;
        r->active = true;
        r->x = x;
        r->y = y;
        r->vx = cosf(angle) * G.rules.rocketSpeed;
        r->vy = sinf(angle) * G.rules.rocketSpeed;
        r->owner = owner;
        r->type = type;
        r->life = 0;
        r->trailT = 0;
        return;
    }
}

void FireWeapon(int owner)
{
    if (G.state != ST_PLAY) return;
    Tank *t = &G.tanks[owner];
    if (!t->alive || t->cooldown > 0 || t->shots <= 0) return;

    WeaponType w = t->weapon;
    t->cooldown = SHOT_DELAY;
    t->shots--;

    Vector2 m = TankMuzzle(t);
    int n = (w == W_TRIPLE) ? 3 : 1;
    for (int i = 0; i < n; i++) {
        float a = t->aimAngle + ((n == 3) ? (i - 1) * 0.085f : 0.0f);
        SpawnRocket(owner, m.x, m.y, a, w);
    }
    FxMuzzle(m.x, m.y, t->aimAngle);
    SfxPlay(SFX_FIRE, 0.7f, 0.15f);

    // Recul du tir : propulse le tank dans le sens opposé au canon.
    // C'est le vrai moteur de déplacement aérien (tirez vers le bas pour monter,
    // vers le bas-arrière pour avancer en sautant, etc.). Réglable par map.
    float recoil = G.rules.recoil * ((w == W_BIG) ? 1.35f : 1.0f);
    t->vx -= cosf(t->aimAngle) * recoil;
    t->vy -= sinf(t->aimAngle) * recoil;
    t->grounded = false;

    if (w != W_ROCKET) {
        t->ammo--;
        if (t->ammo <= 0) t->weapon = W_ROCKET;
    }
}

void Explode(float x, float y, float radius, float dmg, float knock, int owner, bool carve)
{
    if (carve) TerrainCarve(x, y, radius);
    FxExplosion(x, y, radius);
    G.shake = fmaxf(G.shake, Clampf(radius * 0.35f, 4, 16));
    SfxPlay(SFX_BOOM, Clampf(radius / 46.0f, 0.5f, 1.0f), 0.18f);

    for (int i = 0; i < G.tankCount; i++) {
        Tank *t = &G.tanks[i];
        if (!t->alive) continue;
        float dx = t->x - x, dy = t->y - y;
        float d = sqrtf(dx * dx + dy * dy);
        float reff = radius + 16.0f;
        if (d >= reff) continue;
        float f = 1.0f - d / reff;
        if (d < 1.0f) { dx = 0; dy = -1; d = 1; }
        float ux = dx / d, uy = dy / d;

        if (i != owner)
            TankDamage(i, dmg * (0.30f + 0.70f * f), owner);

        // Souffle : projette les tanks touchés (le recul du tir gère l'auto-propulsion,
        // donc on atténue l'auto-souffle pour ne pas cumuler des sauts démesurés).
        float k = knock * (0.40f + 0.60f * f) * ((i == owner) ? 0.85f : 1.0f);
        TankImpulse(t, ux * k, uy * k - k * 0.25f);
    }
}

void RocketsUpdate(float dt)
{
    for (int i = 0; i < G.rocketCap; i++) {
        Rocket *r = &G.rockets[i];
        if (!r->active) continue;

        r->life += dt;
        r->vy += ROCKET_GRAV * dt;

        WeaponDef def = WeaponParams(r->type);
        const WeaponDef *w = &def;
        float dist = sqrtf(r->vx * r->vx + r->vy * r->vy) * dt;
        int steps = (int)(dist / 4.0f) + 1;
        float sdt = dt / steps;

        for (int s = 0; s < steps && r->active; s++) {
            r->x += r->vx * sdt;
            r->y += r->vy * sdt;

            // Traînée de fumée
            r->trailT += sdt;
            while (r->trailT > 0.018f) {
                r->trailT -= 0.018f;
                ParticleAdd(r->x, r->y, Rndf(-12, 12), Rndf(-12, 12),
                            Rndf(0.25f, 0.45f), Rndf(1.5f, 3.0f), 0,
                            (Color){ 200, 200, 200, 150 });
            }

            if (r->x < -60 || r->x > G.worldW + 60 || r->y > G.worldH + 60) {
                r->active = false;
                break;
            }
            // Contact avec l'eau : explose en surface, sans creuser
            if (r->y > G.waterLevel) {
                FxSplash(r->x, G.waterLevel);
                SfxPlay(SFX_SPLASH, 0.6f, 0.2f);
                Explode(r->x, r->y, w->radius * 0.8f, w->dmg * 0.6f, w->knock * 0.8f, r->owner, false);
                r->active = false;
                break;
            }
            // Terrain
            if (SolidAt((int)r->x, (int)r->y)) {
                Explode(r->x, r->y, w->radius, w->dmg, w->knock, r->owner, true);
                r->active = false;
                break;
            }
            // Tanks
            for (int k = 0; k < G.tankCount; k++) {
                Tank *t = &G.tanks[k];
                if (!t->alive) continue;
                if (k == r->owner && r->life < 0.12f) continue;
                float dx = t->x - r->x, dy = (t->y - 2) - r->y;
                if (dx * dx + dy * dy < 13 * 13) {
                    Explode(r->x, r->y, w->radius, w->dmg, w->knock, r->owner, true);
                    r->active = false;
                    break;
                }
            }
        }
    }
}

void RocketsDraw(void)
{
    for (int i = 0; i < G.rocketCap; i++) {
        Rocket *r = &G.rockets[i];
        if (!r->active) continue;
        float a = atan2f(r->vy, r->vx);
        float deg = a * RAD2DEG;
        // Lueur + corps + flamme
        DrawCircleV((Vector2){ r->x, r->y }, 6, Fade(ORANGE, 0.25f));
        DrawRectanglePro((Rectangle){ r->x, r->y, 11, 4.5f }, (Vector2){ 5.5f, 2.25f }, deg,
                         (Color){ 45, 45, 50, 255 });
        float fx = r->x - cosf(a) * 8.0f, fy = r->y - sinf(a) * 8.0f;
        DrawCircleV((Vector2){ fx, fy }, 2.6f, (Color){ 255, 180, 60, 255 });
    }
}
