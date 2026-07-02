// Éditeur de maps : pinceau terrain, navigation libre (clic droit + molette),
// panneau des règles en saisie numérique libre, sauvegarde/chargement
#include "game.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ---- Widgets d'interface immédiate ----
static const char *uiActive = NULL;   // slider en cours de glissement (id = pointeur du label)

bool UiButton(Rectangle r, const char *label)
{
    Vector2 m = GetMousePosition();
    bool over = CheckCollisionPointRec(m, r);
    DrawRectangleRec(r, over ? (Color){ 92, 108, 148, 235 } : (Color){ 52, 62, 92, 220 });
    DrawRectangleLinesEx(r, 2, over ? RAYWHITE : Fade(RAYWHITE, 0.35f));
    int fs = (int)(r.height * 0.5f);
    if (fs < 14) fs = 14;
    if (fs > 22) fs = 22;
    DrawText(label, (int)(r.x + (r.width - MeasureText(label, fs)) / 2),
             (int)(r.y + (r.height - fs) / 2), fs, RAYWHITE);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

float UiSlider(Rectangle r, const char *label, float v, float vmin, float vmax, float step, const char *fmt)
{
    DrawText(label, (int)r.x, (int)r.y - 15, 13, Fade(RAYWHITE, 0.85f));
    const char *val = TextFormat(fmt, v);
    DrawText(val, (int)(r.x + r.width - MeasureText(val, 13)), (int)r.y - 15, 13,
             (Color){ 255, 205, 60, 255 });

    DrawRectangleRec((Rectangle){ r.x, r.y + r.height / 2 - 2, r.width, 4 }, Fade(BLACK, 0.55f));

    Vector2 m = GetMousePosition();
    if (CheckCollisionPointRec(m, r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) uiActive = label;
    if (uiActive == label) {
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            uiActive = NULL;
        } else {
            float t = Clampf((m.x - r.x) / r.width, 0, 1);
            v = vmin + t * (vmax - vmin);
            if (step > 0) v = vmin + roundf((v - vmin) / step) * step;
            v = Clampf(v, vmin, vmax);
        }
    }
    float hx = r.x + (v - vmin) / (vmax - vmin) * r.width;
    DrawCircleV((Vector2){ hx, r.y + r.height / 2 }, 7,
                (uiActive == label) ? (Color){ 255, 205, 60, 255 } : RAYWHITE);
    return v;
}

void UiTextBox(Rectangle r, char *buf, int cap, bool *focus)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        *focus = CheckCollisionPointRec(GetMousePosition(), r);

    DrawRectangleRec(r, Fade(BLACK, 0.55f));
    DrawRectangleLinesEx(r, 2, *focus ? (Color){ 255, 205, 60, 255 } : Fade(RAYWHITE, 0.4f));

    if (*focus) {
        int c;
        while ((c = GetCharPressed()) > 0) {
            int len = (int)strlen(buf);
            bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || c == '-' || c == '_';
            if (ok && len < cap - 1) { buf[len] = (char)c; buf[len + 1] = 0; }
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(buf);
            if (len > 0) buf[len - 1] = 0;
        }
        if (IsKeyPressed(KEY_ENTER)) *focus = false;
    }

    const char *disp = (*focus && ((int)(GetTime() * 2.5f) & 1)) ? TextFormat("%s_", buf) : buf;
    DrawText(disp, (int)r.x + 8, (int)(r.y + (r.height - 16) / 2), 16, RAYWHITE);
}

// ---- Champ numérique : cliquer pour taper une valeur, Entrée ou clic ailleurs valide ----
static const char *sNumFocus = NULL;    // champ en cours d'édition (id = pointeur du label)
static char        sNumBuf[28];
static const char *sNumOrphan = NULL;   // champ dont l'édition a été volée ce frame
static char        sNumOrphanBuf[28];

static bool UiNumEditing(void) { return sNumFocus != NULL; }
static void UiNumCancel(void)  { sNumFocus = NULL; }

static void FmtNum(char *dst, int cap, float v, bool isInt)
{
    if (isInt) { snprintf(dst, cap, "%d", (int)v); return; }
    snprintf(dst, cap, "%.2f", v);
    char *p = dst + strlen(dst) - 1;
    while (p > dst && *p == '0') *p-- = 0;
    if (p > dst && *p == '.') *p = 0;
}

static float ParseNum(const char *s, float fallback)
{
    char *end;
    float v = strtof(s, &end);
    return (end == s) ? fallback : v;
}

