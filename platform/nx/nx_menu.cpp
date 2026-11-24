// Mod menu do Switch: R + Plus abre/fecha; painel encostado à esquerda, abas com L/R. Desenho na CPU (fonte do sistema pelo FreeType), copiado pela apresentação
// para a imagem da swapchain (gxm_vk.cpp). As mudanças vão para game_options.cpp pela ponte nxopt.
#include "nx_menu.h"
#include "vitart.h"
#include "nx_platform.h"
#include "i18n.h"

#include <switch.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace nxmenu {
namespace {
std::atomic<bool> g_active{false}, g_block{false};
PadState g_pad;
bool g_pad_ok = false;
int g_sel = 0;
uint32_t g_val[nxopt::COUNT] = {};
bool g_dirty = true;
int g_hold = 0, g_hold_dir = 0;
int g_confirm = 0; // pop-up de confirmação: 1 = diagnóstico da GPU, 2 = sair do jogo, 3 = voltar ao launcher

// Idioma da interface: 0 português (BR), 1 inglês, 2 espanhol (detectado de vitart_options.ini / idioma do Switch)
enum { UI_PT = 0, UI_EN = 1, UI_ES = 2 };
int g_lang = UI_PT;
bool g_lang_loaded = false;

int parse_ui_lang(const char *v) {
    if (!v || !v[0] || !v[1]) return -1;
    const char a = (char)std::tolower((unsigned char)v[0]);
    const char b = (char)std::tolower((unsigned char)v[1]);
    const char c = v[2];
    if (c != 0 && c != '-' && c != '_') return -1;
    if (a == 'p' && b == 't') return UI_PT;
    if (a == 'e' && b == 'n') return UI_EN;
    if (a == 'e' && b == 's') return UI_ES;
    return -1;
}

int detect_switch_lang() {
    u64 code = 0;
    if (R_SUCCEEDED(setInitialize())) {
        if (R_FAILED(setGetSystemLanguage(&code))) code = 0;
        setExit();
    }
    char tag[9];
    std::memcpy(tag, &code, 8);
    tag[8] = 0;
    const int l = parse_ui_lang(tag);
    return l >= 0 ? l : UI_EN;
}

int load_ui_lang() {
    const int sys = detect_switch_lang();
    int from_launcher = -2, from_lang = -2; // -2 unset, -1 auto
    const std::string ini = nxp::root_dir() + "/data/vitart_options.ini";
    if (std::FILE *f = std::fopen(ini.c_str(), "r")) {
        char line[256];
        while (std::fgets(line, sizeof line, f)) {
            size_t len = std::strlen(line);
            while (len && (unsigned char)line[len - 1] <= 32) line[--len] = 0;
            if (!std::strncmp(line, "launcher_language=", 18)) {
                const char *v = line + 18;
                if (!std::strcmp(v, "auto")) from_launcher = -1;
                else {
                    const int p = parse_ui_lang(v);
                    if (p >= 0) from_launcher = p;
                }
            } else if (!std::strncmp(line, "language=", 9)) {
                const int p = parse_ui_lang(line + 9);
                if (p >= 0) from_lang = p;
            }
        }
        std::fclose(f);
    }
    if (from_launcher == -1) return sys;
    if (from_launcher >= 0) return from_launcher;
    if (from_lang >= 0) return from_lang;
    return sys;
}

// Códigos dos idiomas embutidos (lang/<código>.ini), na ordem de UI_PT/UI_EN/UI_ES.
const char *const kLangCode[3] = {"pt", "en", "es"};

// Abas do menu (o mesmo visual do launcher: Gráficos / Mods / Cheats / Sistema)
enum { T_GFX, T_MODS, T_CHEATS, T_SYS, kTabs };
const char *const kTabChave[kTabs] = {"menu.tab.graphics", "menu.tab.mods", "menu.tab.cheats", "menu.tab.system"};
const char *tab_nome(int t) { return tr(kTabChave[t]); }
// ações que não são opções do jogo
enum { A_CLOSE = -1, A_PRESET = -2, A_RESET = -3, A_LAUNCHER = -4, A_EXIT = -5 };

struct Item {
    int tab, id; // id: nxopt::Id, ou uma ação A_*
    const char *nome, *desc; // chaves de tr()
    const char *n() const { return tr(nome); }
    const char *d() const { return tr(desc); }
};
const Item kItems[] = {
    {T_GFX, A_PRESET, "menu.item.preset.name", "menu.item.preset.desc"},
    {T_GFX, nxopt::RES, "menu.item.res.name", "menu.item.res.desc"},
    {T_GFX, nxopt::RES_DOCK, "menu.item.res_dock.name", "menu.item.res_dock.desc"},
    {T_GFX, nxopt::UPSCALE, "menu.item.upscale.name", "menu.item.upscale.desc"},
    {T_GFX, nxopt::SHARP, "menu.item.sharp.name", "menu.item.sharp.desc"},
    {T_GFX, nxopt::AA, "menu.item.aa.name", "menu.item.aa.desc"},
    {T_GFX, nxopt::ANISO, "menu.item.aniso.name", "menu.item.aniso.desc"},
    {T_GFX, nxopt::GAMMA, "menu.item.gamma.name", "menu.item.gamma.desc"},
    {T_GFX, nxopt::SAT, "menu.item.sat.name", "menu.item.sat.desc"},
    {T_GFX, nxopt::VIB, "menu.item.vib.name", "menu.item.vib.desc"},
    {T_GFX, nxopt::DITHER, "menu.item.dither.name", "menu.item.dither.desc"},
    {T_GFX, nxopt::FGEN, "menu.item.fgen.name", "menu.item.fgen.desc"},
    {T_GFX, nxopt::FGFLOW, "menu.item.fgflow.name", "menu.item.fgflow.desc"},
    {T_MODS, nxopt::TEX, "menu.item.tex.name", "menu.item.tex.desc"},
    {T_MODS, nxopt::HPBARS, "menu.item.hpbars.name", "menu.item.hpbars.desc"},
    {T_MODS, nxopt::QTE, "menu.item.qte.name", "menu.item.qte.desc"},
    {T_CHEATS, nxopt::CH_HP, "menu.item.ch_hp.name", "menu.item.ch_hp.desc"},
    {T_CHEATS, nxopt::CH_MP, "menu.item.ch_mp.name", "menu.item.ch_mp.desc"},
    {T_CHEATS, nxopt::CH_RAGE, "menu.item.ch_rage.name", "menu.item.ch_rage.desc"},
    {T_SYS, nxopt::LANG, "menu.item.lang.name", "menu.item.lang.desc"},
    {T_SYS, nxopt::FPS, "menu.item.fps.name", "menu.item.fps.desc"},
    {T_SYS, nxopt::VSYNC, "menu.item.vsync.name", "menu.item.vsync.desc"},
    {T_SYS, nxopt::COUNTER, "menu.item.counter.name", "menu.item.counter.desc"},
    {T_SYS, nxopt::GPUDIAG, "menu.item.gpudiag.name", "menu.item.gpudiag.desc"},
    {T_SYS, A_RESET, "menu.item.reset.name", "menu.item.reset.desc"},
    {T_SYS, A_CLOSE, "menu.item.close.name", "menu.item.close.desc"},
    {T_SYS, A_LAUNCHER, "menu.item.launcher.name", "menu.item.launcher.desc"},
    {T_SYS, A_EXIT, "menu.item.exit.name", "menu.item.exit.desc"},
};
constexpr int kN = sizeof kItems / sizeof kItems[0];
int g_tab = 0;

// presets (os mesmos do launcher): resolução + taxa de quadros
struct Preset {
    const char *nome; // chave de tr()
    uint32_t res, fps;
};
const Preset kPresets[] = {
    {"menu.preset.performance", 6, 2},
    {"menu.preset.balanced", 5, 2},
    {"menu.preset.quality", 2, 1},
};
constexpr int kNPre = 3;
int preset_atual() {
    for (int i = 0; i < kNPre; i++)
        if (g_val[nxopt::RES] == kPresets[i].res && g_val[nxopt::FPS] == kPresets[i].fps) return i;
    return -1;
}

// resolução: ordem do menu -> código da opção (5 = 720p)
const uint32_t kRes[] = {1, 6, 5, 2, 3, 4};
const char *kResNome[] = {"1x (720x408)", "Vita (960x544)", "720p (1270x720)", "2x (1440x816)", "3x (2160x1224)", "4x (2880x1632)"};

int n_vals(int id) {
    switch (id) {
    case nxopt::RES: return 6;
    case nxopt::RES_DOCK: return 7;
    case nxopt::ANISO: return 5;
    case nxopt::GAMMA: return 9;
    case nxopt::SAT: return 9;
    case nxopt::VIB: return 5;
    case nxopt::SHARP: return 11;
    case nxopt::FPS: return 3;
    case nxopt::FGEN: return 3;
    case nxopt::FGFLOW: return 4;
    case nxopt::LANG: return 3;
    default: return 2;
    }
}
// liga/desliga (desenhado como chave)
bool is_toggle(int id) { return id >= 0 && id != nxopt::LANG && id != nxopt::AA && id != nxopt::UPSCALE && n_vals(id) == 2; }
// opção com barrinha de nível
bool is_level(int id) { return id == nxopt::SHARP || id == nxopt::GAMMA || id == nxopt::SAT || id == nxopt::VIB || id == nxopt::ANISO; }
// valor da opção -> posição na lista
int pos_of(int id, uint32_t v) {
    if (id == nxopt::RES) {
        for (int i = 0; i < 6; i++)
            if (kRes[i] == v) return i;
        return 0;
    }
    if (id == nxopt::RES_DOCK) {
        for (int i = 0; i < 6; i++)
            if (kRes[i] == v) return i + 1;
        return 0;
    }
    if (id == nxopt::ANISO) return v >= 16 ? 4 : v >= 8 ? 3 : v >= 4 ? 2 : v >= 2 ? 1 : 0;
    if (id == nxopt::FPS) return v >= 1 && v <= 3 ? (int)v - 1 : 0;
    if (id == nxopt::FGFLOW) return v >= 1 && v <= 4 ? (int)v - 1 : 1;
    return (int)v;
}
uint32_t val_of(int id, int p) {
    if (id == nxopt::RES) return kRes[p];
    if (id == nxopt::RES_DOCK) return p ? kRes[p - 1] : 0;
    if (id == nxopt::ANISO) return 1u << p;
    if (id == nxopt::FPS) return (uint32_t)p + 1;
    if (id == nxopt::FGFLOW) return (uint32_t)p + 1;
    return (uint32_t)p;
}
std::string val_text(int id, uint32_t v) {
    char b[48];
    switch (id) {
    case A_PRESET: {
        const int p = preset_atual();
        return p >= 0 ? tr(kPresets[p].nome) : tr("menu.preset.custom");
    }
    case nxopt::RES: return kResNome[pos_of(id, v)];
    case nxopt::RES_DOCK: return v ? kResNome[pos_of(id, v) - 1] : tr("menu.val.same_as_handheld");
    case nxopt::SHARP:
        if (!v) return "Off";
        std::snprintf(b, sizeof b, "%u", v);
        return b;
    case nxopt::UPSCALE: return v ? "FSR 1.0" : "Bilinear";
    case nxopt::AA: return v ? "FXAA" : "Off";
    case nxopt::ANISO:
        if (v <= 1) return "Off";
        std::snprintf(b, sizeof b, "%ux", v);
        return b;
    case nxopt::GAMMA:
    case nxopt::SAT:
        if (v == 4) return tr("menu.val.default");
        std::snprintf(b, sizeof b, "%+d", (int)v - 4);
        return b;
    case nxopt::VIB:
        if (!v) return "Off";
        std::snprintf(b, sizeof b, "%u", v);
        return b;
    case nxopt::LANG: return i18n_name(i18n_find(kLangCode[g_lang])); // "_name" do lang/<código>.ini
    case nxopt::FPS: return v == 3 ? tr("menu.val.max") : v == 2 ? "60" : "30";
    case nxopt::FGEN: return v == 2 ? tr("menu.val.quality") : v == 1 ? tr("menu.val.performance") : "Off";
    case nxopt::FGFLOW: return v == 1 ? tr("menu.val.flow_full") : v == 2 ? "1/2" : v == 3 ? "1/3" : tr("menu.val.flow_quarter");
    default: return v ? "ON" : "OFF";
    }
}
bool item_ok(int i) {
    const int id = kItems[i].id;
    if (id == A_LAUNCHER) return nxp::can_return_to_launcher();
    if (id < 0) return true;
    return nxopt::available(id);
}
// itens da aba atual (índices de kItems)
int tab_items(int *out) {
    int n = 0;
    for (int i = 0; i < kN; i++)
        if (kItems[i].tab == g_tab) out[n++] = i;
    return n;
}

void open() {
    for (int i = 0; i < nxopt::COUNT; i++) g_val[i] = nxopt::get(i);
    if (!g_lang_loaded) {
        // textos embutidos + data/lang/<código>.ini do SD (mesma pasta de idiomas do launcher)
        i18n_init((nxp::root_dir() + "/data/lang").c_str());
        g_lang = load_ui_lang();
        i18n_set(kLangCode[g_lang]);
        g_lang_loaded = true;
    }
    int l[kN];
    const int n = tab_items(l);
    if (g_sel >= n || !item_ok(l[g_sel])) g_sel = 0;
    g_hold = g_hold_dir = 0;
    g_active = true, g_block = true, g_dirty = true;
    vt::logf("nxmenu: aberto");
}
void close() {
    g_active = false, g_dirty = true;
    vt::logf("nxmenu: fechado");
}
void set_val(int id, uint32_t v) {
    if (v == g_val[id] || !nxopt::available(id)) return;
    g_val[id] = v;
    nxopt::set(id, v);
}
void change(int dir) {
    int l[kN];
    const int n = tab_items(l);
    if (g_sel >= n) return;
    const int id = kItems[l[g_sel]].id;
    if (id == A_CLOSE) {
        if (dir > 0) close();
        return;
    }
    if (id == A_LAUNCHER) {
        if (dir > 0 && nxp::can_return_to_launcher()) g_confirm = 3, g_dirty = true;
        return;
    }
    if (id == A_EXIT) {
        if (dir > 0) g_confirm = 2, g_dirty = true;
        return;
    }
    if (id == A_RESET) {
        if (dir <= 0) return;
        set_val(nxopt::UPSCALE, 0), set_val(nxopt::SHARP, 0), set_val(nxopt::AA, 0), set_val(nxopt::ANISO, 1);
        set_val(nxopt::GAMMA, 4), set_val(nxopt::SAT, 4), set_val(nxopt::VIB, 0), set_val(nxopt::DITHER, 0);
        g_dirty = true;
        return;
    }
    if (id == A_PRESET) {
        int p = preset_atual();
        p = p < 0 ? (dir > 0 ? 0 : kNPre - 1) : (p + dir + kNPre) % kNPre;
        set_val(nxopt::RES, kPresets[p].res), set_val(nxopt::FPS, kPresets[p].fps);
        g_dirty = true;
        return;
    }
    if (id == nxopt::LANG) {
        g_lang = (g_lang + dir + 3) % 3;
        i18n_set(kLangCode[g_lang]);
        g_val[nxopt::LANG] = (uint32_t)g_lang;
        nxopt::set(nxopt::LANG, (uint32_t)g_lang);
        g_dirty = true;
        return;
    }
    const int nv = n_vals(id);
    int p = pos_of(id, g_val[id]) + dir;
    if (is_level(id)) p = p < 0 ? 0 : p >= nv ? nv - 1 : p;
    else p = (p % nv + nv) % nv;
    const uint32_t v = val_of(id, p);
    if (v == g_val[id]) return;
    if (id == nxopt::GPUDIAG && v) { g_confirm = 1, g_dirty = true; return; } // pede confirmação primeiro
    g_val[id] = v;
    nxopt::set(id, v);
    g_dirty = true;
}
void move(int dir) {
    int l[kN];
    const int n = tab_items(l);
    for (int k = 0; k < n; k++) {
        g_sel = (g_sel + dir + n) % n;
        if (item_ok(l[g_sel])) break;
    }
    g_dirty = true;
}
void tab(int dir) {
    g_tab = (g_tab + dir + kTabs) % kTabs, g_sel = 0;
    int l[kN];
    const int n = tab_items(l);
    if (n && !item_ok(l[0])) move(1);
    g_dirty = true;
}
} // namespace

