<div align="center">

<img src=".github/banner.svg" alt="God of War Collection NX" width="100%">

# God of War Collection — NX Port

**Port nativo de *God of War* y *God of War II* (PS Vita) para Nintendo Switch**

![Switch](https://img.shields.io/badge/Nintendo%20Switch-homebrew-e60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)
![ARM64](https://img.shields.io/badge/ARM64-recompilaci%C3%B3n%20est%C3%A1tica-1f6feb?style=for-the-badge)
![Vulkan](https://img.shields.io/badge/Vulkan-NVK-a41e22?style=for-the-badge&logo=vulkan&logoColor=white)
![FPS](https://img.shields.io/badge/30%20FPS-CPU%201020%20MHz-ecb24a?style=for-the-badge)
![License](https://img.shields.io/badge/licencia-MIT-2ea44f?style=for-the-badge)
[![Ko-fi](https://img.shields.io/badge/Ko--fi-apoyar-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/gerissoncassiano)

🇧🇷 [Português](README.md) · 🇺🇸 [English](README.en.md) · 🇪🇸 **Español**

[Acerca de](#-acerca-de) · [Características](#-características) · [Rendimiento](#-rendimiento) · [Instalación](#-instalación) · [Controles](#-controles) · [Compilar](#%EF%B8%8F-compilar) · [Créditos](#-créditos) · [**Apoyar**](#-apoya-el-proyecto)

</div>

> [!IMPORTANT]
> **Este repositorio no contiene el juego ni binarios generados a partir de él.** Necesitas tu propia copia original
> de *God of War Collection* de PS Vita (PKG estadounidense **PCSA00126** + la licencia `work.bin`) y generar los NRO
> tú mismo siguiendo [docs/BUILDING.md](docs/BUILDING.md).

---

## 📖 Acerca de

Los ejecutables de *God of War* y *God of War II* de PS Vita se **recompilan estáticamente** a C con el recompilador
de [vitaRT](https://github.com/luisxl15/vitaRT) y luego se **compilan de forma nativa para el ARM64 de la Switch**.
**No es un emulador:** no hay interpretación ni JIT de CPU. Solo se reimplementan los bordes del sistema de la Vita
(kernel, archivos, audio, controles y la GPU GXM), y el renderizado se traduce a **Vulkan** y se ejecuta en la GPU de
la Switch con el driver **NVK** (Mesa).

Este repositorio contiene la **capa de Switch** (libnx, Vulkan, audio, entrada, hilos), los **cambios al runtime de
vitaRT** (como parches), el **launcher** y los scripts de compilación.

## ✨ Características

| | |
|---|---|
| 🎮 **Nativo** | Código del juego recompilado a ARM64, sin emulación de CPU |
| ⚔️ **Dos juegos, un launcher** | `gow-nx.nro` abre un menú multilingüe (detección automática del idioma de la Switch) con portada, música, visor de trofeos, opciones y actualizador |
| ⚡ **30 FPS en reloj de serie** | Escenas pesadas de *God of War II* estables a 30 FPS con la CPU a 1020 MHz, sin overclock |
| 🖥️ **Vulkan (NVK)** | GXM traducido a Vulkan, con la grabación de comandos en un hilo dedicado |
| 🧵 **Hilos por núcleo** | Lógica, renderizado, grabación Vulkan y audio fijados en los 3 núcleos de la Switch |
| 🔊 **Audio sin cortes** | ATRAC9 decodificado en la Switch y salida por `audout` con prioridad propia |
| ⚙️ **Opciones en el menú de pausa** | Resolución interna, tasa de cuadros, VSync, texturas HD, audio de PS2, contador de FPS y trucos* |
| 💾 **Partidas y opciones en la SD** | Todo en `sdmc:/switch/gow-nx/data/` |

<sub>* Las opciones extra del menú de pausa vienen del mod de menú de
[GOD-OF-WAR-HD-COLLECTION](https://github.com/luisxl15/GOD-OF-WAR-HD-COLLECTION).</sub>

## 📊 Rendimiento

| Resolución interna | CPU | GPU | Overclock |
|---|---|---|---|
| **1x** (720p) | 1020 MHz | de serie | ✅ **no hace falta** |
| **2x** | 1020 MHz | **614 MHz** | ⚠️ **necesita la GPU a 614 MHz** (p. ej. sys-clk) |

> [!NOTE]
> **v1.2:** *God of War* a 720p corre a **~45–60 FPS con los relojes de fábrica** (FPS máximo), frente a ~22 FPS
> en las escenas pesadas de la v1.0. Detalles en el [CHANGELOG](CHANGELOG.md).

Medido en hardware real, modo portátil. El efecto de cada optimización está en [docs/PERFORMANCE.md](docs/PERFORMANCE.md).

## 📦 Requisitos

- Nintendo Switch con homebrew (Atmosphère + Homebrew Menu)
- Tu copia de *God of War Collection* de PS Vita — **PCSA00126** (EE. UU.) + `work.bin`
- Unos 4 GB libres en la tarjeta SD

> [!TIP]
> Abre el Homebrew Menu **manteniendo R al iniciar un juego** (title takeover), no desde el Álbum. En modo applet hay
> mucha menos memoria y el juego no arranca. El launcher también necesita el Homebrew Menu para iniciar los juegos.

## 🚀 Instalación

1. Genera `gow-nx.nro`, `gow1.nro`, `gow2.nro` y `shaders.pak` con [docs/BUILDING.md](docs/BUILDING.md).
2. Cópialos a la tarjeta SD:

```
sdmc:/switch/gow-nx/
├── gow-nx.nro          launcher (abre este)
├── gow1.nro            God of War
├── gow2.nro            God of War II
├── game/PCSA00126/     archivos del juego extraídos de TU copia
├── shaders/            shaders.pak (SPIR-V traducido)
├── data/               opciones, partidas y mods (se crea en el primer inicio)
└── logs/               vitart_<juego>.log y perfil_<juego>.txt, para reportar errores
```

3. Abre **God of War Collection** desde el Homebrew Menu y elige el juego.

## 🎮 Controles

**Launcher**

| Botón | Acción |
|---|---|
| **Arriba / Abajo** | Elegir juego o abrir **Opciones** (idioma y gráficos) |
| **A** | Jugar / abrir opciones / cambiar valor |
| **Izquierda / Derecha** | Cambiar valor en opciones |
| **B** | Guardar opciones y volver |
| **+** | Salir |

**En el juego** (distribución de PS Vita en el mando de Switch)

| Switch | Vita | | Switch | Vita |
|---|---|---|---|---|
| **B** | ✕ | | **L / R** | L / R |
| **A** | ○ | | **ZL / ZR** | L2 / R2 (gatillos traseros) |
| **Y** | □ | | **+** | Start |
| **X** | △ | | **−** | Select |
| Sticks | Sticks | | L3 / R3 | L3 / R3 |

Mantén **R** y pulsa **+** en el juego para abrir el menú de opciones de Switch (**NX Menu**).

## 🛠️ Compilar

```sh
git clone https://github.com/gerisson1996/gow-collection-nx.git
git clone https://github.com/luisxl15/vitaRT.git
cd gow-collection-nx
VITART=../vitaRT bash scripts/setup.sh
GEN=/ruta/out/gen bash scripts/compile_mods.sh gow1 gow2 libc libfios2
VITART=../vitaRT VULKAN_INCLUDE=/ruta/Include GAME=gow1 bash scripts/build.sh
VITART=../vitaRT VULKAN_INCLUDE=/ruta/Include GAME=gow2 bash scripts/build.sh
bash launcher/build.sh
```

Requiere devkitPro (devkitA64, libnx, switch-freetype, switch-mpg123) y NVK para Switch. Guía completa, incluida la
generación del código desde tu copia: [docs/BUILDING.md](docs/BUILDING.md) (en portugués).

## 📚 Documentación

| Documento | Contenido |
|---|---|
| [CHANGELOG.md](CHANGELOG.md) | Cambios por versión |
| [docs/BUILDING.md](docs/BUILDING.md) | Generar el código, compilar y preparar la SD |
| [docs/USER_GUIDE.md](docs/USER_GUIDE.md) | Launcher, opciones, relojes, partidas y logs |
| [docs/PERFORMANCE.md](docs/PERFORMANCE.md) | Hilos, optimizaciones y mediciones |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Cómo funciona el port por dentro |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Cómo contribuir |

La documentación detallada está en portugués.

## ☕ Apoya el proyecto

El port es gratuito y de código abierto. Las herramientas que uso para hacer estos ports son **de pago**, y cada
donación ayuda a mantenerlas y a seguir con el proyecto. ¿Quieres otro juego portado o una mejora? **Envía tu pedido
junto con tu donación** (escríbelo en el mensaje de Ko-fi, PayPal o Pix).

[![Ko-fi](https://img.shields.io/badge/Ko--fi-apoyar-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/gerissoncassiano)
[![PayPal](https://img.shields.io/badge/PayPal-donar-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.com/donate/?hosted_button_id=B3YTKT29F5ZLG)

O por **PIX** (Brasil, clave e-mail): `pocketneogames@gmail.com`

<img src=".github/pix-qr.png" alt="Código QR PIX" width="200">

PIX copia y pega:

```text
00020126460014br.gov.bcb.pix0124pocketneogames@gmail.com5204000053039865802BR5911RECOMP GUTO6006BRASIL62070503***63048D6B
```

## 🙏 Créditos

- **[vitaRT](https://github.com/luisxl15/vitaRT)** y **[GOD-OF-WAR-HD-COLLECTION](https://github.com/luisxl15/GOD-OF-WAR-HD-COLLECTION)**
  — luisxl15: el recompilador estático y el runtime de PS Vita en los que se basa este port (no incluidos aquí)
- **[devkitPro](https://devkitpro.org/)** — devkitA64, libnx y los portlibs de Switch
- **[Mesa / NVK](https://docs.mesa3d.org/drivers/nvk.html)** — el driver Vulkan usado en Switch
- **[mvhoang92](https://github.com/mvhoang92)** — pruebas en la consola y el diagnóstico de las correcciones de la v1.3 (borde sin dibujar,
  contador de FPS, shaders de *God of War II*)
- Bibliotecas: [libatrac9](https://github.com/Thealexbarney/LibAtrac9), [volk](https://github.com/zeux/volk),
  [mpg123](https://www.mpg123.de/), [FreeType](https://freetype.org/) — ver
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

## ⚖️ Aviso legal

Proyecto de fans, no oficial, sin relación con Sony Interactive Entertainment, Santa Monica Studio ni Nintendo, y sin
su respaldo. *God of War* y PlayStation Vita son marcas de Sony Interactive Entertainment; Nintendo Switch es marca de
Nintendo. **No se distribuye ningún archivo del juego:** usa tu propia copia original. No compartas los archivos del
juego ni los NRO, que se generan a partir de él.

El código de este repositorio está bajo la [licencia MIT](LICENSE). Los parches en `patches/` modifican vitaRT, que
sigue bajo los términos de su autor, y los componentes de terceros conservan sus propias licencias.
