// Terrain destructible : grille de pixels + texture mise à jour à chaque explosion
#include "game.h"
#include <stdlib.h>
#include <string.h>

static unsigned int Hash2(int x, int y)
{
    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Tampon de transfert vers le GPU, agrandi à la demande (taille de map variable)
static Color *sScratch = NULL;
static size_t sScratchCap = 0;

static Color *Scratch(size_t count)
{
    if (count > sScratchCap) {
        sScratch = (Color *)realloc(sScratch, count * sizeof(Color));
        sScratchCap = count;
    }
    return sScratch;
}

bool SolidAt(int x, int y)
{
    if (x < 0 || x >= G.worldW || y < 0 || y >= G.worldH) return false;
    return G.solid[y * G.worldW + x] != 0;
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
    for (int y = y0; y < G.worldH; y++)
        if (SolidAt(xi, y)) return (float)y;
    return (float)G.worldH;
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

static void RecolorColumn(int x)
{
    int depth = -1;
    for (int y = 0; y < G.worldH; y++) {
        if (G.solid[y * G.worldW + x]) {
            depth = (depth < 0) ? 0 : depth + 1;
            G.pix[y * G.worldW + x] = DepthColor(x, y, depth);
        } else {
            depth = -1;
            G.pix[y * G.worldW + x] = BLANK;
        }
    }
}

static void RebuildPixels(void)
{
    for (int x = 0; x < G.worldW; x++) RecolorColumn(x);
}

static void UploadTexture(void)
{
    if (!G.texReady) {
        Image img = { G.pix, G.worldW, G.worldH, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        G.terrainTex = LoadTextureFromImage(img);   // copie vers le GPU, G.pix reste à nous
        G.texReady = true;
    } else {
        UpdateTexture(G.terrainTex, G.pix);
    }
}

void TerrainRefresh(void)
{
    RebuildPixels();
    UploadTexture();
}

// (Ré)alloue les grilles pour une map de w×h. preserve = conserver le dessin
// existant (ancré en bas à gauche, le sol restant près de l'eau).
void TerrainSetSize(int w, int h, bool preserve)
{
    if (w == G.worldW && h == G.worldH && G.solid) return;

    unsigned char *ns = (unsigned char *)calloc((size_t)w * h, 1);
    if (preserve && G.solid) {
        int cw = (w < G.worldW) ? w : G.worldW;
        int ch = (h < G.worldH) ? h : G.worldH;
        for (int y = 0; y < ch; y++)
            memcpy(&ns[(h - ch + y) * w], &G.solid[(G.worldH - ch + y) * G.worldW], cw);
    }
    free(G.solid);
    free(G.solidBackup);
    free(G.pix);
    G.solid = ns;
    G.solidBackup = (unsigned char *)calloc((size_t)w * h, 1);
    G.pix = (Color *)malloc((size_t)w * h * sizeof(Color));
    G.worldW = w;
    G.worldH = h;

    if (G.texReady) { UnloadTexture(G.terrainTex); G.texReady = false; }
    TerrainRefresh();
}

// Recolore et téléverse une plage de colonnes (après un coup de pinceau)
static void RecolorColumns(int x0, int x1)
{
    if (x0 < 0) x0 = 0;
    if (x1 > G.worldW - 1) x1 = G.worldW - 1;
    if (x0 > x1) return;
    for (int x = x0; x <= x1; x++) RecolorColumn(x);

    int w = x1 - x0 + 1;
    if (G.texReady) {
        Color *sc = Scratch((size_t)w * G.worldH);
        for (int y = 0; y < G.worldH; y++)
            memcpy(&sc[y * w], &G.pix[y * G.worldW + x0], w * sizeof(Color));
        UpdateTextureRec(G.terrainTex, (Rectangle){ (float)x0, 0, (float)w, (float)G.worldH }, sc);
    } else {
        UploadTexture();
    }
}

void TerrainPaint(float cx, float cy, float r, bool add)
{
    int x0 = (int)(cx - r) - 2, x1 = (int)(cx + r) + 2;
    int y0 = (int)(cy - r) - 2, y1 = (int)(cy + r) + 2;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > G.worldW - 1) x1 = G.worldW - 1;
    if (y1 > G.worldH - 1) y1 = G.worldH - 1;
    if (x0 > x1 || y0 > y1) return;

    float r2 = r * r;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r2) G.solid[y * G.worldW + x] = add ? 1 : 0;
        }
    RecolorColumns(x0, x1);
}

