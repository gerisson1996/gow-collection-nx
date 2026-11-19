# Guia do usuário

## Abrindo

1. Abra o Homebrew Menu **segurando R ao abrir um jogo** (title takeover). Pelo Álbum (modo applet) a memória não é
   suficiente.
2. Abra **God of War Collection** (`gow-nx.nro`).
3. Escolha o jogo, **Troféus**, **Opções** ou **Buscar atualizações** com **Cima/Baixo** e aperte **A**. O **+** sai.

Em **Troféus**, o launcher lê o `TROPHY.TRP` do seu jogo (`game/PCSA00126/sce_sys/trophy/`) e o progresso gravado pelo
runtime em `data/trophy/<NPWR>/unlocked.txt`, mostrando ícones, porcentagem, filtros (**X**), troféus ocultos (**Y**)
e troca entre *God of War* e *God of War II* (**L/R**).

Em **Opções** você configura o idioma (detecção automática do idioma do Switch ou seleção manual entre inglês, espanhol,
português brasileiro, francês, italiano e alemão) e as opções gráficas compartilhadas entre os dois jogos (resolução
interna, taxa de quadros, VSync, proporção da tela, contador de FPS, texturas HD e barras de vida dos inimigos). Use
**Esquerda/Direita** ou **A** para mudar e **B** para salvar em `data/vitart_options.ini`.

Em **Buscar atualizações** (quando compilado com `switch-curl`), o launcher consulta o último release no GitHub, baixa
e instala a atualização sem tocar nos seus saves/opções (`data/`), arquivos do jogo (`game/`), `logs/` nem
`launcher.cfg`, e reinicia o launcher automaticamente.

O launcher lembra a última escolha (`launcher.cfg`, na mesma pasta) e carrega o NRO do jogo pelo Homebrew Menu. Ao
sair do jogo você volta ao Homebrew Menu. Também dá para abrir `gow1.nro` ou `gow2.nro` direto.

Sem `bg.bgra`/`music.mp3` no romfs nem na pasta do launcher no SD, o launcher abre com fundo preto e sem música.

## Opções no jogo

A qualquer momento durante o jogo, segure **R** e aperte **+** para abrir o **NX Menu** (resolução interna, taxa de
quadros, VSync, contador de FPS, texturas HD e barras de vida dos inimigos), exibido no mesmo idioma escolhido no
launcher (ou no idioma do Switch).

No menu de pausa, **Options** (precisa do mod de menu do
[GOD-OF-WAR-HD-COLLECTION](https://github.com/luisxl15/GOD-OF-WAR-HD-COLLECTION) em `data/mods`):

| Opção | Valores | Observação |
|---|---|---|
| Resolution | 1x a 4x | 1x = 720p. **2x precisa da GPU a 614 MHz** |
| Frame Rate | 30 / 60 / máx | 30 é o recomendado e o testado |
| VSync | liga / desliga | |
| HD Textures | liga / desliga | |
| PS2 Audio | liga / desliga | |
| Auto QTE | liga / desliga | |
| FPS Counter | liga / desliga | |
| Cheats | vida, magia e fúria infinitas | |

As opções ficam em `data/vitart_options.ini`. A resolução do Switch fica em `resolution_switch=`, a preferência do
launcher em `launcher_language=` (`auto` ou código do idioma) e o idioma resolvido em `language=` (`es`, `pt-BR`, `fr`,
`it`, `de`; omitido em inglês).

## Relógios (clocks)

| Uso | CPU | GPU |
|---|---|---|
| Resolução 1x | 1020 MHz (padrão) | padrão |
| Resolução 2x | 1020 MHz | 614 MHz |

Overclock é por sua conta (sys-clk ou similar). Não é preciso subir a CPU.

## Saves

Os saves e as opções ficam em `sdmc:/switch/gow-nx/data/`. Faça backup dessa pasta antes de atualizar.

## Logs e relato de problemas

- `logs/vitart_gow1.log` / `logs/vitart_gow2.log`: log do runtime (início, módulos, erros).
- `logs/perfil_<jogo>.txt`: amostras do perfilador embutido (onde `main` e `Render` passam o tempo).

Mande os dois arquivos junto com a descrição do problema. Veja [CONTRIBUTING.md](../CONTRIBUTING.md).

## Problemas comuns

| Sintoma | Causa provável |
|---|---|
| O jogo fecha logo ao abrir | Homebrew Menu aberto pelo Álbum (modo applet). Use o title takeover |
| Tela preta, log fala de arquivo faltando | `game/PCSA00126` incompleto ou no lugar errado |
| Gráficos faltando | `shaders/shaders.pak` ausente ou de outra versão |
| Lento em 2x | GPU no padrão. Suba a GPU para 614 MHz ou volte para 1x |
| Sem o menu Options | Mod de menu ausente em `data/mods` |
