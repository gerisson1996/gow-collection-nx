// Launcher do gow-nx ("God of War Collection"): dois cards com a capa de cada jogo, painel com save/troféus/extras,
// perfil rápido (presets com L/R) e a tela de opções com tudo do menu R + Plus, em abas. Encadeia o NRO escolhido
// (mesma pasta) via hbloader (envSetNextLoad).
//   Fundo, capas dos cards e música são carregados automaticamente de game/PCSA00126/ (ou overrides no SD).
//   <pasta>/data/launcher/capa_gow1.(webp|png|jpg), capa_gow2.*  capas personalizadas dos cards (opcional)
#include <switch.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <webp/decode.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"
#include "game_assets.h"
#include "trophies.h"
#include "i18n.h"
#ifdef ENABLE_UPDATER
#include "updater.h"
#endif

#define W 1280
#define H 720
#define RGB(r, g, b) ((u32)(r) | (u32)(g) << 8 | (u32)(b) << 16)
static const u32 OURO = RGB(229, 193, 88), OURO_ESC = RGB(150, 112, 44), OURO_MED = RGB(201, 162, 77),
                 CINZA = RGB(176, 166, 152), CINZA_ESC = RGB(110, 102, 92), BRANCO = RGB(242, 234, 222),
                 VERMELHO = RGB(232, 86, 64), FUNDO = RGB(19, 15, 13), PAINEL = RGB(30, 24, 20), ESCURO = RGB(24, 16, 10),
                 VERDE = RGB(120, 200, 110);

#ifndef VERSAO
#define VERSAO "dev"
#endif

typedef struct {
    const char *titulo, *sub, *arquivo, *capitulo, *id, *save, *npwr;  // sub e capitulo são chaves de tr()
    u32 tinta, claro;
} Jogo;
static const Jogo jogos[] = {
    { "God of War", "game.gow1.subtitle", "gow1.nro", "game.gow1.chapter", "gow1", "GOW1", "NPWR05244_00",
      RGB(40, 9, 6), RGB(150, 40, 24) },
    { "God of War II", "game.gow2.subtitle", "gow2.nro", "game.gow2.chapter", "gow2", "GOW2", "NPWR05283_00",
      RGB(10, 14, 30), RGB(40, 70, 130) },
};
#define N (int)(sizeof jogos / sizeof jogos[0])

// Idioma da interface: 0 português (BR), 1 inglês, 2 espanhol
enum { UI_PT = 0, UI_EN = 1, UI_ES = 2, NUI_LANG = 3 };
static int idioma = UI_PT;
static int idioma_sistema = UI_EN;
static char tag_sistema[16] = "en";
// Códigos dos idiomas embutidos (lang/<código>.ini), na ordem de UI_PT/UI_EN/UI_ES.
static const char *const ui_codigo[NUI_LANG] = { "pt", "en", "es" };
static void aplicar_idioma(int ui) { idioma = ui; i18n_set(ui_codigo[ui]); }

static char dir[FS_MAX_PATH] = "sdmc:/switch/gow-nx/";
static char self_nro[FS_MAX_PATH] = "sdmc:/switch/gow-nx/gow-nx.nro";
static int romfs_ativo = 0;
#ifdef ENABLE_UPDATER
static char upd_tag[64];   // versão nova achada na checagem automática ("" = nenhuma)
#endif

static int existe(const char *p) { struct stat st; return stat(p, &st) == 0; }
static void caminho(char *out, size_t n, int i) { snprintf(out, n, "%.700s%s", dir, jogos[i].arquivo); }

static int ui_lang_por_prefixo(const char *s) {
    if (!s || !s[0] || !s[1]) return -1;
    char a = (char)tolower((unsigned char)s[0]);
    char b = (char)tolower((unsigned char)s[1]);
    char c = s[2];
    if (c != 0 && c != '-' && c != '_') return -1;
    if (a == 'p' && b == 't') return UI_PT;
    if (a == 'e' && b == 'n') return UI_EN;
    if (a == 'e' && b == 's') return UI_ES;
    return -1;
}

static int codigo_eq_ci(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);
        if (ca == '_') ca = '-';
        if (cb == '_') cb = '-';
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

static int prefixo_base_eq_ci(const char *a, const char *b) {
    if (!a || !b || !a[0] || !b[0]) return 0;
    while (*a && *a != '-' && *a != '_' && *b && *b != '-' && *b != '_') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return (*a == 0 || *a == '-' || *a == '_') && (*b == 0 || *b == '-' || *b == '_');
}

static int detectar_idioma_switch(void) {
    u64 code = 0;
    if (R_SUCCEEDED(setInitialize())) {
        if (R_FAILED(setGetSystemLanguage(&code))) code = 0;
        setExit();
    }
    char tag[9];
    memcpy(tag, &code, 8);
    tag[8] = 0;
    if (tag[0]) snprintf(tag_sistema, sizeof tag_sistema, "%s", tag);
    else snprintf(tag_sistema, sizeof tag_sistema, "en");
    int l = ui_lang_por_prefixo(tag_sistema);
    return l >= 0 ? l : UI_EN;
}

static int ler_ultimo(void) {
    char p[FS_MAX_PATH]; snprintf(p, sizeof p, "%.700slauncher.cfg", dir);
    FILE *f = fopen(p, "r"); int i = 0;
    if (f) { if (fscanf(f, "%d", &i) != 1) i = 0; fclose(f); }
    return (i >= 0 && i < N) ? i : 0;
}
static void gravar_ultimo(int i) {
    char p[FS_MAX_PATH]; snprintf(p, sizeof p, "%.700slauncher.cfg", dir);
    FILE *f = fopen(p, "w"); if (f) { fprintf(f, "%d\n", i); fclose(f); }
}

// ---------------------------------------------------------------- traduções no SD (data/traducoes/*/)
#define MAX_SD_TRADS 24
typedef struct {
    char codigo[64];
    char nome[64];
    int  ui_lang;   // UI_PT / UI_EN / UI_ES ou -1 se idioma externo (fr, de, ru...)
} TradSD;
static TradSD trad_sd[MAX_SD_TRADS];
static int n_trad_sd = 0;

static void apara(char *s) { size_t n = strlen(s); while (n && (unsigned char)s[n - 1] <= 32) s[--n] = 0; }

static void ler_nome_trad_ini(const char *ini_path, char *out, size_t out_sz) {
    FILE *f = fopen(ini_path, "r");
    if (!f) return;
    char l[256];
    while (fgets(l, sizeof l, f)) {
        apara(l);
        const char *v = NULL;
        if (!strncmp(l, "nome=", 5)) v = l + 5;
        else if (!strncmp(l, "name=", 5)) v = l + 5;
        if (v && v[0]) {
            while (*v == ' ' || *v == '\t') v++;
            if (v[0]) { snprintf(out, out_sz, "%s", v); break; }
        }
    }
    fclose(f);
}

static int cmp_trad_sd(const void *a, const void *b) {
    return strcmp(((const TradSD *)a)->codigo, ((const TradSD *)b)->codigo);
}

static int buscar_trad_sd_por_tag(const char *tag) {
    if (!tag || !tag[0]) return -1;
    for (int i = 0; i < n_trad_sd; i++)
        if (codigo_eq_ci(trad_sd[i].codigo, tag)) return i;
    for (int i = 0; i < n_trad_sd; i++)
        if (prefixo_base_eq_ci(trad_sd[i].codigo, tag)) return i;
    return -1;
}

static int buscar_trad_sd_por_ui(int ui) {
    int best = -1;
    for (int i = 0; i < n_trad_sd; i++) {
        if (trad_sd[i].ui_lang != ui) continue;
        if (codigo_eq_ci(trad_sd[i].codigo, tag_sistema)) return i;
        if (best < 0) best = i;
    }
    return best;
}

// ---------------------------------------------------------------- opções (as mesmas do menu R + Plus)
// Gravadas em <pasta>/data/vitart_options.ini, que os dois jogos leem ao abrir. Linhas que o launcher não conhece
// (mods, ISO...) ficam como estão.
enum { ABA_IMAGEM, ABA_COR, ABA_DESEMPENHO, ABA_JOGO, NABAS };
static const char *const abas_chave[NABAS] = { "options.tab.image", "options.tab.color", "options.tab.performance",
                                              "options.tab.game" };
#define ABA(i) tr(abas_chave[i])

typedef struct {
    int aba; const char *nome, *chave, *desc;
    int n; const char *const *rot, *const *val;
    int padrao, atual, barra;
} Opcao;

