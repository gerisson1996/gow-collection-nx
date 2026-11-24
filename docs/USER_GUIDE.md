# Guia do usuário

## Abrindo

1. Abra o Homebrew Menu **segurando R ao abrir um jogo** (title takeover). Pelo Álbum (modo applet) a memória não é
   suficiente.
2. Abra **God of War Collection** (`gow-nx.nro`).
3. Escolha o jogo com **Esquerda/Direita** e aperte **A** para jogar.

Também dá para abrir `gow1.nro` ou `gow2.nro` direto pelo Homebrew Menu; nesse caso valem as opções já gravadas em
`data/vitart_options.ini`. Ao sair do jogo você volta ao Homebrew Menu.

## Launcher

| Botão | Ação |
|---|---|
| **Esquerda / Direita** | Escolher o jogo |
| **A** | Jogar |
| **Y** | Opções |
| **L / R** | Preset rápido (veja abaixo) |
| **X** | Troféus |
| **−** | Atualizador |
| **+** | Sair |

O launcher lembra o último jogo escolhido (`launcher.cfg`, na mesma pasta), mostra a bateria, verifica se os arquivos
em `game/PCSA00126/exec/` estão presentes e carrega automaticamente o fundo (`sce_sys/pic0.png`), as capas dos cards
(`System.psarc` / `TROPHY.TRP`) e a música tema (`tit1e1_2.at9` de `gow1_psp2_SCEA.psarc`) da sua cópia em
`game/PCSA00126/`. Para personalizar, basta colocar `bg.png`/`bg.bgra`, `music.mp3`/`music.wav` ou
`data/launcher/capa_gow1.*`/`capa_gow2.*` na pasta do launcher no cartão SD.

### Presets rápidos (L / R)

| Preset | Resolução | Quadros |
|---|---|---|
| **Desempenho** | Vita (960x544) | 60 |
| **Equilibrado** | 720p | 60 |
| **Qualidade** | 2x (1440x816) | 30 estáveis |

O preset só troca a resolução do portátil e a taxa de quadros; o resto das opções fica como está. Se você mudar uma
delas à mão, o launcher mostra **Personalizado**.

### Opções (Y)

**Cima/Baixo** escolhe, **Esquerda/Direita** ou **A** muda, **L/R** troca de aba, **X** volta ao padrão da opção e
**B** grava em `data/vitart_options.ini` e volta. As mesmas opções valem para os dois jogos.

**Imagem**

