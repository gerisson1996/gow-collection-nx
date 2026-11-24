#!/usr/bin/env python3
"""Gera platform/nx/i18n_data.c a partir de lang/*.ini (idiomas embutidos no launcher e no jogo).

    python3 tools/gen_i18n.py           # regenera
    python3 tools/gen_i18n.py --check   # falha se o arquivo gerado estiver desatualizado ou faltar chave

Também confere que todo idioma tem as mesmas chaves do inglês e os mesmos especificadores de formato (%s, %d...).
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
LANG_DIR = ROOT / "lang"
OUT = ROOT / "platform" / "nx" / "i18n_data.c"
FMT = re.compile(r"%[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|z)?[diouxXsc%]")
ORDER = ["en", "pt", "es"]  # os primeiros; os demais seguem em ordem alfabética
SOURCES = ["launcher/main.c", "platform/nx/nx_menu.cpp"]
AREAS = "common|launcher|options|opt|val|game|trophies|updater|menu"
KEY_LITERAL = re.compile(r'"((?:' + AREAS + r')\.[a-z0-9_]+(?:\.[a-z0-9_]+)*)"')


def parse(path):
    keys = {}
    for n, raw in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        line = raw.strip()
        if not line or line[0] in "#;":
            continue
        if "=" not in line:
            sys.exit(f"{path.name}:{n}: linha sem '='")
        k, v = (s.strip() for s in line.split("=", 1))
        if not k:
            sys.exit(f"{path.name}:{n}: chave vazia")
        if k in keys:
            sys.exit(f"{path.name}:{n}: chave repetida '{k}'")
        if len(v) >= 2 and v[0] == v[-1] == '"':
            v = v[1:-1]
        keys[k] = v
    return keys


def c_string(text):
    out = []
    for line in text.splitlines(keepends=True):
        esc = line.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\t", "\\t")
        out.append(f'    "{esc}"')
    return "\n".join(out) if out else '    ""'


def main():
    files = sorted(LANG_DIR.glob("*.ini"), key=lambda p: (ORDER.index(p.stem) if p.stem in ORDER else len(ORDER), p.stem))
    if not files or files[0].stem != "en":
        sys.exit("lang/en.ini é obrigatório (é o idioma de recuo)")
    data = {p.stem: parse(p) for p in files}
    en = data["en"]
    problems = []
    for code, keys in data.items():
        if code == "en":
            continue
        for k in en.keys() - keys.keys():
            problems.append(f"{code}.ini: falta '{k}'")
        for k in keys.keys() - en.keys():
            problems.append(f"{code}.ini: chave '{k}' não existe em en.ini")
        for k in en.keys() & keys.keys():
            if FMT.findall(en[k]) != FMT.findall(keys[k]):
                problems.append(f"{code}.ini: '{k}' tem formato diferente do inglês ({FMT.findall(keys[k])} != {FMT.findall(en[k])})")
    # Toda chave usada no código tem de existir no inglês (literais "area.nome" das áreas conhecidas).
    used = set()
    for src in SOURCES:
        for line in (ROOT / src).read_text(encoding="utf-8").splitlines():
            if not line.lstrip().startswith("#include"):
                used.update(KEY_LITERAL.findall(line))
    for k in sorted(used - en.keys()):
        problems.append(f"en.ini: falta '{k}', usada no código")
    if problems:
        sys.exit("\n".join(sorted(problems)))

    parts = ["// Gerado por tools/gen_i18n.py a partir de lang/*.ini. Não edite à mão.",
             '#include "i18n.h"', "", "const I18nBuiltin i18n_builtins[] = {"]
    for p in files:
        parts.append(f'    {{ "{p.stem}",\n{c_string(p.read_text(encoding="utf-8-sig"))} }},')
    parts += ["};", f"const int i18n_builtins_count = {len(files)};", ""]
    text = "\n".join(parts)

    if "--check" in sys.argv:
        if not OUT.exists() or OUT.read_text(encoding="utf-8") != text:
            sys.exit("platform/nx/i18n_data.c está desatualizado: rode python3 tools/gen_i18n.py")
        print(f"ok: {len(files)} idiomas, {len(en)} chaves")
        return
    OUT.write_text(text, encoding="utf-8", newline="\n")
    print(f"gerado {OUT.relative_to(ROOT)}: {len(files)} idiomas, {len(en)} chaves")


if __name__ == "__main__":
    main()
