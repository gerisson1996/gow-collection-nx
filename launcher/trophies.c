// Leitor nativo de troféus do PS Vita (TROPHY.TRP + data/trophy/<NPWR>/unlocked.txt).
// Decodifica TROP*.SFM (com suporte aos idiomas embutidos no TRP) e os ícones PNG via libpng16.
#include "trophies.h"

#include <ctype.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRP_MAX_FILE  (32ULL * 1024ULL * 1024ULL)
#define TRP_MAX_ENTRY (4ULL * 1024ULL * 1024ULL)

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static uint64_t be64(const uint8_t *p) {
    return ((uint64_t)be32(p) << 32) | (uint64_t)be32(p + 4);
}

static int grade_idx(char g) {
    switch (g) {
    case 'P': return 0;
    case 'G': return 1;
    case 'S': return 2;
    default:  return 3;
    }
}

static int grade_pts(char g) {
    switch (g) {
    case 'P': return 180;
    case 'G': return 90;
    case 'S': return 30;
    default:  return 15;
    }
}

// ---------------------------------------------------------------- decodificação XML (TROP*.SFM)
static void decode_xml_text(const char *src, size_t len, char *dst, size_t dst_sz) {
    if (!dst || !dst_sz) return;
    size_t w = 0;
    for (size_t i = 0; i < len && w + 1 < dst_sz; ) {
        if (src[i] == '&') {
            if (i + 5 <= len && !strncmp(src + i, "&amp;", 5)) { dst[w++] = '&'; i += 5; continue; }
            if (i + 4 <= len && !strncmp(src + i, "&lt;", 4))  { dst[w++] = '<'; i += 4; continue; }
            if (i + 4 <= len && !strncmp(src + i, "&gt;", 4))  { dst[w++] = '>'; i += 4; continue; }
            if (i + 6 <= len && !strncmp(src + i, "&quot;", 6)) { dst[w++] = '"'; i += 6; continue; }
            if (i + 6 <= len && !strncmp(src + i, "&apos;", 6)) { dst[w++] = '\''; i += 6; continue; }
            if (i + 3 < len && src[i + 1] == '#') {
                size_t j = i + 2;
                unsigned long cp = 0;
                if (j < len && (src[j] == 'x' || src[j] == 'X')) {
                    j++;
                    while (j < len && isxdigit((unsigned char)src[j])) {
                        char c = src[j++];
                        cp = cp * 16 + (unsigned long)(isdigit((unsigned char)c) ? c - '0' : tolower((unsigned char)c) - 'a' + 10);
                    }
                } else {
                    while (j < len && isdigit((unsigned char)src[j])) cp = cp * 10 + (unsigned long)(src[j++] - '0');
                }
                if (j < len && src[j] == ';' && cp > 0 && cp <= 0x10FFFF) {
                    i = j + 1;
                    if (cp < 0x80) dst[w++] = (char)cp;
                    else if (cp < 0x800 && w + 2 < dst_sz) {
                        dst[w++] = (char)(0xC0 | (cp >> 6));
                        dst[w++] = (char)(0x80 | (cp & 0x3F));
                    } else if (cp < 0x10000 && w + 3 < dst_sz) {
                        dst[w++] = (char)(0xE0 | (cp >> 12));
                        dst[w++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        dst[w++] = (char)(0x80 | (cp & 0x3F));
                    }
                    continue;
                }
            }
        }
        char ch = src[i++];
        if (ch == '\r' || ch == '\n' || ch == '\t') ch = ' ';
        if (ch == ' ' && (w == 0 || dst[w - 1] == ' ')) continue;
        dst[w++] = ch;
    }
    while (w > 0 && dst[w - 1] == ' ') w--;
    dst[w] = 0;
}

static int find_attr(const char *tag, const char *tag_end, const char *attr, char *out, size_t out_sz) {
    size_t alen = strlen(attr);
    const char *p = tag;
    while (p < tag_end) {
        const char *m = strstr(p, attr);
        if (!m || m >= tag_end) return 0;
        if (m > tag && !isspace((unsigned char)m[-1])) { p = m + 1; continue; }
        const char *v = m + alen;
        if (v + 2 > tag_end || v[0] != '=' || (v[1] != '"' && v[1] != '\'')) { p = m + 1; continue; }
        char q = v[1];
        v += 2;
        const char *ve = v;
        while (ve < tag_end && *ve != q) ve++;
        if (ve >= tag_end) return 0;
        size_t n = (size_t)(ve - v);
        if (n >= out_sz) n = out_sz - 1;
        memcpy(out, v, n);
        out[n] = 0;
        return 1;
    }
    return 0;
}

static int extract_tag_text(const char *block, const char *block_end, const char *open_tag,
                            const char *close_tag, char *out, size_t out_sz) {
    out[0] = 0;
    const char *a = strstr(block, open_tag);
    if (!a || a >= block_end) return 0;
    a += strlen(open_tag);
    const char *b = strstr(a, close_tag);
    if (!b || b > block_end) return 0;
    decode_xml_text(a, (size_t)(b - a), out, out_sz);
    return 1;
}

static void recompute_stats(TrophySet *set) {
    set->unlocked_count = 0;
    set->pts_have = 0;
    set->pts_total = 0;
    memset(set->grade_have, 0, sizeof set->grade_have);
    memset(set->grade_total, 0, sizeof set->grade_total);
    for (int i = 0; i < set->count; i++) {
        int gi = grade_idx(set->items[i].grade);
        int pts = grade_pts(set->items[i].grade);
        set->grade_total[gi]++;
        set->pts_total += pts;
        if (set->items[i].unlocked) {
            set->unlocked_count++;
            set->grade_have[gi]++;
            set->pts_have += pts;
        }
    }
}

int trp_parse_sfm(const char *xml, size_t xml_len, TrophySet *set) {
    if (!xml || !set || xml_len == 0) return 0;
    // Copia terminada em '\0' para usar strstr com segurança.
    char *buf = (char *)malloc(xml_len + 1);
    if (!buf) return 0;
    memcpy(buf, xml, xml_len);
    buf[xml_len] = 0;

    set->count = 0;
    const char *p = buf;
    while (set->count < TRP_MAX_TROPHIES) {
        const char *open = strstr(p, "<trophy");
        if (!open) break;
        if (open[7] != ' ' && open[7] != '\t' && open[7] != '\r' && open[7] != '\n') {
            p = open + 7;
            continue;
        }
        const char *tag_end = strchr(open, '>');
        if (!tag_end) break;
        const char *close = strstr(tag_end + 1, "</trophy>");
        if (!close) break;

        char sid[16] = {0}, shid[16] = {0}, stype[8] = {0};
        if (find_attr(open, tag_end, "id", sid, sizeof sid) &&
            find_attr(open, tag_end, "ttype", stype, sizeof stype)) {
            find_attr(open, tag_end, "hidden", shid, sizeof shid);
            TrophyItem *it = &set->items[set->count];
            memset(it, 0, sizeof *it);
            it->id = atoi(sid);
            it->hidden = (!strcmp(shid, "yes") || !strcmp(shid, "true") || !strcmp(shid, "1"));
            char g = (char)toupper((unsigned char)stype[0]);
            it->grade = (g == 'P' || g == 'G' || g == 'S' || g == 'B') ? g : 'B';
            extract_tag_text(tag_end + 1, close, "<name>", "</name>", it->name, sizeof it->name);
            extract_tag_text(tag_end + 1, close, "<detail>", "</detail>", it->detail, sizeof it->detail);
            set->count++;
        }
        p = close + 9;
    }

    free(buf);
    recompute_stats(set);
    return set->count > 0;
}

void trp_apply_unlocked(TrophySet *set, const char *unlocked_text) {
    if (!set || !unlocked_text) return;
    const char *p = unlocked_text;
    while (*p) {
        while (*p && !isdigit((unsigned char)*p)) p++;
        if (!*p) break;
        int id = 0;
        while (isdigit((unsigned char)*p)) {
            if (id < 100000) id = id * 10 + (*p - '0');
            p++;
        }
        for (int i = 0; i < set->count; i++) {
            if (set->items[i].id == id) {
                set->items[i].unlocked = 1;
                break;
            }
        }
    }
    recompute_stats(set);
}

// ---------------------------------------------------------------- decodificação PNG em memória (libpng16)
typedef struct {
    const uint8_t *data;
    size_t size;
    size_t pos;
} PngMem;

static void png_read_mem(png_structp png_ptr, png_bytep out, png_size_t count) {
    PngMem *m = (PngMem *)png_get_io_ptr(png_ptr);
    if (!m || m->pos + count > m->size) png_error(png_ptr, "EOF");
    memcpy(out, m->data + m->pos, count);
    m->pos += count;
}

static uint32_t *decode_png_thumb(const uint8_t *data, size_t size) {
    if (!data || size < 8 || png_sig_cmp(data, 0, 8) != 0) return NULL;
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return NULL;
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, NULL, NULL); return NULL; }

    uint8_t *volatile rgba = NULL;
    png_bytep *volatile rows = NULL;
    uint32_t *volatile thumb = NULL;

    if (setjmp(png_jmpbuf(png))) {
        free((void *)rows);
        free((void *)rgba);
        free((void *)thumb);
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    PngMem mem = { data, size, 0 };
    png_set_read_fn(png, &mem, png_read_mem);
    png_read_info(png, info);

    png_uint_32 w = png_get_image_width(png, info);
    png_uint_32 h = png_get_image_height(png, info);
    int ctype = png_get_color_type(png, info);
    int depth = png_get_bit_depth(png, info);
    if (w == 0 || h == 0 || w > 1024 || h > 1024) png_error(png, "dim");

    if (depth == 16) png_set_strip_16(png);
    if (ctype == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (ctype == PNG_COLOR_TYPE_GRAY && depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (ctype == PNG_COLOR_TYPE_RGB || ctype == PNG_COLOR_TYPE_GRAY || ctype == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (ctype == PNG_COLOR_TYPE_GRAY || ctype == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    rgba = (uint8_t *)malloc((size_t)w * h * 4);
    rows = (png_bytep *)malloc((size_t)h * sizeof(png_bytep));
    thumb = (uint32_t *)malloc(TRP_ICON_SZ * TRP_ICON_SZ * 4);
    if (!rgba || !rows || !thumb) png_error(png, "oom");

    for (png_uint_32 y = 0; y < h; y++) rows[y] = rgba + (size_t)y * w * 4;
    png_read_image(png, rows);
    png_destroy_read_struct(&png, &info, NULL);

    uint32_t *out_thumb = thumb;
    const uint8_t *src_rgba = rgba;
    // Reamostragem em área (box filter) para TRP_ICON_SZ x TRP_ICON_SZ
    for (int ty = 0; ty < TRP_ICON_SZ; ty++) {
        png_uint_32 y0 = (png_uint_32)((uint64_t)ty * h / TRP_ICON_SZ);
        png_uint_32 y1 = (png_uint_32)((uint64_t)(ty + 1) * h / TRP_ICON_SZ);
        if (y1 <= y0) y1 = y0 + 1;
        if (y1 > h) y1 = h;
        for (int tx = 0; tx < TRP_ICON_SZ; tx++) {
            png_uint_32 x0 = (png_uint_32)((uint64_t)tx * w / TRP_ICON_SZ);
            png_uint_32 x1 = (png_uint_32)((uint64_t)(tx + 1) * w / TRP_ICON_SZ);
            if (x1 <= x0) x1 = x0 + 1;
            if (x1 > w) x1 = w;
            uint32_t sr = 0, sg = 0, sb = 0, sa = 0, cnt = 0;
            for (png_uint_32 y = y0; y < y1; y++) {
                for (png_uint_32 x = x0; x < x1; x++) {
                    const uint8_t *px = src_rgba + ((size_t)y * w + x) * 4;
                    sr += px[0]; sg += px[1]; sb += px[2]; sa += px[3];
                    cnt++;
                }
            }
            if (cnt == 0) cnt = 1;
            out_thumb[ty * TRP_ICON_SZ + tx] = (sr / cnt) | ((sg / cnt) << 8) | ((sb / cnt) << 16) | ((sa / cnt) << 24);
        }
    }

    free((void *)rows);
    free((void *)rgba);
    return out_thumb;
}

// ---------------------------------------------------------------- leitura do TROPHY.TRP
typedef struct {
    char name[33];
    uint64_t off;
    uint64_t size;
} TrpEntry;

static const TrpEntry *find_trp_entry(const TrpEntry *ents, uint32_t n, const char *name) {
    for (uint32_t i = 0; i < n; i++)
        if (!strcmp(ents[i].name, name)) return &ents[i];
    return NULL;
}

static const char *sfm_for_lang(const char *lang_code) {
    if (!lang_code) return "TROP.SFM";
    if (!strcmp(lang_code, "fr")) return "TROP_02.SFM";
    if (!strcmp(lang_code, "es")) return "TROP_03.SFM";
    if (!strcmp(lang_code, "de")) return "TROP_04.SFM";
    if (!strcmp(lang_code, "it")) return "TROP_05.SFM";
    if (!strncmp(lang_code, "pt", 2)) return "TROP_17.SFM";
    return "TROP.SFM";
}

void trp_free(TrophySet *set) {
    if (!set) return;
    for (int i = 0; i < set->count; i++) {
        free(set->items[i].icon);
        set->items[i].icon = NULL;
    }
    memset(set, 0, sizeof *set);
}

int trp_load(TrophySet *set, const char *base_dir, const char *comm, const char *lang_code) {
    if (!set || !base_dir || !comm) return 0;
    trp_free(set);

    char path[1024];
    snprintf(path, sizeof path, "%.700sgame/PCSA00126/sce_sys/trophy/%s/TROPHY.TRP", base_dir, comm);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long fsz_l = ftell(f);
    if (fsz_l < 0x40 || (uint64_t)fsz_l > TRP_MAX_FILE) { fclose(f); return 0; }
    uint64_t fsz = (uint64_t)fsz_l;
    fseek(f, 0, SEEK_SET);

    uint8_t hdr[0x40];
    if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr) { fclose(f); return 0; }
    if (hdr[0] != 0xDC || hdr[1] != 0xA2 || hdr[2] != 0x4D || hdr[3] != 0x00) { fclose(f); return 0; }

    uint32_t n = be32(hdr + 16);
    uint32_t esz = be32(hdr + 20);
    if (n == 0 || n > 1024 || esz < 48 || esz > 256) { fclose(f); return 0; }
    if (0x40ULL + (uint64_t)n * esz > fsz) { fclose(f); return 0; }

    TrpEntry *ents = (TrpEntry *)calloc(n, sizeof(TrpEntry));
    uint8_t *ebuf = (uint8_t *)malloc(esz);
    if (!ents || !ebuf) { free(ents); free(ebuf); fclose(f); return 0; }

    for (uint32_t i = 0; i < n; i++) {
        if (fread(ebuf, 1, esz, f) != esz) { free(ents); free(ebuf); fclose(f); return 0; }
        memcpy(ents[i].name, ebuf, 32);
        ents[i].name[32] = 0;
        ents[i].off = be64(ebuf + 32);
        ents[i].size = be64(ebuf + 40);
    }
    free(ebuf);

    // Escolhe o SFM localizado no idioma pedido, com fallback para português (PT-PT) e inglês (TROP.SFM)
    const TrpEntry *sfm = find_trp_entry(ents, n, sfm_for_lang(lang_code));
    if (!sfm && lang_code && !strncmp(lang_code, "pt", 2)) sfm = find_trp_entry(ents, n, "TROP_07.SFM");
    if (!sfm) sfm = find_trp_entry(ents, n, "TROP.SFM");
    if (!sfm || sfm->size == 0 || sfm->size > TRP_MAX_ENTRY || sfm->off + sfm->size > fsz) {
        free(ents); fclose(f); return 0;
    }

    char *xml = (char *)malloc((size_t)sfm->size);
    if (!xml || fseek(f, (long)sfm->off, SEEK_SET) != 0 || fread(xml, 1, (size_t)sfm->size, f) != (size_t)sfm->size) {
        free(xml); free(ents); fclose(f); return 0;
    }
    int ok = trp_parse_sfm(xml, (size_t)sfm->size, set);
    free(xml);
    if (!ok) { free(ents); fclose(f); return 0; }

    // Decodifica ícones PNG (TROP000.PNG ...)
    for (int i = 0; i < set->count; i++) {
        char iname[32];
        snprintf(iname, sizeof iname, "TROP%03d.PNG", set->items[i].id);
        const TrpEntry *ie = find_trp_entry(ents, n, iname);
        if (!ie || ie->size < 8 || ie->size > TRP_MAX_ENTRY || ie->off + ie->size > fsz) continue;
        uint8_t *png_buf = (uint8_t *)malloc((size_t)ie->size);
        if (!png_buf) continue;
        if (fseek(f, (long)ie->off, SEEK_SET) == 0 && fread(png_buf, 1, (size_t)ie->size, f) == (size_t)ie->size) {
            set->items[i].icon = decode_png_thumb(png_buf, (size_t)ie->size);
        }
        free(png_buf);
    }

    free(ents);
    fclose(f);

    // Lê troféus desbloqueados em data/trophy/<comm>/unlocked.txt
    char upath[1024];
    snprintf(upath, sizeof upath, "%.700sdata/trophy/%s/unlocked.txt", base_dir, comm);
    FILE *uf = fopen(upath, "rb");
    if (uf) {
        if (fseek(uf, 0, SEEK_END) == 0) {
            long usz = ftell(uf);
            if (usz > 0 && usz < 65536) {
                fseek(uf, 0, SEEK_SET);
                char *ubuf = (char *)malloc((size_t)usz + 1);
                if (ubuf) {
                    size_t r = fread(ubuf, 1, (size_t)usz, uf);
                    ubuf[r] = 0;
                    trp_apply_unlocked(set, ubuf);
                    free(ubuf);
                }
            }
        }
        fclose(uf);
    }

    set->loaded = 1;
    return 1;
}

int trp_percent(const TrophySet *set) {
    if (!set || set->pts_total <= 0) return 0;
    if (set->pts_have >= set->pts_total) return 100;
    int pct = (set->pts_have * 100) / set->pts_total;
    if (pct < 0) pct = 0;
    if (pct > 99) pct = 99;
    return pct;
}