static const char *const b_rot[] = { "val.off", "val.on" }, *const b_val[] = { "0", "1" };
static const char *const bf_rot[] = { "val.off_f", "val.on_f" };
static const char *const rd_rot[] = { "val.automatic", "deko3d", "Vulkan" }, *const rd_val[] = { "auto", "deko3d", "vulkan" };
static const char *const r_rot[] = { "val.automatic_f", "1x (720x408)", "Vita (960x544)", "720p (1270x720)", "2x (1440x816)", "3x (2160x1224)", "4x (2880x1632)" };
static const char *const rt_rot[] = { "val.same_as_handheld", "1x (720x408)", "Vita (960x544)", "720p (1270x720)", "2x (1440x816)", "3x (2160x1224)", "4x (2880x1632)" };
static const char *const r_val[] = { "0", "1", "6", "5", "2", "3", "4" };
static const char *const as_rot[] = { "val.automatic_f", "16:9", "16:10", "21:9", "32:9", "4:3" };
static const char *const as_val[] = { "auto", "16:9", "16:10", "21:9", "32:9", "4:3" };
static const char *const up_rot[] = { "Bilinear", "FSR 1.0" }, *const up_val[] = { "bilinear", "fsr1" };
static const char *const n10_rot[] = { "val.off_f", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
static const char *const n10_val[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
static const char *const aa_rot[] = { "val.off", "FXAA" }, *const aa_val[] = { "off", "fxaa" };
static const char *const an_rot[] = { "val.aniso_off", "2x", "4x", "8x", "16x" }, *const an_val[] = { "1", "2", "4", "8", "16" };
static const char *const g9_rot[] = { "-4", "-3", "-2", "-1", "val.default", "+1", "+2", "+3", "+4" };
static const char *const v5_rot[] = { "val.off_f", "1", "2", "3", "4" };
static const char *const f_rot[] = { "30", "60", "val.max" }, *const f_val[] = { "30", "60", "max" };
static const char *const fg_rot[] = { "val.off_f", "val.performance", "val.quality" }, *const fg_val[] = { "off", "performance", "quality" };
static const char *const fl_rot[] = { "val.flow_full", "1/2", "1/3", "val.flow_quarter" }, *const fl_val[] = { "1", "2", "3", "4" };
static const char *const ui_nomes[NUI_LANG] = { "Português (BR)", "English", "Español" };
static const char *const lg_val_base[] = { "auto", "pt", "en", "es" };
static const char *lg_val[1 + NUI_LANG + MAX_SD_TRADS] = { "auto", "pt", "en", "es" };
static char lg_rot_buf[1 + NUI_LANG + MAX_SD_TRADS][96];
static const char *lg_rot[1 + NUI_LANG + MAX_SD_TRADS] = { "Auto", "Português (BR)", "English", "Español" };
static int lg_sd_idx[1 + NUI_LANG + MAX_SD_TRADS];

enum { O_REND, O_RES, O_RESD, O_ASP, O_UPS, O_SHARP, O_AA, O_ANISO, O_GAMMA, O_SAT, O_VIB, O_DITHER,
       O_FPS, O_FGEN, O_FLOW, O_VSYNC, O_COUNTER, O_TEX, O_HP, O_QTE, O_VIDA, O_MAGIA, O_FURIA, O_LANG, O_MUSICA, NOP };
static Opcao opcoes[NOP] = {
    [O_REND] = { ABA_IMAGEM, "opt.renderer.name", "renderer=", "opt.renderer.desc", 3, rd_rot, rd_val, 0 },
    [O_RES] = { ABA_IMAGEM, "opt.resolution_switch.name", "resolution_switch=", "opt.resolution_switch.desc", 7, r_rot, r_val, 0 },
    [O_RESD] = { ABA_IMAGEM, "opt.resolution_docked.name", "resolution_docked=", "opt.resolution_docked.desc", 7, rt_rot, r_val, 0 },
    [O_ASP] = { ABA_IMAGEM, "opt.aspect.name", "aspect=", "opt.aspect.desc", 6, as_rot, as_val, 0 },
    [O_UPS] = { ABA_IMAGEM, "opt.upscaler.name", "upscaler=", "opt.upscaler.desc", 2, up_rot, up_val, 0 },
    [O_SHARP] = { ABA_IMAGEM, "opt.sharpness.name", "sharpness=", "opt.sharpness.desc", 11, n10_rot, n10_val, 0, 0, 1 },
    [O_AA] = { ABA_IMAGEM, "opt.anti_aliasing.name", "anti_aliasing=", "opt.anti_aliasing.desc", 2, aa_rot, aa_val, 0 },
    [O_ANISO] = { ABA_IMAGEM, "opt.anisotropic.name", "anisotropic=", "opt.anisotropic.desc", 5, an_rot, an_val, 0, 0, 1 },
    [O_GAMMA] = { ABA_COR, "opt.gamma.name", "gamma=", "opt.gamma.desc", 9, g9_rot, n10_val, 4, 4, 1 },
    [O_SAT] = { ABA_COR, "opt.saturation.name", "saturation=", "opt.saturation.desc", 9, g9_rot, n10_val, 4, 4, 1 },
    [O_VIB] = { ABA_COR, "opt.vibrance.name", "vibrance=", "opt.vibrance.desc", 5, v5_rot, n10_val, 0, 0, 1 },
    [O_DITHER] = { ABA_COR, "opt.debanding.name", "debanding=", "opt.debanding.desc", 2, b_rot, b_val, 0 },
    [O_FPS] = { ABA_DESEMPENHO, "opt.fps.name", "fps=", "opt.fps.desc", 3, f_rot, f_val, 0 },
    [O_FGEN] = { ABA_DESEMPENHO, "opt.frame_generation.name", "frame_generation=", "opt.frame_generation.desc", 3, fg_rot, fg_val, 0 },
    [O_FLOW] = { ABA_DESEMPENHO, "opt.fg_flow.name", "fg_flow=", "opt.fg_flow.desc", 4, fl_rot, fl_val, 1, 1 },
    [O_VSYNC] = { ABA_DESEMPENHO, "opt.vsync.name", "vsync=", "opt.vsync.desc", 2, bf_rot, b_val, 1, 1 },
    [O_COUNTER] = { ABA_DESEMPENHO, "opt.fps_counter.name", "fps_counter=", "opt.fps_counter.desc", 2, b_rot, b_val, 0 },
    [O_TEX] = { ABA_JOGO, "opt.hd_textures.name", "hd_textures=", "opt.hd_textures.desc", 2, bf_rot, b_val, 1, 1 },
    [O_HP] = { ABA_JOGO, "opt.enemy_health_bars.name", "enemy_health_bars=", "opt.enemy_health_bars.desc", 2, bf_rot, b_val, 1, 1 },
    [O_QTE] = { ABA_JOGO, "opt.auto_qte.name", "auto_qte=", "opt.auto_qte.desc", 2, b_rot, b_val, 0 },
    [O_VIDA] = { ABA_JOGO, "opt.cheat_infinite_health.name", "cheat_infinite_health=", "opt.cheat_infinite_health.desc", 2, bf_rot, b_val, 0 },
    [O_MAGIA] = { ABA_JOGO, "opt.cheat_infinite_magic.name", "cheat_infinite_magic=", "opt.cheat_infinite_magic.desc", 2, bf_rot, b_val, 0 },
    [O_FURIA] = { ABA_JOGO, "opt.cheat_infinite_rage.name", "cheat_infinite_rage=", "opt.cheat_infinite_rage.desc", 2, bf_rot, b_val, 0 },
    [O_LANG] = { ABA_JOGO, "opt.language.name", "language=", "opt.language.desc", 4, lg_rot, lg_val, 0 },
    [O_MUSICA] = { ABA_JOGO, "opt.launcher_music.name", "launcher_music=", "opt.launcher_music.desc", 2, bf_rot, b_val, 1, 1 },
};
static const char *op_nome(const Opcao *o) { return tr(o->nome); }
static const char *op_desc(const Opcao *o) { return tr(o->desc); }
static const char *op_rot(const Opcao *o, int j) { return tr(o->rot[j]); }  // números e nomes técnicos não têm chave

static void escanear_traducoes(void) {
    n_trad_sd = 0;
    for (int i = 0; i <= NUI_LANG; i++) { lg_val[i] = lg_val_base[i]; lg_sd_idx[i] = -1; }
    opcoes[O_LANG].n = 1 + NUI_LANG;

    char base[FS_MAX_PATH];
    snprintf(base, sizeof base, "%.700sdata/traducoes", dir);
    DIR *dp = opendir(base);
    if (dp) {
        struct dirent *de;
        while ((de = readdir(dp)) != NULL && n_trad_sd < MAX_SD_TRADS) {
            const char *name = de->d_name;
            if (!name[0] || name[0] == '.') continue;
            if (strlen(name) >= sizeof trad_sd[0].codigo) continue;
            char rec[FS_MAX_PATH + 128];
            snprintf(rec, sizeof rec, "%.700sdata/traducoes/%s/records.tsv", dir, name);
            if (!existe(rec)) continue;

            TradSD *t = &trad_sd[n_trad_sd];
            snprintf(t->codigo, sizeof t->codigo, "%s", name);
            snprintf(t->nome, sizeof t->nome, "%s", name);
            char ini[FS_MAX_PATH + 128];
            snprintf(ini, sizeof ini, "%.700sdata/traducoes/%s/idioma.ini", dir, name);
            ler_nome_trad_ini(ini, t->nome, sizeof t->nome);
            t->ui_lang = ui_lang_por_prefixo(name);
            n_trad_sd++;
        }
        closedir(dp);
    }
    if (n_trad_sd > 1) qsort(trad_sd, (size_t)n_trad_sd, sizeof trad_sd[0], cmp_trad_sd);

    // Atualiza rótulos das opções de idioma (Auto + PT/EN/ES + pastas extras do SD)
    int si_auto = buscar_trad_sd_por_tag(tag_sistema);
    if (si_auto >= 0) {
        const char *lbl = (trad_sd[si_auto].ui_lang == idioma_sistema) ? ui_nomes[idioma_sistema] : trad_sd[si_auto].nome;
        snprintf(lg_rot_buf[0], sizeof lg_rot_buf[0], "Auto (%s) [SD]", lbl);
    } else {
        snprintf(lg_rot_buf[0], sizeof lg_rot_buf[0], "Auto (%s)", ui_nomes[idioma_sistema]);
    }
    lg_rot[0] = lg_rot_buf[0];

    for (int u = 0; u < NUI_LANG; u++) {
        int si = buscar_trad_sd_por_ui(u);
        if (si >= 0) snprintf(lg_rot_buf[1 + u], sizeof lg_rot_buf[1 + u], "%s [SD]", ui_nomes[u]);
        else snprintf(lg_rot_buf[1 + u], sizeof lg_rot_buf[1 + u], "%s", ui_nomes[u]);
        lg_rot[1 + u] = lg_rot_buf[1 + u];
    }

    int claimed[NUI_LANG] = { 0 };
    for (int i = 0; i < n_trad_sd; i++) {
        int ul = trad_sd[i].ui_lang;
        if (ul >= 0 && ul < NUI_LANG && !claimed[ul] && buscar_trad_sd_por_ui(ul) == i) {
            claimed[ul] = 1;
            continue;
        }
        int slot = opcoes[O_LANG].n;
        lg_val[slot] = trad_sd[i].codigo;
        lg_sd_idx[slot] = i;
        snprintf(lg_rot_buf[slot], sizeof lg_rot_buf[slot], "%s [SD]", trad_sd[i].nome);
        lg_rot[slot] = lg_rot_buf[slot];
        opcoes[O_LANG].n = slot + 1;
    }
}

static int resolver_ui_idioma(void) {
    int a = opcoes[O_LANG].atual;
    if (a >= 1 && a <= NUI_LANG) return a - 1;
    if (a > NUI_LANG && a < opcoes[O_LANG].n) {
        int si = lg_sd_idx[a];
        if (si >= 0 && si < n_trad_sd && trad_sd[si].ui_lang >= 0) return trad_sd[si].ui_lang;
    }
    return idioma_sistema;
}

static const char *idioma_codigo_vitart_atual(void) {
    int a = opcoes[O_LANG].atual;
    if (a == 0) {
        int si = buscar_trad_sd_por_tag(tag_sistema);
        if (si >= 0) return trad_sd[si].codigo;
        return lg_val_base[1 + idioma_sistema];
    }
    if (a >= 1 && a <= NUI_LANG) {
        int si = buscar_trad_sd_por_ui(a - 1);
        if (si >= 0) return trad_sd[si].codigo;
        return lg_val_base[a];
    }
    if (a > NUI_LANG && a < opcoes[O_LANG].n) return opcoes[O_LANG].val[a];
    return lg_val_base[1 + idioma_sistema];
}

static int parse_idioma_opt(const char *v) {
    if (!v || !v[0]) return -1;
    for (int j = 0; j < opcoes[O_LANG].n; j++)
        if (codigo_eq_ci(v, opcoes[O_LANG].val[j])) return j;
    for (int i = 0; i < n_trad_sd; i++) {
        if (!codigo_eq_ci(v, trad_sd[i].codigo)) continue;
        for (int j = 1 + NUI_LANG; j < opcoes[O_LANG].n; j++)
            if (lg_sd_idx[j] == i) return j;
        if (trad_sd[i].ui_lang >= 0) return trad_sd[i].ui_lang + 1;
    }
    int ul = ui_lang_por_prefixo(v);
    if (ul >= 0) return ul + 1;
    return -1;
}

static void ini_caminho(char *p, size_t n) { snprintf(p, n, "%.700sdata/vitart_options.ini", dir); }

static int ini_tem_arquivo = 0;
static char ini_language_lido[64] = "";

static void opcoes_ler(void) {
    ini_tem_arquivo = 0;
    ini_language_lido[0] = 0;
    char p[FS_MAX_PATH], l[1024]; ini_caminho(p, sizeof p);
    FILE *f = fopen(p, "r");
    if (!f) { aplicar_idioma(resolver_ui_idioma()); return; }
    ini_tem_arquivo = 1;
    int lang_cfg = -1, lang_ini = -1;
    while (fgets(l, sizeof l, f)) {
        apara(l);
        if (!strncmp(l, "launcher_language=", 18)) {
            int idx = parse_idioma_opt(l + 18);
            if (idx >= 0) lang_cfg = idx;
            continue;
        }
        for (int i = 0; i < NOP; i++) {
            size_t k = strlen(opcoes[i].chave);
            if (strncmp(l, opcoes[i].chave, k)) continue;
            const char *v = l + k;
            if (i == O_FPS) { opcoes[i].atual = !strcmp(v, "max") ? 2 : atoi(v) == 60 ? 1 : atoi(v) > 60 ? 2 : 0; continue; }
            if (i == O_LANG) {
                snprintf(ini_language_lido, sizeof ini_language_lido, "%s", v);
                int idx = parse_idioma_opt(v);
                if (idx >= 0) lang_ini = idx;
                continue;
            }
            for (int j = 0; j < opcoes[i].n; j++) if (!strcmp(v, opcoes[i].val[j])) opcoes[i].atual = j;
        }
    }
    fclose(f);
    if (lang_cfg >= 0) opcoes[O_LANG].atual = lang_cfg;
    else if (lang_ini >= 0) opcoes[O_LANG].atual = lang_ini;
    aplicar_idioma(resolver_ui_idioma());
}

static int opcoes_gravar(void) {
    char p[FS_MAX_PATH], np[FS_MAX_PATH + 8], l[1024]; ini_caminho(p, sizeof p);
    { char d[FS_MAX_PATH]; snprintf(d, sizeof d, "%.700sdata", dir); mkdir(d, 0777); }
    snprintf(np, sizeof np, "%s.novo", p);
    FILE *o = fopen(np, "w"); if (!o) return 0;
    FILE *f = fopen(p, "r");
    if (f) {
        while (fgets(l, sizeof l, f)) {
            apara(l);
            int nossa = !strncmp(l, "launcher_language=", 18);
            for (int i = 0; i < NOP; i++) nossa |= !strncmp(l, opcoes[i].chave, strlen(opcoes[i].chave));
            if (!nossa) fprintf(o, "%s\n", l);
        }
        fclose(f);
    }
    fprintf(o, "launcher_language=%s\n", opcoes[O_LANG].val[opcoes[O_LANG].atual]);
    const char *vc = idioma_codigo_vitart_atual();
    for (int i = 0; i < NOP; i++) {
        if (i == O_LANG) {
            if (vc[0]) fprintf(o, "language=%s\n", vc);
        } else {
            fprintf(o, "%s%s\n", opcoes[i].chave, opcoes[i].val[opcoes[i].atual]);
        }
    }
    int ok = !ferror(o);
    if (fclose(o) != 0) ok = 0;
    if (!ok) { remove(np); return 0; }
    remove(p);
    if (rename(np, p) != 0) return 0;
    ini_tem_arquivo = 1;
    snprintf(ini_language_lido, sizeof ini_language_lido, "%s", vc);
    return 1;
}

static void sincronizar_idioma_auto(void) {
    if (opcoes[O_LANG].atual != 0) return;
    int si = buscar_trad_sd_por_tag(tag_sistema);
    if (si < 0) return;
    if (!ini_tem_arquivo || strcmp(ini_language_lido, trad_sd[si].codigo) != 0)
        opcoes_gravar();
}

// Perfil rápido (L/R na tela principal): mexe só na resolução do portátil e na taxa de quadros.
typedef struct { const char *nome; int res, fps; } Preset;  // nome é chave de tr()
static const Preset presets[] = {
    { "launcher.preset.performance", 2, 1 },
    { "launcher.preset.balanced", 3, 1 },
    { "launcher.preset.quality", 4, 0 },
};
#define NPRE (int)(sizeof presets / sizeof presets[0])
static int preset_atual(void) {
    for (int i = 0; i < NPRE; i++)
        if (opcoes[O_RES].atual == presets[i].res && opcoes[O_FPS].atual == presets[i].fps) return i;
    return -1;
}
static void preset_aplicar(int i) { opcoes[O_RES].atual = presets[i].res; opcoes[O_FPS].atual = presets[i].fps; }

// ---------------------------------------------------------------- informações de cada jogo (save, troféus, extras)
typedef struct { time_t save; int tem_save, trof_total, trof_ok, videos, texpack, nro, dump; } Info;
static Info info[N];

static u32 be32(const u8 *p) { return (u32)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
static u64 be64(const u8 *p) { return (u64)be32(p) << 32 | be32(p + 4); }

static int trp_total(const char *p) {   // nº de troféus no TROP.SFM de dentro do TROPHY.TRP
    FILE *f = fopen(p, "rb"); if (!f) return 0;
    u8 h[0x40]; int total = 0;
    if (fread(h, 1, sizeof h, f) == sizeof h && be32(h) == 0xDCA24D00u) {
        u32 n = be32(h + 16), esz = be32(h + 20);
        for (u32 i = 0; i < n && i < 64 && esz >= 48 && esz <= 0x100; i++) {
            u8 e[0x100];
            if (fseek(f, 0x40 + (long)i * esz, SEEK_SET) || fread(e, 1, esz, f) != esz) break;
            if (strncmp((const char *)e, "TROP.SFM", 32)) continue;
            u64 off = be64(e + 32), sz = be64(e + 40);
            if (sz == 0 || sz > (4u << 20)) break;
            char *x = malloc(sz + 1);
            if (x && !fseek(f, (long)off, SEEK_SET) && fread(x, 1, sz, f) == sz) {
                x[sz] = 0;
                for (char *q = x; (q = strstr(q, "<trophy id=\"")); q++) total++;
            }
            free(x);
            break;
        }
    }
    fclose(f);
    return total;
}

static int conta_linhas(const char *p) {
    FILE *f = fopen(p, "r"); if (!f) return 0;
    char l[128]; int n = 0;
    while (fgets(l, sizeof l, f)) { apara(l); if (*l) n++; }
    fclose(f);
    return n;
}

static int conta_arquivos(const char *p, const char *ext) {   // ext NULL = qualquer arquivo
    DIR *d = opendir(p); if (!d) return 0;
    int n = 0; struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        size_t a = strlen(e->d_name), b = ext ? strlen(ext) : 0;
        if (!ext || (a > b && !strcasecmp(e->d_name + a - b, ext))) n++;
        if (!ext && n) break;
    }
    closedir(d);
    return n;
}

static void info_ler(void) {
    for (int i = 0; i < N; i++) {
        char p[FS_MAX_PATH]; struct stat st;
        Info *o = &info[i];
        snprintf(p, sizeof p, "%.700sdata/savedata0/%s/MASTER.BIN", dir, jogos[i].save);
        if (stat(p, &st) == 0) o->tem_save = 1, o->save = st.st_mtime;
        snprintf(p, sizeof p, "%.700sgame/PCSA00126/sce_sys/trophy/%s/TROPHY.TRP", dir, jogos[i].npwr);
        o->trof_total = trp_total(p);
        snprintf(p, sizeof p, "%.700sdata/trophy/%s/unlocked.txt", dir, jogos[i].npwr);
        o->trof_ok = conta_linhas(p);
        if (o->trof_total && o->trof_ok > o->trof_total) o->trof_ok = o->trof_total;
        snprintf(p, sizeof p, "%.700sdata/videos/%s", dir, jogos[i].id);
        o->videos = conta_arquivos(p, ".mp4");
        snprintf(p, sizeof p, "%.700sdata/texpack/%s", dir, jogos[i].id);
        o->texpack = conta_arquivos(p, NULL) > 0;
        caminho(p, sizeof p, i);
        o->nro = existe(p);
        o->dump = ga_game_dump_ok(dir, i);
    }
}

// ---------------------------------------------------------------- música
#define AFRAMES 2048                       // quadros por buffer (~43 ms a 48 kHz)
#define ABYTES  ((AFRAMES * 4 + 0xfff) & ~0xfff)
#define NBUF 4
static Thread a_thr;
static volatile int a_parar = 0, a_ok = 0;
static void *a_mem[NBUF];
static AudioOutBuffer a_buf[NBUF];
static GaAudio *a_stream = NULL;

static void musica(void *arg) {
    (void)arg;
    GaAudio *st = a_stream;
    if (!st) return;
    long rate = ga_audio_rate(st);

    // Reamostragem linear rate -> 48000 (o audout só toca a 48 kHz). in[0] guarda a última amostra do bloco anterior.
    const double passo = (double)rate / 48000.0;
    static int16_t in[8192 * 2];
    size_t n_in = 0; double pos = 0;
    int16_t prev[2] = { 0, 0 };
    float ganho = 0.f;                    // sobe de 0 (fade-in) e desce ao sair (fade-out)
    int fila = 0, vazio = 0, mudo = 0;

    while (1) {
        AudioOutBuffer *b = NULL;
        if (fila < NBUF) b = &a_buf[fila++];
        else { u32 cnt = 0; audoutWaitPlayFinish(&b, &cnt, UINT64_MAX); if (!b) continue; }
        int16_t *out = (int16_t *)b->buffer;
        for (int f = 0; f < AFRAMES; f++) {
            while (!mudo && pos + 1 >= (double)n_in) {
                if (n_in) { prev[0] = in[(n_in - 1) * 2]; prev[1] = in[(n_in - 1) * 2 + 1]; pos -= n_in; }
                size_t got_frames = ga_audio_read_stereo_s16(st, in + 2, 8191);
                in[0] = prev[0]; in[1] = prev[1];
                n_in = got_frames + 1; pos += 1;
                if (got_frames) vazio = 0; else if (++vazio > 64) mudo = 1;
            }
            if (a_parar) ganho -= 1.f / (48000 * 0.35f); else if (ganho < 1.f) ganho += 1.f / (48000 * 1.5f);
            if (ganho < 0) ganho = 0;
            if (mudo) { out[f * 2] = out[f * 2 + 1] = 0; continue; }
            size_t i = (size_t)pos; float t = (float)(pos - i), g = ganho * ganho * 0.85f;
            for (int c = 0; c < 2; c++) {
                float v = in[i * 2 + c] + (in[(i + 1) * 2 + c] - in[i * 2 + c]) * t;
                out[f * 2 + c] = (int16_t)(v * g);
            }
            pos += passo;
        }
        b->data_size = AFRAMES * 4;
        audoutAppendAudioOutBuffer(b);
        if (a_parar && ganho <= 0) break;
    }
    for (int k = 0; k < NBUF; k++) { AudioOutBuffer *r = NULL; u32 c = 0; audoutWaitPlayFinish(&r, &c, 200000000ULL); }
}

static void musica_iniciar(void) {
    if (a_ok || !opcoes[O_MUSICA].atual) return;
    a_stream = ga_audio_open(dir, romfs_ativo);
    if (!a_stream) return;
    if (R_FAILED(audoutInitialize())) { ga_audio_close(a_stream); a_stream = NULL; return; }
    if (R_FAILED(audoutStartAudioOut())) { audoutExit(); ga_audio_close(a_stream); a_stream = NULL; return; }
    a_parar = 0;
    for (int i = 0; i < NBUF; i++) a_mem[i] = NULL;
    for (int i = 0; i < NBUF; i++) {
        a_mem[i] = aligned_alloc(0x1000, ABYTES);
        if (!a_mem[i]) {
            audoutStopAudioOut(); audoutExit(); ga_audio_close(a_stream); a_stream = NULL;
            for (int j = 0; j < i; j++) { free(a_mem[j]); a_mem[j] = NULL; }
            return;
        }
        memset(a_mem[i], 0, ABYTES);
        a_buf[i].next = NULL; a_buf[i].buffer = a_mem[i]; a_buf[i].buffer_size = ABYTES;
        a_buf[i].data_size = AFRAMES * 4; a_buf[i].data_offset = 0;
    }
    if (R_FAILED(threadCreate(&a_thr, musica, NULL, NULL, 0x40000, 0x2C, -2))) {
        audoutStopAudioOut(); audoutExit(); ga_audio_close(a_stream); a_stream = NULL;
        for (int i = 0; i < NBUF; i++) { free(a_mem[i]); a_mem[i] = NULL; }
        return;
    }
    threadStart(&a_thr); a_ok = 1;
}

static void musica_parar(void) {                 // fade-out curto e libera o áudio para o jogo
    if (!a_ok) return;
    a_parar = 1;
    threadWaitForExit(&a_thr); threadClose(&a_thr);
    audoutStopAudioOut(); audoutExit();
    ga_audio_close(a_stream); a_stream = NULL;
    for (int i = 0; i < NBUF; i++) { free(a_mem[i]); a_mem[i] = NULL; }
    a_ok = 0;
}

// ---------------------------------------------------------------- desenho básico
static u32 *bg;                                   // fundo pronto (formato do framebuffer)
static FT_Library ft;
static FT_Face f_txt, f_ext;                      // fonte do sistema + fonte com os botões da Nintendo
static u32 *px; static u32 pitch;                 // destino atual (pitch em pixels)

static inline void mistura(int x, int y, u32 cor, int a) {
    if ((unsigned)x >= W || (unsigned)y >= H || a <= 0) return;
    u32 *d = &px[y * pitch + x];
    if (a >= 255) { *d = cor | 0xff000000u; return; }
    u32 o = *d;
    int r = ((o & 0xff) * (255 - a) + (cor & 0xff) * a) / 255;
    int g = (((o >> 8) & 0xff) * (255 - a) + ((cor >> 8) & 0xff) * a) / 255;
    int b = (((o >> 16) & 0xff) * (255 - a) + ((cor >> 16) & 0xff) * a) / 255;
    *d = r | g << 8 | b << 16 | 0xff000000u;
}

static u32 mix(u32 a, u32 b, float t) {          // a -> b
    int r = (a & 0xff) + (int)(((int)(b & 0xff) - (int)(a & 0xff)) * t);
    int g = (a >> 8 & 0xff) + (int)(((int)(b >> 8 & 0xff) - (int)(a >> 8 & 0xff)) * t);
    int bl = (a >> 16 & 0xff) + (int)(((int)(b >> 16 & 0xff) - (int)(a >> 16 & 0xff)) * t);
    return RGB(r, g, bl);
}

static u32 utf8(const char **s) {
    const unsigned char *p = (const unsigned char *)*s; u32 c = *p++;
    if (c >= 0xf0) { c = (c & 7) << 18 | (p[0] & 63) << 12 | (p[1] & 63) << 6 | (p[2] & 63); p += 3; }
    else if (c >= 0xe0) { c = (c & 15) << 12 | (p[0] & 63) << 6 | (p[1] & 63); p += 2; }
    else if (c >= 0xc0) { c = (c & 31) << 6 | (p[0] & 63); p += 1; }
    *s = (const char *)p; return c;
}

static FT_Face face_de(u32 c) {
    if (c >= 0xE000 && c <= 0xF8FF && f_ext && FT_Get_Char_Index(f_ext, c)) return f_ext;
    return f_txt;
}

// Cache de glifos já rasterizados (FreeType a cada quadro custava a maior parte do tempo).
typedef struct { u32 c; int tam, w, h, esq, topo, av; u8 *bmp; } Glifo;
#define NGLIFO 2048
static Glifo glifos[NGLIFO];

static const Glifo *glifo(u32 c, int tam) {
    u32 h = (c * 2654435761u ^ (u32)tam * 40503u) & (NGLIFO - 1);
    for (int k = 0; k < 16; k++, h = (h + 1) & (NGLIFO - 1)) {
        Glifo *g = &glifos[h];
        if (g->tam == tam && g->c == c) return g;
        if (g->tam) continue;
        FT_Face f = face_de(c);
        if (!f) return NULL;
        FT_Set_Pixel_Sizes(f, 0, tam);
        if (FT_Load_Char(f, c, FT_LOAD_RENDER)) return NULL;
        FT_GlyphSlot s = f->glyph;
        g->c = c, g->tam = tam, g->w = s->bitmap.width, g->h = s->bitmap.rows;
        g->esq = s->bitmap_left, g->topo = s->bitmap_top, g->av = s->advance.x >> 6;
        g->bmp = malloc((size_t)g->w * g->h + 1);
        if (g->bmp) for (int y = 0; y < g->h; y++) memcpy(g->bmp + y * g->w, s->bitmap.buffer + y * s->bitmap.pitch, g->w);
        else g->w = g->h = 0;
        return g;
    }
    return NULL;   // cache cheio nessa região: sem o glifo
}

// Texto UTF-8 com a linha de base em y e `esp` pixels extras entre letras. Devolve a largura.
static int texto_e(int x, int y, int tam, u32 cor, int alfa, const char *s, int esp) {
    int x0 = x;
    while (*s) {
        u32 c = utf8(&s);
        const Glifo *g = glifo(c, tam);
        if (!g) continue;
        if (alfa > 0)
            for (int yy = 0; yy < g->h; yy++)
                for (int xx = 0; xx < g->w; xx++) {
                    int a = g->bmp[yy * g->w + xx];
                    if (a) mistura(x + g->esq + xx, y - g->topo + yy, cor, a * alfa / 255);
                }
        x += g->av + (*s ? esp : 0);
    }
    return x - x0;
}
static int texto(int x, int y, int tam, u32 cor, int alfa, const char *s) { return texto_e(x, y, tam, cor, alfa, s, 0); }
static int largura(int tam, const char *s, int esp) { return texto_e(0, 0, tam, 0, 0, s, esp); }
static int texto_sombra(int x, int y, int tam, u32 cor, int alfa, const char *s) {
    texto(x + 1, y + 2, tam, 0x000000, alfa * 2 / 3, s);
    return texto(x, y, tam, cor, alfa, s);
}
static int negrito(int x, int y, int tam, u32 cor, int alfa, const char *s, int sombra) {
    if (sombra) { texto(x + 2, y + 3, tam, 0, alfa * 2 / 3, s); texto(x + 3, y + 3, tam, 0, alfa * 2 / 3, s); }
    texto(x, y, tam, cor, alfa, s);
    return texto(x + 1, y, tam, cor, alfa, s) + 1;
}
static int texto_dir(int xd, int y, int tam, u32 cor, int alfa, const char *s) {   // alinhado à direita em xd
    int w = largura(tam, s, 0);
    texto_sombra(xd - w, y, tam, cor, alfa, s);
    return w;
}

static void retangulo(int x, int y, int w, int h, u32 cor, int a) {
    for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) mistura(xx, yy, cor, a);
}

// cobertura (0..1) do pixel (i, j) num retângulo w x h com cantos de raio r
static inline float cobertura(int i, int j, int w, int h, float r) {
    if (i < 0 || j < 0 || i >= w || j >= h) return 0;
    float cx = i + .5f, cy = j + .5f, dx = 0, dy = 0;
    if (cx < r) dx = r - cx; else if (cx > w - r) dx = cx - (w - r);
    if (cy < r) dy = r - cy; else if (cy > h - r) dy = cy - (h - r);
    if (dx <= 0 || dy <= 0) return 1;
    float c = r - sqrtf(dx * dx + dy * dy) + .5f;
    return c < 0 ? 0 : c > 1 ? 1 : c;
}

static void rrect(int x, int y, int w, int h, int r, u32 cor, int a) {
    for (int j = 0; j < h; j++) {
        int canto = j < r || j >= h - r;
        for (int i = 0; i < w; i++) {
            if (!canto || (i >= r && i < w - r)) { mistura(x + i, y + j, cor, a); continue; }
            mistura(x + i, y + j, cor, (int)(a * cobertura(i, j, w, h, r)));
        }
    }
}

static void rborda(int x, int y, int w, int h, int r, int e, u32 cor, int a) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            if (i >= r && i < w - r && j >= e && j < h - e) { i = w - r - 1; continue; }   // miolo vazio
            if (j >= r && j < h - r && i >= e && i < w - e) { i = w - e - 1; continue; }
            float c = cobertura(i, j, w, h, r) - cobertura(i - e, j - e, w - 2 * e, h - 2 * e, r - e > 0 ? r - e : 0);
            if (c > 0) mistura(x + i, y + j, cor, (int)(a * c));
        }
}

