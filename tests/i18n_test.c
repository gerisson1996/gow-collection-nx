// Testes do leitor de idiomas (platform/nx/i18n.c), compilados no PC: ver tests/run_i18n_tests.sh.
#include "i18n.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

// Idiomas "embutidos" só para os testes (no launcher e no jogo vêm de i18n_data.c).
const I18nBuiltin i18n_builtins[] = {
    { "en", "_name = English\nhello = Hello\nonly_en = Only in English\nfmt = %d of %s\n" },
    { "pt", "_name = Português (BR)\nhello = Olá\nfmt = %d de %s\n" },
};
const int i18n_builtins_count = 2;

static int failures, checks;

#define CHECK_STR(got, want)                                                                        \
    do {                                                                                            \
        const char *g_ = (got), *w_ = (want);                                                       \
        checks++;                                                                                   \
        if (!g_ || strcmp(g_, w_)) {                                                                \
            failures++;                                                                             \
            printf("FAIL %s:%d: %s -> \"%s\", esperado \"%s\"\n", __FILE__, __LINE__, #got,         \
                   g_ ? g_ : "(null)", w_);                                                         \
        }                                                                                           \
    } while (0)

#define CHECK(cond)                                                                                 \
    do {                                                                                            \
        checks++;                                                                                   \
        if (!(cond)) { failures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); }         \
    } while (0)

static void add(const char *code, const char *text) { CHECK(i18n_add_text(code, text, strlen(text)) == 0); }

static void test_parsing(void) {
    i18n_reset();
    add("en",
        "\xEF\xBB\xBF# comentário\r\n"
        "; outro comentário\n"
        "\n"
        "   spaced   =   value with spaces   \r\n"
        "quoted = \"  keep  \"\n"
        "escapes = a\\nb\\tc\\\\d\\\"e\n"
        "equals = a = b = c\n"
        "empty =\n"
        "no equals sign here\n"
        " = no key\n"
        "utf8 = ação Ñandú 日本\n"
        "dup = first\n"
        "dup = second\n"
        "glyphs = Press {A} or {PLUS}, {Z} stays\n"
        "last = no newline at end");
    i18n_set("en");
    CHECK_STR(tr("spaced"), "value with spaces");
    CHECK_STR(tr("quoted"), "  keep  ");
    CHECK_STR(tr("escapes"), "a\nb\tc\\d\"e");
    CHECK_STR(tr("equals"), "a = b = c");
    CHECK_STR(tr("empty"), "");
    CHECK_STR(tr("utf8"), "ação Ñandú 日本");
    CHECK_STR(tr("dup"), "second");
    CHECK_STR(tr("glyphs"), "Press \xEE\x83\xA0 or \xEE\x83\xAF, {Z} stays");
    CHECK_STR(tr("last"), "no newline at end");
    CHECK_STR(tr("no equals sign here"), "no equals sign here");  // ignorada: volta a chave
    CHECK_STR(tr(""), "");
}

static void test_fallback_and_override(void) {
    i18n_init(NULL);
    CHECK(i18n_count() == 2);
    CHECK(i18n_set("pt") == 0);
    CHECK_STR(tr("hello"), "Olá");
    CHECK_STR(tr("only_en"), "Only in English");   // recua para o inglês
    CHECK_STR(tr("missing.key"), "missing.key");   // nem o inglês tem: devolve a chave
    CHECK_STR(i18n_name(i18n_find("pt")), "Português (BR)");

    add("pt", "hello = Oi\nnew = Nova\n");          // arquivo do SD por cima do embutido
    CHECK_STR(tr("hello"), "Oi");
    CHECK_STR(tr("new"), "Nova");
    CHECK_STR(tr("fmt"), "%d de %s");               // o resto do embutido continua
}

static void test_format_guard(void) {
    i18n_init(NULL);
    i18n_set("pt");
    // o inglês embutido tem "fmt = %d of %s": textos do SD com outros especificadores são ignorados
    add("pt", "fmt = %s de %d\nhello = Olá %s\n");
    CHECK_STR(tr("fmt"), "%d de %s");
    CHECK_STR(tr("hello"), "Olá");
    add("pt", "fmt = %2d de 100%% %-5s\n");               // largura e %% não mudam a assinatura
    CHECK_STR(tr("fmt"), "%2d de 100%% %-5s");
    add("en", "fmt = %d of %s!\nfmt2 = %ld\n");
    i18n_set("en");
    CHECK_STR(tr("fmt"), "%d of %s!");                    // o próprio inglês do SD também é conferido
    CHECK_STR(tr("fmt2"), "%ld");                         // chave nova sem referência: aceita
}