bool active() { return g_active; }
bool input_blocked() { return g_block; }

void update() {
    if (!g_pad_ok) {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&g_pad);
        g_pad_ok = true;
    }
    padUpdate(&g_pad);
    const u64 held = padGetButtons(&g_pad), down = padGetButtonsDown(&g_pad);
    if (!g_active) {
        if ((held & HidNpadButton_R) && (down & HidNpadButton_Plus)) open();
        else if (g_block && !(held & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_Plus | HidNpadButton_R | HidNpadButton_AnyUp |
                                      HidNpadButton_AnyDown | HidNpadButton_AnyLeft | HidNpadButton_AnyRight)))
            g_block = false; // só devolve o controle ao jogo com os botões do menu soltos
        return;
    }
    if (g_confirm) {
        if (down & HidNpadButton_A) {
            if (g_confirm == 2 || g_confirm == 3) {
                const int act = g_confirm;
                g_confirm = 0;
                close();
                if (act == 3) {
                    vt::logf("nxmenu: voltar ao launcher");
                    nxp::request_return_to_launcher();
                } else {
                    vt::logf("nxmenu: sair do jogo");
                    nxp::request_exit();
                }
                return;
            }
            g_confirm = 0, g_val[nxopt::GPUDIAG] = 1, g_dirty = true;
            nxopt::set(nxopt::GPUDIAG, 1);
        } else if (down & (HidNpadButton_B | HidNpadButton_Plus)) g_confirm = 0, g_dirty = true;
        return;
    }
    if (down & (HidNpadButton_B | HidNpadButton_Plus)) { close(); return; }
    if (down & HidNpadButton_A) change(+1);
    if (!g_active) return;
    if (down & (HidNpadButton_R | HidNpadButton_ZR)) tab(+1);
    if (down & (HidNpadButton_L | HidNpadButton_ZL)) tab(-1);
    if (down & HidNpadButton_AnyLeft) change(-1);
    if (down & HidNpadButton_AnyRight) change(+1);
    const int dir = (held & HidNpadButton_AnyUp) ? -1 : (held & HidNpadButton_AnyDown) ? 1 : 0;
    if (dir != g_hold_dir) g_hold_dir = dir, g_hold = 0;
    if (dir) {
        if (g_hold == 0 || (g_hold >= 18 && g_hold % 4 == 0)) move(dir);
        g_hold++;
    }
}