static void linha_h(int x, int y, int w, u32 cor, int a) {   // linha que some nas pontas
    for (int i = 0; i < w; i++) {
        float t = (float)i / w, k = t < .2f ? t / .2f : t > .8f ? (1 - t) / .2f : 1;
        mistura(x + i, y, cor, (int)(a * k));
    }
}

// botões da Nintendo (fonte NintendoExt)
#define BT_A "\xEE\x83\xA0"
#define BT_B "\xEE\x83\xA1"
#define BT_X "\xEE\x83\xA2"
#define BT_Y "\xEE\x83\xA3"
#define BT_L "\xEE\x83\xA4"
#define BT_R "\xEE\x83\xA5"
#define BT_MAIS "\xEE\x83\xAF"
#define BT_MENOS ""

static int dica(int x, int yb, const char *bt, const char *txt) {   // "Ⓐ Jogar" no rodapé; devolve o x seguinte
    x += texto_sombra(x, yb + 2, 26, BRANCO, 255, bt);
    x += texto_sombra(x + 6, yb, 19, CINZA, 235, txt) + 6;
    return x + 34;
}

static void assinatura(int yb) {
    const char *ass = "Guto Recomp";
    texto_dir(W - 56, yb, 19, OURO_MED, 230, ass);
}

// ---------------------------------------------------------------- imagens (capas)
typedef struct { int w, h; u32 *p; } Img;     // pixels no formato do framebuffer (r | g << 8 | b << 16 | a << 24)