static void test_limits(void) {
    i18n_reset();
    int before = i18n_count();
    CHECK(i18n_add_text("xx", "a = b", (size_t)600 * 1024) == -1); // maior que o limite
    CHECK(i18n_count() == before);                        // não deixa um idioma vazio na lista
}

static void test_codes(void) {
    i18n_init(NULL);
    CHECK(i18n_find("pt") == i18n_find("pt-BR"));
    CHECK(i18n_find("PT") >= 0);
    CHECK(i18n_find("pt_br") >= 0);
    CHECK(i18n_find("fr") == -1);
    CHECK(i18n_find("") == -1);
    CHECK(i18n_find(NULL) == -1);
    CHECK(i18n_set("fr") == -1);
    CHECK_STR(i18n_current(), "en");                // idioma inexistente fica no inglês
    CHECK_STR(tr("hello"), "Hello");
    CHECK(i18n_set("pt-BR") == 0);
    CHECK_STR(i18n_current(), "pt");
    add("es-419", "_name = Español (Latinoamérica)\nhello = Hola\n");
    CHECK(i18n_find("es") == i18n_find("es-419"));
    CHECK_STR(i18n_name(i18n_find("es")), "Español (Latinoamérica)");
    CHECK_STR(i18n_name(99), "");
    CHECK_STR(i18n_code(-1), "");
}

static void test_many_keys(void) {
    i18n_reset();
    size_t cap = 64 * 1024, len = 0;
    char *buf = malloc(cap);
    for (int i = 999; i >= 0; i--) len += (size_t)snprintf(buf + len, cap - len, "k%03d = v%d\n", i, i);
    CHECK(i18n_add_text("en", buf, len) == 0);
    free(buf);
    i18n_set("en");
    char key[16], want[16];
    int bad = 0;
    for (int i = 0; i < 1000; i++) {
        snprintf(key, sizeof key, "k%03d", i);
        snprintf(want, sizeof want, "v%d", i);
        if (strcmp(tr(key), want)) bad++;
    }
    CHECK(bad == 0);
}

static void write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "wb");
    CHECK(f != NULL);
    if (f) { fputs(text, f); fclose(f); }
}

static void test_sd_folder(void) {
    const char *dir = "i18n_test_sd";
    MKDIR(dir);
    write_file("i18n_test_sd/fr.ini", "_name = Français\nhello = Bonjour\n");
    write_file("i18n_test_sd/en.ini", "hello = Hi from SD\n");
    write_file("i18n_test_sd/readme.txt", "não é idioma\n");
    write_file("i18n_test_sd/en.backup.ini", "hello = backup\n");
    i18n_init(NULL);
    CHECK(i18n_set("pt") == 0);
    i18n_init(dir);
    CHECK(i18n_count() == 3);                       // en, pt (embutidos) e fr (SD)
    CHECK_STR(i18n_code(0), "en");
    CHECK_STR(i18n_code(1), "pt");
    CHECK_STR(i18n_code(2), "fr");
    CHECK_STR(i18n_current(), "pt");                // init mantém o idioma ativo
    CHECK(i18n_set("fr") == 0);
    CHECK_STR(tr("hello"), "Bonjour");
    CHECK_STR(tr("only_en"), "Only in English");
    i18n_set("en");
    CHECK_STR(tr("hello"), "Hi from SD");           // o SD substitui o embutido
    i18n_init("pasta_que_nao_existe");
    CHECK(i18n_count() == 2);
    remove("i18n_test_sd/fr.ini");
    remove("i18n_test_sd/en.ini");
    remove("i18n_test_sd/readme.txt");
    remove("i18n_test_sd/en.backup.ini");
}

int main(void) {
    test_parsing();
    test_fallback_and_override();
    test_format_guard();
    test_limits();
    test_codes();
    test_many_keys();
    test_sd_folder();
    i18n_reset();
    printf("%d/%d verificações ok\n", checks - failures, checks);
    return failures ? 1 : 0;
}