// ---------------------------------------------------------------- desenho
namespace {
FT_Library g_ft = nullptr;
FT_Face g_face = nullptr;
bool g_ft_tried = false;
std::vector<uint32_t> g_px;
uint32_t g_w = 0, g_h = 0, g_for_h = 0;

bool font() {
    if (g_face) return true;
    if (g_ft_tried) return false;
    g_ft_tried = true;
    if (R_FAILED(plInitialize(PlServiceType_User))) return false;
    PlFontData fd;
    if (R_FAILED(plGetSharedFontByType(&fd, PlSharedFontType_Standard))) return false;
    if (FT_Init_FreeType(&g_ft)) return false;
    if (FT_New_Memory_Face(g_ft, (const FT_Byte *)fd.address, fd.size, 0, &g_face)) { g_face = nullptr; return false; }
    return true;
}

uint32_t utf8(const char *&s) {
    const uint8_t c = (uint8_t)*s++;
    if (c < 0x80) return c;
    int n = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : 1;
    uint32_t cp = c & (0x3f >> n);
    while (n-- && (*s & 0xc0) == 0x80) cp = cp << 6 | ((uint8_t)*s++ & 0x3f);
    return cp;
}

void rect(int x, int y, int w, int h, uint32_t c) {
    for (int j = y < 0 ? 0 : y; j < y + h && j < (int)g_h; j++)
        for (int i = x < 0 ? 0 : x; i < x + w && i < (int)g_w; i++) g_px[(size_t)j * g_w + i] = c;
}

uint32_t mix(uint32_t d, uint32_t c, uint32_t a) {
    uint32_t r = 0xff000000u;
    for (int sh = 0; sh < 24; sh += 8) {
        const uint32_t dc = d >> sh & 0xff, cc = c >> sh & 0xff;
        r |= ((dc * (255 - a) + cc * a) / 255) << sh;
    }
    return r;
}

// largura do texto em pixels
int text_w(const char *s, int size) {
    if (!font()) return (int)std::strlen(s) * size / 2;
    FT_Set_Pixel_Sizes(g_face, 0, size);
    int w = 0;
    while (*s) {
        const uint32_t cp = utf8(s);
        if (!FT_Load_Char(g_face, cp, FT_LOAD_DEFAULT)) w += g_face->glyph->advance.x >> 6;
    }
    return w;
}

// y = linha de base
void text(int x, int y, const char *s, int size, uint32_t c) {
    if (!font()) return;
    FT_Set_Pixel_Sizes(g_face, 0, size);
    while (*s) {
        const uint32_t cp = utf8(s);
        if (FT_Load_Char(g_face, cp, FT_LOAD_RENDER)) continue;
        const FT_GlyphSlot g = g_face->glyph;
        const FT_Bitmap &bm = g->bitmap;
        for (unsigned j = 0; j < bm.rows; j++) {
            const int py = y - g->bitmap_top + (int)j;
            if (py < 0 || py >= (int)g_h) continue;
            for (unsigned i = 0; i < bm.width; i++) {
                const int px = x + g->bitmap_left + (int)i;
                if (px < 0 || px >= (int)g_w) continue;
                const uint32_t a = bm.buffer[j * bm.pitch + i];
                if (a) {
                    uint32_t &d = g_px[(size_t)py * g_w + px];
                    d = mix(d, c, a);
                }
            }
        }
        x += g->advance.x >> 6;
    }
}

// retângulo com cantos arredondados (raio r), misturado com alfa a
void rrect(int x, int y, int w, int h, int r, uint32_t c, uint32_t a) {
    for (int j = 0; j < h; j++) {
        const int py = y + j;
        if (py < 0 || py >= (int)g_h) continue;
        for (int i = 0; i < w; i++) {
            const int px = x + i;
            if (px < 0 || px >= (int)g_w) continue;
            const int dx = i < r ? r - i : i >= w - r ? i - (w - r - 1) : 0, dy = j < r ? r - j : j >= h - r ? j - (h - r - 1) : 0;
            if (dx * dx + dy * dy > r * r) continue;
            uint32_t &d = g_px[(size_t)py * g_w + px];
            d = mix(d, c, a);
        }
    }
}
void blend(int x, int y, int w, int h, uint32_t c, uint32_t a) { rrect(x, y, w, h, 0, c, a); }
void disc(int cx, int cy, int r, uint32_t c) {
    for (int j = -r; j <= r; j++)
        for (int i = -r; i <= r; i++) {
            const float d = std::sqrt((float)(i * i + j * j));
            const float k = r + 0.5f - d;
            if (k <= 0) continue;
            const int px = cx + i, py = cy + j;
            if (px < 0 || py < 0 || px >= (int)g_w || py >= (int)g_h) continue;
            uint32_t &p = g_px[(size_t)py * g_w + px];
            p = mix(p, c, (uint32_t)(255 * (k > 1 ? 1 : k)));
        }
}
void ring(int cx, int cy, int r, uint32_t c) {
    for (int j = -r - 2; j <= r + 2; j++)
        for (int i = -r - 2; i <= r + 2; i++) {
            const float d = std::sqrt((float)(i * i + j * j)), k = 1.2f - std::fabs(d - r);
            if (k <= 0) continue;
            const int px = cx + i, py = cy + j;
            if (px < 0 || py < 0 || px >= (int)g_w || py >= (int)g_h) continue;
            uint32_t &p = g_px[(size_t)py * g_w + px];
            p = mix(p, c, (uint32_t)(255 * (k > 1 ? 1 : k)));
        }
}

// cores (0xAARRGGBB)
constexpr uint32_t kGold = 0xffe5c158u, kGoldMid = 0xffc9a24du, kGoldDark = 0xff8a6a2cu, kWhite = 0xfff3ede2u,
                   kText = 0xffcfc6b8u, kGrey = 0xff9e9484u, kDim = 0xff5c554bu, kRed = 0xff991b1bu, kInk = 0xff140f07u;

// texto cortado em maxw pixels
void text_fit(int x, int y, const char *s, int size, uint32_t c, int maxw) {
    if (text_w(s, size) <= maxw) return text(x, y, s, size, c);
    std::string t = s;
    while (!t.empty()) {
        do t.pop_back();
        while (!t.empty() && ((uint8_t)t.back() & 0xc0) == 0x80);
        if (!t.empty()) t.pop_back();
        if (text_w((t + "…").c_str(), size) <= maxw) break;
    }
    text(x, y, (t + "…").c_str(), size, c);
}

// quebra a descrição em até 2 linhas
void text_wrap(int x, int y, const char *s, int size, uint32_t c, int maxw, int lh) {
    std::string linha, resto = s;
    for (int l = 0; l < 2 && !resto.empty(); l++) {
        size_t corte = resto.size();
        if (text_w(resto.c_str(), size) > maxw) {
            corte = 0;
            for (size_t i = 0; i < resto.size(); i++)
                if (resto[i] == ' ' && text_w(resto.substr(0, i).c_str(), size) <= maxw) corte = i;
            if (!corte || l == 1) {
                text_fit(x, y + l * lh, resto.c_str(), size, c, maxw);
                return;
            }
        }
        text(x, y + l * lh, resto.substr(0, corte).c_str(), size, c);
        resto = corte < resto.size() ? resto.substr(corte + 1) : "";
    }
}

// botão redondo do controle ("A", "B", "L"...) com o texto ao lado; devolve o x seguinte
int hint(int x, int base, int S8, const char *bt, bool cheio, const char *txt, int size) {
    const int r = S8;
    if (cheio) disc(x + r, base - size / 3, r, bt[0] == 'B' ? kWhite : kGold);
    else {
        disc(x + r, base - size / 3, r, kGoldDark);
        disc(x + r, base - size / 3, r - 1, 0xff2a241eu);
    }
    const int bw = text_w(bt, size - 2);
    text(x + r - bw / 2, base - size / 3 + (size - 2) * 7 / 20, bt, size - 2, cheio ? kInk : kGold);
    return x + 2 * r + S8 / 2;
}

void draw(uint32_t screen_h) {
    const float k = screen_h / 720.f;
    auto S = [k](float v) { return (int)(v * k + 0.5f); };
    const int margem = S(16);
    g_w = (uint32_t)S(470), g_h = (uint32_t)(screen_h - 2 * margem);
    // fundo: obsidiana com degradê vertical
    g_px.resize((size_t)g_w * g_h);
    for (uint32_t y = 0; y < g_h; y++) {
        const uint32_t c = mix(0xff120e0cu, 0xff0b0908u, y * 255 / g_h);
        for (uint32_t x = 0; x < g_w; x++) g_px[(size_t)y * g_w + x] = c;
    }
    // moldura dourada fina + filete vermelho/dourado no topo
    blend(0, 0, (int)g_w, 1, kGoldMid, 90), blend(0, (int)g_h - 1, (int)g_w, 1, kGoldMid, 90);
    blend(0, 0, 1, (int)g_h, kGoldMid, 90), blend((int)g_w - 1, 0, 1, (int)g_h, kGoldMid, 90);
    for (int x = 0; x < (int)g_w; x++) {
        const float t = (float)x / g_w;
        const uint32_t c = t < .5f ? mix(kRed, kGold, (uint32_t)(t * 2 * 255)) : kGold;
        const uint32_t a = t < .5f ? 255 : (uint32_t)(255 * (1 - (t - .5f) * 2));
        blend(x, 0, 1, S(2), c, a);
    }

    // cabeçalho: brasão Ω, título e combinação para abrir
    const int hy = S(16), hh = S(52);
    disc(S(30), hy + hh / 2, S(13), 0xff4c1414u);
    ring(S(30), hy + hh / 2, S(13), kGoldMid);
    const int ow = text_w("Ω", S(16));
    text(S(30) - ow / 2, hy + hh / 2 + S(6), "Ω", S(16), kGold);
    text(S(54), hy + S(24), "NX MOD MENU", S(17), kGold);
#ifdef GOW_GAME_GOW2
    const char *jogo = "God of War II · deko3d";
#else
    const char *jogo = "God of War · deko3d";
#endif
    text(S(54), hy + S(44), jogo, S(13), kGrey);
    const char *combo = "R + Plus";
    const int cw = text_w(combo, S(13));
    rrect((int)g_w - S(18) - cw - S(16), hy + S(14), cw + S(16), S(24), S(5), 0xffffffffu, 14);
    text((int)g_w - S(18) - cw - S(8), hy + S(31), combo, S(13), 0xffc7bfb2u);
    const int ty = hy + hh + S(8);
    blend(0, ty - 1, (int)g_w, 1, kGoldMid, 50);

    // abas
    const int th = S(38), tw = (int)g_w / kTabs;
    blend(0, ty, (int)g_w, th, 0xff000000u, 77);
    for (int t = 0; t < kTabs; t++) {
        const bool on = t == g_tab;
        const int x = t * tw, w = text_w(tab_nome(t), S(15));
        if (on) blend(x, ty, tw, th, kGoldMid, 26), blend(x, ty + th - S(2), tw, S(2), kGold, 255);
        text(x + (tw - w) / 2, ty + th / 2 + S(6), tab_nome(t), S(15), on ? kGold : 0xff8e8577u);
    }
    blend(0, ty + th, (int)g_w, 1, kGoldMid, 38);

    // lista
    int l[kN];
    const int n = tab_items(l);
    const int foot = S(116), ly = ty + th + S(8), row = S(42);
    const int vis = std::max(1, ((int)g_h - foot - ly) / row);
    const int first = std::max(0, std::min(g_sel - vis / 2, n - vis));
    for (int r = first; r < n && r < first + vis; r++) {
        const int i = l[r], id = kItems[i].id, y = ly + (r - first) * row;
        const bool ok = item_ok(i), sel = r == g_sel;
        if (sel) {
            for (int x = 0; x < (int)g_w; x++) {
                const float t = (float)x / g_w;
                blend(x, y, 1, row, mix(kGoldMid, kRed, (uint32_t)(t * 255)), (uint32_t)(41 - 5 * t));
            }
            blend(0, y, S(3), row, kGold, 255);
        } else if (r != first) blend(S(16), y, (int)g_w - S(32), 1, 0xffffffffu, 10);
        const int base = y + row / 2 + S(6);
        const uint32_t tc = !ok ? kDim : sel ? kWhite : kText;
        const int xr = (int)g_w - S(18);
        if (id == A_CLOSE || id == A_RESET || id == A_LAUNCHER || id == A_EXIT) {
            if (!ok) {
                const char *u = tr("menu.unavailable");
                text(xr - text_w(u, S(13)), base, u, S(13), kDim);
                text_fit(S(18), base, kItems[i].n(), S(16), tc, (int)g_w - S(150));
                continue;
            }
            const char *e = id == A_CLOSE ? tr("menu.btn.close")
                          : id == A_LAUNCHER ? tr("menu.btn.launcher")
                          : id == A_EXIT ? tr("menu.btn.exit")
                          : tr("menu.btn.run");
            const int ew = text_w(e, S(13));
            rrect(xr - ew - S(16), y + row / 2 - S(11), ew + S(16), S(22), S(4), kGoldMid, 51);
            text(xr - ew - S(8), base - S(1), e, S(13), kGold);
            text_fit(S(18), base, kItems[i].n(), S(16), tc, xr - ew - S(40) - S(18));
            continue;
        }
        if (id >= 0 && !ok) {
            const char *u = tr("menu.unavailable");
            text(xr - text_w(u, S(13)), base, u, S(13), kDim);
            text_fit(S(18), base, kItems[i].n(), S(16), tc, (int)g_w - S(150));
            continue;
        }
        const uint32_t v = id >= 0 ? g_val[id] : 0;
        int wv = 0;
        if (is_toggle(id)) {   // chave ON/OFF
            const bool on = v != 0;
            const char *t = on ? "ON" : "OFF";
            const int lw = text_w(t, S(12)), sw = S(28), sh = S(16);
            const int sx = xr - lw - S(6) - sw, sy = y + (row - sh) / 2;
            rrect(sx, sy, sw, sh, sh / 2, on ? 0xffb88c34u : 0xffffffffu, on ? 255 : 26);
            disc(on ? sx + sw - sh / 2 : sx + sh / 2, sy + sh / 2, sh / 2 - S(2), on ? kInk : 0xff8e8577u);
            text(xr - lw, base - S(1), t, S(12), on ? kGold : 0xff7c7365u);
            wv = lw + S(6) + sw;
        } else {               // ‹ valor ›
            const std::string t = val_text(id, v);
            const int vw = text_w(t.c_str(), S(14)), aw = text_w("›", S(14));
            text(xr - aw, base, "›", S(14), 0xff8e8577u);
            text(xr - aw - S(6) - vw, base, t.c_str(), S(14), kGold);
            text(xr - aw - S(12) - vw - aw, base, "‹", S(14), 0xff8e8577u);
            wv = vw + 2 * aw + S(12);
            if (is_level(id)) {   // barrinha de nível
                const int bw = S(56), bx = xr - wv - S(10) - bw, by = y + row / 2 - S(1);
                const int nv = n_vals(id), p = pos_of(id, v);
                blend(bx, by, bw, S(3), 0xffffffffu, 26);
                blend(bx, by, bw * p / (nv - 1), S(3), sel ? kGold : kGoldDark, 255);
                wv += bw + S(10);
            }
        }
        text_fit(S(18), base, kItems[i].n(), S(16), tc, xr - wv - S(14) - S(18));
    }
    auto seta = [&](int cy, bool cima) { // triângulo pequeno: tem mais itens acima/abaixo
        for (int j = 0; j < S(5); j++) blend((int)g_w / 2 - j, cima ? cy + j : cy - j, 2 * j + 1, 1, kGrey, 255);
    };
    if (first > 0) seta(ly - S(6), true);
    if (first + vis < n) seta(ly + vis * row + S(5), false);

    // descrição + dicas do controle
    const int fy = (int)g_h - foot;
    blend(0, fy, (int)g_w, foot, 0xff000000u, 140);
    blend(0, fy, (int)g_w, 1, kGoldMid, 51);
    if (g_sel < n) text_wrap(S(18), fy + S(26), kItems[l[g_sel]].d(), S(14), 0xffb8ae9eu, (int)g_w - S(36), S(19));
    const int hy2 = (int)g_h - S(22);
    blend(S(18), hy2 - S(26), (int)g_w - S(36), 1, 0xffffffffu, 13);
    int x = S(18);
    x = hint(x, hy2, S(9), "A", true, nullptr, S(14));
    const char *alt = tr("menu.hint.change");
    text(x, hy2, alt, S(14), 0xffa89f91u);
    x += text_w(alt, S(14)) + S(16);
    x = hint(x, hy2, S(9), "L", false, nullptr, S(14));
    x = hint(x - S(2), hy2, S(9), "R", false, nullptr, S(14));
    text(x, hy2, tr("menu.hint.tabs"), S(14), 0xffa89f91u);
    const char *fec = tr("menu.btn.close");
    const int fw = text_w(fec, S(14));
    const int bx = (int)g_w - S(18) - fw - S(24);
    hint(bx, hy2, S(9), "B", true, nullptr, S(14));
    text((int)g_w - S(18) - fw, hy2, fec, S(14), 0xffa89f91u);

    if (g_confirm) {
        for (auto &p : g_px) p = mix(p, 0xff000000u, 150); // escurece o menu atrás
        const int bw = (int)g_w - S(40), bh = S(230), bx2 = S(20), by = ((int)g_h - bh) / 2;
        rrect(bx2, by, bw, bh, S(10), 0xff1c1712u, 255);
        blend(bx2, by, bw, S(2), kGold, 255);
        static const char *const kConfirm[3][4] = {
            {"menu.confirm.gpu_diag.title", "menu.confirm.gpu_diag.line1", "menu.confirm.gpu_diag.line2", "menu.confirm.gpu_diag.line3"},
            {"menu.confirm.exit.title", "menu.confirm.exit.line1", "menu.confirm.exit.line2", "menu.confirm.exit.line3"},
            {"menu.confirm.launcher.title", "menu.confirm.launcher.line1", "menu.confirm.launcher.line2", "menu.confirm.launcher.line3"},
        };
        const char *const *l2 = kConfirm[g_confirm == 3 ? 2 : g_confirm == 2 ? 1 : 0];
        text(bx2 + (bw - text_w(tr(l2[0]), S(24))) / 2, by + S(44), tr(l2[0]), S(24), kGold);
        for (int i = 1; i < 4; i++) text(bx2 + (bw - text_w(tr(l2[i]), S(16))) / 2, by + S(64) + S(28) * i, tr(l2[i]), S(16), kWhite);
        const char *b = g_confirm == 3 ? tr("menu.confirm.launcher.buttons")
                      : g_confirm == 2 ? tr("menu.confirm.exit.buttons")
                                       : tr("menu.confirm.gpu_diag.buttons");
        text(bx2 + (bw - text_w(b, S(16))) / 2, by + bh - S(22), b, S(16), kGrey);
    }
}
} // namespace

bool panel(uint32_t screen_h, const uint32_t *&px, uint32_t &w, uint32_t &h, bool &dirty) {
    if (!g_active) return false;
    dirty = g_dirty || screen_h != g_for_h;
    if (dirty) {
        draw(screen_h);
        g_for_h = screen_h;
        g_dirty = false;
    }
    px = g_px.data(), w = g_w, h = g_h;
    return true;
}
} // namespace nxmenu

namespace nxmenu {
uint32_t panel_x(uint32_t screen_h) { return (uint32_t)(16.f * screen_h / 720.f + 0.5f); }
} // namespace nxmenu