static int img_ler(const char *base, Img *o) {
    static const char *const ext[] = { ".webp", ".png", ".jpg", ".jpeg" };
    for (int e = 0; e < 4; e++) {
        char p[FS_MAX_PATH]; snprintf(p, sizeof p, "%s%s", base, ext[e]);
        FILE *f = fopen(p, "rb"); if (!f) continue;
        fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
        u8 *d = n > 0 && n < (32 << 20) ? malloc(n) : NULL;
        int ok = d && fread(d, 1, n, f) == (size_t)n;
        fclose(f);
        if (!ok) { free(d); continue; }
        int w = 0, h = 0; u8 *rgba = NULL;
        if (e == 0) {
            rgba = WebPDecodeRGBA(d, n, &w, &h);
            if (rgba) { o->p = malloc((size_t)w * h * 4); if (o->p) memcpy(o->p, rgba, (size_t)w * h * 4); WebPFree(rgba); }
        } else {
            int c; rgba = stbi_load_from_memory(d, (int)n, &w, &h, &c, 4);
            if (rgba) { o->p = malloc((size_t)w * h * 4); if (o->p) memcpy(o->p, rgba, (size_t)w * h * 4); stbi_image_free(rgba); }
        }
        free(d);
        if (o->p) { o->w = w, o->h = h; return 1; }
    }
    return 0;
}

// Reduz/amplia `s` para w x h cobrindo a área (corta o excesso no centro).
static int img_cobrir(const Img *s, int w, int h, Img *o) {
    o->p = malloc((size_t)w * h * 4); if (!o->p) return 0;
    o->w = w, o->h = h;
    float esc = fmaxf((float)w / s->w, (float)h / s->h);
    float sw = w / esc, sh = h / esc, x0 = (s->w - sw) * .5f, y0 = (s->h - sh) * .5f, passo = 1 / esc;
    if (passo < 1.f) {
        for (int j = 0; j < h; j++) {
            float fy = y0 + (j + .5f) * passo - .5f;
            if (fy < 0.f) fy = 0.f;
            int y1 = (int)fy, y2 = y1 + 1;
            float wy = fy - y1;
            if (y1 >= s->h - 1) { y1 = y2 = s->h - 1; wy = 0.f; }
            for (int i = 0; i < w; i++) {
                float fx = x0 + (i + .5f) * passo - .5f;
                if (fx < 0.f) fx = 0.f;
                int x1 = (int)fx, x2 = x1 + 1;
                float wx = fx - x1;
                if (x1 >= s->w - 1) { x1 = x2 = s->w - 1; wx = 0.f; }
                u32 p00 = s->p[y1 * s->w + x1], p10 = s->p[y1 * s->w + x2];
                u32 p01 = s->p[y2 * s->w + x1], p11 = s->p[y2 * s->w + x2];
                float w00 = (1.f - wx) * (1.f - wy), w10 = wx * (1.f - wy);
                float w01 = (1.f - wx) * wy,         w11 = wx * wy;
                u32 r = (u32)((p00 & 0xff) * w00 + (p10 & 0xff) * w10 + (p01 & 0xff) * w01 + (p11 & 0xff) * w11 + .5f);
                u32 g = (u32)(((p00 >> 8) & 0xff) * w00 + ((p10 >> 8) & 0xff) * w10 + ((p01 >> 8) & 0xff) * w01 + ((p11 >> 8) & 0xff) * w11 + .5f);
                u32 b = (u32)(((p00 >> 16) & 0xff) * w00 + ((p10 >> 16) & 0xff) * w10 + ((p01 >> 16) & 0xff) * w01 + ((p11 >> 16) & 0xff) * w11 + .5f);
                o->p[j * w + i] = RGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
            }
        }
        return 1;
    }
    for (int j = 0; j < h; j++) {
        int ya = (int)(y0 + j * passo), yb = (int)(y0 + (j + 1) * passo);
        if (yb <= ya) yb = ya + 1;
        if (yb > s->h) yb = s->h;
        for (int i = 0; i < w; i++) {
            int xa = (int)(x0 + i * passo), xb = (int)(x0 + (i + 1) * passo);
            if (xb <= xa) xb = xa + 1;
            if (xb > s->w) xb = s->w;
            u32 r = 0, g = 0, b = 0, n = 0;
            for (int y = ya; y < yb; y++)
                for (int x = xa; x < xb; x++) { u32 v = s->p[y * s->w + x]; r += v & 0xff; g += v >> 8 & 0xff; b += v >> 16 & 0xff; n++; }
            o->p[j * w + i] = n ? RGB(r / n, g / n, b / n) : 0;
        }
    }
    return 1;
}

// tamanhos do card selecionado e do outro
#define CW1 252
#define CH1 360
#define CW0 200
#define CH0 286
static Img capa_g[N], capa_p[N];               // capa já reduzida para o card grande e para o pequeno
static int tem_capa[N];

static void capas_carregar(void) {
    for (int i = 0; i < N; i++) {
        char base[FS_MAX_PATH]; snprintf(base, sizeof base, "%.700sdata/launcher/capa_%s", dir, jogos[i].id);
        Img o = { 0 };
        if (!img_ler(base, &o)) {               // sem capa própria no SD: tenta romfs:/ ou game/PCSA00126/
            snprintf(base, sizeof base, "romfs:/capa_%s", jogos[i].id);
            if (!romfs_ativo || !img_ler(base, &o)) {
                if (!ga_load_game_card_rgba(dir, i, jogos[i].tinta, &o.p, &o.w, &o.h))
                    continue;
            }
        }
        tem_capa[i] = img_cobrir(&o, CW1, CH1, &capa_g[i]) && img_cobrir(&o, CW0, CH0, &capa_p[i]);
        free(o.p);
    }
}

