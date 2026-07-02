// Effets : particules, fil d'actualité des éliminations, sons synthétisés au démarrage
#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define SFX_RATE 22050

// ---- Sons : pools d'alias pour superposer plusieurs lectures ----
typedef struct { Sound base; Sound alias[4]; int next; bool ok; } SndPool;
static SndPool pools[SFX_COUNT];

static float SynthNoise(unsigned int *st)
{
    *st = *st * 1664525u + 1013904223u;
    return ((*st >> 9) & 0xFFFF) / 32768.0f - 1.0f;
}

static Sound FromSamples(short *data, int n)
{
    Wave w = { 0 };
    w.frameCount = (unsigned int)n;
    w.sampleRate = SFX_RATE;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = data;
    Sound s = LoadSoundFromWave(w);   // copie les échantillons
    free(data);
    return s;
}

static short Pack(float v)
{
    v = Clampf(v, -1.0f, 1.0f);
    return (short)(v * 32000.0f);
}

static Sound MakeFire(void)
{
    int n = (int)(0.16f * SFX_RATE);
    short *d = malloc(n * sizeof(short));
    unsigned int st = 777;
    float phase = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float f = 380.0f - 240.0f * t;
        phase += 2 * PI * f / SFX_RATE;
        float saw = 2.0f * (phase / (2 * PI) - floorf(phase / (2 * PI))) - 1.0f;
        float env = powf(1.0f - t, 1.8f);
        d[i] = Pack((saw * 0.55f + SynthNoise(&st) * 0.30f) * env * 0.8f);
    }
    return FromSamples(d, n);
}

static Sound MakeBoom(float dur, float thumpHz)
{
    int n = (int)(dur * SFX_RATE);
    short *d = malloc(n * sizeof(short));
    unsigned int st = 4242;
    float brown = 0, lp = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float ts = (float)i / SFX_RATE;
        brown = Clampf(brown * 0.985f + SynthNoise(&st) * 0.28f, -1, 1);
        lp += (brown - lp) * 0.25f;
        float env = powf(1.0f - t, 2.1f);
        float thump = sinf(2 * PI * thumpHz * ts) * expf(-ts * 9.0f) * 0.85f;
        d[i] = Pack((lp * 1.2f + thump) * env);
    }
    return FromSamples(d, n);
}

static Sound MakePick(void)
{
    int n = (int)(0.16f * SFX_RATE);
    short *d = malloc(n * sizeof(short));
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float ts = (float)i / SFX_RATE;
        float f = (t < 0.5f) ? 660.0f : 990.0f;
        float env = powf(1.0f - t, 1.2f);
        d[i] = Pack(sinf(2 * PI * f * ts) * env * 0.45f);
    }
    return FromSamples(d, n);
}

static Sound MakeSplash(void)
{
    int n = (int)(0.30f * SFX_RATE);
    short *d = malloc(n * sizeof(short));
    unsigned int st = 991;
    float prev = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float w = SynthNoise(&st);
        float hp = w - prev;   // passe-haut simple : bruit "mouillé"
        prev = w;
        d[i] = Pack(hp * powf(1.0f - t, 2.0f) * 0.5f);
    }
    return FromSamples(d, n);
}

static void PoolInit(SndPool *p, Sound s)
{
    p->base = s;
    for (int i = 0; i < 4; i++) p->alias[i] = LoadSoundAlias(s);
    p->next = 0;
    p->ok = true;
}

void FxInitSounds(void)
{
    if (!G.audio) return;
    PoolInit(&pools[SFX_FIRE],   MakeFire());
    PoolInit(&pools[SFX_BOOM],   MakeBoom(0.55f, 55));
    PoolInit(&pools[SFX_PICK],   MakePick());
    PoolInit(&pools[SFX_SPLASH], MakeSplash());
    PoolInit(&pools[SFX_DIE],    MakeBoom(0.85f, 40));
}

void FxUnloadSounds(void)
{
    if (!G.audio) return;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (!pools[i].ok) continue;
        for (int k = 0; k < 4; k++) UnloadSoundAlias(pools[i].alias[k]);
        UnloadSound(pools[i].base);
        pools[i].ok = false;
    }
}

