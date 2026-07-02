// Maps : fichier maps/<nom>.tsm = magie "TSM2" + règles + terrain compressé (RLE).
// Les fichiers "TSM1" (ancien format sans recul ni taille de map) restent lisibles.
#include "game.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

static const char MAGIC2[4] = { 'T', 'S', 'M', '2' };
static const char MAGIC1[4] = { 'T', 'S', 'M', '1' };
#define TSM1_RULES_SIZE 40   // les 10 premiers champs de MapRules (4 octets chacun)

MapRules DefaultRules(void)
{
    MapRules r = { 0 };
    r.numBots      = 7;
    r.tankHp       = 100;
    r.tankSpeed    = 115;
    r.rocketSlots  = 3;
    r.reloadTime   = 2.0f;
    r.rocketSpeed  = 540;
    r.rocketDmg    = 34;
    r.rocketRadius = 26;
    r.waterGrace   = 18;
    r.waterSpeed   = 6;
    r.recoil       = 155;
    r.worldW       = DEF_WORLD_W;
    r.worldH       = DEF_WORLD_H;
    return r;
}

// Garde-fous techniques uniquement (mémoire, divisions par zéro, boucles infinies).
// Aucune limite de gameplay : les valeurs extrêmes sont permises.
void ClampRules(MapRules *r)
{
    r->numBots      = (int)Clampf((float)r->numBots, 0, 999);
    r->tankHp       = Clampf(r->tankHp, 1, 1e7f);
    r->tankSpeed    = Clampf(r->tankSpeed, 0, 20000);
    r->rocketSlots  = (int)Clampf((float)r->rocketSlots, 1, 999);
    r->reloadTime   = Clampf(r->reloadTime, 0.02f, 1e6f);
    r->rocketSpeed  = Clampf(r->rocketSpeed, 20, 20000);
    r->rocketDmg    = Clampf(r->rocketDmg, 0, 1e7f);
    r->rocketRadius = Clampf(r->rocketRadius, 1, 2000);
    r->waterGrace   = Clampf(r->waterGrace, 0, 1e7f);
    r->waterSpeed   = Clampf(r->waterSpeed, 0, 1e6f);
    r->recoil       = Clampf(r->recoil, 0, 1e6f);
    r->worldW       = (int)Clampf((float)r->worldW, 400, 8192);
    r->worldH       = (int)Clampf((float)r->worldH, 300, 8192);
}

bool MapSave(const char *name)
{
    MKDIR("maps");
    char path[300];
    snprintf(path, sizeof(path), "maps/%s.tsm", name);
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    G.rules.worldW = G.worldW;   // la map porte toujours sa taille réelle
    G.rules.worldH = G.worldH;
    fwrite(MAGIC2, 1, 4, f);
    fwrite(&G.rules, sizeof(MapRules), 1, f);   // champs de 4 octets, pas de padding

    long long total = (long long)G.worldW * G.worldH, i = 0;
    while (i < total) {
        unsigned char v = G.solid[i];
        int run = 1;
        while (i + run < total && G.solid[i + run] == v && run < 65535) run++;
        unsigned short r16 = (unsigned short)run;
        fwrite(&r16, 2, 1, f);
        fwrite(&v, 1, 1, f);
        i += run;
    }
    fclose(f);
    return true;
}

bool MapLoad(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    char m[4];
    if (fread(m, 1, 4, f) != 4) { fclose(f); return false; }

    MapRules r = DefaultRules();
    if (memcmp(m, MAGIC2, 4) == 0) {
        if (fread(&r, sizeof(MapRules), 1, f) != 1) { fclose(f); return false; }
    } else if (memcmp(m, MAGIC1, 4) == 0) {
        // Ancien format : mêmes 10 premiers champs, recul et taille par défaut
        if (fread(&r, TSM1_RULES_SIZE, 1, f) != 1) { fclose(f); return false; }
        r.recoil = 155;
        r.worldW = DEF_WORLD_W;
        r.worldH = DEF_WORLD_H;
    } else {
        fclose(f);
        return false;
    }
    ClampRules(&r);

    G.rules = r;
    TerrainSetSize(r.worldW, r.worldH, false);

    long long total = (long long)G.worldW * G.worldH, i = 0;
    while (i < total) {
        unsigned short run;
        unsigned char v;
        if (fread(&run, 2, 1, f) != 1 || fread(&v, 1, 1, f) != 1) break;
        if (run == 0) break;
        if (i + run > total) run = (unsigned short)(total - i);
        memset(&G.solid[i], v ? 1 : 0, run);
        i += run;
    }
    fclose(f);
    if (i < total) memset(&G.solid[i], 0, (size_t)(total - i));

    TerrainRefresh();
    return true;
}

int MapListNames(char names[][48], int max)
{
    if (!DirectoryExists("maps")) return 0;
    FilePathList fl = LoadDirectoryFilesEx("maps", ".tsm", false);
    int n = 0;
    for (unsigned int i = 0; i < fl.count && n < max; i++)
        snprintf(names[n++], 48, "%s", GetFileNameWithoutExt(fl.paths[i]));
    UnloadDirectoryFiles(fl);
    return n;
}