// ---------------------------------------------------------------- partes da tela
static void omega(int cx, int cy, int r, u32 cor, int a) {      // brasão: anel com Ω no meio
    for (int j = -r - 2; j <= r + 2; j++)
        for (int i = -r - 2; i <= r + 2; i++) {
            float d = sqrtf((float)(i * i + j * j)), c = 1.2f - fabsf(d - r);
            if (c > 0) mistura(cx + i, cy + j, cor, (int)(a * (c > 1 ? 1 : c)));
        }
    int w = largura(r + 6, "Ω", 0);
    texto(cx - w / 2, cy + (r + 6) * 7 / 20, r + 6, cor, a, "Ω");
}

static void bateria(int x, int y, int pct, int carregando) {   // ícone 26x13 + "87%"
    rborda(x, y, 26, 13, 3, 2, CINZA, 230);
    retangulo(x + 26, y + 4, 2, 5, CINZA, 230);
    int w = (22 * (pct < 0 ? 0 : pct > 100 ? 100 : pct) + 50) / 100;
    retangulo(x + 2, y + 2, w, 9, carregando ? VERDE : pct <= 15 ? VERMELHO : BRANCO, 230);
}

static int bat_pct = -1, bat_carga = 0;

static void cabecalho(void) {
    omega(68, 40, 17, OURO, 255);
    texto_e(100, 48, 20, OURO, 255, "GOD OF WAR", 4);
    int x = 100 + largura(20, "GOD OF WAR", 4) + 14;
    texto(x, 47, 20, OURO_ESC, 255, "·");
    texto_e(x + 18, 48, 20, CINZA, 235, "COLLECTION", 4);

    // direita: versão, relógio, bateria
    int xd = W - 56;
    if (bat_pct >= 0) {
        char b[8]; snprintf(b, sizeof b, "%d%%", bat_pct);
        xd -= texto_dir(xd, 47, 18, CINZA, 235, b) + 34;
        bateria(xd, 35, bat_pct, bat_carga);
        xd -= 24;
    }
    time_t t = time(NULL); struct tm *tm = localtime(&t);
    if (tm) {
        char c[8]; snprintf(c, sizeof c, "%02d:%02d", tm->tm_hour, tm->tm_min);
        xd -= texto_dir(xd, 48, 21, BRANCO, 245, c) + 24;
    }
    const char *ver = "v" VERSAO;
    int wv = largura(15, ver, 1);
    rborda(xd - wv - 20, 28, wv + 20, 26, 13, 1, OURO_ESC, 255);
    texto_e(xd - wv - 10, 46, 15, OURO_MED, 255, ver, 1);

    linha_h(48, 78, W - 96, OURO_ESC, 200);
}

// Card do jogo i em (x, y) com tamanho w x h. s = 0 (pequeno) .. 1 (selecionado). A borda dourada (que pulsa) é
// desenhada à parte em bordas(), por cima do quadro em cache.
typedef struct { int x, y, w, h; float s; } Ret;
static Ret cr[N];
static void card(int i, int x, int y, int w, int h, float s, int ultimo) {
    cr[i] = (Ret){ x, y, w, h, s };
    const Img *im = !tem_capa[i] ? NULL : (w == CW1 && h == CH1) ? &capa_g[i] : (w == CW0 && h == CH0) ? &capa_p[i] : &capa_g[i];
    const float r = 14, dim = .45f * (1 - s);
    const u32 tinta = jogos[i].tinta;
    // sombra
    rrect(x + 6, y + 12, w, h, 16, 0, 90);
    for (int j = 0; j < h; j++) {
        float t = (float)j / h, g = t < .42f ? 0 : t < .68f ? (t - .42f) / .26f * .55f : .55f + (t - .68f) / .32f * .41f;
        int canto = j < r || j >= h - r;
        const int sj = im ? j * im->h / h : 0;
        for (int k = 0; k < w; k++) {
            float c = canto && (k < r || k >= w - r) ? cobertura(k, j, w, h, r) : 1;
            if (c <= 0) continue;
            u32 v;
            if (im) v = im->p[sj * im->w + k * im->w / w] & 0xffffff;
            else {  // sem capa: degradê na cor do jogo
                float d = sqrtf((k - w * .5f) * (k - w * .5f) + (j - h * .35f) * (j - h * .35f)) / (h * .7f);
                v = mix(jogos[i].claro, tinta, d > 1 ? 1 : d);
            }
            v = mix(v, tinta, g);
            if (dim > 0) v = mix(v, 0, dim);
            mistura(x + k, y + j, v, (int)(255 * c));
        }
    }
    if (!im) {
        int tam = (int)(h * .36f), wo = largura(tam, "Ω", 0);
        texto(x + (w - wo) / 2, y + (int)(h * .5f), tam, mix(OURO, tinta, .25f + dim), 200, "Ω");
    }
    // título no pé do card
    int a = (int)(255 * (1 - dim));
    int tt = s > .5f ? 26 : 22;
    texto_e(x + 18, y + h - 52 - (tt - 22), 13, OURO, a, tr(jogos[i].capitulo), 3);
    negrito(x + 18, y + h - 22, tt, BRANCO, a, jogos[i].titulo, 1);
    if (s <= .02f) rborda(x, y, w, h, (int)r, 1, OURO_ESC, 160);
    if (ultimo) {
        const char *u = tr("launcher.card.last_played");
        int wu = largura(12, u, 2);
        rrect(x + 14, y + 14, wu + 20, 24, 12, OURO, (int)(235 * (1 - dim)));
        texto_e(x + 24, y + 31, 12, ESCURO, a, u, 2);
    }
    if (!info[i].nro) {
        const char *m = tr("launcher.card.nro_missing");
        rrect(x + 14, y + 46, largura(13, m, 0) + 20, 24, 12, VERMELHO, 220);
        texto(x + 24, y + 63, 13, BRANCO, 255, m);
    } else if (!info[i].dump) {
        const char *m = tr("launcher.card.game_missing");
        rrect(x + 14, y + 46, largura(13, m, 0) + 20, 24, 12, VERMELHO, 220);
        texto(x + 24, y + 63, 13, BRANCO, 255, m);
    }
}

static void bordas(float pulso) {
    for (int i = 0; i < N; i++) {
        const Ret *c = &cr[i];
        if (c->s <= .02f) continue;
        for (int g = 1; g <= 6; g++) rborda(c->x - g, c->y - g, c->w + 2 * g, c->h + 2 * g, 14 + g, 1, OURO, (int)(c->s * (40 + 30 * pulso) / g));
        rborda(c->x, c->y, c->w, c->h, 14, 3, mix(OURO_MED, OURO, pulso), (int)(255 * c->s));
    }
}

static int pill(int x, int y, const char *t, int ativo, int a) {   // selo arredondado; devolve a largura
    int w = largura(16, t, 0) + 30;
    rrect(x, y, w, 32, 16, ativo ? OURO : CINZA_ESC, ativo ? a * 30 / 255 : a * 25 / 255);
    rborda(x, y, w, 32, 16, 1, ativo ? OURO_MED : CINZA_ESC, a);
    texto(x + 15, y + 22, 16, ativo ? OURO : CINZA, a, t);
    return w;
}

static void linha_perfil(int x, int y, int w, const char *rotulo, const char *valor, u32 cor, int a) {
    texto(x + 22, y + 27, 19, CINZA, a, rotulo);
    int wv = largura(19, valor, 0);
    texto(x + w - 22 - wv, y + 27, 19, cor, a, valor);
}

static void desenhar(int sel, float cs[N], int ultimo, const char *msg, float fade) {
    for (int y = 0; y < H; y++) memcpy(&px[y * pitch], &bg[y * W], W * 4);
    cabecalho();

    // cards (o selecionado cresce)
    int ws[N], hs[N], xc = 72;
    for (int i = 0; i < N; i++) {
        ws[i] = (int)(CW0 + (CW1 - CW0) * cs[i] + .5f), hs[i] = (int)(CH0 + (CH1 - CH0) * cs[i] + .5f);
        if (cs[i] > .999f) ws[i] = CW1, hs[i] = CH1;
        if (cs[i] < .001f) ws[i] = CW0, hs[i] = CH0;
    }
    int xs[N];
    for (int i = 0, x = xc; i < N; i++) { xs[i] = x; x += ws[i] + 26; }
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < N; i++)
            if ((i == sel) == pass) card(i, xs[i], 386 - hs[i] / 2, ws[i], hs[i], cs[i], i == ultimo);

    // painel da direita
    const Jogo *j = &jogos[sel]; const Info *in = &info[sel];
    const int X = 640, PW = W - 56 - X, a = (int)(255 * fade);
    const int dx = (int)((1 - fade) * 24);
    texto_e(X + dx, 150, 15, OURO, a, tr(j->capitulo), 4);
    negrito(X + dx, 206, 50, BRANCO, a, j->titulo, 1);
    texto_sombra(X + dx, 240, 21, CINZA, a, tr(j->sub));

    char meta[160], dt[48] = "";
    if (in->tem_save && in->save > 0) {
        struct tm *tm = localtime(&in->save);
        if (tm) snprintf(dt, sizeof dt, tr("launcher.panel.save_date"), tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    }
    if (!*dt) snprintf(dt, sizeof dt, "%s", in->tem_save ? tr("launcher.panel.save_found") : tr("launcher.panel.no_save"));
    if (in->trof_total) snprintf(meta, sizeof meta, tr("launcher.panel.save_trophies"), dt, in->trof_ok, in->trof_total);
    else snprintf(meta, sizeof meta, "%s", dt);
    texto_sombra(X + dx, 278, 18, BRANCO, a * 200 / 255, meta);
    if (in->trof_total) {   // barrinha dos troféus
        retangulo(X + dx, 290, 280, 3, CINZA_ESC, a * 160 / 255);
        retangulo(X + dx, 290, 280 * in->trof_ok / in->trof_total, 3, OURO, a);
    }

    int bx = X + dx, by = 308;
    if (in->texpack) bx += pill(bx, by, opcoes[O_TEX].atual ? tr("launcher.panel.hd_textures") : tr("launcher.panel.hd_textures_off"), opcoes[O_TEX].atual, a) + 10;
    if (in->videos) { char v[32]; snprintf(v, sizeof v, tr("launcher.panel.hd_videos"), in->videos); bx += pill(bx, by, v, 1, a) + 10; }
    if (opcoes[O_FGEN].atual && sel == 0) bx += pill(bx, by, tr("launcher.panel.frame_generation"), 1, a) + 10;
    if (opcoes[O_VIDA].atual || opcoes[O_MAGIA].atual || opcoes[O_FURIA].atual) bx += pill(bx, by, tr("launcher.panel.cheats_on"), 0, a) + 10;

    // perfil rápido
    const char *lbl_perf = tr("launcher.preset.title");
    texto_e(X, 380, 14, CINZA, 235, lbl_perf, 3);
    int wl = largura(14, lbl_perf, 3);
    linha_h(X + wl + 14, 375, PW - wl - 14, OURO_ESC, 160);
    const int py = 392, rh = 42;
    rrect(X, py, PW, rh * 4 + 8, 14, PAINEL, 215);
    rborda(X, py, PW, rh * 4 + 8, 14, 1, RGB(70, 56, 40), 255);
    int pre = preset_atual();
    char pv[64]; snprintf(pv, sizeof pv, "‹  %s  ›", pre >= 0 ? tr(presets[pre].nome) : tr("launcher.preset.custom"));
    {   // linha do preset: rótulo + L/R
        int y = py + 4;
        retangulo(X + 1, y, PW - 2, rh, OURO, 18);
        int xl = X + 22 + texto(X + 22, y + 27, 19, BRANCO, 255, "Preset") + 10;
        texto(xl, y + 29, 22, CINZA, 230, BT_L);
        texto(xl + 26, y + 29, 22, CINZA, 230, BT_R);
        int wv = largura(19, pv, 0);
        texto(X + PW - 22 - wv, y + 27, 19, OURO, 255, pv);
    }
    for (int k = 1; k < 4; k++) retangulo(X + 18, py + 4 + rh * k, PW - 36, 1, RGB(62, 50, 38), 255);
    const char *res = op_rot(&opcoes[O_RES], opcoes[O_RES].atual);
    char rs[96];
    if (opcoes[O_RESD].atual) snprintf(rs, sizeof rs, "%s  ·  TV %s", res, op_rot(&opcoes[O_RESD], opcoes[O_RESD].atual));
    else snprintf(rs, sizeof rs, "%s", res);
    linha_perfil(X, py + 4 + rh, PW, tr("launcher.preset.resolution"), rs, BRANCO, 255);
    char fq[64];
    snprintf(fq, sizeof fq, "%s%s", opcoes[O_FPS].atual == 2 ? tr("launcher.preset.fps_max") : opcoes[O_FPS].atual ? tr("launcher.preset.fps_60") : tr("launcher.preset.fps_30"),
             sel == 0 && opcoes[O_FGEN].atual ? tr("launcher.preset.with_generation") : "");
    linha_perfil(X, py + 4 + rh * 2, PW, tr("launcher.preset.frame_rate"), fq, BRANCO, 255);
    linha_perfil(X, py + 4 + rh * 3, PW, tr("launcher.preset.renderer"), op_rot(&opcoes[O_REND], opcoes[O_REND].atual), BRANCO, 255);

    // botões
    const int yb = 600;
    int pode = in->nro && in->dump;
    const char *lbl_jogar = tr("launcher.menu.play");
    const char *lbl_opc = tr("launcher.menu.options");
    rrect(X, yb, 230, 54, 27, pode ? OURO : CINZA_ESC, 255);
    int wj = largura(24, lbl_jogar, 0) + 34;
    texto(X + (230 - wj) / 2, yb + 38, 28, ESCURO, 255, BT_A);
    negrito(X + (230 - wj) / 2 + 34, yb + 36, 24, ESCURO, 255, lbl_jogar, 0);
    rrect(X + 246, yb, 200, 54, 27, PAINEL, 200);
    rborda(X + 246, yb, 200, 54, 27, 1, OURO_ESC, 255);
    int wo = largura(21, lbl_opc, 0) + 32;
    texto(X + 246 + (200 - wo) / 2, yb + 37, 26, BRANCO, 255, BT_Y);
    texto(X + 246 + (200 - wo) / 2 + 32, yb + 35, 21, BRANCO, 245, lbl_opc);
    if (msg) texto_sombra(X, 584, 17, VERMELHO, 255, msg);

    // rodapé
    linha_h(48, 676, W - 96, OURO_ESC, 140);
    int x = 56, yr = 706;
    x = dica(x, yr, BT_A, lbl_jogar);
    x = dica(x, yr, BT_Y, lbl_opc);
    x = dica(x, yr, "‹ ›", tr("launcher.menu.game"));
    x = dica(x, yr, BT_L BT_R, "Preset");
    x = dica(x, yr, BT_X, tr("launcher.menu.trophies"));
#ifdef ENABLE_UPDATER
    {
        char u[80]; snprintf(u, sizeof u, upd_tag[0] ? tr("launcher.menu.update_to") : tr("launcher.menu.update"), upd_tag);
        if (upd_tag[0]) rrect(x - 10, yr - 26, largura(19, u, 0) + 60, 36, 18, OURO, 45);
        x = dica(x, yr, BT_MENOS, u);
    }
#endif
    dica(x, yr, BT_MAIS, tr("launcher.menu.exit"));
    assinatura(yr);
}