void SfxPlay(SfxId id, float vol, float pitchVar)
{
    if (!G.audio || !pools[id].ok) return;
    SndPool *p = &pools[id];
    Sound s = p->alias[p->next];
    p->next = (p->next + 1) & 3;
    SetSoundVolume(s, vol);
    SetSoundPitch(s, 1.0f + Rndf(-pitchVar, pitchVar));
    PlaySound(s);
}

// ---- Particules ----
void ParticleAdd(float x, float y, float vx, float vy, float life, float size, float grav, Color c)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &G.particles[i];
        if (p->active) continue;
        p->active = true;
        p->x = x; p->y = y; p->vx = vx; p->vy = vy;
        p->life = p->maxLife = life;
        p->size = size;
        p->grav = grav;
        p->color = c;
        return;
    }
}

void FxExplosion(float x, float y, float r)
{
    ParticleAdd(x, y, 0, 0, 0.09f, r * 0.9f, 0, (Color){ 255, 250, 220, 255 });   // flash
    int nFire = (int)(r * 0.7f);
    for (int i = 0; i < nFire; i++) {
        float a = Rndf(0, 2 * PI), sp = Rndf(40, 40 + r * 5);
        Color c = (RndU() % 2) ? (Color){ 255, 160, 40, 255 } : (Color){ 255, 220, 80, 255 };
        ParticleAdd(x, y, cosf(a) * sp, sinf(a) * sp - 30,
                    Rndf(0.30f, 0.65f), Rndf(2.5f, 5.5f), 0.35f, c);
    }
    for (int i = 0; i < 10; i++) {   // fumée
        ParticleAdd(x + Rndf(-r * 0.4f, r * 0.4f), y + Rndf(-r * 0.4f, r * 0.4f),
                    Rndf(-25, 25), Rndf(-60, -15),
                    Rndf(0.7f, 1.4f), Rndf(4, 9), -0.05f, (Color){ 110, 110, 115, 190 });
    }
    for (int i = 0; i < 12; i++) {   // débris de terre
        float a = Rndf(-PI, 0), sp = Rndf(100, 100 + r * 5);
        ParticleAdd(x, y, cosf(a) * sp, sinf(a) * sp,
                    Rndf(0.5f, 1.1f), Rndf(1.5f, 3.0f), 1.0f, (Color){ 115, 82, 55, 255 });
    }
}

void FxSplash(float x, float y)
{
    for (int i = 0; i < 16; i++) {
        float a = Rndf(-PI * 0.85f, -PI * 0.15f), sp = Rndf(60, 220);
        ParticleAdd(x, y, cosf(a) * sp, sinf(a) * sp,
                    Rndf(0.4f, 0.8f), Rndf(1.5f, 3.5f), 1.0f, (Color){ 160, 210, 255, 220 });
    }
}

void FxMuzzle(float x, float y, float angle)
{
    for (int i = 0; i < 6; i++) {
        float a = angle + Rndf(-0.25f, 0.25f);
        float sp = Rndf(80, 190);
        ParticleAdd(x, y, cosf(a) * sp, sinf(a) * sp,
                    Rndf(0.08f, 0.18f), Rndf(1.5f, 3.0f), 0, (Color){ 255, 210, 110, 255 });
    }
}

void ParticlesUpdate(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &G.particles[i];
        if (!p->active) continue;
        p->life -= dt;
        if (p->life <= 0) { p->active = false; continue; }
        p->vy += GRAVITY * p->grav * dt;
        p->x += p->vx * dt;
        p->y += p->vy * dt;
    }
}

void ParticlesDraw(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &G.particles[i];
        if (!p->active) continue;
        float f = p->life / p->maxLife;
        Color c = Fade(p->color, f * (p->color.a / 255.0f));
        DrawCircleV((Vector2){ p->x, p->y }, p->size * (0.5f + 0.5f * f), c);
    }
}

// ---- Fil des éliminations ----
void AddFeed(const char *fmt, ...)
{
    for (int i = MAX_FEED - 1; i > 0; i--) G.feed[i] = G.feed[i - 1];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(G.feed[0].text, sizeof(G.feed[0].text), fmt, ap);
    va_end(ap);
    G.feed[0].t = 5.0f;
}
