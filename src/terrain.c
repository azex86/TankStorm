// Terrain destructible : grille de pixels + texture mise à jour à chaque explosion
#include "game.h"
#include <string.h>

static unsigned int Hash2(int x, int y)
{
    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

bool SolidAt(int x, int y)
{
    if (x < 0 || x >= WORLD_W || y < 0 || y >= WORLD_H) return false;
    return G.solid[y * WORLD_W + x] != 0;
}

bool BoxSolid(float fx0, float fy0, float fx1, float fy1)
{
    int x0 = (int)floorf(fx0), x1 = (int)floorf(fx1);
    int y0 = (int)floorf(fy0), y1 = (int)floorf(fy1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (SolidAt(x, y)) return true;
    return false;
}

float SurfaceYAt(float x, float fromY)
{
    int xi = (int)x;
    int y0 = (int)fromY;
    if (y0 < 0) y0 = 0;
    for (int y = y0; y < WORLD_H; y++)
        if (SolidAt(xi, y)) return (float)y;
    return (float)WORLD_H;
}

static Color DepthColor(int x, int y, int depth)
{
    unsigned int h = Hash2(x, y);
    int n = (int)(h % 29) - 14;   // bruit -14..14
    Color c;
    if (depth == 0)      c = (Color){ 120, 200, 82, 255 };   // liseré d'herbe claire
    else if (depth <= 3) c = (Color){ 62, 152, 60, 255 };    // herbe
    else if (depth <= 6) c = (Color){ 104, 120, 52, 255 };   // transition
    else {
        c = (Color){ 126, 88, 58, 255 };                     // terre
        if (h % 37 == 0) c = (Color){ 110, 108, 112, 255 };  // cailloux
        float dark = 1.0f - Clampf(depth * 0.0011f, 0.0f, 0.38f);
        c.r = (unsigned char)(c.r * dark);
        c.g = (unsigned char)(c.g * dark);
        c.b = (unsigned char)(c.b * dark);
    }
    int r = c.r + n, g = c.g + n, b = c.b + n;
    c.r = (unsigned char)Clampf((float)r, 0, 255);
    c.g = (unsigned char)Clampf((float)g, 0, 255);
    c.b = (unsigned char)Clampf((float)b, 0, 255);
    return c;
}

static void RebuildPixels(void)
{
    for (int x = 0; x < WORLD_W; x++) {
        int depth = -1;
        for (int y = 0; y < WORLD_H; y++) {
            if (G.solid[y * WORLD_W + x]) {
                depth = (depth < 0) ? 0 : depth + 1;
                G.pix[y * WORLD_W + x] = DepthColor(x, y, depth);
            } else {
                depth = -1;
                G.pix[y * WORLD_W + x] = BLANK;
            }
        }
    }
}

void TerrainGen(void)
{
    memset(G.solid, 0, WORLD_W * WORLD_H);

    // Île principale : somme de sinusoïdes + affaissement des bords sous l'eau
    float p0 = Rndf(0, 6.28f), p1 = Rndf(0, 6.28f), p2 = Rndf(0, 6.28f), p3 = Rndf(0, 6.28f);
    for (int x = 0; x < WORLD_W; x++) {
        float h = 630.0f;
        h += sinf(x * 0.0035f + p0) * 92.0f;
        h += sinf(x * 0.0080f + p1) * 46.0f;
        h += sinf(x * 0.0210f + p2) * 17.0f;
        h += sinf(x * 0.0520f + p3) * 6.0f;
        float m = 170.0f, e = 0.0f;
        if (x < m) e = (m - x) / m;
        else if (x > WORLD_W - m) e = (x - (WORLD_W - m)) / m;
        h += e * e * 460.0f;
        int top = (int)h;
        if (top < 0) top = 0;
        for (int y = top; y < WORLD_H; y++) G.solid[y * WORLD_W + x] = 1;
    }

    // Îles flottantes
    int n = RndI(2, 3);
    for (int i = 0; i < n; i++) {
        float cx = Rndf(240, WORLD_W - 240);
        float cy = Rndf(300, 430);
        float rx = Rndf(85, 150);
        float ry = Rndf(26, 42);
        int x0 = (int)(cx - rx), x1 = (int)(cx + rx);
        for (int x = x0; x <= x1; x++) {
            if (x < 0 || x >= WORLD_W) continue;
            float dx = (x - cx) / rx;
            float bump = sinf(x * 0.09f + p1) * 4.0f;
            for (int y = (int)(cy - ry + bump); y <= (int)(cy + ry); y++) {
                if (y < 0 || y >= WORLD_H) continue;
                float dy = (y - cy) / ry;
                if (dx * dx + dy * dy <= 1.0f) G.solid[y * WORLD_W + x] = 1;
            }
        }
    }

    RebuildPixels();

    if (!G.texReady) {
        Image img = { G.pix, WORLD_W, WORLD_H, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        G.terrainTex = LoadTextureFromImage(img);   // copie vers le GPU, G.pix reste à nous
        G.texReady = true;
    } else {
        UpdateTexture(G.terrainTex, G.pix);
    }
}

void TerrainCarve(float cx, float cy, float r)
{
    int x0 = (int)(cx - r) - 4, x1 = (int)(cx + r) + 4;
    int y0 = (int)(cy - r) - 4, y1 = (int)(cy + r) + 4;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > WORLD_W - 1) x1 = WORLD_W - 1;
    if (y1 > WORLD_H - 1) y1 = WORLD_H - 1;
    if (x0 > x1 || y0 > y1) return;

    float r2 = r * r;
    float ring2 = (r + 3.5f) * (r + 3.5f);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float dx = x - cx, dy = y - cy;
            float d2 = dx * dx + dy * dy;
            int idx = y * WORLD_W + x;
            if (d2 <= r2) {
                G.solid[idx] = 0;
                G.pix[idx] = BLANK;
            } else if (d2 <= ring2 && G.solid[idx]) {
                Color c = G.pix[idx];   // bord roussi
                c.r = (unsigned char)(c.r * 0.55f);
                c.g = (unsigned char)(c.g * 0.55f);
                c.b = (unsigned char)(c.b * 0.55f);
                G.pix[idx] = c;
            }
        }
    }

    // Mise à jour de la zone modifiée uniquement
    static Color scratch[220 * 220];
    int w = x1 - x0 + 1, h = y1 - y0 + 1;
    if (w <= 220 && h <= 220) {
        for (int y = 0; y < h; y++)
            memcpy(&scratch[y * w], &G.pix[(y0 + y) * WORLD_W + x0], w * sizeof(Color));
        UpdateTextureRec(G.terrainTex, (Rectangle){ (float)x0, (float)y0, (float)w, (float)h }, scratch);
    } else {
        UpdateTexture(G.terrainTex, G.pix);
    }
}

void TerrainDraw(void)
{
    DrawTexture(G.terrainTex, 0, 0, WHITE);
}