static float UiNumBox(Rectangle row, const char *label, float v, bool isInt)
{
    // Récupérer la saisie si un autre champ a pris le focus pendant qu'on éditait
    if (sNumOrphan == label) { v = ParseNum(sNumOrphanBuf, v); sNumOrphan = NULL; }

    Rectangle box = { row.x + row.width - 92, row.y, 92, row.height };
    Vector2 m = GetMousePosition();
    bool over = CheckCollisionPointRec(m, box);
    bool focused = (sNumFocus == label);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (over && !focused) {
            if (sNumFocus) {   // l'ancien champ récupérera son texte via sNumOrphan
                snprintf(sNumOrphanBuf, sizeof(sNumOrphanBuf), "%s", sNumBuf);
                sNumOrphan = sNumFocus;
            }
            sNumFocus = label;
            FmtNum(sNumBuf, sizeof(sNumBuf), v, isInt);
            focused = true;
        } else if (!over && focused) {
            v = ParseNum(sNumBuf, v);
            sNumFocus = NULL;
            focused = false;
        }
    }

    if (focused) {
        int c;
        while ((c = GetCharPressed()) > 0) {
            if (c == ',') c = '.';
            int len = (int)strlen(sNumBuf);
            bool ok = (c >= '0' && c <= '9') || c == '.' || c == '-';
            if (ok && len < (int)sizeof(sNumBuf) - 1) { sNumBuf[len] = (char)c; sNumBuf[len + 1] = 0; }
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(sNumBuf);
            if (len > 0) sNumBuf[len - 1] = 0;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            v = ParseNum(sNumBuf, v);
            sNumFocus = NULL;
            focused = false;
        }
    }

    DrawText(label, (int)row.x, (int)(row.y + (row.height - 13) / 2), 13, Fade(RAYWHITE, 0.85f));
    DrawRectangleRec(box, Fade(BLACK, focused ? 0.75f : 0.55f));
    DrawRectangleLinesEx(box, focused ? 2.0f : 1.0f,
                         focused ? (Color){ 255, 205, 60, 255 } : Fade(RAYWHITE, over ? 0.7f : 0.35f));
    char disp[36];
    if (focused)
        snprintf(disp, sizeof(disp), "%s%s", sNumBuf, ((int)(GetTime() * 2.5f) & 1) ? "_" : "");
    else
        FmtNum(disp, sizeof(disp), v, isInt);
    DrawText(disp, (int)box.x + 7, (int)(box.y + (box.height - 14) / 2), 14,
             focused ? (Color){ 255, 205, 60, 255 } : RAYWHITE);
    return v;
}

// Bouton d'outil avec état enfoncé (outil sélectionné)
static bool ToolButton(Rectangle r, const char *label, bool active)
{
    Vector2 m = GetMousePosition();
    bool over = CheckCollisionPointRec(m, r);
    Color bg = active ? (Color){ 170, 130, 30, 235 }
                      : over ? (Color){ 92, 108, 148, 235 } : (Color){ 52, 62, 92, 220 };
    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, 2, active ? (Color){ 255, 205, 60, 255 }
                                      : over ? RAYWHITE : Fade(RAYWHITE, 0.35f));
    int fs = 15;
    DrawText(label, (int)(r.x + (r.width - MeasureText(label, fs)) / 2),
             (int)(r.y + (r.height - fs) / 2), fs, RAYWHITE);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ---- Éditeur ----
static float   sBrush = 28;
static bool    sDigTool = false;      // false = ajouter de la terre, true = creuser
static Vector2 sCamPos;               // point du monde visé par la caméra
static float   sZoom = 0;             // 0 = à recadrer
static char    sMapName[40] = "ma-map";
static bool    sNameFocus = false;
static bool    sPanel = true;
static char    sMsg[64] = "";
static float   sMsgT = 0;
static float   sLastX, sLastY;
static bool    sWasPainting = false;

static float PanelViewCx(void)
{
    return sPanel ? (SCREEN_W - 330) / 2.0f : SCREEN_W / 2.0f;
}

static void EditorFitView(void)
{
    float availW = sPanel ? (SCREEN_W - 340.0f) : (float)SCREEN_W;
    sZoom = fminf(availW / G.worldW, (float)SCREEN_H / G.worldH) * 0.95f;
    sCamPos = (Vector2){ G.worldW / 2.0f, G.worldH / 2.0f };
}

