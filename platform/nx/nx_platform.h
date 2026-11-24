// Camada do Switch (libnx) para o vitart: memória do guest, threads, caminhos no SD e módulos ligados no NRO.
#pragma once
#include <cstdint>
#include <string>

extern "C" {
#include "arc_abi.h"
}

namespace nxp {

// Inicialização do processo (antes de qualquer outra coisa do vitart).
void init();
void register_main_thread(); // a thread que chama entra na lista do vigia
void dump_threads();        // registra onde cada thread está (pausando uma por vez)
void prof_start();          // amostragem da thread main do jogo (logs/perfil_<jogo>.txt)
void dump_fs_ops(bool recent);       // operações no SD pendentes e as últimas concluídas
void set_clocks(); // CPU e GPU no máximo do modo (clkrst)
void log_clocks(); // relógios atuais no log
void gpu_720p(bool on); // GPU 460,8 MHz com 720p no portátil (a cada quadro; só age quando muda)
bool docked(); // na base (TV)
// Base da janela do guest: guest_window() + endereço do Vita (0x80000000..0xC0000000) é memória válida.
uint8_t *guest_window();
uint8_t *g_window_base(); // a mesma base, sem alocar (nullptr antes do guest_window)
// sdmc:/switch/gow-nx
const std::string &root_dir();
// "gow1" ou "gow2" (definido na compilação do NRO).
const char *game_name();
// Pede para o processo sair (HOME/plus) e devolve se já foi pedido.
void request_exit();
bool exit_requested();
// Encadeia gow-nx.nro via envSetNextLoad e pede para sair (volta direto ao launcher).
bool can_return_to_launcher();
void request_return_to_launcher();
// Sai do processo na hora (HOME, fim do jogo): grava os arquivos e encerra sem destrutores, com as outras threads paradas.
[[noreturn]] void hard_exit(int code);
// Aplet em primeiro plano: false durante o menu HOME (o jogo continua, o som e a imagem seguem normalmente).
bool applet_loop();

struct StaticModule {
    const char *name;
    int (*bind)(const void *api, uint8_t *guest_mem);
    const arc_func_entry *(*get_table)(unsigned *count);
    const arc_segment *(*get_segments)(unsigned *count);
    const arc_module_info *(*get_module)(void);
};
const StaticModule *find_module(const char *name);

} // namespace nxp