// Tela de opções: abas (L/R) com as opções do menu do jogo.
static int itens_da_aba(int aba, int *lista) {
    int n = 0;
    for (int i = 0; i < NOP; i++) if (opcoes[i].aba == aba) lista[n++] = i;
    return n;
}

static void desenhar_opcoes(int aba, int sel, const char *msg) {
    for (int y = 0; y < H; y++) memcpy(&px[y * pitch], &bg[y * W], W * 4);
    retangulo(0, 80, W, 596, FUNDO, 120);
    cabecalho();

    // abas
    const int X = 96, PW = W - 2 * X;
    int x = X + 40;
    texto(X, 140, 26, CINZA, 230, BT_L);
    for (int a = 0; a < NABAS; a++) {
        int w = largura(21, ABA(a), 0);
        if (a == aba) {
            rrect(x - 18, 108, w + 36, 44, 22, OURO, 40);
            rborda(x - 18, 108, w + 36, 44, 22, 1, OURO_MED, 255);
            negrito(x, 138, 21, OURO, 255, ABA(a), 0);
        } else texto(x, 138, 21, CINZA, 220, ABA(a));
        x += w + 60;
    }
    texto(x - 14, 140, 26, CINZA, 230, BT_R);
    const char *aviso = tr("options.note_both_games");
    texto_dir(W - X, 138, 16, CINZA_ESC, 255, aviso);

    // lista
    int lista[NOP], n = itens_da_aba(aba, lista);
    const int y0 = 172, rh = 44;
    rrect(X, y0 - 6, PW, n * rh + 12, 16, PAINEL, 200);
    rborda(X, y0 - 6, PW, n * rh + 12, 16, 1, RGB(70, 56, 40), 255);
    for (int k = 0; k < n; k++) {
        const Opcao *o = &opcoes[lista[k]];
        int y = y0 + k * rh, s = k == sel;
        if (s) {
            rrect(X + 8, y + 2, PW - 16, rh - 4, 12, OURO, 34);
            rrect(X + 8, y + 10, 4, rh - 20, 2, OURO, 255);
        } else if (k) retangulo(X + 24, y, PW - 48, 1, RGB(56, 46, 36), 255);
        texto(X + 30, y + 29, 20, s ? BRANCO : CINZA, 255, op_nome(o));
        if (lista[k] == O_FGEN || lista[k] == O_FLOW) texto(X + 34 + largura(20, op_nome(o), 0), y + 28, 14, OURO_ESC, 255, tr("options.gow1_only"));
        const char *v = op_rot(o, o->atual);
        int xd = X + PW - 30;
        if (s) {
            int wv = largura(20, v, 0);
            texto(xd - 12, y + 29, 20, OURO, 255, "›");
            texto(xd - 30 - wv, y + 29, 20, BRANCO, 255, v);
            texto(xd - 52 - wv, y + 29, 20, OURO, 255, "‹");
            xd -= 66 + wv;
        } else {
            int wv = largura(20, v, 0);
            texto(xd - wv, y + 29, 20, o->atual != o->padrao ? OURO_MED : CINZA, 235, v);
            xd -= wv + 16;
        }
        if (o->barra) {     // barrinha de nível
            const int bw = 150, bx = xd - bw - 10;
            retangulo(bx, y + 21, bw, 4, CINZA_ESC, 200);
            retangulo(bx, y + 21, bw * o->atual / (o->n - 1), 4, s ? OURO : OURO_ESC, 255);
            if (o->padrao && o->padrao < o->n - 1) retangulo(bx + bw * o->padrao / (o->n - 1), y + 17, 2, 12, CINZA, 200);
        }
    }

    // descrição da opção escolhida
    if (n) {
        const Opcao *o = &opcoes[lista[sel]];
        const int yd = y0 + n * rh + 22;
        texto(X + 6, yd + 18, 17, CINZA, 235, op_desc(o));
        if (o->atual != o->padrao) {
            char p[96]; snprintf(p, sizeof p, tr("options.default_value"), op_rot(o, o->padrao));
            texto(X + 6, yd + 44, 15, CINZA_ESC, 255, p);
        }
    }
    if (msg) texto_sombra(X, 660, 18, VERMELHO, 255, msg);

    linha_h(48, 676, W - 96, OURO_ESC, 140);
    x = 56; int yr = 706;
    x = dica(x, yr, BT_A, tr("options.btn.change"));
    x = dica(x, yr, BT_L BT_R, tr("options.btn.tab"));
    x = dica(x, yr, BT_X, tr("options.btn.default"));
    dica(x, yr, BT_B, tr("options.btn.save_back"));
    assinatura(yr);
}

// ---------------------------------------------------------------- troféus (visualizador do PR #13: trophies.c)
// Lê o TROPHY.TRP do jogo (nomes, descrições e ícones no idioma ativo) e o data/trophy/<npwr>/unlocked.txt gravado pelo vitart.
static TrophySet trof[N];
static int trof_lang[N] = { -1, -1 };
static const TrophySet *trof_do(int g) {
    if (trof_lang[g] != idioma) {
        static const char *const kTrpLang[NUI_LANG] = { "pt-BR", "en", "es" };
        trp_free(&trof[g]);
        trof_lang[g] = idioma;
        trp_load(&trof[g], dir, jogos[g].npwr, kTrpLang[idioma]);
    }
    return &trof[g];
}
static const u32 COR_GRAU[4] = { RGB(150, 200, 235), RGB(229, 193, 88), RGB(196, 200, 208), RGB(196, 128, 72) };
static int grau_i(char g) { return g == 'P' ? 0 : g == 'G' ? 1 : g == 'S' ? 2 : 3; }

// texto cortado em `maxw` pixels (com reticências)
static void texto_corte(int x, int y, int tam, u32 cor, int a, const char *s, int maxw) {
    if (largura(tam, s, 0) <= maxw) { texto(x, y, tam, cor, a, s); return; }
    char b[400]; snprintf(b, sizeof b, "%s", s);
    size_t n = strlen(b);
    while (n > 0) {
        do n--; while (n > 0 && ((unsigned char)b[n] & 0xc0) == 0x80);   // não corta no meio de um caractere UTF-8
        strcpy(b + n, "…");
        if (largura(tam, b, 0) <= maxw) break;
    }
    texto(x, y, tam, cor, a, b);
}

static void icone_trofeu(int x, int y, const TrophyItem *t) {
    const int S = TRP_ICON_SZ;
    if (!t->icon) { rrect(x, y, S, S, 8, RGB(48, 40, 32), 255); return; }
    for (int j = 0; j < S; j++)
        for (int i = 0; i < S; i++) {
            u32 v = t->icon[j * S + i];
            u32 r = v & 0xff, g = v >> 8 & 0xff, b = v >> 16 & 0xff;
            if (!t->unlocked) { u32 l = (r * 77 + g * 151 + b * 28) >> 8; r = g = b = l * 2 / 5; }   // bloqueado: cinza escuro
            mistura(x + i, y + j, RGB(r, g, b), 255);
        }
    rborda(x - 2, y - 2, S + 4, S + 4, 6, 2, t->unlocked ? COR_GRAU[grau_i(t->grade)] : RGB(70, 60, 50), 255);
}

// filtro: 0 todos, 1 conquistados, 2 que faltam
static int trof_lista(const TrophySet *ts, int filtro, int *out) {
    int n = 0;
    for (int i = 0; i < ts->count; i++) {
        const TrophyItem *t = &ts->items[i];
        if (filtro == 1 && !t->unlocked) continue;
        if (filtro == 2 && t->unlocked) continue;
        out[n++] = i;
    }
    return n;
}