void EditorInit(bool keepTerrain, const char *name)
{
    ClampRules(&G.rules);
    TerrainSetSize(G.rules.worldW, G.rules.worldH, keepTerrain);
    if (!keepTerrain) TerrainGen();
    if (name) snprintf(sMapName, sizeof(sMapName), "%s", name);
    sNameFocus = false;
    sWasPainting = false;
    UiNumCancel();
    sMsg[0] = 0;
    sMsgT = 0;
    EditorFitView();
}

static void PaintStroke(float x, float y, bool add)
{
    if (sWasPainting) {
        float dx = x - sLastX, dy = y - sLastY;
        float d = sqrtf(dx * dx + dy * dy);
        int steps = (int)(d / (sBrush * 0.4f)) + 1;
        for (int i = 1; i <= steps; i++)
            TerrainPaint(sLastX + dx * i / steps, sLastY + dy * i / steps, sBrush, add);
    } else {
        TerrainPaint(x, y, sBrush, add);
    }
    sLastX = x;
    sLastY = y;
    sWasPainting = true;
}

int EditorFrame(float dt)
{
    (void)dt;
    int action = 0;
    if (sZoom <= 0) EditorFitView();

    Camera2D cam = { 0 };
    cam.offset = (Vector2){ PanelViewCx(), SCREEN_H / 2.0f };
    cam.zoom = sZoom;

    Rectangle panel = { SCREEN_W - 330, 0, 330, SCREEN_H };
    Vector2 mouse = GetMousePosition();
    bool overUI = sPanel && CheckCollisionPointRec(mouse, panel);
    bool editing = sNameFocus || UiNumEditing();

    if (IsKeyPressed(KEY_TAB) && !editing) sPanel = !sPanel;
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (editing) { UiNumCancel(); sNameFocus = false; }   // annule la saisie en cours
        else action = 2;
    }

    // ---- Navigation : clic droit maintenu = déplacer, molette = zoom, Ctrl+molette = pinceau
    float wheel = GetMouseWheelMove();
    if (!overUI && wheel != 0) {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
            sBrush = Clampf(sBrush + wheel * 4, 2, 500);
        } else {
            cam.target = sCamPos;
            Vector2 before = GetScreenToWorld2D(mouse, cam);
            sZoom = Clampf(sZoom * powf(1.15f, wheel), 0.04f, 12.0f);
            cam.zoom = sZoom;
            Vector2 after = GetScreenToWorld2D(mouse, cam);
            sCamPos.x += before.x - after.x;   // le point sous le curseur reste en place
            sCamPos.y += before.y - after.y;
        }
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !overUI) {
        Vector2 d = GetMouseDelta();
        sCamPos.x -= d.x / sZoom;
        sCamPos.y -= d.y / sZoom;
    }
    sCamPos.x = Clampf(sCamPos.x, -300, G.worldW + 300.0f);
    sCamPos.y = Clampf(sCamPos.y, -300, G.worldH + 300.0f);
    cam.target = sCamPos;
    Vector2 mw = GetScreenToWorld2D(mouse, cam);

    // ---- Pinceau : clic gauche = outil courant, Maj = outil inverse ----
    bool dig = sDigTool != (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    if (!overUI && !editing && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        PaintStroke(mw.x, mw.y, !dig);
    else
        sWasPainting = false;

    // ---- Rendu du monde ----
    DrawSky();
    BeginMode2D(cam);
    DrawRectangle(0, 0, G.worldW, G.worldH, Fade(BLACK, 0.15f));
    TerrainDraw();
    DrawRectangleLinesEx((Rectangle){ 0, 0, (float)G.worldW, (float)G.worldH }, 3.0f / sZoom,
                         Fade(RAYWHITE, 0.5f));
    DrawLineEx((Vector2){ 0, WaterStartY() }, (Vector2){ (float)G.worldW, WaterStartY() },
               3.0f / sZoom, (Color){ 60, 150, 230, 200 });
    if (!overUI)
        DrawCircleLines((int)mw.x, (int)mw.y, sBrush, Fade(dig ? RED : RAYWHITE, 0.8f));
    EndMode2D();

    DrawText(TextFormat("Zoom %.0f%% · Ligne bleue : niveau initial de l'eau", sZoom * 100),
             12, SCREEN_H - 64, 14, Fade(RAYWHITE, 0.7f));
    DrawText("Clic G : dessiner · Maj : outil inverse · Clic D maintenu : déplacer la vue", 12,
             SCREEN_H - 44, 14, Fade(RAYWHITE, 0.7f));
    DrawText("Molette : zoom · Ctrl+Molette : pinceau · Tab : panneau · Échap : menu", 12,
             SCREEN_H - 24, 14, Fade(RAYWHITE, 0.7f));

    // ---- Panneau ----
    if (sPanel) {
        DrawRectangleRec(panel, Fade((Color){ 18, 22, 34, 255 }, 0.92f));
        DrawLineEx((Vector2){ panel.x, 0 }, (Vector2){ panel.x, SCREEN_H }, 2, Fade(RAYWHITE, 0.3f));

        int x = (int)panel.x + 18;
        float w = panel.width - 36;
        float half = (w - 10) / 2;
        DrawText("ÉDITEUR DE MAPS", x, 12, 20, (Color){ 255, 205, 60, 255 });

        float y = 42;
        if (ToolButton((Rectangle){ (float)x, y, half, 28 }, "Terre", !sDigTool)) sDigTool = false;
        if (ToolButton((Rectangle){ x + half + 10, y, half, 28 }, "Creuser", sDigTool)) sDigTool = true;
        y += 50;
        sBrush = UiSlider((Rectangle){ (float)x, y, w, 14 }, "Taille du pinceau", sBrush, 2, 500, 1, "%.0f px");
        y += 24;

        DrawText("- Règles de la map (valeurs libres) -", x, (int)y, 13, Fade(RAYWHITE, 0.6f));
        y += 20;

        MapRules *R = &G.rules;
        const float rowH = 20, rowStep = 25;
        R->numBots      = (int)UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Nombre de bots", (float)R->numBots, true);          y += rowStep;
        R->tankHp       = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "PV des tanks", R->tankHp, false);                        y += rowStep;
        R->tankSpeed    = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Vitesse des tanks (px/s)", R->tankSpeed, false);         y += rowStep;
        R->rocketSlots  = (int)UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Emplacements de roquettes", (float)R->rocketSlots, true); y += rowStep;
        R->reloadTime   = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Rechargement (s)", R->reloadTime, false);                y += rowStep;
        R->rocketSpeed  = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Vitesse des roquettes (px/s)", R->rocketSpeed, false);   y += rowStep;
        R->rocketDmg    = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Dégâts des roquettes", R->rocketDmg, false);             y += rowStep;
        R->rocketRadius = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Rayon d'explosion (px)", R->rocketRadius, false);        y += rowStep;
        R->recoil       = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Recul du tir (px/s)", R->recoil, false);                 y += rowStep;
        R->waterGrace   = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Montée des eaux après (s)", R->waterGrace, false);       y += rowStep;
        R->waterSpeed   = UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Vitesse de l'eau (px/s)", R->waterSpeed, false);         y += rowStep;
        R->worldW       = (int)UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Largeur de la map (px)", (float)R->worldW, true);   y += rowStep;
        R->worldH       = (int)UiNumBox((Rectangle){ (float)x, y, w, rowH }, "Hauteur de la map (px)", (float)R->worldH, true);   y += rowStep;
        ClampRules(R);

        // Redimensionnement : conserve le dessin existant (ancré en bas à gauche)
        if (R->worldW != G.worldW || R->worldH != G.worldH) {
            TerrainSetSize(R->worldW, R->worldH, true);
            EditorFitView();
        }

        y += 4;
        if (UiButton((Rectangle){ (float)x, y, half, 30 }, "Île aléatoire")) TerrainGen();
        if (UiButton((Rectangle){ x + half + 10, y, half, 30 }, "Vider")) TerrainClear();
        y += 38;

        UiTextBox((Rectangle){ (float)x, y, w, 27 }, sMapName, sizeof(sMapName), &sNameFocus);
        y += 35;

        if (UiButton((Rectangle){ (float)x, y, half, 30 }, "Sauvegarder")) {
            if (sMapName[0] == 0) snprintf(sMapName, sizeof(sMapName), "ma-map");
            if (MapSave(sMapName)) snprintf(sMsg, sizeof(sMsg), "Sauvegardé : maps/%s.tsm", sMapName);
            else snprintf(sMsg, sizeof(sMsg), "Échec de la sauvegarde !");
            sMsgT = 3;
        }
        if (UiButton((Rectangle){ x + half + 10, y, half, 30 }, "Charger")) action = 3;
        y += 38;

        if (UiButton((Rectangle){ (float)x, y, w, 32 }, "Tester la map")) action = 1;
        y += 40;
        if (UiButton((Rectangle){ (float)x, y, w, 28 }, "Retour au menu")) action = 2;
        y += 34;

        if (sMsgT > 0) {
            sMsgT -= GetFrameTime();
            DrawText(sMsg, x, (int)y, 13, (Color){ 140, 255, 140, 255 });
        }
    }

    return action;
}
