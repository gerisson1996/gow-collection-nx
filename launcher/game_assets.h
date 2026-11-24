// Carregamento de capa (BGRA / PNG / TGA via PSARC) e música (MP3 / WAV / ATRAC9 via PSARC)
// diretamente da pasta do launcher ou da cópia legal em game/PCSA00126/.
#ifndef LAUNCHER_GAME_ASSETS_H
#define LAUNCHER_GAME_ASSETS_H

#include <stddef.h>
#include <stdint.h>

// Extrai um arquivo de dentro de um arquivo PSARC (v1.4, compressão zlib).
// target_path é comparado ignorando maiúsculas/minúsculas e '/' inicial.
// Retorna buffer alocado com malloc (liberar com free) ou NULL se não encontrado/inválido.
uint8_t *ga_psarc_extract(const char *psarc_path, const char *target_path,
                          size_t max_bytes, size_t *out_size);

// Carrega a capa em formato RGBA8888 (0xFFBBGGRR, pixels dst_w * dst_h).
// Ordem de prioridade:
//   1. <dir>bg.bgra (override do usuário no SD)
//   2. <dir>bg.png  (override PNG do usuário no SD)
//   3. romfs:/bg.bgra (se romfs_ativo)
//   4. <dir>game/PCSA00126/sce_sys/pic0.png
//   5. <dir>game/PCSA00126/sce_sys/livearea/contents/backgrounds/bg0.png
//   6. <dir>game/PCSA00126/System.psarc -> /System/AppSel_Bg_US.tga (ou EU)
// Retorna 1 se carregou uma imagem, 0 caso contrário (out_rgba fica zerado).
int ga_load_cover_rgba(const char *dir, int romfs_ativo,
                       uint32_t *out_rgba, int dst_w, int dst_h);

// Carrega a capa do card de um jogo (0 = GoW 1, 1 = GoW 2) de game/PCSA00126/.
// Ordem de prioridade:
//   1. <dir>game/PCSA00126/System.psarc -> /System/Icon_GOW{1,2}_{US,EU,JP}.tga
//   2. <dir>game/PCSA00126/sce_sys/trophy/NPWR{05244,05283}_00/TROPHY.TRP -> ICON0.PNG
// Pixels transparentes são compostos sobre bg_rgb (0x00BBGGRR).
// Retorna 1 e aloca *out_rgba (malloc, liberar com free) com *out_w x *out_h pixels RGBA8888.
int ga_load_game_card_rgba(const char *dir, int game_idx, uint32_t bg_rgb,
                           uint32_t **out_rgba, int *out_w, int *out_h);

// Verifica se o arquivo PSARC principal do jogo existe em <dir>game/PCSA00126/exec/.
// game_idx: 0 = God of War (gow1_psp2_SCEA.psarc), 1 = God of War II (gow2_psp2_SCEA.psarc).
int ga_game_dump_ok(const char *dir, int game_idx);

// Streamer de música de fundo (MP3, WAV PCM 16-bit ou ATRAC9 extraído do PSARC).
typedef struct GaAudio GaAudio;

// Abre a música seguindo a ordem:
//   1. <dir>music.mp3
//   2. <dir>music.wav
//   3. romfs:/music.mp3 (se romfs_ativo)
//   4. <dir>game/PCSA00126/exec/gow1_psp2_SCEA.psarc -> wad/_sound/dvd_streams/PSP2/tit1e1_2.at9 (se ENABLE_ATRAC9)
// Retorna ponteiro opaco ou NULL se nenhuma fonte de áudio existir/abrir.
GaAudio *ga_audio_open(const char *dir, int romfs_ativo);
long ga_audio_rate(const GaAudio *a);
// Lê até max_frames quadros estéreo int16 (2 * max_frames shorts), fazendo loop automático.
size_t ga_audio_read_stereo_s16(GaAudio *a, int16_t *dst, size_t max_frames);
void ga_audio_close(GaAudio *a);

#endif