void TerrainClear(void)
{
    memset(G.solid, 0, (size_t)G.worldW * G.worldH);
    TerrainRefresh();
}

void TerrainGen(void)
{
    int W = G.worldW, H = G.worldH;
    memset(G.solid, 0, (size_t)W * H);

    // Île principale : somme de sinusoïdes + affaissement des bords sous l'eau.
    // Les hauteurs suivent la taille de la map ; le relief garde une échelle fixe.
    float scaleY = H / 960.0f;
    float base = H * 0.656f;
    float p0 = Rndf(0, 6.28f), p1 = Rndf(0, 6.28f), p2 = Rndf(0, 6.28f), p3 = Rndf(0, 6.28f);
    for (int x = 0; x < W; x++) {
        float h = base;
        h += sinf(x * 0.0035f + p0) * 92.0f * scaleY;
        h += sinf(x * 0.0080f + p1) * 46.0f * scaleY;
        h += sinf(x * 0.0210f + p2) * 17.0f;
        h += sinf(x * 0.0520f + p3) * 6.0f;
        float m = fminf(170.0f, W * 0.11f), e = 0.0f;
        if (x < m) e = (m - x) / m;
        else if (x > W - m) e = (x - (W - m)) / m;
        h += e * e * (H * 0.48f);
        int top = (int)h;
        if (top < 0) top = 0;
        for (int y = top; y < H; y++) G.solid[y * W + x] = 1;
    }

    // Îles flottantes : environ une paire par map de largeur standard
    int n = (int)Clampf(RndI(2, 3) * (W / 1600.0f), 1, 14);
    for (int i = 0; i < n; i++) {
        float cx = Rndf(W * 0.15f, W * 0.85f);
        float cy = Rndf(H * 0.31f, H * 0.45f);
        float rx = Rndf(85, 150);
        float ry = Rndf(26, 42);
        int x0 = (int)(cx - rx), x1 = (int)(cx + rx);
        for (int x = x0; x <= x1; x++) {
            if (x < 0 || x >= W) continue;
            float dx = (x - cx) / rx;
            float bump = sinf(x * 0.09f + p1) * 4.0f;
            for (int y = (int)(cy - ry + bump); y <= (int)(cy + ry); y++) {
                if (y < 0 || y >= H) continue;
                float dy = (y - cy) / ry;
                if (dx * dx + dy * dy <= 1.0f) G.solid[y * W + x] = 1;
            }
        }
    }

    TerrainRefresh();
}

void TerrainCarve(float cx, float cy, float r)
{
    int x0 = (int)(cx - r) - 4, x1 = (int)(cx + r) + 4;
    int y0 = (int)(cy - r) - 4, y1 = (int)(cy + r) + 4;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > G.worldW - 1) x1 = G.worldW - 1;
    if (y1 > G.worldH - 1) y1 = G.worldH - 1;
    if (x0 > x1 || y0 > y1) return;

    float r2 = r * r;
    float ring2 = (r + 3.5f) * (r + 3.5f);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float dx = x - cx, dy = y - cy;
            float d2 = dx * dx + dy * dy;
            int idx = y * G.worldW + x;
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
    if (!G.texReady) { UploadTexture(); return; }
    int w = x1 - x0 + 1, h = y1 - y0 + 1;
    Color *sc = Scratch((size_t)w * h);
    for (int y = 0; y < h; y++)
        memcpy(&sc[y * w], &G.pix[(y0 + y) * G.worldW + x0], w * sizeof(Color));
    UpdateTextureRec(G.terrainTex, (Rectangle){ (float)x0, (float)y0, (float)w, (float)h }, sc);
}

void TerrainDraw(void)
{
    DrawTexture(G.terrainTex, 0, 0, WHITE);
}
