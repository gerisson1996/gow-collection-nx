// Textos da interface por idioma: ver i18n.h.
#include "i18n.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const I18nBuiltin i18n_builtins[];
extern const int i18n_builtins_count;

#define I18N_MAX_LANGS   32
#define I18N_MAX_SOURCES 4          // embutido + até 3 arquivos do SD por idioma
#define I18N_MAX_FILE    (512 * 1024)

typedef struct {
    const char *key, *val;
    int order;                      // ordem de leitura: a última definição de uma chave vence
} Entry;

typedef struct {
    char code[16];
    char *bufs[I18N_MAX_SOURCES];
    int nbufs;
    Entry *e;
    int n, cap;
} Lang;

static Lang langs[I18N_MAX_LANGS];
static int nlangs;
static int cur = -1, base = -1;     // idioma ativo e inglês (recuo)

static int lang_index(const char *code) {
    for (int i = 0; i < nlangs; i++)
        if (!strcmp(langs[i].code, code)) return i;
    return -1;
}

// Código em minúsculas, só letras, dígitos, '-' e '_' (até 15 caracteres). Devolve 0 se ficou vazio.
static int normalize_code(const char *in, char *out, size_t cap) {
    size_t n = 0;
    for (; *in && n + 1 < cap; in++) {
        unsigned char c = (unsigned char)*in;
        if (isalnum(c) || c == '-' || c == '_') out[n++] = (char)tolower(c);
        else break;
    }
    out[n] = 0;
    return n > 0;
}

static Lang *lang_get(const char *code) {
    char c[16];
    if (!normalize_code(code, c, sizeof c)) return NULL;
    int i = lang_index(c);
    if (i >= 0) return &langs[i];
    if (nlangs >= I18N_MAX_LANGS) return NULL;
    Lang *l = &langs[nlangs++];
    memset(l, 0, sizeof *l);
    memcpy(l->code, c, sizeof l->code);
    if (!strcmp(c, "en")) base = (int)(l - langs);
    return l;
}

static int push_entry(Lang *l, const char *key, const char *val) {
    if (l->n == l->cap) {
        int cap = l->cap ? l->cap * 2 : 128;
        Entry *e = (Entry *)realloc(l->e, (size_t)cap * sizeof *e);
        if (!e) return -1;
        l->e = e;
        l->cap = cap;
    }
    l->e[l->n] = (Entry){ key, val, l->n };
    l->n++;
    return 0;
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t')) *--e = 0;
    return s;
}

// \n, \t, \\ e \" viram os caracteres correspondentes (no próprio buffer).
static void unescape(char *s) {
    char *w = s;
    for (char *r = s; *r; r++) {
        if (*r == '\\' && r[1]) {
            r++;
            *w++ = *r == 'n' ? '\n' : *r == 't' ? '\t' : *r;
        } else {
            *w++ = *r;
        }
    }
    *w = 0;
}

// {A}, {B}, {X}, {Y}, {L}, {R} e {PLUS} viram os glifos de botão da fonte do sistema (NintendoExt).
// Cada glifo ocupa no máximo o tamanho do token, então a troca é feita no próprio buffer.
static const struct { const char *token, *glyph; } kGlyphs[] = {
    { "{A}", "\xEE\x83\xA0" }, { "{B}", "\xEE\x83\xA1" }, { "{X}", "\xEE\x83\xA2" }, { "{Y}", "\xEE\x83\xA3" },
    { "{L}", "\xEE\x83\xA4" }, { "{R}", "\xEE\x83\xA5" }, { "{PLUS}", "\xEE\x83\xAF" },
};

static void expand_glyphs(char *s) {
    char *w = s;
    for (char *r = s; *r;) {
        size_t i = 0, n = sizeof kGlyphs / sizeof kGlyphs[0];
        if (*r == '{')
            for (; i < n; i++)
                if (!strncmp(r, kGlyphs[i].token, strlen(kGlyphs[i].token))) break;
        if (*r == '{' && i < n) {
            size_t g = strlen(kGlyphs[i].glyph);
            memmove(w, kGlyphs[i].glyph, g);
            w += g;
            r += strlen(kGlyphs[i].token);
        } else {
            *w++ = *r++;
        }
    }
    *w = 0;
}