static void desenhar_trofeus(int g, int sel, int topo, int filtro, int spoiler) {
    for (int y = 0; y < H; y++) memcpy(&px[y * pitch], &bg[y * W], W * 4);
    retangulo(0, 80, W, 596, FUNDO, 150);
    cabecalho();
    const TrophySet *ts = trof_do(g);

    // jogo (L/R)
    int x = 96 + 40;
    texto(96, 140, 26, CINZA, 230, BT_L);
    for (int i = 0; i < N; i++) {
        int w = largura(21, jogos[i].titulo, 0);
        if (i == g) {
            rrect(x - 18, 108, w + 36, 44, 22, OURO, 40);
            rborda(x - 18, 108, w + 36, 44, 22, 1, OURO_MED, 255);
            negrito(x, 138, 21, OURO, 255, jogos[i].titulo, 0);
        } else texto(x, 138, 21, CINZA, 220, jogos[i].titulo);
        x += w + 60;
    }
    texto(x - 14, 140, 26, CINZA, 230, BT_R);
    static const char *const filtros[] = { "trophies.filter.all", "trophies.filter.earned", "trophies.filter.missing" };
    char ft[64]; snprintf(ft, sizeof ft, tr("trophies.showing"), tr(filtros[filtro]));
    texto_dir(W - 96, 138, 16, CINZA, 235, ft);

    if (!ts->loaded) {
        texto(96, 220, 22, BRANCO, 255, tr("trophies.not_found"));
        texto(96, 252, 17, CINZA, 235, tr("trophies.not_found_detail"));
    } else {
        // resumo à esquerda
        const int LX = 96, LW = 300, LY = 176;
        rrect(LX, LY, LW, 400, 16, PAINEL, 210);
        rborda(LX, LY, LW, 400, 16, 1, RGB(70, 56, 40), 255);
        char pc[16]; snprintf(pc, sizeof pc, "%d%%", trp_percent(ts));
        negrito(LX + 28, LY + 78, 56, OURO, 255, pc, 1);
        char qt[48]; snprintf(qt, sizeof qt, tr("trophies.count"), ts->unlocked_count, ts->count);
        texto(LX + 28, LY + 112, 18, BRANCO, 235, qt);
        retangulo(LX + 28, LY + 128, LW - 56, 4, CINZA_ESC, 200);
        retangulo(LX + 28, LY + 128, (LW - 56) * trp_percent(ts) / 100, 4, OURO, 255);
        static const char *const nomes[4] = { "trophies.grade.platinum", "trophies.grade.gold", "trophies.grade.silver",
                                              "trophies.grade.bronze" };
        for (int k = 0; k < 4; k++) {
            int y = LY + 178 + k * 48;
            for (int j = -7; j <= 7; j++)
                for (int i = -7; i <= 7; i++)
                    if (i * i + j * j <= 49) mistura(LX + 40 + i, y - 6 + j, COR_GRAU[k], 255);
            texto(LX + 60, y, 19, BRANCO, 240, tr(nomes[k]));
            char c[24]; snprintf(c, sizeof c, "%d / %d", ts->grade_have[k], ts->grade_total[k]);
            texto_dir(LX + LW - 28, y, 19, ts->grade_total[k] && ts->grade_have[k] == ts->grade_total[k] ? OURO : CINZA, 240, c);
        }
        char pt[48]; snprintf(pt, sizeof pt, tr("trophies.points"), ts->pts_have, ts->pts_total);
        texto(LX + 28, LY + 376, 15, CINZA_ESC, 255, pt);

        // lista à direita
        int lista[TRP_MAX_TROPHIES], n = trof_lista(ts, filtro, lista);
        const int RX = LX + LW + 24, RW = W - 96 - RX, RY = 176, rh = 78, vis = 6;
        rrect(RX, RY, RW, vis * rh + 4, 16, PAINEL, 210);
        rborda(RX, RY, RW, vis * rh + 4, 16, 1, RGB(70, 56, 40), 255);
        if (!n) texto(RX + 28, RY + 46, 19, CINZA, 235, filtro == 1 ? tr("trophies.none_earned") : tr("trophies.all_earned"));
        for (int k = topo; k < n && k < topo + vis; k++) {
            const TrophyItem *t = &ts->items[lista[k]];
            int y = RY + 2 + (k - topo) * rh, s = k == sel;
            if (s) { rrect(RX + 8, y + 4, RW - 16, rh - 8, 12, OURO, 34); rrect(RX + 8, y + 16, 4, rh - 32, 2, OURO, 255); }
            else if (k != topo) retangulo(RX + 24, y, RW - 48, 1, RGB(56, 46, 36), 255);
            icone_trofeu(RX + 26, y + (rh - TRP_ICON_SZ) / 2, t);
            const int tx = RX + 26 + TRP_ICON_SZ + 18, tw = RW - (tx - RX) - 110;
            int oculto = t->hidden && !t->unlocked && !spoiler;
            texto_corte(tx, y + 32, 19, t->unlocked ? BRANCO : CINZA, 255, oculto ? tr("trophies.hidden") : t->name, tw);
            texto_corte(tx, y + 58, 15, CINZA, t->unlocked ? 230 : 170,
                        oculto ? tr("trophies.hidden_hint") : t->detail, tw);
            static const char *const gn[4] = { "trophies.grade_caps.platinum", "trophies.grade_caps.gold",
                                               "trophies.grade_caps.silver", "trophies.grade_caps.bronze" };
            texto_dir(RX + RW - 28, y + 32, 13, COR_GRAU[grau_i(t->grade)], t->unlocked ? 255 : 150, tr(gn[grau_i(t->grade)]));
            if (t->unlocked) texto_dir(RX + RW - 28, y + 56, 14, VERDE, 255, "✓");
        }
        if (topo > 0) texto(RX + RW / 2, RY - 4, 16, CINZA, 230, "▲");
        if (topo + vis < n) texto(RX + RW / 2, RY + vis * rh + 22, 16, CINZA, 230, "▼");
    }

    linha_h(48, 676, W - 96, OURO_ESC, 140);
    x = 56; int yr = 706;
    x = dica(x, yr, BT_L BT_R, tr("launcher.menu.game"));
    x = dica(x, yr, BT_X, tr("trophies.btn.filter"));
    x = dica(x, yr, BT_Y, spoiler ? tr("trophies.btn.hide_hidden") : tr("trophies.btn.show_hidden"));
    dica(x, yr, BT_B, tr("common.back"));
    assinatura(yr);
}

// ---------------------------------------------------------------- atualizador (updater.c: GitHub Releases do projeto)
#ifdef ENABLE_UPDATER
static void mb(char *o, size_t n, uint64_t v) { snprintf(o, n, "%.1f MB", v / 1048576.0); }

static void desenhar_atualizar(const UpdStatus *st) {
    for (int y = 0; y < H; y++) memcpy(&px[y * pitch], &bg[y * W], W * 4);
    retangulo(0, 80, W, 596, FUNDO, 170);
    cabecalho();
    const int PW = 720, X = (W - PW) / 2, Y = 170, PH = 380;
    rrect(X, Y, PW, PH, 20, PAINEL, 235);
    rborda(X, Y, PW, PH, 20, 1, OURO_ESC, 255);
    retangulo(X + 40, Y, PW - 80, 2, OURO, 200);
    omega(X + 60, Y + 62, 17, OURO, 255);
    texto_e(X + 92, Y + 70, 18, OURO, 255, tr("updater.title"), 4);
    char instalado[48];
    snprintf(instalado, sizeof instalado, tr("updater.installed"), VERSAO);
    texto_dir(X + PW - 40, Y + 70, 17, CINZA, 235, instalado);
    linha_h(X + 30, Y + 98, PW - 60, OURO_ESC, 160);

    const char *l1 = "", *l2 = "";
    char b1[160] = "", b2[200] = "";
    int barra = -1;   // 0..1000
    switch (st->state) {
    case UPD_IDLE:
    case UPD_CHECKING: l1 = tr("updater.checking"); l2 = tr("updater.connecting"); break;
    case UPD_UP_TO_DATE:
        snprintf(b1, sizeof b1, tr("updater.up_to_date"), st->latest_tag[0] ? st->latest_tag : "v" VERSAO);
        l1 = b1; l2 = st->asset_url[0] ? tr("updater.reinstall_hint") : ""; break;
    case UPD_AVAILABLE: {
        char t[24]; mb(t, sizeof t, st->asset_size);
        snprintf(b1, sizeof b1, tr("updater.available"), st->latest_tag);
        snprintf(b2, sizeof b2, tr("updater.available_detail"), st->asset_name, t);
        l1 = b1; l2 = b2; break;
    }
    case UPD_DOWNLOADING: {
        char a[24], t[24]; mb(a, sizeof a, st->dl_now); mb(t, sizeof t, st->dl_total ? st->dl_total : st->asset_size);
        snprintf(b1, sizeof b1, tr("updater.downloading"), st->latest_tag);
        snprintf(b2, sizeof b2, tr("updater.progress"), a, t);
        uint64_t tot = st->dl_total ? st->dl_total : st->asset_size;
        barra = tot ? (int)(st->dl_now * 1000 / tot) : 0;
        l1 = b1; l2 = b2; break;
    }
    case UPD_EXTRACTING:
        snprintf(b1, sizeof b1, tr("updater.installing"), st->files_done, st->files_total);
        snprintf(b2, sizeof b2, "%s", st->current_file);
        barra = st->files_total ? st->files_done * 1000 / st->files_total : 0;
        l1 = b1; l2 = b2; break;
    case UPD_DONE:
        snprintf(b1, sizeof b1, tr("updater.done"), st->latest_tag);
        l1 = b1; l2 = tr("updater.reopen_hint"); break;
    case UPD_ERROR: {
        const char *const e[] = {
            tr("updater.err.unknown"),
            tr("updater.err.no_internet"),
            tr("updater.err.refused"),
            tr("updater.err.no_asset"),
            tr("updater.err.sd_write"),
            tr("updater.err.corrupt"),
            tr("updater.err.canceled"),
        };
        l1 = st->err < (int)(sizeof e / sizeof e[0]) ? e[st->err] : e[0];
        if (st->err == UPD_ERR_HTTP) { snprintf(b2, sizeof b2, tr("updater.err.http"), st->http_code); l2 = b2; }
        break;
    }
    }
    negrito(X + 40, Y + 160, 26, st->state == UPD_ERROR ? VERMELHO : BRANCO, 255, l1, 1);
    texto_corte(X + 40, Y + 200, 18, CINZA, 235, l2, PW - 80);
    if (barra >= 0) {
        rrect(X + 40, Y + 236, PW - 80, 14, 7, RGB(50, 40, 32), 255);
        int w = (PW - 80) * (barra > 1000 ? 1000 : barra) / 1000;
        if (w > 14) rrect(X + 40, Y + 236, w, 14, 7, OURO, 255);
    }
    if (st->state == UPD_DOWNLOADING || st->state == UPD_EXTRACTING)
        texto(X + 40, Y + 300, 15, CINZA_ESC, 255, tr("updater.warn_power"));
    else if (st->state == UPD_AVAILABLE)
        texto(X + 40, Y + 300, 15, CINZA_ESC, 255, tr("updater.keeps_data"));

    linha_h(48, 676, W - 96, OURO_ESC, 140);
    int x = 56, yr = 706;
    if (st->state == UPD_AVAILABLE) x = dica(x, yr, BT_A, tr("launcher.menu.update"));
    if (st->state == UPD_DONE) x = dica(x, yr, BT_A, tr("updater.btn.reopen"));
    if (st->state == UPD_ERROR) x = dica(x, yr, BT_Y, tr("updater.btn.retry"));
    if (st->state != UPD_DONE) dica(x, yr, BT_B, st->state == UPD_DOWNLOADING ? tr("updater.btn.cancel") : tr("common.back"));
    assinatura(yr);
}
#endif

static void carregar_fundo(void) {
    bg = malloc(W * H * 4);
    if (!bg) return;
    if (!ga_load_cover_rgba(dir, romfs_ativo, bg, W, H)) {
        for (int i = 0; i < W * H; i++) bg[i] = FUNDO;
    }
    // arte bem escurecida e puxada para o marrom do layout, com vinheta
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float dx = (x - W * .5f) / (W * .62f), dy = (y - H * .45f) / (H * .75f), d = dx * dx + dy * dy;
            float k = .80f + .16f * (d > 1 ? 1 : d);
            if (x < 600) k += .06f * (1 - x / 600.f);    // mais escuro atrás dos cards
            bg[y * W + x] = mix(bg[y * W + x], FUNDO, k > .98f ? .98f : k) | 0xff000000u;
        }
}

// Copia o quadro em cache para a tela; com `pulso`, desenha por cima a borda dourada do card. Sem pulso e fora das
// opções, escurece (ao lançar o jogo).
static u32 *quadro;
static int escurecer = 0;
static void apresentar(Framebuffer *fb, const float *pulso) {
    u32 st; u32 *d = framebufferBegin(fb, &st); st /= 4;
    for (int y = 0; y < H; y++) memcpy(&d[y * st], &quadro[y * W], W * 4);
    px = d, pitch = st;
    if (pulso) bordas(*pulso);
    if (escurecer) retangulo(0, 0, W, H, 0, 140);
    framebufferEnd(fb);
    px = quadro, pitch = W;
}

