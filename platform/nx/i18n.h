// Textos da interface (launcher e mod menu) carregados de arquivos por idioma.
//
// Cada idioma é um arquivo lang/<código>.ini com linhas "chave = texto". Os idiomas de lang/ vão embutidos
// no binário (i18n_data.c, gerado por tools/gen_i18n.py) e um <código>.ini numa pasta do SD substitui ou
// acrescenta textos (ou carrega um idioma novo, que fica disponível em i18n_count()/i18n_code()). Chave ausente
// cai no inglês e, se nem ele tiver, devolve a própria chave. Textos do SD com especificadores de formato (%s,
// %d...) diferentes dos do inglês são ignorados.
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *code;   // "en", "pt", "es"...
    const char *text;   // conteúdo do .ini
} I18nBuiltin;

// Registra os idiomas embutidos e procura <código>.ini em sd_dir (pode ser NULL). Pode ser chamada de novo,
// mas libera os textos anteriores: ponteiros devolvidos antes por tr() deixam de valer.
void i18n_init(const char *sd_dir);

// Idiomas disponíveis (embutidos + SD), sem repetição, na ordem: embutidos e depois os do SD por código.
int i18n_count(void);
const char *i18n_code(int i);
const char *i18n_name(int i);   // valor da chave "_name" do idioma, ou o código
int i18n_find(const char *code); // índice do idioma pelo código (aceita "pt-BR" ~ "pt"), -1 se não houver

// Troca o idioma ativo. Devolve 0 se o idioma existe; senão fica no inglês e devolve -1.
int i18n_set(const char *code);
const char *i18n_current(void);

// Texto da chave no idioma ativo, com recuo para o inglês e depois para a própria chave. Nunca devolve NULL.
const char *i18n_tr(const char *key);
// Atalho local (static inline) para não exportar um símbolo global "tr" junto do código do jogo.
static inline const char *tr(const char *key) { return i18n_tr(key); }

// Para testes: carrega um idioma a partir de um texto em memória, como se fosse um arquivo do SD.
int i18n_add_text(const char *code, const char *text, size_t len);
void i18n_reset(void);

#ifdef __cplusplus
}
#endif
