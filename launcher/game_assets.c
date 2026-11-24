// Implementação do carregamento de capa e música a partir do SD ou de game/PCSA00126/.
#include "game_assets.h"

#include <ctype.h>
#include <mpg123.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

#ifdef ENABLE_ATRAC9
#include "libatrac9.h"
#endif

#define GA_MAX_DIM       4096
#define GA_MAX_PSARC_TOC (16u * 1024u * 1024u)
#define GA_MAX_COVER_SZ  (16u * 1024u * 1024u)
#define GA_MAX_AUDIO_SZ  (16u * 1024u * 1024u)

static int ga_exists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0;
}

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static uint64_t be40(const uint8_t *p) {
    return ((uint64_t)p[0] << 32) | ((uint64_t)p[1] << 24) |
           ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 8) | (uint64_t)p[4];
}
static uint64_t be64(const uint8_t *p) {
    return ((uint64_t)be32(p) << 32) | (uint64_t)be32(p + 4);
}

static int path_eq_ci(const char *a, const char *b) {
    while (*a == '/') a++;
    while (*b == '/') b++;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

// ---------------------------------------------------------------- PSARC v1.4 (zlib)
static uint8_t *psarc_read_entry(FILE *f, const uint8_t *toc, uint32_t toc_entry_sz,
                                 const uint8_t *blk_tbl, uint32_t nblks, int zsize_bytes,
                                 uint32_t blk_sz, int is_zlib, uint32_t idx,
                                 size_t max_bytes, size_t *out_size) {
    const uint8_t *e = toc + (size_t)idx * toc_entry_sz;
    uint32_t blk_idx = be32(e + 16);
    uint64_t ulen64 = be40(e + 20);
    uint64_t off64 = be40(e + 25);
    if (ulen64 == 0 || ulen64 > (uint64_t)max_bytes) return NULL;
    size_t ulen = (size_t)ulen64;

    uint8_t *out = (uint8_t *)malloc(ulen + 1);
    uint8_t *raw = (uint8_t *)malloc(blk_sz);
    if (!out || !raw) {
        free(out);
        free(raw);
        return NULL;
    }

    if (fseeko(f, (off_t)off64, SEEK_SET) != 0) {
        free(out);
        free(raw);
        return NULL;
    }

    size_t written = 0;
    uint32_t bi = blk_idx;
    while (written < ulen) {
        if (bi >= nblks) { free(out); free(raw); return NULL; }
        uint32_t csz = 0;
        const uint8_t *bp = blk_tbl + (size_t)bi * (size_t)zsize_bytes;
        for (int k = 0; k < zsize_bytes; k++) csz = (csz << 8) | bp[k];
        bi++;

        size_t rem = ulen - written;
        size_t expected = rem < blk_sz ? rem : blk_sz;
        if (csz == 0 || csz > blk_sz) {
            if (csz > blk_sz) { free(out); free(raw); return NULL; }
            if (fread(out + written, 1, expected, f) != expected) {
                free(out);
                free(raw);
                return NULL;
            }
            written += expected;
        } else {
            if (fread(raw, 1, csz, f) != csz) {
                free(out);
                free(raw);
                return NULL;
            }
            if (is_zlib && csz >= 2 && raw[0] == 0x78) {
                uLongf dlen = (uLongf)rem;
                if (uncompress(out + written, &dlen, raw, (uLong)csz) == Z_OK && dlen > 0) {
                    written += (size_t)dlen;
                    continue;
                }
            }
            size_t copy = csz < rem ? csz : rem;
            memcpy(out + written, raw, copy);
            written += copy;
        }
    }

    free(raw);
    out[ulen] = 0;
    if (out_size) *out_size = ulen;
    return out;
}

uint8_t *ga_psarc_extract(const char *psarc_path, const char *target_path,
                          size_t max_bytes, size_t *out_size) {
    if (out_size) *out_size = 0;
    if (!psarc_path || !target_path || max_bytes == 0) return NULL;
    FILE *f = fopen(psarc_path, "rb");
    if (!f) return NULL;

    uint8_t hdr[32];
    if (fread(hdr, 1, 32, f) != 32 || memcmp(hdr, "PSAR", 4) != 0) {
        fclose(f);
        return NULL;
    }
    int is_zlib = (memcmp(hdr + 8, "zlib", 4) == 0);
    uint32_t toc_len = be32(hdr + 12);
    uint32_t toc_entry_sz = be32(hdr + 16);
    uint32_t num_files = be32(hdr + 20);
    uint32_t blk_sz = be32(hdr + 24);

    if (toc_entry_sz < 30 || num_files < 2 || blk_sz < 256 || blk_sz > (16u * 1024u * 1024u)) {
        fclose(f);
        return NULL;
    }
    if (toc_len < 32 || toc_len > GA_MAX_PSARC_TOC) {
        fclose(f);
        return NULL;
    }
    uint64_t entries_bytes = (uint64_t)num_files * (uint64_t)toc_entry_sz;
    if (entries_bytes > (uint64_t)(toc_len - 32)) {
        fclose(f);
        return NULL;
    }

    size_t toc_data_len = (size_t)(toc_len - 32);
    uint8_t *toc_buf = (uint8_t *)malloc(toc_data_len);
    if (!toc_buf) { fclose(f); return NULL; }
    if (fread(toc_buf, 1, toc_data_len, f) != toc_data_len) {
        free(toc_buf);
        fclose(f);
        return NULL;
    }

    int zsize_bytes = blk_sz <= 256 ? 1 : (blk_sz <= 65536 ? 2 : (blk_sz <= 16777216 ? 3 : 4));
    const uint8_t *blk_tbl = toc_buf + (size_t)entries_bytes;
    uint32_t nblks = (uint32_t)((toc_data_len - (size_t)entries_bytes) / (size_t)zsize_bytes);

    size_t man_sz = 0;
    uint8_t *manifest = psarc_read_entry(f, toc_buf, toc_entry_sz, blk_tbl, nblks,
                                         zsize_bytes, blk_sz, is_zlib, 0,
                                         4u * 1024u * 1024u, &man_sz);
    uint8_t *result = NULL;
    if (manifest) {
        uint32_t idx = 1;
        char *p = (char *)manifest;
        while (*p && idx < num_files) {
            char *nl = strchr(p, '\n');
            if (nl) *nl = 0;
            size_t ln = strlen(p);
            if (ln > 0 && p[ln - 1] == '\r') p[ln - 1] = 0;
            if (path_eq_ci(p, target_path)) {
                result = psarc_read_entry(f, toc_buf, toc_entry_sz, blk_tbl, nblks,
                                          zsize_bytes, blk_sz, is_zlib, idx,
                                          max_bytes, out_size);
                break;
            }
            if (!nl) break;
            p = nl + 1;
            idx++;
        }
        free(manifest);
    }

    free(toc_buf);
    fclose(f);
    return result;
}

// ---------------------------------------------------------------- capa (BGRA / PNG / TGA)
static void scale_rgba_bilinear(const uint8_t *src, int sw, int sh,
                                uint32_t *dst, int dw, int dh) {
    if (!src || !dst || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
    if (sw == dw && sh == dh) {
        for (int i = 0; i < dw * dh; i++) {
            const uint8_t *p = src + (size_t)i * 4;
            uint32_t a = p[3];
            uint32_t r = (p[0] * a) / 255u;
            uint32_t g = (p[1] * a) / 255u;
            uint32_t b = (p[2] * a) / 255u;
            dst[i] = r | (g << 8) | (b << 16) | 0xff000000u;
        }
        return;
    }
    for (int y = 0; y < dh; y++) {
        float fy = ((float)y + 0.5f) * (float)sh / (float)dh - 0.5f;
        if (fy < 0.f) fy = 0.f;
        int y0 = (int)fy;
        int y1 = y0 + 1;
        float wy = fy - (float)y0;
        if (y0 >= sh - 1) { y0 = y1 = sh - 1; wy = 0.f; }
        for (int x = 0; x < dw; x++) {
            float fx = ((float)x + 0.5f) * (float)sw / (float)dw - 0.5f;
            if (fx < 0.f) fx = 0.f;
            int x0 = (int)fx;
            int x1 = x0 + 1;
            float wx = fx - (float)x0;
            if (x0 >= sw - 1) { x0 = x1 = sw - 1; wx = 0.f; }

            const uint8_t *p00 = src + ((size_t)y0 * (size_t)sw + (size_t)x0) * 4;
            const uint8_t *p10 = src + ((size_t)y0 * (size_t)sw + (size_t)x1) * 4;
            const uint8_t *p01 = src + ((size_t)y1 * (size_t)sw + (size_t)x0) * 4;
            const uint8_t *p11 = src + ((size_t)y1 * (size_t)sw + (size_t)x1) * 4;

            float w00 = (1.f - wx) * (1.f - wy);
            float w10 = wx * (1.f - wy);
            float w01 = (1.f - wx) * wy;
            float w11 = wx * wy;

            float a = p00[3] * w00 + p10[3] * w10 + p01[3] * w01 + p11[3] * w11;
            float af = a / 255.f;
            int r = (int)((p00[0] * w00 + p10[0] * w10 + p01[0] * w01 + p11[0] * w11) * af + 0.5f);
            int g = (int)((p00[1] * w00 + p10[1] * w10 + p01[1] * w01 + p11[1] * w11) * af + 0.5f);
            int b = (int)((p00[2] * w00 + p10[2] * w10 + p01[2] * w01 + p11[2] * w11) * af + 0.5f);
            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;
            dst[(size_t)y * (size_t)dw + (size_t)x] =
                (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0xff000000u;
        }
    }
}

static int load_bgra_file(const char *path, uint32_t *out_rgba, int dst_w, int dst_h) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t need = (size_t)dst_w * (size_t)dst_h;
    size_t got = fread(out_rgba, 4, need, f);
    fclose(f);
    if (got != need) return 0;
    for (size_t i = 0; i < need; i++) {
        uint32_t v = out_rgba[i];
        out_rgba[i] = ((v >> 16) & 0xffu) | (v & 0xff00u) | ((v & 0xffu) << 16) | 0xff000000u;
    }
    return 1;
}

typedef struct {
    FILE *f;
    png_structp png;
    png_infop info;
    uint8_t *rgba;
    png_bytep *rows;
} PngLoadCtx;

static int load_png_file(const char *path, uint32_t *out_rgba, int dst_w, int dst_h) {
    PngLoadCtx ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.f = fopen(path, "rb");
    if (!ctx.f) return 0;

    uint8_t sig[8];
    if (fread(sig, 1, 8, ctx.f) != 8 || png_sig_cmp(sig, 0, 8) != 0) {
        fclose(ctx.f);
        return 0;
    }

    ctx.png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!ctx.png) { fclose(ctx.f); return 0; }
    ctx.info = png_create_info_struct(ctx.png);
    if (!ctx.info) {
        png_destroy_read_struct(&ctx.png, NULL, NULL);
        fclose(ctx.f);
        return 0;
    }

    int ok = 0;
    if (setjmp(png_jmpbuf(ctx.png)) == 0) {
        png_init_io(ctx.png, ctx.f);
        png_set_sig_bytes(ctx.png, 8);
        png_read_info(ctx.png, ctx.info);

        png_uint_32 w = png_get_image_width(ctx.png, ctx.info);
        png_uint_32 h = png_get_image_height(ctx.png, ctx.info);
        int bit_depth = png_get_bit_depth(ctx.png, ctx.info);
        int color_type = png_get_color_type(ctx.png, ctx.info);

        if (w > 0 && h > 0 && w <= GA_MAX_DIM && h <= GA_MAX_DIM) {
            if (bit_depth == 16) png_set_strip_16(ctx.png);
            if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(ctx.png);
            if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
                png_set_expand_gray_1_2_4_to_8(ctx.png);
            if (png_get_valid(ctx.png, ctx.info, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(ctx.png);
            if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                png_set_gray_to_rgb(ctx.png);
            if (color_type == PNG_COLOR_TYPE_RGB ||
                color_type == PNG_COLOR_TYPE_GRAY ||
                color_type == PNG_COLOR_TYPE_PALETTE)
                png_set_filler(ctx.png, 0xFF, PNG_FILLER_AFTER);
            png_read_update_info(ctx.png, ctx.info);

            ctx.rgba = (uint8_t *)malloc((size_t)w * (size_t)h * 4u);
            ctx.rows = (png_bytep *)malloc((size_t)h * sizeof(png_bytep));
            if (ctx.rgba && ctx.rows) {
                for (png_uint_32 y = 0; y < h; y++)
                    ctx.rows[y] = ctx.rgba + (size_t)y * (size_t)w * 4u;
                png_read_image(ctx.png, ctx.rows);
                png_read_end(ctx.png, NULL);
                scale_rgba_bilinear(ctx.rgba, (int)w, (int)h, out_rgba, dst_w, dst_h);
                ok = 1;
            }
        }
    }

    png_destroy_read_struct(&ctx.png, &ctx.info, NULL);
    free(ctx.rows);
    free(ctx.rgba);
    fclose(ctx.f);
    return ok;
}

static int decode_tga_raw(const uint8_t *buf, size_t len,
                          uint8_t **out_rgba, int *out_w, int *out_h) {
    if (!buf || len < 18 || !out_rgba || !out_w || !out_h) return 0;
    uint8_t id_len = buf[0];
    uint8_t cmap_type = buf[1];
    uint8_t img_type = buf[2];
    int w = (int)le16(buf + 12);
    int h = (int)le16(buf + 14);
    uint8_t bpp = buf[16];
    uint8_t desc = buf[17];
    if (cmap_type != 0 || img_type != 2 || (bpp != 24 && bpp != 32)) return 0;
    if (w <= 0 || h <= 0 || w > GA_MAX_DIM || h > GA_MAX_DIM) return 0;
    size_t px_bytes = (size_t)(bpp / 8);
    size_t hdr_end = 18u + (size_t)id_len;
    size_t need = (size_t)w * (size_t)h * px_bytes;
    if (hdr_end > len || need > len - hdr_end) return 0;

    uint8_t *rgba = (uint8_t *)malloc((size_t)w * (size_t)h * 4u);
    if (!rgba) return 0;

    const uint8_t *src = buf + hdr_end;
    int top_down = (desc & 0x20) != 0;
    for (int y = 0; y < h; y++) {
        int sy = top_down ? y : (h - 1 - y);
        const uint8_t *srow = src + (size_t)sy * (size_t)w * px_bytes;
        uint8_t *drow = rgba + (size_t)y * (size_t)w * 4u;
        for (int x = 0; x < w; x++) {
            drow[x * 4 + 0] = srow[x * px_bytes + 2]; // R
            drow[x * 4 + 1] = srow[x * px_bytes + 1]; // G
            drow[x * 4 + 2] = srow[x * px_bytes + 0]; // B
            drow[x * 4 + 3] = (px_bytes == 4) ? srow[x * px_bytes + 3] : 0xffu;
        }
    }
    *out_rgba = rgba;
    *out_w = w;
    *out_h = h;
    return 1;
}

static int decode_tga_mem(const uint8_t *buf, size_t len, uint32_t *out_rgba, int dst_w, int dst_h) {
    uint8_t *rgba = NULL;
    int w = 0, h = 0;
    if (!decode_tga_raw(buf, len, &rgba, &w, &h)) return 0;
    scale_rgba_bilinear(rgba, w, h, out_rgba, dst_w, dst_h);
    free(rgba);
    return 1;
}

typedef struct {
    const uint8_t *buf;
    size_t len;
    size_t pos;
} PngMemReader;

static void png_mem_read(png_structp png, png_bytep dst, png_size_t n) {
    PngMemReader *r = (PngMemReader *)png_get_io_ptr(png);
    if (!r || n > r->len - r->pos) {
        png_error(png, "EOF");
        return;
    }
    memcpy(dst, r->buf + r->pos, n);
    r->pos += n;
}

static int decode_png_raw(const uint8_t *buf, size_t len,
                          uint8_t **out_rgba, int *out_w, int *out_h) {
    if (!buf || len < 8 || !out_rgba || !out_w || !out_h) return 0;
    if (png_sig_cmp(buf, 0, 8) != 0) return 0;

    PngLoadCtx ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!ctx.png) return 0;
    ctx.info = png_create_info_struct(ctx.png);
    if (!ctx.info) {
        png_destroy_read_struct(&ctx.png, NULL, NULL);
        return 0;
    }

    PngMemReader rd = { buf, len, 8 };
    int ok = 0;
    int img_w = 0, img_h = 0;
    if (setjmp(png_jmpbuf(ctx.png)) == 0) {
        png_set_read_fn(ctx.png, &rd, png_mem_read);
        png_set_sig_bytes(ctx.png, 8);
        png_read_info(ctx.png, ctx.info);

        png_uint_32 w = png_get_image_width(ctx.png, ctx.info);
        png_uint_32 h = png_get_image_height(ctx.png, ctx.info);
        int bit_depth = png_get_bit_depth(ctx.png, ctx.info);
        int color_type = png_get_color_type(ctx.png, ctx.info);

        if (w > 0 && h > 0 && w <= GA_MAX_DIM && h <= GA_MAX_DIM) {
            if (bit_depth == 16) png_set_strip_16(ctx.png);
            if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(ctx.png);
            if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
                png_set_expand_gray_1_2_4_to_8(ctx.png);
            if (png_get_valid(ctx.png, ctx.info, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(ctx.png);
            if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                png_set_gray_to_rgb(ctx.png);
            if (color_type == PNG_COLOR_TYPE_RGB ||
                color_type == PNG_COLOR_TYPE_GRAY ||
                color_type == PNG_COLOR_TYPE_PALETTE)
                png_set_filler(ctx.png, 0xFF, PNG_FILLER_AFTER);
            png_read_update_info(ctx.png, ctx.info);

            ctx.rgba = (uint8_t *)malloc((size_t)w * (size_t)h * 4u);
            ctx.rows = (png_bytep *)malloc((size_t)h * sizeof(png_bytep));
            if (ctx.rgba && ctx.rows) {
                for (png_uint_32 y = 0; y < h; y++)
                    ctx.rows[y] = ctx.rgba + (size_t)y * (size_t)w * 4u;
                png_read_image(ctx.png, ctx.rows);
                png_read_end(ctx.png, NULL);
                img_w = (int)w;
                img_h = (int)h;
                ok = 1;
            }
        }
    }

    png_destroy_read_struct(&ctx.png, &ctx.info, NULL);
    free(ctx.rows);
    if (ok) {
        *out_rgba = ctx.rgba;
        *out_w = img_w;
        *out_h = img_h;
    } else {
        free(ctx.rgba);
    }
    return ok;
}

static uint32_t *composite_rgba_over_rgb(const uint8_t *rgba, int w, int h, uint32_t bg_rgb) {
    size_t n = (size_t)w * (size_t)h;
    uint32_t *out = (uint32_t *)malloc(n * 4u);
    if (!out) return NULL;
    uint32_t br = bg_rgb & 0xffu;
    uint32_t bg = (bg_rgb >> 8) & 0xffu;
    uint32_t bb = (bg_rgb >> 16) & 0xffu;
    for (size_t i = 0; i < n; i++) {
        const uint8_t *p = rgba + i * 4u;
        uint32_t a = p[3];
        uint32_t r = (a == 255u) ? p[0] : ((uint32_t)p[0] * a + br * (255u - a) + 127u) / 255u;
        uint32_t g = (a == 255u) ? p[1] : ((uint32_t)p[1] * a + bg * (255u - a) + 127u) / 255u;
        uint32_t b = (a == 255u) ? p[2] : ((uint32_t)p[2] * a + bb * (255u - a) + 127u) / 255u;
        out[i] = r | (g << 8) | (b << 16) | 0xff000000u;
    }
    return out;
}

static uint8_t *trp_extract_entry(const char *trp_path, const char *entry_name,
                                  size_t max_bytes, size_t *out_size) {
    if (out_size) *out_size = 0;
    FILE *f = fopen(trp_path, "rb");
    if (!f) return NULL;
    uint8_t h[0x40];
    uint8_t *res = NULL;
    if (fread(h, 1, sizeof h, f) == sizeof h && be32(h) == 0xDCA24D00u) {
        uint32_t n = be32(h + 16);
        uint32_t esz = be32(h + 20);
        if (esz >= 48 && esz <= 256) {
            uint8_t e[256];
            for (uint32_t i = 0; i < n && i < 256; i++) {
                if (fseeko(f, (off_t)(0x40u + (uint64_t)i * (uint64_t)esz), SEEK_SET) != 0 ||
                    fread(e, 1, esz, f) != esz)
                    break;
                if (strncmp((const char *)e, entry_name, 32) != 0) continue;
                uint64_t off = be64(e + 32);
                uint64_t sz = be64(e + 40);
                if (sz == 0 || sz > (uint64_t)max_bytes) break;
                uint8_t *buf = (uint8_t *)malloc((size_t)sz);
                if (buf && fseeko(f, (off_t)off, SEEK_SET) == 0 &&
                    fread(buf, 1, (size_t)sz, f) == (size_t)sz) {
                    res = buf;
                    if (out_size) *out_size = (size_t)sz;
                } else {
                    free(buf);
                }
                break;
            }
        }
    }
    fclose(f);
    return res;
}

int ga_load_game_card_rgba(const char *dir, int game_idx, uint32_t bg_rgb,
                           uint32_t **out_rgba, int *out_w, int *out_h) {
    if (!dir || game_idx < 0 || game_idx > 1 || !out_rgba || !out_w || !out_h) return 0;
    *out_rgba = NULL;
    *out_w = 0;
    *out_h = 0;

    char p[1024];
    uint8_t *raw_rgba = NULL;
    int w = 0, h = 0;

    // 1. Arte do seletor de jogos do Vita dentro de System.psarc
    snprintf(p, sizeof p, "%.800sgame/PCSA00126/System.psarc", dir);
    if (ga_exists(p)) {
        static const char *const kIcons[2][3] = {
            { "/System/Icon_GOW1_US.tga", "/System/Icon_GOW1_EU.tga", "/System/Icon_GOW1_JP.tga" },
            { "/System/Icon_GOW2_US.tga", "/System/Icon_GOW2_EU.tga", "/System/Icon_GOW2_JP.tga" },
        };
        for (size_t i = 0; i < 3; i++) {
            size_t sz = 0;
            uint8_t *tga = ga_psarc_extract(p, kIcons[game_idx][i], GA_MAX_COVER_SZ, &sz);
            if (tga) {
                int ok = decode_tga_raw(tga, sz, &raw_rgba, &w, &h);
                free(tga);
                if (ok) break;
            }
        }
    }

    // 2. Fallback: ICON0.PNG de dentro do TROPHY.TRP de cada jogo
    if (!raw_rgba) {
        const char *npwr = game_idx == 0 ? "NPWR05244_00" : "NPWR05283_00";
        snprintf(p, sizeof p, "%.800sgame/PCSA00126/sce_sys/trophy/%s/TROPHY.TRP", dir, npwr);
        size_t sz = 0;
        uint8_t *png = trp_extract_entry(p, "ICON0.PNG", GA_MAX_COVER_SZ, &sz);
        if (png) {
            decode_png_raw(png, sz, &raw_rgba, &w, &h);
            free(png);
        }
    }

    if (!raw_rgba) return 0;
    uint32_t *dst = composite_rgba_over_rgb(raw_rgba, w, h, bg_rgb);
    free(raw_rgba);
    if (!dst) return 0;
    *out_rgba = dst;
    *out_w = w;
    *out_h = h;
    return 1;
}

int ga_load_cover_rgba(const char *dir, int romfs_ativo,
                       uint32_t *out_rgba, int dst_w, int dst_h) {
    if (!dir || !out_rgba || dst_w <= 0 || dst_h <= 0) return 0;
    char p[1024];

    // 1. Override do usuário no SD: bg.bgra ou bg.png
    snprintf(p, sizeof p, "%.800sbg.bgra", dir);
    if (load_bgra_file(p, out_rgba, dst_w, dst_h)) return 1;

    snprintf(p, sizeof p, "%.800sbg.png", dir);
    if (load_png_file(p, out_rgba, dst_w, dst_h)) return 1;

    // 2. Fallback para romfs:/bg.bgra (builds customizados que embutem capa manualmente)
    if (romfs_ativo && load_bgra_file("romfs:/bg.bgra", out_rgba, dst_w, dst_h)) return 1;

    // 3. Arte oficial diretamente da cópia do jogo do usuário em game/PCSA00126/
    snprintf(p, sizeof p, "%.800sgame/PCSA00126/sce_sys/pic0.png", dir);
    if (load_png_file(p, out_rgba, dst_w, dst_h)) return 1;

    snprintf(p, sizeof p, "%.800sgame/PCSA00126/sce_sys/livearea/contents/backgrounds/bg0.png", dir);
    if (load_png_file(p, out_rgba, dst_w, dst_h)) return 1;

    // 4. Fallback adicional: fundo do seletor de jogos do Vita dentro de System.psarc
    snprintf(p, sizeof p, "%.800sgame/PCSA00126/System.psarc", dir);
    if (ga_exists(p)) {
        static const char *const kTgas[] = { "/System/AppSel_Bg_US.tga", "/System/AppSel_Bg_EU.tga" };
        for (size_t i = 0; i < sizeof kTgas / sizeof kTgas[0]; i++) {
            size_t sz = 0;
            uint8_t *tga = ga_psarc_extract(p, kTgas[i], GA_MAX_COVER_SZ, &sz);
            if (tga) {
                int ok = decode_tga_mem(tga, sz, out_rgba, dst_w, dst_h);
                free(tga);
                if (ok) return 1;
            }
        }
    }

    memset(out_rgba, 0, (size_t)dst_w * (size_t)dst_h * 4u);
    return 0;
}

int ga_game_dump_ok(const char *dir, int game_idx) {
    if (!dir || game_idx < 0 || game_idx > 1) return 0;
    const char *psarc = game_idx == 0 ? "gow1_psp2_SCEA.psarc" : "gow2_psp2_SCEA.psarc";
    char p[1024];
    snprintf(p, sizeof p, "%.800sgame/PCSA00126/exec/%s", dir, psarc);
    return ga_exists(p);
}

// ---------------------------------------------------------------- música (MP3 / WAV / ATRAC9)
enum { GA_AUDIO_MP3 = 1, GA_AUDIO_WAV = 2, GA_AUDIO_AT9 = 3 };

struct GaAudio {
    int kind;
    long rate;
    // MP3
    mpg123_handle *mh;
    int mpg_inited;
    // WAV (PCM 16-bit)
    FILE *wf;
    long wav_data_off;
    size_t wav_data_sz;
    size_t wav_pos;
    int wav_ch;
#ifdef ENABLE_ATRAC9
    // ATRAC9 (em memória, extraído de gow1_psp2_SCEA.psarc)
    void *at9_handle;
    uint8_t *at9_buf;
    size_t at9_data_off;
    size_t at9_data_sz;
    size_t at9_sf_pos;
    int at9_sf_size;
    int at9_sf_frames;
    int at9_frame_idx;
    size_t at9_in_sf_off;
    int at9_frame_samples;
    int at9_ch;
    int16_t *at9_dec_tmp;
    int16_t *at9_stash;
    size_t at9_stash_pos;
    size_t at9_stash_len;
#endif
};

static GaAudio *open_mp3(const char *path) {
    if (!ga_exists(path)) return NULL;
    mpg123_init();
    mpg123_handle *mh = mpg123_new(NULL, NULL);
    if (!mh) { mpg123_exit(); return NULL; }
    mpg123_param(mh, MPG123_FLAGS, MPG123_FORCE_STEREO | MPG123_QUIET, 0);
    if (mpg123_open(mh, path) != MPG123_OK) {
        mpg123_delete(mh);
        mpg123_exit();
        return NULL;
    }
    long rate = 44100;
    int ch = 2, enc = 0;
    if (mpg123_getformat(mh, &rate, &ch, &enc) != MPG123_OK || rate < 8000 || rate > 192000)
        rate = 44100;
    mpg123_format_none(mh);
    mpg123_format(mh, rate, MPG123_STEREO, MPG123_ENC_SIGNED_16);

    GaAudio *a = (GaAudio *)calloc(1, sizeof *a);
    if (!a) {
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return NULL;
    }
    a->kind = GA_AUDIO_MP3;
    a->rate = rate;
    a->mh = mh;
    a->mpg_inited = 1;
    return a;
}

static GaAudio *open_wav(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint8_t riff[12];
    if (fread(riff, 1, 12, f) != 12 || memcmp(riff, "RIFF", 4) != 0 || memcmp(riff + 8, "WAVE", 4) != 0) {
        fclose(f);
        return NULL;
    }
    int fmt_ok = 0, ch = 0, bps = 0;
    long rate = 0;
    long data_off = 0;
    size_t data_sz = 0;

    uint8_t chdr[8];
    while (fread(chdr, 1, 8, f) == 8) {
        uint32_t csz = le32(chdr + 4);
        long cpos = ftell(f);
        if (cpos < 0) break;
        if (memcmp(chdr, "fmt ", 4) == 0 && csz >= 16) {
            uint8_t fb[16];
            if (fread(fb, 1, 16, f) == 16) {
                uint16_t tag = le16(fb + 0);
                ch = (int)le16(fb + 2);
                rate = (long)le32(fb + 4);
                bps = (int)le16(fb + 14);
                if (tag == 1 && (ch == 1 || ch == 2) && bps == 16 && rate >= 8000 && rate <= 192000)
                    fmt_ok = 1;
            }
        } else if (memcmp(chdr, "data", 4) == 0 && csz > 0) {
            data_off = cpos;
            data_sz = (size_t)csz;
            if (fmt_ok) break;
        }
        long next = cpos + (long)csz + (long)(csz & 1u);
        if (fseek(f, next, SEEK_SET) != 0) break;
    }

    if (!fmt_ok || data_sz < (size_t)(ch * 2) || fseek(f, data_off, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    // Alinha o tamanho do bloco ao tamanho de 1 quadro completo
    size_t frame_bytes = (size_t)ch * 2u;
    data_sz -= data_sz % frame_bytes;
    if (data_sz == 0) { fclose(f); return NULL; }

    GaAudio *a = (GaAudio *)calloc(1, sizeof *a);
    if (!a) { fclose(f); return NULL; }
    a->kind = GA_AUDIO_WAV;
    a->rate = rate;
    a->wf = f;
    a->wav_data_off = data_off;
    a->wav_data_sz = data_sz;
    a->wav_pos = 0;
    a->wav_ch = ch;
    return a;
}

#ifdef ENABLE_ATRAC9
static const uint8_t kAtrac9Guid[16] = {
    0xD2, 0x42, 0xE1, 0x47, 0xBA, 0x36, 0x8D, 0x4D,
    0x88, 0xFC, 0x61, 0x65, 0x4F, 0x8C, 0x83, 0x6C
};

static GaAudio *open_at9_mem(uint8_t *buf, size_t len) {
    if (!buf || len < 64 || memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) {
        free(buf);
        return NULL;
    }
    int fmt_ok = 0;
    unsigned char cfg[4] = {0, 0, 0, 0};
    size_t data_off = 0, data_sz = 0;

    size_t pos = 12;
    while (pos + 8 <= len) {
        const uint8_t *ch = buf + pos;
        uint32_t csz = le32(ch + 4);
        size_t cstart = pos + 8;
        if (csz > len - cstart) break;
        if (memcmp(ch, "fmt ", 4) == 0 && csz >= 48) {
            uint16_t tag = le16(buf + cstart);
            if (tag == 0xFFFE && memcmp(buf + cstart + 24, kAtrac9Guid, 16) == 0 &&
                buf[cstart + 44] == 0xFE) {
                memcpy(cfg, buf + cstart + 44, 4);
                fmt_ok = 1;
            }
        } else if (memcmp(ch, "data", 4) == 0 && csz > 0) {
            data_off = cstart;
            data_sz = csz;
        }
        size_t adv = (size_t)csz + (size_t)(csz & 1u);
        if (adv > len - cstart) break;
        pos = cstart + adv;
    }

    if (!fmt_ok || data_sz == 0) { free(buf); return NULL; }

    void *h = Atrac9GetHandle();
    if (!h) { free(buf); return NULL; }
    if (Atrac9InitDecoder(h, cfg) != 0) {
        Atrac9ReleaseHandle(h);
        free(buf);
        return NULL;
    }
    Atrac9CodecInfo info;
    memset(&info, 0, sizeof info);
    if (Atrac9GetCodecInfo(h, &info) != 0 ||
        info.superframeSize <= 0 || info.framesInSuperframe <= 0 ||
        info.frameSamples <= 0 || info.frameSamples > 4096 ||
        info.channels < 1 || info.channels > 8 ||
        info.samplingRate < 8000 || info.samplingRate > 192000 ||
        data_sz < (size_t)info.superframeSize) {
        Atrac9ReleaseHandle(h);
        free(buf);
        return NULL;
    }

    GaAudio *a = (GaAudio *)calloc(1, sizeof *a);
    int16_t *dec_tmp = (int16_t *)malloc((size_t)info.frameSamples * (size_t)info.channels * sizeof(int16_t));
    int16_t *stash = (int16_t *)malloc((size_t)info.frameSamples * 2u * sizeof(int16_t));
    if (!a || !dec_tmp || !stash) {
        free(a);
        free(dec_tmp);
        free(stash);
        Atrac9ReleaseHandle(h);
        free(buf);
        return NULL;
    }
    a->kind = GA_AUDIO_AT9;
    a->rate = (long)info.samplingRate;
    a->at9_handle = h;
    a->at9_buf = buf;
    a->at9_data_off = data_off;
    a->at9_data_sz = data_sz;
    a->at9_sf_pos = 0;
    a->at9_sf_size = info.superframeSize;
    a->at9_sf_frames = info.framesInSuperframe;
    a->at9_frame_idx = 0;
    a->at9_in_sf_off = 0;
    a->at9_frame_samples = info.frameSamples;
    a->at9_ch = info.channels;
    a->at9_dec_tmp = dec_tmp;
    a->at9_stash = stash;
    return a;
}

static GaAudio *open_at9_from_game(const char *dir) {
    char psarc[1024];
    snprintf(psarc, sizeof psarc, "%.800sgame/PCSA00126/exec/gow1_psp2_SCEA.psarc", dir);
    if (!ga_exists(psarc)) return NULL;
    static const char *const kTracks[] = {
        "wad/_sound/dvd_streams/PSP2/tit1e1_2.at9",
        "wad/_sound/dvd_streams/PSP2/credits1.at9",
    };
    for (size_t i = 0; i < sizeof kTracks / sizeof kTracks[0]; i++) {
        size_t sz = 0;
        uint8_t *buf = ga_psarc_extract(psarc, kTracks[i], GA_MAX_AUDIO_SZ, &sz);
        if (!buf) continue;
        GaAudio *a = open_at9_mem(buf, sz);
        if (a) return a;
    }
    return NULL;
}
#endif

GaAudio *ga_audio_open(const char *dir, int romfs_ativo) {
    if (!dir) return NULL;
    char p[1024];
    GaAudio *a = NULL;

    // 1. Overrides do usuário no SD
    snprintf(p, sizeof p, "%.800smusic.mp3", dir);
    if ((a = open_mp3(p)) != NULL) return a;

    snprintf(p, sizeof p, "%.800smusic.wav", dir);
    if ((a = open_wav(p)) != NULL) return a;

    // 2. Fallback para romfs:/music.mp3 (builds customizados)
    if (romfs_ativo && (a = open_mp3("romfs:/music.mp3")) != NULL) return a;

    // 3. Tema oficial diretamente da cópia legal em game/PCSA00126/
#ifdef ENABLE_ATRAC9
    if ((a = open_at9_from_game(dir)) != NULL) return a;
#endif
    return NULL;
}

long ga_audio_rate(const GaAudio *a) {
    return a ? a->rate : 48000;
}

size_t ga_audio_read_stereo_s16(GaAudio *a, int16_t *dst, size_t max_frames) {
    if (!a || !dst || max_frames == 0) return 0;

    if (a->kind == GA_AUDIO_MP3) {
        size_t got = 0;
        int r = mpg123_read(a->mh, dst, max_frames * 4u, &got);
        if (r == MPG123_DONE) mpg123_seek(a->mh, 0, SEEK_SET);
        return got / 4u;
    }

    if (a->kind == GA_AUDIO_WAV) {
        size_t frame_bytes = (size_t)a->wav_ch * 2u;
        size_t rem_bytes = a->wav_data_sz - a->wav_pos;
        if (rem_bytes < frame_bytes) {
            if (fseek(a->wf, a->wav_data_off, SEEK_SET) != 0) return 0;
            a->wav_pos = 0;
            rem_bytes = a->wav_data_sz;
        }
        size_t want_frames = rem_bytes / frame_bytes;
        if (want_frames > max_frames) want_frames = max_frames;
        if (a->wav_ch == 2) {
            size_t got = fread(dst, 4u, want_frames, a->wf);
            a->wav_pos += got * 4u;
            if (a->wav_pos >= a->wav_data_sz) {
                fseek(a->wf, a->wav_data_off, SEEK_SET);
                a->wav_pos = 0;
            }
            return got;
        } else {
            size_t got = fread(dst, 2u, want_frames, a->wf);
            a->wav_pos += got * 2u;
            for (size_t i = got; i > 0; i--) {
                int16_t s = dst[i - 1];
                dst[(i - 1) * 2] = s;
                dst[(i - 1) * 2 + 1] = s;
            }
            if (a->wav_pos >= a->wav_data_sz) {
                fseek(a->wf, a->wav_data_off, SEEK_SET);
                a->wav_pos = 0;
            }
            return got;
        }
    }

#ifdef ENABLE_ATRAC9
    if (a->kind == GA_AUDIO_AT9) {
        size_t total = 0;
        int errs = 0;
        while (total < max_frames) {
            if (a->at9_stash_pos < a->at9_stash_len) {
                size_t avail = a->at9_stash_len - a->at9_stash_pos;
                size_t take = max_frames - total;
                if (take > avail) take = avail;
                memcpy(dst + total * 2u, a->at9_stash + a->at9_stash_pos * 2u, take * 4u);
                a->at9_stash_pos += take;
                total += take;
                continue;
            }
            if (a->at9_sf_pos + (size_t)a->at9_sf_size > a->at9_data_sz) {
                a->at9_sf_pos = 0;
                a->at9_frame_idx = 0;
                a->at9_in_sf_off = 0;
            }
            size_t byte_off = a->at9_data_off + a->at9_sf_pos + a->at9_in_sf_off;
            if (a->at9_in_sf_off >= (size_t)a->at9_sf_size) {
                a->at9_sf_pos += (size_t)a->at9_sf_size;
                a->at9_frame_idx = 0;
                a->at9_in_sf_off = 0;
                continue;
            }
            int used = 0;
            int err = Atrac9Decode(a->at9_handle, a->at9_buf + byte_off, a->at9_dec_tmp, &used);
            if (err != 0 || used <= 0) {
                a->at9_sf_pos += (size_t)a->at9_sf_size;
                a->at9_frame_idx = 0;
                a->at9_in_sf_off = 0;
                if (++errs > 8) break;
                continue;
            }
            a->at9_in_sf_off += (size_t)used;
            if (++a->at9_frame_idx >= a->at9_sf_frames) {
                a->at9_sf_pos += (size_t)a->at9_sf_size;
                a->at9_frame_idx = 0;
                a->at9_in_sf_off = 0;
            }
            int ns = a->at9_frame_samples;
            int ch = a->at9_ch;
            for (int i = 0; i < ns; i++) {
                if (ch == 1) {
                    int16_t s = a->at9_dec_tmp[i];
                    a->at9_stash[i * 2] = s;
                    a->at9_stash[i * 2 + 1] = s;
                } else {
                    a->at9_stash[i * 2] = a->at9_dec_tmp[i * ch];
                    a->at9_stash[i * 2 + 1] = a->at9_dec_tmp[i * ch + 1];
                }
            }
            a->at9_stash_pos = 0;
            a->at9_stash_len = (size_t)ns;
        }
        return total;
    }
#endif

    return 0;
}

void ga_audio_close(GaAudio *a) {
    if (!a) return;
    if (a->kind == GA_AUDIO_MP3) {
        if (a->mh) { mpg123_close(a->mh); mpg123_delete(a->mh); }
        if (a->mpg_inited) mpg123_exit();
    } else if (a->kind == GA_AUDIO_WAV) {
        if (a->wf) fclose(a->wf);
    }
#ifdef ENABLE_ATRAC9
    else if (a->kind == GA_AUDIO_AT9) {
        if (a->at9_handle) Atrac9ReleaseHandle(a->at9_handle);
        free(a->at9_buf);
        free(a->at9_dec_tmp);
        free(a->at9_stash);
    }
#endif
    free(a);
}