// Sequência de conversões printf (%d, %s, %ld...) do texto, sem os "%%". Serve para recusar um texto do SD cujos
// especificadores não batem com os do inglês: o launcher usa vários textos como formato de snprintf.
static void fmt_signature(const char *s, char *out, size_t cap) {
    size_t n = 0;
    for (; *s; s++) {
        if (*s != '%') continue;
        if (s[1] == '%') { s++; continue; }
        s++;
        while (*s && strchr("-+ #0123456789.", *s)) s++;
        while (*s && strchr("hlz", *s) && n + 1 < cap) out[n++] = *s++;
        if (!*s) break;
        if (n + 1 < cap) out[n++] = *s;
    }
    out[n] = 0;
}

static int same_format(const char *a, const char *b) {
    char fa[32], fb[32];
    fmt_signature(a, fa, sizeof fa);
    fmt_signature(b, fb, sizeof fb);
    return !strcmp(fa, fb);
}

static int cmp_entry(const void *a, const void *b) {
    const Entry *x = (const Entry *)a, *y = (const Entry *)b;
    int c = strcmp(x->key, y->key);
    return c ? c : x->order - y->order;
}

// Ordena por chave e deixa só a última definição de cada uma.
static void finish(Lang *l) {
    qsort(l->e, (size_t)l->n, sizeof *l->e, cmp_entry);
    int w = 0;
    for (int i = 0; i < l->n; i++) {
        if (i + 1 < l->n && !strcmp(l->e[i].key, l->e[i + 1].key)) continue;
        l->e[w] = l->e[i];
        l->e[w].order = w;
        w++;
    }
    l->n = w;
}

static const char *lookup_n(int li, const char *key, int n);
static const char *lookup(int li, const char *key);

int i18n_add_text(const char *code, const char *text, size_t len) {
    int existed = 0;
    char c[16];
    if (normalize_code(code, c, sizeof c)) existed = lang_index(c) >= 0;
    Lang *l = lang_get(code);
    if (!l) return -1;
    if (l->nbufs >= I18N_MAX_SOURCES || len > I18N_MAX_FILE) {
        if (!existed && l == &langs[nlangs - 1]) nlangs--;  // não deixa um idioma vazio na lista
        return -1;
    }
    char *buf = (char *)malloc(len + 1);
    if (!buf) {
        if (!existed && l == &langs[nlangs - 1]) nlangs--;
        return -1;
    }
    memcpy(buf, text, len);
    buf[len] = 0;
    l->bufs[l->nbufs++] = buf;
    // As entradas já ordenadas continuam valendo; as novas entram depois e vencem nas chaves repetidas.
    for (int i = 0; i < l->n; i++) l->e[i].order = i;
    const int li = (int)(l - langs), sorted = l->n;
    int rc = 0;

    char *p = buf;
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) p += 3;
    while (*p) {
        char *line = p;
        char *nl = strchr(p, '\n');
        if (nl) { *nl = 0; p = nl + 1; } else p += strlen(p);
        size_t ll = strlen(line);
        if (ll && line[ll - 1] == '\r') line[ll - 1] = 0;
        line = trim(line);
        if (!*line || *line == '#' || *line == ';') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = trim(line), *val = trim(eq + 1);
        if (!*key) continue;
        size_t vl = strlen(val);
        if (vl >= 2 && val[0] == '"' && val[vl - 1] == '"') { val[vl - 1] = 0; val++; }
        unescape(val);
        expand_glyphs(val);
        // O inglês embutido é carregado primeiro e serve de referência para os especificadores de formato.
        const char *ref = base < 0 ? NULL : base == li ? lookup_n(li, key, sorted) : lookup(base, key);
        if (ref && !same_format(ref, val)) continue;
        if (push_entry(l, key, val)) { rc = -1; break; }
    }
    finish(l);
    return rc;
}

// Busca binária nas n primeiras entradas (as já ordenadas) do idioma li.
static const char *lookup_n(int li, const char *key, int n) {
    if (li < 0) return NULL;
    const Lang *l = &langs[li];
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = strcmp(key, l->e[mid].key);
        if (!c) return l->e[mid].val;
        if (c < 0) hi = mid - 1; else lo = mid + 1;
    }
    return NULL;
}

