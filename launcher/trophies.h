#pragma once
#include <stddef.h>
#include <stdint.h>

#define TRP_MAX_TROPHIES 128
#define TRP_ICON_SZ      52

typedef struct {
    int id;
    int hidden;
    int unlocked;
    char grade;          // 'P', 'G', 'S', 'B'
    char name[128];
    char detail[384];
    uint32_t *icon;      // TRP_ICON_SZ * TRP_ICON_SZ RGBA (R | G<<8 | B<<16 | A<<24), opcional
} TrophyItem;

typedef struct {
    int loaded;
    int count;
    int unlocked_count;
    int pts_have;
    int pts_total;
    int grade_have[4];   // 0=P, 1=G, 2=S, 3=B
    int grade_total[4];
    TrophyItem items[TRP_MAX_TROPHIES];
} TrophySet;

// Carrega o TROPHY.TRP e o unlocked.txt de um jogo (comm = "NPWR05244_00" ou "NPWR05283_00").
// lang_code: "en", "es", "pt-BR", "fr", "it", "de".
// Retorna 1 se carregou ao menos 1 troféu, 0 se o TROPHY.TRP não existe ou é inválido.
int trp_load(TrophySet *set, const char *base_dir, const char *comm, const char *lang_code);

// Libera memória dos ícones decodificados.
void trp_free(TrophySet *set);

// Porcentagem estilo PlayStation (0..100).
int trp_percent(const TrophySet *set);

// Auxiliares puros expostos para testes unitários:
int trp_parse_sfm(const char *xml, size_t xml_len, TrophySet *set);
void trp_apply_unlocked(TrophySet *set, const char *unlocked_text);