| Opção | Chave | Valores |
|---|---|---|
| Renderizador | `renderer=` | `auto`, `deko3d`, `vulkan` (veja [Renderizador](#renderizador)) |
| Resolução (portátil) | `resolution_switch=` | `0` automática, `1` 1x (720x408), `6` Vita (960x544), `5` 720p (1270x720), `2` 2x (1440x816), `3` 3x (2160x1224), `4` 4x (2880x1632) |
| Resolução (na TV) | `resolution_docked=` | os mesmos valores; `0` = igual ao portátil |
| Proporção da tela | `aspect=` | `auto`, `16:9`, `16:10`, `21:9`, `32:9`, `4:3` |
| Upscaler | `upscaler=` | `bilinear`, `fsr1` (AMD FSR 1.0) |
| Nitidez (RCAS) | `sharpness=` | `0` a `10` |
| Anti-aliasing | `anti_aliasing=` | `off`, `fxaa` |
| Filtro anisotrópico | `anisotropic=` | `1`, `2`, `4`, `8`, `16` |

**Cor**

| Opção | Chave | Valores |
|---|---|---|
| Brilho | `gamma=` | `0` a `8` (`4` = neutro; aparece como -4 a +4) |
| Saturação | `saturation=` | `0` a `8` (`4` = neutro) |
| Vibrância | `vibrance=` | `0` a `4` |
| Debanding | `debanding=` | `0` / `1` |

**Desempenho**

| Opção | Chave | Valores |
|---|---|---|
| Taxa de quadros | `fps=` | `30`, `60`, `max` (máximo usa a taxa da tela sem acelerar o jogo) |
| Geração de quadros | `frame_generation=` | `off`, `performance`, `quality` |
| Fluxo da geração | `fg_flow=` | `1` inteiro, `2` metade (padrão), `3` um terço, `4` um quarto |
| VSync | `vsync=` | `0` / `1` (padrão ligado) |
| Contador de FPS | `fps_counter=` | `0` / `1` |

**Jogo**

| Opção | Chave | Valores |
|---|---|---|
| Texturas HD | `hd_textures=` | `0` / `1` (usa `data/texpack` quando existir) |
| Barras de vida dos inimigos | `enemy_health_bars=` | `0` / `1` |
| QTE automático | `auto_qte=` | `0` / `1` |
| Vida / Magia / Fúria infinitas | `cheat_infinite_health=`, `cheat_infinite_magic=`, `cheat_infinite_rage=` | `0` / `1` |
| Idioma / Language | `launcher_language=`, `language=` | `auto` (idioma do Switch + `data/traducoes/*/records.tsv`), `pt`, `en`, `es` ou pastas extras `[SD]` |
| Música do launcher | `launcher_music=` | `0` / `1` |

### Troféus (X)

O launcher lê o `TROPHY.TRP` do seu jogo (`game/PCSA00126/sce_sys/trophy/`) e o progresso gravado pelo runtime em
`data/trophy/<NPWR>/unlocked.txt`. Mostra ícones e porcentagem. **L/R** troca entre *God of War* e
*God of War II*, **X** filtra (todos, ganhos, faltando), **Y** revela a descrição dos troféus ocultos e **B** volta.

### Atualizador (−)

Consulta o último release em `github.com/gerisson1996/gow-collection-nx`. Ao abrir o launcher a busca é feita em
silêncio e, se houver versão nova, o rodapé mostra **Atualizar para vX.Y**.

1. Aperte **−**. Com atualização disponível, **A** baixa e instala; **B** volta. Com tudo em dia, **X** reinstala a
   versão atual e **Y** busca de novo.
2. O `gow-nx.zip` do release é baixado e o launcher, o `gow1.nro` e o `gow2.nro` são trocados de uma vez. **Não são
   tocados:** `data/` (saves e opções), `game/`, `shaders/`, `lsfg/`, `logs/` e `launcher.cfg`.
3. No fim, **A** reabre o launcher novo.

Precisa de internet no Switch e de um launcher compilado com `switch-curl`.

## Renderizador

| Valor | Quando usar |
|---|---|
| `deko3d` | **Recomendado.** API nativa da GPU do Switch. Necessário para a geração de quadros. Precisa de `shaders/shaders_dk.pak` |
| `vulkan` | Vulkan pelo NVK (Mesa). Alternativa se algo der errado no deko3d |
| `auto` | Deixa o jogo escolher (hoje, Vulkan) |

O `shaders_dk.pak` precisa ter sido gerado a partir do **mesmo** `shaders.pak` que está no SD
([BUILDING.md](BUILDING.md#shaders-do-deko3d-shaders_dkpak)). Um shader que faltar é pulado e aparece no log como
`dk: shader <hash> ausente no shaders_dk.pak`; o resto do jogo funciona.

O renderizador só muda pelo launcher (ou editando o `.ini`), porque o jogo precisa reabrir.

## FSR 1.0 e nitidez

Com `upscaler=fsr1` a imagem passa pelo **EASU** (ampliação que reconstrói bordas) e pelo **RCAS** (nitidez). A
**Nitidez** vai de 0 a 10; com FSR e nitidez em 0, vale o padrão da AMD. Valores altos podem criar contornos claros.

Quando a resolução interna já está perto da tela (720p no portátil), o EASU não tem o que ampliar: o port pula essa
passada e aplica só o RCAS, economizando GPU. Para ver o FSR ampliando de verdade, use uma resolução menor (1x ou Vita)
ou jogue na TV.

## Geração de quadros (LSFG)

Cria um quadro intermediário entre cada par de quadros reais, levando a tela a **60 quadros com o jogo a 30**.

- Funciona **só no *God of War* (I)** e **só com o renderizador deko3d**. No *God of War II* ou no Vulkan a opção
  não tem efeito.
- **Desempenho** é mais leve; **Qualidade** usa a rede maior do Lossless Scaling.
- **Fluxo da geração** define a resolução do fluxo óptico: 1/2 é o padrão e o melhor equilíbrio; 1/3 e 1/4 aliviam a
  GPU em resoluções altas. Se o fluxo escolhido não couber na resolução, o jogo tenta sozinho o fluxo maior seguinte (ex.: 1/4 → 1/3).
- Se a GPU passar de ~32 ms por quadro real, a geração pausa por 5 s e só volta se o jogo mais a geração couberem em
  30 ms, sem travar o jogo. O log mostra `dk: geração de quadros pausada:` com os tempos.
- Deixe **Taxa de quadros** em **30**: a geração dobra a partir do quadro real.

**Os shaders não vêm com o port.** Eles saem do `Lossless.dll` da sua cópia do *Lossless Scaling* (Steam) e são
convertidos no PC para `sdmc:/switch/gow-nx/lsfg/` ([BUILDING.md](BUILDING.md#geração-de-quadros-lsfg)). Sem essa
pasta, ou com o cache de outra versão, a opção não liga e o log mostra `dk: geração de quadros desligada:` com o
motivo.

Medido no portátil, 720p, fluxo 1/2: GPU ~9–10 ms na cena + ~10 ms na geração, 30 quadros reais e 60 na tela.

## Mod menu (R + Plus)

Segure **R** e aperte **+** durante o jogo. O menu pausa a partida e aplica as mudanças na hora, sem reiniciar.
**L/R** troca de aba, **Cima/Baixo** escolhe, **Esquerda/Direita** muda, **B** fecha. Ele aparece no idioma escolhido
no launcher.

| Aba | Opções |
|---|---|
| **Gráficos** | Preset rápido, resolução (portátil e TV), upscaler, nitidez, anti-aliasing, anisotrópico, brilho, saturação, vibrância, debanding, geração de quadros, fluxo da geração |
| **Mods** | Texturas HD, barras de vida dos inimigos, QTE automático |
| **Cheats** | Vida, magia e fúria infinitas |
| **Sistema** | Idioma (PT-BR, EN, ES), taxa de quadros, VSync, contador de FPS, diagnóstico da GPU (grava os tempos por passada no log), restaurar padrões, fechar menu, **voltar ao launcher**, **sair do jogo** |

**Voltar ao launcher** e **Sair do jogo** pedem confirmação (**A** confirma, **B** cancela) e encerram do jeito certo
(voltando direto ao `gow-nx.nro` via `envSetNextLoad` ou ao Homebrew Menu). O que não foi salvo no próprio jogo se
perde. Prefira essas opções ao HOME → fechar, que não dá tempo ao port de encerrar as threads.

O que for mudado no mod menu é gravado em `data/vitart_options.ini` e vale também da próxima vez.

## Menu de pausa (Options)

No menu de pausa do jogo, **Options** (precisa do mod de menu do
[GOD-OF-WAR-HD-COLLECTION](https://github.com/luisxl15/GOD-OF-WAR-HD-COLLECTION) em `data/mods`) traz Resolution,
Frame Rate, VSync, HD Textures, **PS2 Audio**, Auto QTE, FPS Counter e os cheats. O **PS2 Audio** só existe aqui
(`ps2_audio=` no `.ini`).

## Relógios (clocks)

| Modo | CPU | GPU | Memória |
|---|---|---|---|
| Portátil | 1020 MHz | **460,8 MHz** | 1331 MHz |
| Na TV | 1020 MHz | 768 MHz | 1600 MHz |

No portátil o port pede ao sistema o **perfil oficial de desempenho** do modo portátil (o mesmo que jogos comerciais
usam), que leva a GPU de 307 para 460,8 MHz e é mantido a partida toda. Não é overclock e não precisa de sys-clk. Ao
sair, o perfil normal volta. Para desligar, rode com `VITART_GPU460=0`.

Com sys-clk ou similar você pode subir mais a GPU por sua conta; a CPU não precisa subir.

## Saves

Os saves, as opções e os troféus ficam em `sdmc:/switch/gow-nx/data/`. Faça backup dessa pasta de vez em quando; o
atualizador não mexe nela.

## Logs e relato de problemas

- `logs/vitart_gow1.log` / `logs/vitart_gow2.log`: log do runtime (início, renderizador, relógios, shaders, geração de
  quadros, erros). Linhas `nx: saída inesperada:` indicam um fechamento anormal.
- `logs/perfil_<jogo>.txt`: amostras do perfilador embutido (onde `main` e `Render` passam o tempo).
- **Diagnóstico da GPU** (mod menu → Sistema) grava no log quanto tempo de GPU cada passada leva.

Mande os arquivos junto com a descrição do problema. Veja [CONTRIBUTING.md](../CONTRIBUTING.md).

## Problemas comuns

| Sintoma | Causa provável |
|---|---|
| O jogo fecha logo ao abrir | Homebrew Menu aberto pelo Álbum (modo applet). Use o title takeover |
| Tela preta, log fala de arquivo faltando | `game/PCSA00126` incompleto ou no lugar errado |
| Gráficos faltando no Vulkan | `shaders/shaders.pak` ausente ou incompleto ([BUILDING.md](BUILDING.md)) |
| Gráficos faltando só no deko3d | `shaders_dk.pak` ausente ou gerado de outro `shaders.pak`. Gere de novo |
| *God of War II* parado na primeira tela de carregamento | `shaders.pak` sem os shaders do GoW II ([BUILDING.md](BUILDING.md#completar-os-shaders-jogando-necessário-para-o-god-of-war-ii)) |
| Geração de quadros não liga | Renderizador não é deko3d, jogo é o GoW II, ou falta a pasta `lsfg/`. O log diz qual |
| Geração de quadros some e volta | A GPU não deu conta naquele trecho. Use fluxo 1/3 ou 1/4, ou uma resolução menor |
| Lento em 2x ou acima | Resolução alta demais para a GPU do portátil. Use 720p ou o preset **Equilibrado** |
| Sem o menu Options na pausa | Mod de menu ausente em `data/mods` |
| Atualizador não acha versão | Sem internet, ou limite de consultas do GitHub. Tente de novo mais tarde |