int main(int argc, char **argv) {
    // Pasta do launcher (onde também ficam gow1.nro / gow2.nro), tirada do argv[0].
    if (argc > 0 && argv[0] && strchr(argv[0], '/')) {
        const char *a = argv[0];
        if (strncmp(a, "sdmc:", 5) != 0 && a[0] == '/') snprintf(dir, sizeof dir, "sdmc:%.700s", a);
        else snprintf(dir, sizeof dir, "%.700s", a);
        snprintf(self_nro, sizeof self_nro, "%.700s", dir);
        char *b = strrchr(dir, '/'); if (b) b[1] = 0;
    }
    romfs_ativo = R_SUCCEEDED(romfsInit());
    carregar_fundo();

    plInitialize(PlServiceType_User);
    psmInitialize();
    FT_Init_FreeType(&ft);
    PlFontData fd;
    if (R_SUCCEEDED(plGetSharedFontByType(&fd, PlSharedFontType_Standard)))
        FT_New_Memory_Face(ft, fd.address, fd.size, 0, &f_txt);
    if (R_SUCCEEDED(plGetSharedFontByType(&fd, PlSharedFontType_NintendoExt)))
        FT_New_Memory_Face(ft, fd.address, fd.size, 0, &f_ext);

    Framebuffer fb;
    framebufferCreate(&fb, nwindowGetDefault(), W, H, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad; padInitializeDefault(&pad);

    // Textos da interface: embutidos + data/lang/<código>.ini do SD. Tudo fica na RAM, então a tela de
    // atualização continua funcionando depois do romfsExit().
    char lang_dir[FS_MAX_PATH];
    snprintf(lang_dir, sizeof lang_dir, "%.700sdata/lang", dir);
    i18n_init(lang_dir);
    idioma_sistema = detectar_idioma_switch();
    escanear_traducoes();
    opcoes_ler();
    sincronizar_idioma_auto();
    musica_iniciar();
    info_ler();
    capas_carregar();

    const int ultimo = ler_ultimo();
    int sel = ultimo, lancar = 0, tela_op = 0, aba = 0, sel_op = 0, rep = 0, mudou = 0, sujo = 1;
    quadro = malloc(W * H * 4);
    float cs[N] = { 0 }, fade = 1;
    cs[sel] = 1;
    const char *msg_hb = envHasNextLoad() ? NULL : tr("launcher.msg.open_from_hbmenu");
    const char *msg = msg_hb;
    u64 tick = 0;
    int tela_trof = 0, t_jogo = 0, t_sel = 0, t_topo = 0, t_filtro = 0, t_spoiler = 0;
#ifdef ENABLE_UPDATER
    int tela_upd = 0, upd_auto = 1, upd_ultimo = -1;
    upd_start_check(VERSAO);          // checagem silenciosa ao abrir; o aviso aparece no rodapé
#endif
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 k = padGetButtonsDown(&pad), seg = padGetButtons(&pad);
        const u64 dirs = HidNpadButton_AnyUp | HidNpadButton_AnyDown | HidNpadButton_AnyLeft | HidNpadButton_AnyRight;
        if (seg & dirs) { if (++rep > 22 && rep % 5 == 0) k |= seg & dirs; } else rep = 0;
        if (tick % 120 == 0) {
            u32 p = 0; PsmChargerType ct = PsmChargerType_Unconnected;
            if (R_SUCCEEDED(psmGetBatteryChargePercentage(&p))) bat_pct = (int)p;
            if (R_SUCCEEDED(psmGetChargerType(&ct))) bat_carga = ct != PsmChargerType_Unconnected;
        }

#ifdef ENABLE_UPDATER
        UpdStatus st; upd_get_status(&st);
        if (upd_auto && st.state != UPD_CHECKING && st.state != UPD_IDLE) {
            upd_auto = 0;
            if (st.state == UPD_AVAILABLE) { snprintf(upd_tag, sizeof upd_tag, "%s", st.latest_tag); sujo = 1; }
        }
        if (tela_upd) {
            if (st.state == UPD_DONE) {
                if (k & HidNpadButton_A) {          // reabre o launcher novo
                    upd_shutdown();
                    char args[FS_MAX_PATH + 4]; snprintf(args, sizeof args, "\"%.760s\"", self_nro);
                    if (envHasNextLoad() && R_SUCCEEDED(envSetNextLoad(self_nro, args))) break;
                    break;
                }
            } else if (st.state == UPD_AVAILABLE || (st.state == UPD_UP_TO_DATE && st.asset_url[0])) {
                if ((st.state == UPD_AVAILABLE && (k & HidNpadButton_A)) || (st.state == UPD_UP_TO_DATE && (k & HidNpadButton_X))) {
                    musica_parar();                  // solta o romfs:/ antes de trocar o próprio gow-nx.nro no SD
                    if (romfs_ativo) { romfsExit(); romfs_ativo = 0; }
                    upd_start_install(dir, self_nro);
                }
                else if (k & (HidNpadButton_B | HidNpadButton_Minus)) tela_upd = 0;
            } else if (st.state == UPD_CHECKING || st.state == UPD_DOWNLOADING || st.state == UPD_EXTRACTING) {
                if (st.state != UPD_EXTRACTING && (k & HidNpadButton_B)) upd_cancel(0);
            } else {
                if (k & HidNpadButton_Y) upd_start_check(VERSAO);
                else if (k & (HidNpadButton_B | HidNpadButton_Minus)) {
                    tela_upd = 0;
                    if (!romfs_ativo && R_SUCCEEDED(romfsInit())) romfs_ativo = 1;
                    musica_iniciar();
                }
            }
            int marca = (int)st.state * 100000 + (int)(st.dl_now >> 16) + st.files_done * 7;
            if (marca != upd_ultimo || k) sujo = 1, upd_ultimo = marca;
            if (sujo) { px = quadro, pitch = W; desenhar_atualizar(&st); sujo = 0; }
            tick++;
            apresentar(&fb, NULL);
            if (!tela_upd) sujo = 1;
            continue;
        }
#endif
        if (tela_trof) {
            const TrophySet *ts = trof_do(t_jogo);
            int lista[TRP_MAX_TROPHIES], n = ts->loaded ? trof_lista(ts, t_filtro, lista) : 0;
            if (k & HidNpadButton_AnyUp) t_sel = t_sel > 0 ? t_sel - 1 : 0;
            if (k & HidNpadButton_AnyDown) t_sel = t_sel < n - 1 ? t_sel + 1 : t_sel;
            if (k & (HidNpadButton_R | HidNpadButton_ZR)) t_jogo = (t_jogo + 1) % N, t_sel = t_topo = 0;
            if (k & (HidNpadButton_L | HidNpadButton_ZL)) t_jogo = (t_jogo + N - 1) % N, t_sel = t_topo = 0;
            if (k & HidNpadButton_X) t_filtro = (t_filtro + 1) % 3, t_sel = t_topo = 0;
            if (k & HidNpadButton_Y) t_spoiler ^= 1;
            if (k & (HidNpadButton_B | HidNpadButton_Plus)) tela_trof = 0;
            if (t_sel < t_topo) t_topo = t_sel;
            if (t_sel >= t_topo + 6) t_topo = t_sel - 5;
            if (k || tick % 60 == 0) sujo = 1;
            if (sujo && tela_trof) { px = quadro, pitch = W; desenhar_trofeus(t_jogo, t_sel, t_topo, t_filtro, t_spoiler); sujo = 0; }
            tick++;
            apresentar(&fb, NULL);
            if (!tela_trof) sujo = 1;
            continue;
        }

        if (tela_op) {
            int lista[NOP], n = itens_da_aba(aba, lista);
            Opcao *o = &opcoes[lista[sel_op]];
            int antes_mus = opcoes[O_MUSICA].atual;
            if (k & HidNpadButton_AnyUp) sel_op = (sel_op + n - 1) % n;
            if (k & HidNpadButton_AnyDown) sel_op = (sel_op + 1) % n;
            if (k & (HidNpadButton_AnyRight | HidNpadButton_A)) o->atual = (o->atual + 1) % o->n, mudou = 1;
            if (k & HidNpadButton_AnyLeft) o->atual = (o->atual + o->n - 1) % o->n, mudou = 1;
            if (k & HidNpadButton_X) o->atual = o->padrao, mudou = 1;
            if (opcoes[O_MUSICA].atual != antes_mus) {
                if (opcoes[O_MUSICA].atual) musica_iniciar();
                else musica_parar();
            }
            {
                int novo_ui = resolver_ui_idioma();
                if (idioma != novo_ui) {
                    const int era_hb = msg == msg_hb;
                    aplicar_idioma(novo_ui);
                    msg_hb = envHasNextLoad() ? NULL : tr("launcher.msg.open_from_hbmenu");
                    if (era_hb) msg = msg_hb;
                }
            }
            if (k & (HidNpadButton_R | HidNpadButton_ZR)) aba = (aba + 1) % NABAS, sel_op = 0;
            if (k & (HidNpadButton_L | HidNpadButton_ZL)) aba = (aba + NABAS - 1) % NABAS, sel_op = 0;
            if (k & (HidNpadButton_B | HidNpadButton_Plus)) {
                if (mudou && !opcoes_gravar()) msg = tr("launcher.msg.ini_write_failed");
                else msg = msg_hb, mudou = 0;
                tela_op = 0;
            }
            if (k || tick % 60 == 0) sujo = 1;
            if (sujo) { px = quadro, pitch = W; desenhar_opcoes(aba, sel_op, msg == msg_hb ? NULL : msg); sujo = 0; }
            tick++;
            apresentar(&fb, NULL);
            if (!tela_op) sujo = 1;
            continue;
        }

        if (k & HidNpadButton_Plus) break;
        int antes = sel;
        if (k & HidNpadButton_AnyRight) sel = (sel + 1) % N;
        if (k & HidNpadButton_AnyLeft) sel = (sel + N - 1) % N;
        if (sel != antes) fade = 0;
        if (k & (HidNpadButton_R | HidNpadButton_L)) {
            int p = preset_atual();
            p = (k & HidNpadButton_R) ? (p + 1) % NPRE : p <= 0 ? NPRE - 1 : p - 1;
            preset_aplicar(p); mudou = 1;
        }
        if (k & HidNpadButton_Y) { tela_op = 1; sel_op = 0; msg = msg_hb; sujo = 1; continue; }
        if (k & HidNpadButton_X) { tela_trof = 1; t_jogo = sel; t_sel = t_topo = 0; sujo = 1; continue; }
#ifdef ENABLE_UPDATER
        if (k & HidNpadButton_Minus) {
            if (st.state != UPD_AVAILABLE && st.state != UPD_CHECKING) upd_start_check(VERSAO);
            upd_auto = 0; tela_upd = 1; upd_ultimo = -1; sujo = 1; continue;
        }
#endif
        else if (k & HidNpadButton_A) {
            char p[FS_MAX_PATH]; caminho(p, sizeof p, sel);
            info[sel].nro = existe(p);
            info[sel].dump = ga_game_dump_ok(dir, sel);
            if (!info[sel].nro) msg = tr("launcher.msg.game_file_missing");
            else if (!info[sel].dump) msg = tr("launcher.msg.vita_files_missing");
            else if (envHasNextLoad()) {
                if ((mudou || opcoes[O_LANG].atual == 0) && !opcoes_gravar())
                    msg = tr("launcher.msg.ini_write_failed");
                char args[FS_MAX_PATH + 4]; snprintf(args, sizeof args, "\"%.760s\"", p);
                if (R_SUCCEEDED(envSetNextLoad(p, args))) { gravar_ultimo(sel); lancar = 1; }
                else msg = tr("launcher.msg.prepare_failed");
            }
        }
        for (int i = 0; i < N; i++) {
            float alvo = i == sel ? 1.f : 0.f;
            cs[i] += (alvo - cs[i]) * .25f;
            if (fabsf(alvo - cs[i]) < .004f) cs[i] = alvo;
        }
        if (fade < 1) { fade += (1 - fade) * .2f; if (fade > .995f) fade = 1; sujo = 1; }
        for (int i = 0; i < N; i++) if (cs[i] != (i == sel)) sujo = 1;
        if (k || tick % 60 == 0) sujo = 1;
        if (sujo) { px = quadro, pitch = W; desenhar(sel, cs, ultimo, msg, fade); sujo = 0; }
        float pulso = .5f + .5f * sinf(tick++ * .06f);
        if (lancar) escurecer = 1;
        apresentar(&fb, &pulso);
        if (lancar) break;
    }
    if (mudou && !lancar) opcoes_gravar();   // preset trocado e saiu com +

#ifdef ENABLE_UPDATER
    upd_shutdown();
#endif
    musica_parar();
    framebufferClose(&fb);
    if (f_txt) FT_Done_Face(f_txt);
    if (f_ext) FT_Done_Face(f_ext);
    FT_Done_FreeType(ft);
    psmExit();
    plExit();
    if (romfs_ativo) romfsExit();
    free(bg); free(quadro);
    for (int i = 0; i < N; i++) trp_free(&trof[i]);
    for (int i = 0; i < N; i++) { free(capa_g[i].p); free(capa_p[i].p); }
    return 0;
}