static const char *lookup(int li, const char *key) { return li < 0 ? NULL : lookup_n(li, key, langs[li].n); }

void i18n_reset(void) {
    for (int i = 0; i < nlangs; i++) {
        for (int b = 0; b < langs[i].nbufs; b++) free(langs[i].bufs[b]);
        free(langs[i].e);
    }
    memset(langs, 0, sizeof langs);
    nlangs = 0;
    cur = base = -1;
}

static int cmp_name(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }

// Nome de arquivo vale como idioma só se for inteiro um código válido: "fr.ini" sim, "en.backup.ini" não.
static int valid_lang_file(const char *name) {
    char c[16];
    size_t stem = strlen(name) - 4;
    return normalize_code(name, c, sizeof c) && strlen(c) == stem;
}

static void load_sd(const char *dir) {
    DIR *d = opendir(dir);
    if (!d) return;
    // Lê os nomes primeiro e ordena, para a ordem dos idiomas do SD não depender do sistema de arquivos.
    char names[I18N_MAX_LANGS][64];
    int n = 0;
    struct dirent *de;
    while ((de = readdir(d)) && n < I18N_MAX_LANGS) {
        size_t len = strlen(de->d_name);
        if (len < 5 || len >= sizeof names[0] || strcmp(de->d_name + len - 4, ".ini") || !valid_lang_file(de->d_name)) continue;
        memcpy(names[n++], de->d_name, len + 1);
    }
    closedir(d);
    qsort(names, (size_t)n, sizeof names[0], cmp_name);
    for (int i = 0; i < n; i++) {
        char path[768];
        if (strlen(dir) > 700) break;
        snprintf(path, sizeof path, "%.700s/%.63s", dir, names[i]);
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        long size = fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
        rewind(f);
        char *buf = size >= 0 && size <= I18N_MAX_FILE ? (char *)malloc((size_t)size + 1) : NULL;
        size_t len = buf ? fread(buf, 1, (size_t)size, f) : 0;
        fclose(f);
        if (buf && len == (size_t)size) {
            names[i][strlen(names[i]) - 4] = 0;
            i18n_add_text(names[i], buf, len);
        }
        free(buf);
    }
}

void i18n_init(const char *sd_dir) {
    char keep[16] = "";
    if (cur >= 0) memcpy(keep, langs[cur].code, sizeof keep);
    i18n_reset();
    for (int i = 0; i < i18n_builtins_count; i++)
        i18n_add_text(i18n_builtins[i].code, i18n_builtins[i].text, strlen(i18n_builtins[i].text));
    if (sd_dir && *sd_dir) load_sd(sd_dir);
    if (!keep[0] || i18n_set(keep)) i18n_set("en");
}

int i18n_count(void) { return nlangs; }
const char *i18n_code(int i) { return i >= 0 && i < nlangs ? langs[i].code : ""; }

const char *i18n_name(int i) {
    if (i < 0 || i >= nlangs) return "";
    const char *n = lookup(i, "_name");
    return n && *n ? n : langs[i].code;
}

int i18n_find(const char *code) {
    char c[16];
    if (!code || !normalize_code(code, c, sizeof c)) return -1;
    int i = lang_index(c);
    if (i >= 0) return i;
    // "pt-BR" acha "pt" e "pt" acha "pt-br": compara a parte antes do '-'.
    size_t p = strcspn(c, "-_");
    for (int j = 0; j < nlangs; j++)
        if (strcspn(langs[j].code, "-_") == p && !strncmp(langs[j].code, c, p)) return j;
    return -1;
}

int i18n_set(const char *code) {
    int i = i18n_find(code);
    if (i >= 0) { cur = i; return 0; }
    cur = base;
    return -1;
}

const char *i18n_current(void) { return cur >= 0 ? langs[cur].code : "en"; }

const char *i18n_tr(const char *key) {
    const char *v = lookup(cur, key);
    if (!v && cur != base) v = lookup(base, key);
    return v ? v : key;
}
