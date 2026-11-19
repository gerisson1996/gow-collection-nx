<div align="center">

<img src=".github/banner.svg" alt="God of War Collection NX" width="100%">

# God of War Collection — NX Port

**Native port of *God of War* and *God of War II* (PS Vita) to Nintendo Switch**

![Switch](https://img.shields.io/badge/Nintendo%20Switch-homebrew-e60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)
![ARM64](https://img.shields.io/badge/ARM64-static%20recompilation-1f6feb?style=for-the-badge)
![Vulkan](https://img.shields.io/badge/Vulkan-NVK-a41e22?style=for-the-badge&logo=vulkan&logoColor=white)
![FPS](https://img.shields.io/badge/30%20FPS-CPU%201020%20MHz-ecb24a?style=for-the-badge)
![License](https://img.shields.io/badge/license-MIT-2ea44f?style=for-the-badge)
[![Ko-fi](https://img.shields.io/badge/Ko--fi-support-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/gerissoncassiano)

🇧🇷 [Português](README.md) · 🇺🇸 **English** · 🇪🇸 [Español](README.es.md)

[About](#-about) · [Features](#-features) · [Performance](#-performance) · [Installation](#-installation) · [Controls](#-controls) · [Building](#%EF%B8%8F-building) · [Credits](#-credits) · [**Support**](#-support-the-project)

</div>

> [!IMPORTANT]
> **This repository contains neither the game nor any binaries built from it.** You need your own original copy of
> the PS Vita *God of War Collection* (US PKG **PCSA00126** + the `work.bin` license), and you build the NROs yourself
> by following [docs/BUILDING.md](docs/BUILDING.md).

---

## 📖 About

The PS Vita executables of *God of War* and *God of War II* are **statically recompiled** to C by the
[vitaRT](https://github.com/luisxl15/vitaRT) recompiler and then **compiled natively for the Switch's ARM64 CPU**.
**This is not an emulator:** there is no CPU interpretation or JIT. Only the Vita's system boundaries are
reimplemented (kernel, files, audio, input and the GXM GPU), and rendering is translated to **Vulkan** and runs on the
Switch GPU through the **NVK** (Mesa) driver.

This repository holds the **Switch layer** (libnx, Vulkan, audio, input, threads), the **changes to the vitaRT
runtime** (as patches), the **launcher** and the build scripts.

## ✨ Features

| | |
|---|---|
| 🎮 **Native** | Game code recompiled to ARM64, no CPU emulation |
| ⚔️ **Two games, one launcher** | `gow-nx.nro` opens a multilingual menu (Switch language auto-detection) with cover art, music, trophy viewer, options and updater |
| ⚡ **30 FPS at stock clocks** | Heavy *God of War II* scenes hold 30 FPS with the CPU at 1020 MHz, no overclock |
| 🖥️ **Vulkan (NVK)** | GXM translated to Vulkan, with command recording on a dedicated thread |
| 🧵 **Per-core threads** | Game logic, rendering, Vulkan recording and audio pinned to the Switch's 3 cores |
| 🔊 **Stutter-free audio** | ATRAC9 decoded on the Switch, output through `audout` at its own priority |
| ⚙️ **Pause-menu options** | Internal resolution, frame rate, VSync, HD textures, PS2 audio, FPS counter and cheats* |
| 💾 **Saves and options on SD** | Everything in `sdmc:/switch/gow-nx/data/` |

<sub>* The extra pause-menu options come from the menu mod in
[GOD-OF-WAR-HD-COLLECTION](https://github.com/luisxl15/GOD-OF-WAR-HD-COLLECTION).</sub>

## 📊 Performance

| Internal resolution | CPU | GPU | Overclock |
|---|---|---|---|
| **1x** (720p) | 1020 MHz | stock | ✅ **not needed** |
| **2x** | 1020 MHz | **614 MHz** | ⚠️ **needs the GPU at 614 MHz** (e.g. sys-clk) |

> [!NOTE]
> **v1.2:** *God of War* at 720p runs at **~45–60 FPS on stock clocks** (max FPS), up from ~22 FPS in heavy
> scenes on v1.0. Details in the [CHANGELOG](CHANGELOG.en.md).

Measured on real hardware, handheld mode. The effect of each optimization is in [docs/PERFORMANCE.md](docs/PERFORMANCE.md).

## 📦 Requirements

- A Nintendo Switch running homebrew (Atmosphère + Homebrew Menu)
- Your copy of the PS Vita *God of War Collection* — **PCSA00126** (US) + `work.bin`
- About 4 GB free on the SD card

> [!TIP]
> Open the Homebrew Menu by **holding R while launching a game** (title takeover), not from the Album. Applet mode
> has far less memory and the game will not start. The launcher also needs the Homebrew Menu to boot the games.

## 🚀 Installation

1. Build `gow-nx.nro`, `gow1.nro`, `gow2.nro` and `shaders.pak` with [docs/BUILDING.md](docs/BUILDING.md).
2. Copy them to the SD card:

```
sdmc:/switch/gow-nx/
├── gow-nx.nro          launcher (open this one)
├── gow1.nro            God of War
├── gow2.nro            God of War II
├── game/PCSA00126/     game files extracted from YOUR copy
├── shaders/            shaders.pak (translated SPIR-V)
├── data/               options, saves and mods (created on first boot)
└── logs/               vitart_<game>.log and perfil_<game>.txt, for bug reports
```

3. Open **God of War Collection** from the Homebrew Menu and pick a game.

## 🎮 Controls

**Launcher**

| Button | Action |
|---|---|
| **Up / Down** | Choose a game or **Options** (language and graphics) |
| **A** | Play / open options / change value |
| **Left / Right** | Change value in options |
| **B** | Save options and go back |
| **+** | Exit |

**In game** (PS Vita layout on the Switch controller)

| Switch | Vita | | Switch | Vita |
|---|---|---|---|---|
| **B** | ✕ | | **L / R** | L / R |
| **A** | ○ | | **ZL / ZR** | L2 / R2 (rear touch triggers) |
| **Y** | □ | | **+** | Start |
| **X** | △ | | **−** | Select |
| Sticks | Sticks | | L3 / R3 | L3 / R3 |

Hold **R** and press **+** in game to open the Switch options overlay (**NX Menu**).

## 🛠️ Building

```sh
git clone https://github.com/gerisson1996/gow-collection-nx.git
git clone https://github.com/luisxl15/vitaRT.git
cd gow-collection-nx
VITART=../vitaRT bash scripts/setup.sh
GEN=/path/out/gen bash scripts/compile_mods.sh gow1 gow2 libc libfios2
VITART=../vitaRT VULKAN_INCLUDE=/path/Include GAME=gow1 bash scripts/build.sh
VITART=../vitaRT VULKAN_INCLUDE=/path/Include GAME=gow2 bash scripts/build.sh
bash launcher/build.sh
```

Requires devkitPro (devkitA64, libnx, switch-freetype, switch-mpg123) and NVK for Switch. Full walkthrough, including
generating the code from your copy: [docs/BUILDING.md](docs/BUILDING.md) (Portuguese).

## 📚 Documentation

| Document | Contents |
|---|---|
| [CHANGELOG.en.md](CHANGELOG.en.md) | Changes per version |
| [docs/BUILDING.md](docs/BUILDING.md) | Generating the code, building and laying out the SD |
| [docs/USER_GUIDE.md](docs/USER_GUIDE.md) | Launcher, options, clocks, saves and logs |
| [docs/PERFORMANCE.md](docs/PERFORMANCE.md) | Threads, optimizations and measurements |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | How the port works internally |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |

The detailed docs are in Portuguese.

## ☕ Support the project

The port is free and open source. The tools I use to make these ports are **paid**, and every donation helps keep
them and the project going. Want another game ported or an improvement? **Send your request with your donation**
(write it in the Ko-fi, PayPal or Pix message).

[![Ko-fi](https://img.shields.io/badge/Ko--fi-support-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/gerissoncassiano)
[![PayPal](https://img.shields.io/badge/PayPal-donate-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.com/donate/?hosted_button_id=B3YTKT29F5ZLG)

Or via **PIX** (Brazil, e-mail key): `pocketneogames@gmail.com`

<img src=".github/pix-qr.png" alt="PIX QR code" width="200">

PIX copy-and-paste code:

```text
00020126460014br.gov.bcb.pix0124pocketneogames@gmail.com5204000053039865802BR5911RECOMP GUTO6006BRASIL62070503***63048D6B
```

## 🙏 Credits

- **[vitaRT](https://github.com/luisxl15/vitaRT)** and **[GOD-OF-WAR-HD-COLLECTION](https://github.com/luisxl15/GOD-OF-WAR-HD-COLLECTION)**
  — luisxl15: the static recompiler and PS Vita runtime this port builds on (not included here)
- **[devkitPro](https://devkitpro.org/)** — devkitA64, libnx and the Switch portlibs
- **[Mesa / NVK](https://docs.mesa3d.org/drivers/nvk.html)** — the Vulkan driver used on Switch
- **[mvhoang92](https://github.com/mvhoang92)** — console testing and the diagnosis behind the v1.3 fixes (undrawn edge, FPS counter,
  *God of War II* shaders)
- Libraries: [libatrac9](https://github.com/Thealexbarney/LibAtrac9), [volk](https://github.com/zeux/volk),
  [mpg123](https://www.mpg123.de/), [FreeType](https://freetype.org/) — see
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

## ⚖️ Legal notice

Unofficial fan project, not affiliated with or endorsed by Sony Interactive Entertainment, Santa Monica Studio or
Nintendo. *God of War* and PlayStation Vita are trademarks of Sony Interactive Entertainment; Nintendo Switch is a
trademark of Nintendo. **No game files are distributed:** use your own original copy. Do not share the game files or
the NROs, which are built from them.

The code in this repository is under the [MIT license](LICENSE). The patches in `patches/` modify vitaRT, which stays
under its author's terms, and third-party components keep their own licenses.
