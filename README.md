<div align="center">

<img src=".github/banner.svg" alt="God of War Collection NX" width="100%">

# God of War Collection — NX Port

**Port nativo de *God of War* e *God of War II* (PS Vita) para Nintendo Switch**

![Switch](https://img.shields.io/badge/Nintendo%20Switch-homebrew-e60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)
![ARM64](https://img.shields.io/badge/ARM64-recompila%C3%A7%C3%A3o%20est%C3%A1tica-1f6feb?style=for-the-badge)
![Vulkan](https://img.shields.io/badge/Vulkan-NVK-a41e22?style=for-the-badge&logo=vulkan&logoColor=white)
![FPS](https://img.shields.io/badge/30%20FPS-CPU%201020%20MHz-ecb24a?style=for-the-badge)
![License](https://img.shields.io/badge/licen%C3%A7a-MIT-2ea44f?style=for-the-badge)
[![Ko-fi](https://img.shields.io/badge/Ko--fi-apoiar-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/gerissoncassiano)

🇧🇷 **Português** · 🇺🇸 [English](README.en.md) · 🇪🇸 [Español](README.es.md)

[Sobre](#-sobre) · [Recursos](#-recursos) · [Desempenho](#-desempenho) · [Instalação](#-instalação) · [Controles](#-controles) · [Compilar](#%EF%B8%8F-compilar) · [Créditos](#-créditos) · [**Apoiar**](#-apoie-o-projeto)

</div>

> [!IMPORTANT]
> **Este repositório não contém o jogo nem binários dele.** Você precisa da sua própria cópia original da
> *God of War Collection* de PS Vita (PKG americano **PCSA00126** + a licença `work.bin`) e de gerar os NROs
> você mesmo, seguindo [docs/BUILDING.md](docs/BUILDING.md).

---

## 📖 Sobre

Os executáveis de *God of War* e *God of War II* do PS Vita são **recompilados estaticamente** para C pelo
recompilador do [vitaRT](https://github.com/luisxl15/vitaRT) e depois **compilados nativamente para o ARM64 do
Switch**. **Não é um emulador:** não há interpretação nem JIT de CPU. O que é reproduzido são as bordas do sistema
do Vita (kernel, arquivos, áudio, controles e a GPU GXM), e a renderização é traduzida para **Vulkan** e executada
na GPU do Switch pelo driver **NVK** (Mesa).

Este repositório traz a **camada do Switch** (libnx, Vulkan, áudio, entrada e threads), as **mudanças sobre o
runtime do vitaRT** (em forma de patches), o **launcher** e os scripts de compilação.

## ✨ Recursos

| | |
|---|---|
| 🎮 **Nativo** | Código do jogo recompilado para ARM64, sem emulação de CPU |
| ⚔️ **Dois jogos, um launcher** | `gow-nx.nro` abre um menu multilíngue (detecção automática do idioma do Switch) com capa, música, troféus, opções e atualizador |
| ⚡ **30 FPS no relógio padrão** | Cenas pesadas de *God of War II* estáveis a 30 FPS com a CPU em 1020 MHz, sem overclock |
| 🖥️ **Vulkan (NVK)** | GXM traduzido para Vulkan, com os comandos gravados numa thread dedicada |
| 🧵 **Threads por núcleo** | Lógica, renderização, gravação Vulkan e áudio fixados nos 3 núcleos do Switch |
| 🔊 **Áudio sem engasgos** | ATRAC9 decodificado no Switch e saída por `audout` com prioridade própria |
| ⚙️ **Opções no menu de pausa** | Resolução interna, taxa de quadros, VSync, texturas HD, áudio do PS2, contador de FPS e cheats* |
| 💾 **Saves e opções no SD** | Tudo em `sdmc:/switch/gow-nx/data/` |

<sub>* As opções extras do menu de pausa vêm do mod de menu do
[GOD-OF-WAR-HD-COLLECTION](https://github.com/luisxl15/GOD-OF-WAR-HD-COLLECTION).</sub>

## 📊 Desempenho

| Resolução interna | CPU | GPU | Overclock |
|---|---|---|---|
| **1x** (720p) | 1020 MHz | padrão | ✅ **não precisa** |
| **2x** | 1020 MHz | **614 MHz** | ⚠️ **precisa da GPU a 614 MHz** (ex.: sys-clk) |

> [!NOTE]
> **v1.2:** *God of War* em 720p roda a **~45–60 FPS no relógio padrão** (FPS máximo), contra ~22 FPS nas cenas
> pesadas da v1.0. Detalhes no [CHANGELOG](CHANGELOG.md).

Medido no Switch real, modo portátil. A evolução de cada otimização está em [docs/PERFORMANCE.md](docs/PERFORMANCE.md).

## 📦 Requisitos

- Nintendo Switch com homebrew (Atmosphère + Homebrew Menu)
- Sua cópia da *God of War Collection* de PS Vita — **PCSA00126** (EUA) + `work.bin`
- Uns 4 GB livres no cartão SD

> [!TIP]
> Abra o Homebrew Menu **segurando R enquanto abre um jogo** (modo title takeover), e não pelo Álbum. No modo
> applet a memória é bem menor e o jogo não abre. O launcher também precisa do Homebrew Menu para iniciar os jogos.

## 🚀 Instalação

1. Gere `gow-nx.nro`, `gow1.nro`, `gow2.nro` e `shaders.pak` com [docs/BUILDING.md](docs/BUILDING.md).
2. Copie para o cartão SD:

```
sdmc:/switch/gow-nx/
├── gow-nx.nro          launcher (abra este)
├── gow1.nro            God of War
├── gow2.nro            God of War II
├── game/PCSA00126/     arquivos do jogo extraídos da SUA cópia
├── shaders/            shaders.pak (SPIR-V traduzido)
├── data/               opções, saves e mods (criada no primeiro início)
└── logs/               vitart_<jogo>.log e perfil_<jogo>.txt, para relatar problemas
```

3. Abra **God of War Collection** pelo Homebrew Menu e escolha o jogo.

## 🎮 Controles

**Launcher**

| Botão | Ação |
|---|---|
| **Cima / Baixo** | Escolher o jogo ou abrir **Opções** (idioma e gráficos) |
| **A** | Jogar / abrir opções / mudar valor |
| **Esquerda / Direita** | Mudar valor nas opções |
| **B** | Salvar opções e voltar |
| **+** | Sair |

**No jogo** (layout do PS Vita no controle do Switch)

| Switch | Vita | | Switch | Vita |
|---|---|---|---|---|
| **B** | ✕ | | **L / R** | L / R |
| **A** | ○ | | **ZL / ZR** | L2 / R2 (gatilhos traseiros) |
| **Y** | □ | | **+** | Start |
| **X** | △ | | **−** | Select |
| Analógicos | Analógicos | | L3 / R3 | L3 / R3 |

Segure **R** e aperte **+** no jogo para abrir o menu de opções do Switch (**NX Menu**). Os ícones de botão do jogo aparecem com o layout da Nintendo.

## 🛠️ Compilar

```sh
git clone https://github.com/gerisson1996/gow-collection-nx.git
git clone https://github.com/luisxl15/vitaRT.git
cd gow-collection-nx
VITART=../vitaRT bash scripts/setup.sh
GEN=/caminho/out/gen bash scripts/compile_mods.sh gow1 gow2 libc libfios2
VITART=../vitaRT VULKAN_INCLUDE=/caminho/Include GAME=gow1 bash scripts/build.sh
VITART=../vitaRT VULKAN_INCLUDE=/caminho/Include GAME=gow2 bash scripts/build.sh
bash launcher/build.sh
```

Precisa do devkitPro (devkitA64, libnx, switch-freetype, switch-mpg123) e do NVK para Switch. O passo a passo
completo, incluindo a geração do código a partir da sua cópia: [docs/BUILDING.md](docs/BUILDING.md).

## 📚 Documentação

| Documento | Conteúdo |
|---|---|
| [CHANGELOG.md](CHANGELOG.md) | O que mudou em cada versão |
| [docs/BUILDING.md](docs/BUILDING.md) | Gerar o código, compilar e montar o SD |
| [docs/USER_GUIDE.md](docs/USER_GUIDE.md) | Launcher, opções, relógios, saves e logs |
| [docs/PERFORMANCE.md](docs/PERFORMANCE.md) | Threads, otimizações e números medidos |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Como o port funciona por dentro |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Como contribuir |

## ☕ Apoie o projeto

O port é grátis e o código é aberto. As ferramentas que uso para fazer estes ports são **pagas**, e cada doação
ajuda a mantê-las e a continuar o projeto. Quer ver outro jogo portado ou uma melhoria? **Faça seu pedido junto com
a sua doação** (escreva na mensagem do Ko-fi, do PayPal ou do Pix).

[![Ko-fi](https://img.shields.io/badge/Ko--fi-apoiar-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/gerissoncassiano)
[![PayPal](https://img.shields.io/badge/PayPal-doar-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.com/donate/?hosted_button_id=B3YTKT29F5ZLG)

Ou por **PIX** (chave e-mail): `pocketneogames@gmail.com`

<img src=".github/pix-qr.png" alt="QR Code PIX" width="200">

PIX copia e cola:

```text
00020126460014br.gov.bcb.pix0124pocketneogames@gmail.com5204000053039865802BR5911RECOMP GUTO6006BRASIL62070503***63048D6B
```

## 🙏 Créditos

- **[vitaRT](https://github.com/luisxl15/vitaRT)** e **[GOD-OF-WAR-HD-COLLECTION](https://github.com/luisxl15/GOD-OF-WAR-HD-COLLECTION)**
  — luisxl15: o recompilador estático e o runtime do PS Vita em que este port se baseia (não incluídos aqui)
- **[devkitPro](https://devkitpro.org/)** — devkitA64, libnx e os portlibs do Switch
- **[Mesa / NVK](https://docs.mesa3d.org/drivers/nvk.html)** — o driver Vulkan usado no Switch
- **[mvhoang92](https://github.com/mvhoang92)** — testes no console e diagnóstico das correções da v1.3 (borda sem desenho, contador de FPS,
  shaders do *God of War II*)
- Bibliotecas: [libatrac9](https://github.com/Thealexbarney/LibAtrac9), [volk](https://github.com/zeux/volk),
  [mpg123](https://www.mpg123.de/), [FreeType](https://freetype.org/) — detalhes em
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

## ⚖️ Aviso legal

Projeto de fã, não oficial, sem ligação com a Sony Interactive Entertainment, a Santa Monica Studio ou a Nintendo,
e sem o apoio delas. *God of War* é marca da Sony Interactive Entertainment, PlayStation Vita é marca da Sony
Interactive Entertainment e Nintendo Switch é marca da Nintendo. **Nenhum arquivo do jogo é distribuído:** use a sua
própria cópia original. Não compartilhe os arquivos do jogo nem os NROs, que são gerados a partir dele.

O código deste repositório está sob a [licença MIT](LICENSE). Os patches em `patches/` modificam o vitaRT, que
continua sob os termos do autor, e os componentes de terceiros mantêm as próprias licenças.
