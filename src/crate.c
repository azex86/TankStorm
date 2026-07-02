// Caisses de ravitaillement : tombent en parachute, soins ou armes spéciales
#include "game.h"

static void CrateApply(Crate *c, int tankIdx)
{
    Tank *t = &G.tanks[tankIdx];
    switch (c->type) {
    case CR_HEAL:
        t->hp = fminf(G.rules.tankHp, t->hp + 40);
        for (int i = 0; i < 10; i++)
            ParticleAdd(t->x + Rndf(-10, 10), t->y + Rndf(-10, 4), Rndf(-15, 15), Rndf(-70, -30),
                        Rndf(0.5f, 0.9f), Rndf(2, 3.5f), -0.1f, (Color){ 120, 255, 120, 220 });
        break;
    case CR_TRIPLE: t->weapon = W_TRIPLE; t->ammo = 4; break;
    case CR_BIG:    t->weapon = W_BIG;    t->ammo = 3; break;
    }
    SfxPlay(SFX_PICK, 0.8f, 0.1f);
    c->active = false;
}

void CratesUpdate(float dt)
{
    G.crateTimer -= dt;
    if (G.crateTimer <= 0) {
        G.crateTimer = Rndf(7, 12);
        for (int i = 0; i < MAX_CRATES; i++) {
            Crate *c = &G.crates[i];
            if (c->active) continue;
            c->active = true;
            c->landed = false;
            c->x = Rndf(fminf(140, G.worldW * 0.1f), G.worldW - fminf(140, G.worldW * 0.1f));
            c->y = -30;
            c->vy = 45;
            unsigned int r = RndU() % 100;
            c->type = (r < 35) ? CR_HEAL : (r < 70) ? CR_TRIPLE : CR_BIG;
            break;
        }
    }

    for (int i = 0; i < MAX_CRATES; i++) {
        Crate *c = &G.crates[i];
        if (!c->active) continue;

        if (!c->landed) {
            c->vy = fminf(c->vy + 30 * dt, 55);   // parachute : chute plafonnée
            c->y += c->vy * dt;
            if (SolidAt((int)c->x, (int)(c->y + 8))) c->landed = true;
        } else if (!SolidAt((int)c->x, (int)(c->y + 8))) {
            c->landed = false;                    // le sol a été creusé sous la caisse
            c->vy = 20;
        }

        if (c->y > G.waterLevel - 4) {
            FxSplash(c->x, G.waterLevel);
            c->active = false;
            continue;
        }

        for (int k = 0; k < G.tankCount; k++) {
            Tank *t = &G.tanks[k];
            if (!t->alive) continue;
            if (fabsf(t->x - c->x) < 18 && fabsf(t->y - c->y) < 18) {
                CrateApply(c, k);
                break;
            }
        }
    }
}

void CratesDraw(void)
{
    for (int i = 0; i < MAX_CRATES; i++) {
        Crate *c = &G.crates[i];
        if (!c->active) continue;

        if (!c->landed) {
            // Parachute
            DrawCircleSector((Vector2){ c->x, c->y - 16 }, 14, 180, 360, 12, Fade(RAYWHITE, 0.9f));
            DrawLineEx((Vector2){ c->x - 12, c->y - 14 }, (Vector2){ c->x - 6, c->y - 5 }, 1.5f, LIGHTGRAY);
            DrawLineEx((Vector2){ c->x + 12, c->y - 14 }, (Vector2){ c->x + 6, c->y - 5 }, 1.5f, LIGHTGRAY);
        }

        // Caisse
        DrawRectangle((int)c->x - 8, (int)c->y - 6, 16, 13, (Color){ 150, 105, 60, 255 });
        DrawRectangleLinesEx((Rectangle){ c->x - 8, c->y - 6, 16, 13 }, 2, (Color){ 100, 68, 36, 255 });

        // Icône du contenu
        switch (c->type) {
        case CR_HEAL:
            DrawRectangle((int)c->x - 1, (int)c->y - 3, 3, 8, RAYWHITE);
            DrawRectangle((int)c->x - 4, (int)c->y, 9, 3, RAYWHITE);
            break;
        case CR_TRIPLE:
            for (int k = -1; k <= 1; k++)
                DrawCircle((int)c->x + k * 4, (int)c->y + 1, 1.6f, (Color){ 255, 230, 90, 255 });
            break;
        case CR_BIG:
            DrawCircle((int)c->x, (int)c->y + 1, 3.5f, (Color){ 255, 120, 60, 255 });
            break;
        }
    }
}
