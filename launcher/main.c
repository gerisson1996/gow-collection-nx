// Launcher do gow-nx: capa em tela cheia, menu para escolher God of War / God of War II, música de fundo em loop,
// detecção automática do idioma do Switch (com seleção manual) e encadeamento do NRO escolhido via envSetNextLoad.
//   romfs:/bg.bgra    capa 1280x720, BGRA 8 bits (gerada de res/launcher_bg.jpg)
//   romfs:/music.mp3  música de fundo (decodificada com libmpg123)
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <mpg123.h>
#include "trophies.h"
#ifdef ENABLE_UPDATER
#include "updater.h"
#endif

#define W 1280
#define H 720

// ---------------------------------------------------------------- idiomas
enum { LANG_EN = 0, LANG_ES, LANG_PT, LANG_FR, LANG_IT, LANG_DE, NLANG };

typedef struct {
    const char *nome_idioma;
    const char *escolher;
    const char *sub_jogos[2];
    const char *opcoes_menu;
    const char *opcoes_titulo;
    const char *op_nomes[8];
    const char *auto_tela;
    const char *auto_prop;
    const char *max_fps;
    const char *desligado;
    const char *ligado;
    const char *nota_opcoes;
    const char *falta_arq;
    const char *msg_hbmenu;
    const char *msg_applet;
    const char *msg_sem_jogo;
    const char *msg_falha_prep;
    const char *msg_falha_ini;
    const char *btn_jogar;
    const char *btn_abrir;
    const char *btn_sair;
    const char *menu_sair;
    const char *btn_mudar;
    const char *btn_salvar;
    const char *upd_menu;
    const char *upd_titulo;
    const char *upd_ver_atual;
    const char *upd_ver_nova;
    const char *upd_buscando;
    const char *upd_atualizado;
    const char *upd_disponivel;
    const char *upd_baixando;
    const char *upd_extraindo;
    const char *upd_pronto;
    const char *upd_err_net;
    const char *upd_err_http;
    const char *upd_err_asset;
    const char *upd_err_disk;
    const char *upd_err_zip;
    const char *upd_err_cancel;
    const char *btn_instalar;
    const char *btn_reinstalar;
    const char *btn_reiniciar;
    const char *btn_cancelar;
    const char *btn_voltar;
    const char *trp_menu;
    const char *trp_titulo;
    const char *trp_oculto;
    const char *trp_oculto_desc;
    const char *trp_sem_trp;
    const char *trp_vazio;
    const char *trp_filtros[3];
    const char *trp_graus[4];
    const char *trp_desbloq;
    const char *trp_btn_jogo;
    const char *trp_btn_filtro;
    const char *trp_btn_spoil;
} Textos;

static const Textos kTextos[NLANG] = {
    [LANG_EN] = {
        .nome_idioma = "English",
        .escolher = "CHOOSE GAME",
        .sub_jogos = { "Kratos's journey against Ares", "Vengeance against Zeus" },
        .opcoes_menu = "Options",
        .opcoes_titulo = "OPTIONS",
        .op_nomes = { "Language", "Internal resolution", "Frame rate", "VSync",
                      "Aspect ratio", "FPS counter", "HD textures", "Enemy health bars" },
        .auto_tela = "Auto (screen)",
        .auto_prop = "Auto",
        .max_fps = "Max",
        .desligado = "Off",
        .ligado = "On",
        .nota_opcoes = "Applies to both games. Graphics also change in-game with R + Plus.",
        .falta_arq = "file not found in launcher folder",
        .msg_hbmenu = "Open from hbmenu to be able to launch the games.",
        .msg_applet = "Applet mode: open hbmenu holding R on a game (title takeover).",
        .msg_sem_jogo = "Game file not found in the launcher folder.",
        .msg_falha_prep = "Failed to prepare the selected game.",
        .msg_falha_ini = "Could not save data/vitart_options.ini.",
        .btn_jogar = " Play",
        .btn_abrir = " Open",
        .btn_sair = " Exit",
        .menu_sair = "Exit",
        .btn_mudar = " Change",
        .btn_salvar = " Save and back",
        .upd_menu = "Check for updates",
        .upd_titulo = "UPDATER",
        .upd_ver_atual = "Installed version: v%s",
        .upd_ver_nova = "Latest release: %s",
        .upd_buscando = "Checking GitHub for updates...",
        .upd_atualizado = "Already up to date.",
        .upd_disponivel = "New version available! Saves and game files are kept.",
        .upd_baixando = "Downloading update...",
        .upd_extraindo = "Installing update...",
        .upd_pronto = "Update installed! Press A to restart the launcher.",
        .upd_err_net = "Connection error. Check your Wi-Fi connection.",
        .upd_err_http = "GitHub returned HTTP error %ld.",
        .upd_err_asset = "No downloadable package was found in the latest release.",
        .upd_err_disk = "Could not write update files to the SD card.",
        .upd_err_zip = "Downloaded update package is invalid or corrupted.",
        .upd_err_cancel = "Update cancelled.",
        .btn_instalar = " Install",
        .btn_reinstalar = " Reinstall",
        .btn_reiniciar = " Restart",
        .btn_cancelar = " Cancel",
        .btn_voltar = " Back",
        .trp_menu = "Trophies",
        .trp_titulo = "TROPHIES",
        .trp_oculto = "Hidden trophy",
        .trp_oculto_desc = "Press Y to reveal hidden trophy details.",
        .trp_sem_trp = "TROPHY.TRP not found in game/PCSA00126/sce_sys/trophy/.",
        .trp_vazio = "No trophies match this filter.",
        .trp_filtros = { "All", "Unlocked", "Locked" },
        .trp_graus = { "Platinum", "Gold", "Silver", "Bronze" },
        .trp_desbloq = "Unlocked",
        .trp_btn_jogo = " Game",
        .trp_btn_filtro = " Filter",
        .trp_btn_spoil = " Hidden",
    },
    [LANG_ES] = {
        .nome_idioma = "Español",
        .escolher = "ELIGE EL JUEGO",
        .sub_jogos = { "El viaje de Kratos contra Ares", "La venganza contra Zeus" },
        .opcoes_menu = "Opciones",
        .opcoes_titulo = "OPCIONES",
        .op_nomes = { "Idioma", "Resolución interna", "Tasa de cuadros", "VSync",
                      "Relación de aspecto", "Contador de FPS", "Texturas HD", "Barras de vida enemigas" },
        .auto_tela = "Automática (pantalla)",
        .auto_prop = "Automática",
        .max_fps = "Máximo",
        .desligado = "Desactivado",
        .ligado = "Activado",
        .nota_opcoes = "Se aplica a ambos juegos. Los gráficos también cambian en juego con R + Plus.",
        .falta_arq = "archivo no encontrado en la carpeta del launcher",
        .msg_hbmenu = "Abre desde hbmenu para poder iniciar los juegos.",
        .msg_applet = "Modo applet: abre hbmenu manteniendo R en un juego (title takeover).",
        .msg_sem_jogo = "Archivo del juego no encontrado en la carpeta del launcher.",
        .msg_falha_prep = "Error al preparar el juego elegido.",
        .msg_falha_ini = "No se pudo guardar data/vitart_options.ini.",
        .btn_jogar = " Jugar",
        .btn_abrir = " Abrir",
        .btn_sair = " Salir",
        .menu_sair = "Salir",
        .btn_mudar = " Cambiar",
        .btn_salvar = " Guardar y volver",
        .upd_menu = "Buscar actualizaciones",
        .upd_titulo = "ACTUALIZADOR",
        .upd_ver_atual = "Versión instalada: v%s",
        .upd_ver_nova = "Última versión: %s",
        .upd_buscando = "Buscando actualizaciones en GitHub...",
        .upd_atualizado = "Ya tienes la última versión.",
        .upd_disponivel = "¡Nueva versión disponible! Partidas y archivos del juego se conservan.",
        .upd_baixando = "Descargando actualización...",
        .upd_extraindo = "Instalando actualización...",
        .upd_pronto = "¡Actualización instalada! Pulsa A para reiniciar el launcher.",
        .upd_err_net = "Error de conexión. Comprueba tu Wi-Fi.",
        .upd_err_http = "GitHub devolvió error HTTP %ld.",
        .upd_err_asset = "No se encontró ningún paquete descargable en el release.",
        .upd_err_disk = "No se pudieron escribir los archivos en la tarjeta SD.",
        .upd_err_zip = "El paquete descargado es inválido o está dañado.",
        .upd_err_cancel = "Actualización cancelada.",
        .btn_instalar = " Instalar",
        .btn_reinstalar = " Reinstalar",
        .btn_reiniciar = " Reiniciar",
        .btn_cancelar = " Cancelar",
        .btn_voltar = " Volver",
        .trp_menu = "Trofeos",
        .trp_titulo = "TROFEOS",
        .trp_oculto = "Trofeo oculto",
        .trp_oculto_desc = "Pulsa Y para ver los detalles de trofeos ocultos.",
        .trp_sem_trp = "No se encontró TROPHY.TRP en game/PCSA00126/sce_sys/trophy/.",
        .trp_vazio = "No hay trofeos para este filtro.",
        .trp_filtros = { "Todos", "Desbloqueados", "Pendientes" },
        .trp_graus = { "Platino", "Oro", "Plata", "Bronce" },
        .trp_desbloq = "Desbloqueado",
        .trp_btn_jogo = " Juego",
        .trp_btn_filtro = " Filtro",
        .trp_btn_spoil = " Ocultos",
    },
    [LANG_PT] = {
        .nome_idioma = "Português (BR)",
        .escolher = "ESCOLHA O JOGO",
        .sub_jogos = { "A jornada de Kratos contra Ares", "A vingança contra Zeus" },
        .opcoes_menu = "Opções",
        .opcoes_titulo = "OPÇÕES",
        .op_nomes = { "Idioma", "Resolução interna", "Taxa de quadros", "VSync",
                      "Proporção da tela", "Contador de FPS", "Texturas HD", "Barras de vida dos inimigos" },
        .auto_tela = "Automática (tela)",
        .auto_prop = "Automática",
        .max_fps = "Máximo",
        .desligado = "Desligado",
        .ligado = "Ligado",
        .nota_opcoes = "Valem para os dois jogos. Gráficos também mudam no jogo com R + Plus.",
        .falta_arq = "arquivo não encontrado na pasta do launcher",
        .msg_hbmenu = "Abra pelo hbmenu para poder iniciar os jogos.",
        .msg_applet = "Modo applet: abra o hbmenu segurando R num jogo (title takeover).",
        .msg_sem_jogo = "Arquivo do jogo não encontrado na pasta do launcher.",
        .msg_falha_prep = "Falha ao preparar o jogo escolhido.",
        .msg_falha_ini = "Não foi possível gravar data/vitart_options.ini.",
        .btn_jogar = " Jogar",
        .btn_abrir = " Abrir",
        .btn_sair = " Sair",
        .menu_sair = "Sair",
        .btn_mudar = " Mudar",
        .btn_salvar = " Salvar e voltar",
        .upd_menu = "Buscar atualizações",
        .upd_titulo = "ATUALIZADOR",
        .upd_ver_atual = "Versão instalada: v%s",
        .upd_ver_nova = "Última versão: %s",
        .upd_buscando = "Verificando atualizações no GitHub...",
        .upd_atualizado = "Já está na versão mais recente.",
        .upd_disponivel = "Nova versão disponível! Saves e arquivos do jogo são preservados.",
        .upd_baixando = "Baixando atualização...",
        .upd_extraindo = "Instalando atualização...",
        .upd_pronto = "Atualização instalada! Aperte A para reiniciar o launcher.",
        .upd_err_net = "Erro de conexão. Verifique seu Wi-Fi.",
        .upd_err_http = "GitHub retornou erro HTTP %ld.",
        .upd_err_asset = "Nenhum pacote baixável foi encontrado no release.",
        .upd_err_disk = "Não foi possível gravar a atualização no cartão SD.",
        .upd_err_zip = "O pacote baixado é inválido ou está corrompido.",
        .upd_err_cancel = "Atualização cancelada.",
        .btn_instalar = " Instalar",
        .btn_reinstalar = " Reinstalar",
        .btn_reiniciar = " Reiniciar",
        .btn_cancelar = " Cancelar",
        .btn_voltar = " Voltar",
        .trp_menu = "Troféus",
        .trp_titulo = "TROFÉUS",
        .trp_oculto = "Troféu oculto",
        .trp_oculto_desc = "Aperte Y para revelar os detalhes dos troféus ocultos.",
        .trp_sem_trp = "TROPHY.TRP não encontrado em game/PCSA00126/sce_sys/trophy/.",
        .trp_vazio = "Nenhum troféu corresponde a este filtro.",
        .trp_filtros = { "Todos", "Desbloqueados", "Bloqueados" },
        .trp_graus = { "Platina", "Ouro", "Prata", "Bronze" },
        .trp_desbloq = "Desbloqueado",
        .trp_btn_jogo = " Jogo",
        .trp_btn_filtro = " Filtro",
        .trp_btn_spoil = " Ocultos",
    },
    [LANG_FR] = {
        .nome_idioma = "Français",
        .escolher = "CHOISIR UN JEU",
        .sub_jogos = { "Le voyage de Kratos contre Arès", "La vengeance contre Zeus" },
        .opcoes_menu = "Options",
        .opcoes_titulo = "OPTIONS",
        .op_nomes = { "Langue", "Résolution interne", "Images par seconde", "VSync",
                      "Format d'image", "Compteur de FPS", "Textures HD", "Barres de vie ennemies" },
        .auto_tela = "Auto (écran)",
        .auto_prop = "Automatique",
        .max_fps = "Max",
        .desligado = "Désactivé",
        .ligado = "Activé",
        .nota_opcoes = "Valable pour les deux jeux. Les graphismes changent aussi en jeu avec R + Plus.",
        .falta_arq = "fichier introuvable dans le dossier du lanceur",
        .msg_hbmenu = "Ouvrez via hbmenu pour pouvoir lancer les jeux.",
        .msg_applet = "Mode applet : ouvrez hbmenu en maintenant R sur un jeu (title takeover).",
        .msg_sem_jogo = "Fichier du jeu introuvable dans le dossier du lanceur.",
        .msg_falha_prep = "Échec de la préparation du jeu choisi.",
        .msg_falha_ini = "Impossible d'enregistrer data/vitart_options.ini.",
        .btn_jogar = " Jouer",
        .btn_abrir = " Ouvrir",
        .btn_sair = " Quitter",
        .menu_sair = "Quitter",
        .btn_mudar = " Changer",
        .btn_salvar = " Enregistrer et retour",
        .upd_menu = "Rechercher des mises à jour",
        .upd_titulo = "MISE À JOUR",
        .upd_ver_atual = "Version installée : v%s",
        .upd_ver_nova = "Dernière version : %s",
        .upd_buscando = "Recherche de mises à jour sur GitHub...",
        .upd_atualizado = "Déjà à jour.",
        .upd_disponivel = "Nouvelle version disponible ! Sauvegardes et fichiers du jeu conservés.",
        .upd_baixando = "Téléchargement de la mise à jour...",
        .upd_extraindo = "Installation de la mise à jour...",
        .upd_pronto = "Mise à jour installée ! Appuyez sur A pour relancer le lanceur.",
        .upd_err_net = "Erreur de connexion. Vérifiez votre Wi-Fi.",
        .upd_err_http = "GitHub a renvoyé l'erreur HTTP %ld.",
        .upd_err_asset = "Aucun paquet téléchargeable trouvé dans la release.",
        .upd_err_disk = "Impossible d'écrire la mise à jour sur la carte SD.",
        .upd_err_zip = "Le paquet téléchargé est invalide ou corrompu.",
        .upd_err_cancel = "Mise à jour annulée.",
        .btn_instalar = " Installer",
        .btn_reinstalar = " Réinstaller",
        .btn_reiniciar = " Redémarrer",
        .btn_cancelar = " Annuler",
        .btn_voltar = " Retour",
        .trp_menu = "Trophées",
        .trp_titulo = "TROPHÉES",
        .trp_oculto = "Trophée masqué",
        .trp_oculto_desc = "Appuyez sur Y pour afficher les trophées masqués.",
        .trp_sem_trp = "TROPHY.TRP introuvable dans game/PCSA00126/sce_sys/trophy/.",
        .trp_vazio = "Aucun trophée pour ce filtre.",
        .trp_filtros = { "Tous", "Débloqués", "Verrouillés" },
        .trp_graus = { "Platine", "Or", "Argent", "Bronze" },
        .trp_desbloq = "Débloqué",
        .trp_btn_jogo = " Jeu",
        .trp_btn_filtro = " Filtre",
        .trp_btn_spoil = " Masqués",
    },
    [LANG_IT] = {
        .nome_idioma = "Italiano",
        .escolher = "SCEGLI IL GIOCO",
        .sub_jogos = { "Il viaggio di Kratos contro Ares", "La vendetta contro Zeus" },
        .opcoes_menu = "Opzioni",
        .opcoes_titulo = "OPZIONI",
        .op_nomes = { "Lingua", "Risoluzione interna", "Frequenza fotogrammi", "VSync",
                      "Proporzioni schermo", "Contatore FPS", "Texture HD", "Barre vita nemici" },
        .auto_tela = "Automatica (schermo)",
        .auto_prop = "Automatico",
        .max_fps = "Massimo",
        .desligado = "Disattivato",
        .ligado = "Attivato",
        .nota_opcoes = "Valido per entrambi i giochi. La grafica cambia anche in gioco con R + Plus.",
        .falta_arq = "file non trovato nella cartella del launcher",
        .msg_hbmenu = "Apri da hbmenu per poter avviare i giochi.",
        .msg_applet = "Modalità applet: apri hbmenu tenendo premuto R su un gioco (title takeover).",
        .msg_sem_jogo = "File del gioco non trovato nella cartella del launcher.",
        .msg_falha_prep = "Impossibile preparare il gioco selezionato.",
        .msg_falha_ini = "Impossibile salvare data/vitart_options.ini.",
        .btn_jogar = " Gioca",
        .btn_abrir = " Apri",
        .btn_sair = " Esci",
        .menu_sair = "Esci",
        .btn_mudar = " Cambia",
        .btn_salvar = " Salva e torna",
        .upd_menu = "Cerca aggiornamenti",
        .upd_titulo = "AGGIORNAMENTO",
        .upd_ver_atual = "Versione installata: v%s",
        .upd_ver_nova = "Ultima versione: %s",
        .upd_buscando = "Ricerca aggiornamenti su GitHub...",
        .upd_atualizado = "Già aggiornato.",
        .upd_disponivel = "Nuova versione disponibile! Salvataggi e file di gioco preservati.",
        .upd_baixando = "Download aggiornamento...",
        .upd_extraindo = "Installazione aggiornamento...",
        .upd_pronto = "Aggiornamento installato! Premi A per riavviare il launcher.",
        .upd_err_net = "Errore di connessione. Controlla il Wi-Fi.",
        .upd_err_http = "GitHub ha restituito l'errore HTTP %ld.",
        .upd_err_asset = "Nessun pacchetto scaricabile trovato nella release.",
        .upd_err_disk = "Impossibile scrivere l'aggiornamento sulla scheda SD.",
        .upd_err_zip = "Il pacchetto scaricato non è valido o è danneggiato.",
        .upd_err_cancel = "Aggiornamento annullato.",
        .btn_instalar = " Installa",
        .btn_reinstalar = " Reinstalla",
        .btn_reiniciar = " Riavvia",
        .btn_cancelar = " Annulla",
        .btn_voltar = " Indietro",
        .trp_menu = "Trofei",
        .trp_titulo = "TROFEI",
        .trp_oculto = "Trofeo nascosto",
        .trp_oculto_desc = "Premi Y per mostrare i dettagli dei trofei nascosti.",
        .trp_sem_trp = "TROPHY.TRP non trovato in game/PCSA00126/sce_sys/trophy/.",
        .trp_vazio = "Nessun trofeo per questo filtro.",
        .trp_filtros = { "Tutti", "Sbloccati", "Bloccati" },
        .trp_graus = { "Platino", "Oro", "Argento", "Bronzo" },
        .trp_desbloq = "Sbloccato",
        .trp_btn_jogo = " Gioco",
        .trp_btn_filtro = " Filtro",
        .trp_btn_spoil = " Nascosti",
    },
    [LANG_DE] = {
        .nome_idioma = "Deutsch",
        .escolher = "SPIEL WÄHLEN",
        .sub_jogos = { "Kratos' Reise gegen Ares", "Die Rache an Zeus" },
        .opcoes_menu = "Optionen",
        .opcoes_titulo = "OPTIONEN",
        .op_nomes = { "Sprache", "Interne Auflösung", "Bildrate", "VSync",
                      "Seitenverhältnis", "FPS-Zähler", "HD-Texturen", "Gegner-Lebensleisten" },
        .auto_tela = "Automatisch (Display)",
        .auto_prop = "Automatisch",
        .max_fps = "Maximum",
        .desligado = "Aus",
        .ligado = "Ein",
        .nota_opcoes = "Gilt für beide Spiele. Grafik ändert sich im Spiel auch mit R + Plus.",
        .falta_arq = "Datei im Launcher-Ordner nicht gefunden",
        .msg_hbmenu = "Über hbmenu öffnen, um die Spiele starten zu können.",
        .msg_applet = "Applet-Modus: hbmenu mit gedrücktem R über ein Spiel öffnen (Title Takeover).",
        .msg_sem_jogo = "Spieldatei im Launcher-Ordner nicht gefunden.",
        .msg_falha_prep = "Ausgewähltes Spiel konnte nicht vorbereitet werden.",
        .msg_falha_ini = "data/vitart_options.ini konnte nicht gespeichert werden.",
        .btn_jogar = " Spielen",
        .btn_abrir = " Öffnen",
        .btn_sair = " Beenden",
        .menu_sair = "Beenden",
        .btn_mudar = " Ändern",
        .btn_salvar = " Speichern & zurück",
        .upd_menu = "Nach Updates suchen",
        .upd_titulo = "UPDATER",
        .upd_ver_atual = "Installierte Version: v%s",
        .upd_ver_nova = "Neueste Version: %s",
        .upd_buscando = "Suche nach Updates auf GitHub...",
        .upd_atualizado = "Bereits aktuell.",
        .upd_disponivel = "Neue Version verfügbar! Spielstände und Spieldateien bleiben erhalten.",
        .upd_baixando = "Update wird heruntergeladen...",
        .upd_extraindo = "Update wird installiert...",
        .upd_pronto = "Update installiert! Drücke A zum Neustarten des Launchers.",
        .upd_err_net = "Verbindungsfehler. Prüfe deine WLAN-Verbindung.",
        .upd_err_http = "GitHub hat HTTP-Fehler %ld gemeldet.",
        .upd_err_asset = "Kein herunterladbares Paket im Release gefunden.",
        .upd_err_disk = "Update-Dateien konnten nicht auf die SD-Karte geschrieben werden.",
        .upd_err_zip = "Das heruntergeladene Update-Paket ist ungültig oder beschädigt.",
        .upd_err_cancel = "Update abgebrochen.",
        .btn_instalar = " Installieren",
        .btn_reinstalar = " Neu installieren",
        .btn_reiniciar = " Neustarten",
        .btn_cancelar = " Abbrechen",
        .btn_voltar = " Zurück",
        .trp_menu = "Trophäen",
        .trp_titulo = "TROPHÄEN",
        .trp_oculto = "Versteckte Trophäe",
        .trp_oculto_desc = "Drücke Y, um versteckte Trophäen anzuzeigen.",
        .trp_sem_trp = "TROPHY.TRP nicht in game/PCSA00126/sce_sys/trophy/ gefunden.",
        .trp_vazio = "Keine Trophäen für diesen Filter.",
        .trp_filtros = { "Alle", "Freigeschaltet", "Gesperrt" },
        .trp_graus = { "Platin", "Gold", "Silber", "Bronze" },
        .trp_desbloq = "Freigeschaltet",
        .trp_btn_jogo = " Spiel",
        .trp_btn_filtro = " Filter",
        .trp_btn_spoil = " Versteckte",
    },
};

enum { MSG_NONE = 0, MSG_HBMENU, MSG_APPLET, MSG_SEM_JOGO, MSG_FALHA_PREP, MSG_FALHA_INI };

static int idioma_sistema = LANG_EN;

static int detectar_idioma_switch(void) {
    u64 code = 0;
    if (R_SUCCEEDED(setInitialize())) {
        if (R_FAILED(setGetSystemLanguage(&code))) code = 0;
        setExit();
    }
    char tag[9];
    memcpy(tag, &code, 8);
    tag[8] = 0;
    if (!strncmp(tag, "es", 2)) return LANG_ES;
    if (!strncmp(tag, "pt", 2)) return LANG_PT;
    if (!strncmp(tag, "fr", 2)) return LANG_FR;
    if (!strncmp(tag, "it", 2)) return LANG_IT;
    if (!strncmp(tag, "de", 2)) return LANG_DE;
    return LANG_EN;
}

typedef struct { const char *titulo, *arquivo, *trp_comm; } Jogo;
static const Jogo jogos[] = {
    { "God of War", "gow1.nro", "NPWR05244_00" },
    { "God of War II", "gow2.nro", "NPWR05283_00" },
};
#define N (int)(sizeof jogos / sizeof jogos[0])
#define ITEM_TROFEUS N
#define ITEM_OPCOES  (N + 1)
#ifdef ENABLE_UPDATER
#define ITEM_UPD     (N + 2)
#define ITEM_SAIR    (N + 3)
#define NMENU        (N + 4)
#else
#define ITEM_SAIR    (N + 2)
#define NMENU        (N + 3)
#endif
static int jogo_ok[N];

static char dir[FS_MAX_PATH] = "sdmc:/switch/gow-nx/";
static char self_nro[FS_MAX_PATH] = "sdmc:/switch/gow-nx/gow-nx.nro";
static int romfs_ativo = 0;

static int existe(const char *p) { struct stat st; return stat(p, &st) == 0; }

#ifdef ENABLE_UPDATER
// Copia um arquivo do romfs:/ para a pasta do launcher no SD (se ainda não existir lá).
// Usado antes de uma atualização, para a capa/música sobreviverem a um .nro sem esses arquivos embutidos.
static void salvar_asset_no_sd(const char *nome) {
    char src[64], dst[FS_MAX_PATH], tmp[FS_MAX_PATH + 8];
    snprintf(src, sizeof src, "romfs:/%s", nome);
    snprintf(dst, sizeof dst, "%.700s%s", dir, nome);
    if (existe(dst) || !existe(src)) return;
    snprintf(tmp, sizeof tmp, "%s.tmp", dst);
    FILE *in = fopen(src, "rb");
    if (!in) return;
    FILE *out = fopen(tmp, "wb");
    if (!out) { fclose(in); return; }
    static char buf[64 * 1024];
    size_t n; int ok = 1;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        if (fwrite(buf, 1, n, out) != n) { ok = 0; break; }
    if (ferror(in)) ok = 0;
    fclose(in);
    if (fclose(out) != 0) ok = 0;
    if (ok) ok = rename(tmp, dst) == 0;
    if (!ok) remove(tmp);
}
#endif
static void caminho(char *out, size_t n, int i) { snprintf(out, n, "%.700s%s", dir, jogos[i].arquivo); }
static void atualizar_jogos(void) {
    for (int i = 0; i < N; i++) {
        char p[FS_MAX_PATH]; caminho(p, sizeof p, i);
        jogo_ok[i] = existe(p);
    }
}

static int ler_ultimo(void) {
    char p[FS_MAX_PATH]; snprintf(p, sizeof p, "%.700slauncher.cfg", dir);
    FILE *f = fopen(p, "r"); int i = 0;
    if (f) { if (fscanf(f, "%d", &i) != 1) i = 0; fclose(f); }
    return (i >= 0 && i < N) ? i : 0;
}
static void gravar_ultimo(int i) {
    char p[FS_MAX_PATH]; snprintf(p, sizeof p, "%.700slauncher.cfg", dir);
    FILE *f = fopen(p, "w"); if (f) { fprintf(f, "%d\n", i); fclose(f); }
}

#ifndef VERSAO
#define VERSAO "dev"
#endif

// ---------------------------------------------------------------- opções
// As opções gráficas (as mesmas do menu R + Plus) e o idioma ficam em <pasta>/data/vitart_options.ini.
// As linhas que o launcher não conhece (mods, cheats, ISO...) são preservadas como estão.
enum { OP_IDIOMA = 0, OP_RES, OP_FPS, OP_VSYNC, OP_ASPECTO, OP_FPS_CNT, OP_TEX_HD, OP_BARRAS, NOP };

typedef struct { const char *chave; int n; const char *const *val; int atual; } Opcao;
static const char *const l_val[] = {"auto", "en", "es", "pt-BR", "fr", "it", "de"};
static const char *const r_val[] = {"0", "1", "6", "5", "2", "3", "4"};
static const char *const r_fix[] = {"", "1x (720x408)", "Vita (960x544)", "720p (1270x720)", "2x (1440x816)", "3x (2160x1224)", "4x (2880x1632)"};
static const char *const f_val[] = {"30", "60", "max"};
static const char *const a_val[] = {"auto", "16:9", "16:10", "4:3"};
static const char *const b_val[] = {"0", "1"};
static Opcao opcoes[NOP] = {
    [OP_IDIOMA]  = { "language=",          7, l_val, 0 },
    [OP_RES]     = { "resolution_switch=", 7, r_val, 0 },
    [OP_FPS]     = { "fps=",               3, f_val, 0 },
    [OP_VSYNC]   = { "vsync=",             2, b_val, 1 },
    [OP_ASPECTO] = { "aspect=",            4, a_val, 0 },
    [OP_FPS_CNT] = { "fps_counter=",       2, b_val, 0 },
    [OP_TEX_HD]  = { "hd_textures=",       2, b_val, 1 },
    [OP_BARRAS]  = { "enemy_health_bars=", 2, b_val, 1 },
};

static int idioma_atual(void) {
    int v = opcoes[OP_IDIOMA].atual;
    return (v >= 1 && v <= NLANG) ? (v - 1) : idioma_sistema;
}
static const Textos *txt(void) { return &kTextos[idioma_atual()]; }

static const char *msg_texto(int id) {
    const Textos *t = txt();
    switch (id) {
    case MSG_HBMENU:     return t->msg_hbmenu;
    case MSG_APPLET:     return t->msg_applet;
    case MSG_SEM_JOGO:   return t->msg_sem_jogo;
    case MSG_FALHA_PREP: return t->msg_falha_prep;
    case MSG_FALHA_INI:  return t->msg_falha_ini;
    default:             return NULL;
    }
}

static int msg_inicial(void) {
    if (!envHasNextLoad()) return MSG_HBMENU;
    AppletType at = appletGetAppletType();
    if (at != AppletType_Application && at != AppletType_SystemApplication) return MSG_APPLET;
    return MSG_NONE;
}

static void opcao_valor_texto(int i, char *out, size_t n) {
    const Textos *t = txt();
    int a = opcoes[i].atual;
    switch (i) {
    case OP_IDIOMA:
        if (a == 0) snprintf(out, n, "Auto (%s)", kTextos[idioma_sistema].nome_idioma);
        else snprintf(out, n, "%s", kTextos[a - 1].nome_idioma);
        break;
    case OP_RES:
        snprintf(out, n, "%s", a == 0 ? t->auto_tela : r_fix[a]);
        break;
    case OP_FPS:
        snprintf(out, n, "%s", a == 2 ? t->max_fps : f_val[a]);
        break;
    case OP_ASPECTO:
        snprintf(out, n, "%s", a == 0 ? t->auto_prop : a_val[a]);
        break;
    default:
        snprintf(out, n, "%s", a ? t->ligado : t->desligado);
        break;
    }
}

static void ini_caminho(char *p, size_t n) { snprintf(p, n, "%.700sdata/vitart_options.ini", dir); }
static void apara(char *s) { size_t n = strlen(s); while (n && (unsigned char)s[n - 1] <= 32) s[--n] = 0; }

static const char *idioma_codigo_vitart(int lang) {
    // O vitart (hle_io.cpp) aplica data/traducoes/<codigo>/ quando language=<codigo>; vazio/ausente = inglês original.
    switch (lang) {
    case LANG_ES: return "es";
    case LANG_PT: return "pt-BR";
    case LANG_FR: return "fr";
    case LANG_IT: return "it";
    case LANG_DE: return "de";
    default:      return "";
    }
}

static int parse_idioma(const char *v) {
    if (!strcmp(v, "pt") || !strcmp(v, "pt_BR") || !strcmp(v, "pt-br")) return 3;
    for (int j = 0; j < opcoes[OP_IDIOMA].n; j++)
        if (!strcmp(v, opcoes[OP_IDIOMA].val[j])) return j;
    return -1;
}

static void opcoes_ler(void) {
    char p[FS_MAX_PATH], l[1024]; ini_caminho(p, sizeof p);
    FILE *f = fopen(p, "r"); if (!f) return;
    int lang_cfg = -1, lang_ini = -1;
    while (fgets(l, sizeof l, f)) {
        apara(l);
        if (!strncmp(l, "launcher_language=", 18)) {
            int idx = parse_idioma(l + 18);
            if (idx >= 0) lang_cfg = idx;
            continue;
        }
        for (int i = 0; i < NOP; i++) {
            size_t k = strlen(opcoes[i].chave);
            if (strncmp(l, opcoes[i].chave, k)) continue;
            const char *v = l + k;
            if (i == OP_FPS) { opcoes[i].atual = !strcmp(v, "max") ? 2 : atoi(v) == 60 ? 1 : atoi(v) > 60 ? 2 : 0; continue; }
            if (i == OP_IDIOMA) {
                int idx = parse_idioma(v);
                if (idx >= 0) lang_ini = idx;
                continue;
            }
            for (int j = 0; j < opcoes[i].n; j++) if (!strcmp(v, opcoes[i].val[j])) opcoes[i].atual = j;
        }
    }
    if (lang_cfg >= 0) opcoes[OP_IDIOMA].atual = lang_cfg;
    else if (lang_ini >= 0) opcoes[OP_IDIOMA].atual = lang_ini;
    fclose(f);
}

static int opcoes_gravar(void) {
    char p[FS_MAX_PATH], np[FS_MAX_PATH + 8], l[1024]; ini_caminho(p, sizeof p);
    { char d[FS_MAX_PATH]; snprintf(d, sizeof d, "%.700sdata", dir); mkdir(d, 0777); }
    snprintf(np, sizeof np, "%s.novo", p);
    FILE *o = fopen(np, "w"); if (!o) return 0;
    FILE *f = fopen(p, "r");
    if (f) {
        while (fgets(l, sizeof l, f)) {
            apara(l);
            int nossa = !strncmp(l, "launcher_language=", 18);
            for (int i = 0; i < NOP; i++) nossa |= !strncmp(l, opcoes[i].chave, strlen(opcoes[i].chave));
            if (!nossa) fprintf(o, "%s\n", l);
        }
        fclose(f);
    }
    fprintf(o, "launcher_language=%s\n", opcoes[OP_IDIOMA].val[opcoes[OP_IDIOMA].atual]);
    const char *vc = idioma_codigo_vitart(idioma_atual());
    if (vc[0]) fprintf(o, "language=%s\n", vc);
    for (int i = 1; i < NOP; i++) fprintf(o, "%s%s\n", opcoes[i].chave, opcoes[i].val[opcoes[i].atual]);
    int ok = !ferror(o);
    if (fclose(o) != 0) ok = 0;
    if (!ok) { remove(np); return 0; }
    remove(p);
    return rename(np, p) == 0;
}

// ---------------------------------------------------------------- música
#define AFRAMES 2048                       // quadros por buffer (~43 ms a 48 kHz)
#define ABYTES  ((AFRAMES * 4 + 0xfff) & ~0xfff)
#define NBUF 4
static Thread a_thr;
static volatile int a_parar = 0, a_ok = 0;
static void *a_mem[NBUF];
static AudioOutBuffer a_buf[NBUF];
static char musica_arq[FS_MAX_PATH];

static int resolver_musica(char *out, size_t n) {
    if (romfs_ativo && existe("romfs:/music.mp3")) {
        snprintf(out, n, "romfs:/music.mp3");
        return 1;
    }
    snprintf(out, n, "%.700smusic.mp3", dir);
    return existe(out);
}

static void musica(void *arg) {
    (void)arg;
    mpg123_handle *mh = mpg123_new(NULL, NULL);
    if (!mh) return;
    mpg123_param(mh, MPG123_FLAGS, MPG123_FORCE_STEREO | MPG123_QUIET, 0);
    if (mpg123_open(mh, musica_arq) != MPG123_OK) { mpg123_delete(mh); return; }
    long rate = 44100; int ch, enc;
    mpg123_getformat(mh, &rate, &ch, &enc);
    mpg123_format_none(mh);
    mpg123_format(mh, rate, MPG123_STEREO, MPG123_ENC_SIGNED_16);

    // Reamostragem linear rate -> 48000 (o audout só toca a 48 kHz). in[0] guarda a última amostra do bloco anterior.
    const double passo = (double)rate / 48000.0;
    static int16_t in[8192 * 2];
    size_t n_in = 0; double pos = 0;
    int16_t prev[2] = { 0, 0 };
    float ganho = 0.f;                    // sobe de 0 (fade-in) e desce ao sair (fade-out)
    int fila = 0, vazio = 0, mudo = 0;

    while (1) {
        AudioOutBuffer *b = NULL;
        if (fila < NBUF) b = &a_buf[fila++];
        else { u32 cnt = 0; audoutWaitPlayFinish(&b, &cnt, UINT64_MAX); if (!b) continue; }
        int16_t *out = (int16_t *)b->buffer;
        for (int f = 0; f < AFRAMES; f++) {
            while (!mudo && pos + 1 >= (double)n_in) {
                if (n_in) { prev[0] = in[(n_in - 1) * 2]; prev[1] = in[(n_in - 1) * 2 + 1]; pos -= n_in; }
                size_t got = 0;
                int r = mpg123_read(mh, in + 2, sizeof in - 4, &got);
                in[0] = prev[0]; in[1] = prev[1];
                n_in = got / 4 + 1; pos += 1;
                if (got) vazio = 0; else if (++vazio > 64) mudo = 1;
                if (r == MPG123_DONE) mpg123_seek(mh, 0, SEEK_SET);
            }
            if (a_parar) ganho -= 1.f / (48000 * 0.35f); else if (ganho < 1.f) ganho += 1.f / (48000 * 1.5f);
            if (ganho < 0) ganho = 0;
            if (mudo) { out[f * 2] = out[f * 2 + 1] = 0; continue; }
            size_t i = (size_t)pos; float t = (float)(pos - i), g = ganho * ganho * 0.85f;
            for (int c = 0; c < 2; c++) {
                float v = in[i * 2 + c] + (in[(i + 1) * 2 + c] - in[i * 2 + c]) * t;
                out[f * 2 + c] = (int16_t)(v * g);
            }
            pos += passo;
        }
        b->data_size = AFRAMES * 4;
        audoutAppendAudioOutBuffer(b);
        if (a_parar && ganho <= 0) break;
    }
    for (int k = 0; k < NBUF; k++) { AudioOutBuffer *r = NULL; u32 c = 0; audoutWaitPlayFinish(&r, &c, 200000000ULL); }
    mpg123_close(mh); mpg123_delete(mh);
}

static void musica_iniciar(void) {
    if (a_ok) return;
    if (!resolver_musica(musica_arq, sizeof musica_arq)) return;
    if (R_FAILED(audoutInitialize())) return;
    if (R_FAILED(audoutStartAudioOut())) { audoutExit(); return; }
    mpg123_init();
    a_parar = 0;
    for (int i = 0; i < NBUF; i++) a_mem[i] = NULL;
    for (int i = 0; i < NBUF; i++) {
        a_mem[i] = aligned_alloc(0x1000, ABYTES);
        if (!a_mem[i]) {
            audoutStopAudioOut(); audoutExit(); mpg123_exit();
            for (int j = 0; j < i; j++) { free(a_mem[j]); a_mem[j] = NULL; }
            return;
        }
        memset(a_mem[i], 0, ABYTES);
        a_buf[i].next = NULL; a_buf[i].buffer = a_mem[i]; a_buf[i].buffer_size = ABYTES;
        a_buf[i].data_size = AFRAMES * 4; a_buf[i].data_offset = 0;
    }
    if (R_FAILED(threadCreate(&a_thr, musica, NULL, NULL, 0x40000, 0x2C, -2))) {
        audoutStopAudioOut(); audoutExit(); mpg123_exit();
        for (int i = 0; i < NBUF; i++) { free(a_mem[i]); a_mem[i] = NULL; }
        return;
    }
    threadStart(&a_thr); a_ok = 1;
}

static void musica_parar(void) {                 // fade-out curto e libera o áudio para o jogo
    if (!a_ok) return;
    a_parar = 1;
    threadWaitForExit(&a_thr); threadClose(&a_thr);
    audoutStopAudioOut(); audoutExit(); mpg123_exit();
    for (int i = 0; i < NBUF; i++) { free(a_mem[i]); a_mem[i] = NULL; }
    a_ok = 0;
}

// ---------------------------------------------------------------- desenho
static u32 *bg;                                   // capa já em RGBA (formato do framebuffer), com o lado esquerdo escurecido
static FT_Library ft;
static FT_Face f_txt, f_ext;                      // fonte do sistema + fonte com os botões da Nintendo
static u32 *px; static u32 pitch;                 // destino atual (pitch em pixels)

static inline void mistura(int x, int y, u32 cor, int a) {
    if ((unsigned)x >= W || (unsigned)y >= H || a <= 0) return;
    if (a > 255) a = 255;
    u32 *d = &px[y * pitch + x], o = *d;
    int r = ((o & 0xff) * (255 - a) + (cor & 0xff) * a) / 255;
    int g = (((o >> 8) & 0xff) * (255 - a) + ((cor >> 8) & 0xff) * a) / 255;
    int b = (((o >> 16) & 0xff) * (255 - a) + ((cor >> 16) & 0xff) * a) / 255;
    *d = r | g << 8 | b << 16 | 0xff000000u;
}

static u32 utf8(const char **s) {
    const unsigned char *p = (const unsigned char *)*s; u32 c = *p++;
    if (c >= 0xf0) { c = (c & 7) << 18 | (p[0] & 63) << 12 | (p[1] & 63) << 6 | (p[2] & 63); p += 3; }
    else if (c >= 0xe0) { c = (c & 15) << 12 | (p[0] & 63) << 6 | (p[1] & 63); p += 2; }
    else if (c >= 0xc0) { c = (c & 31) << 6 | (p[0] & 63); p += 1; }
    *s = (const char *)p; return c;
}

// Desenha texto UTF-8 com a linha de base em y. Devolve a largura.
static int texto(int x, int y, int tam, u32 cor, int alfa, const char *s) {
    int x0 = x;
    while (*s) {
        u32 c = utf8(&s);
        FT_Face f = f_txt;
        if (c >= 0xE000 && c <= 0xF8FF && f_ext && FT_Get_Char_Index(f_ext, c)) f = f_ext;
        if (!f) break;
        FT_Set_Pixel_Sizes(f, 0, tam);
        if (FT_Load_Char(f, c, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g = f->glyph;
        for (unsigned yy = 0; yy < g->bitmap.rows; yy++)
            for (unsigned xx = 0; xx < g->bitmap.width; xx++) {
                int a = g->bitmap.buffer[yy * g->bitmap.pitch + xx] * alfa / 255;
                mistura(x + g->bitmap_left + xx, y - g->bitmap_top + yy, cor, a);
            }
        x += g->advance.x >> 6;
    }
    return x - x0;
}

static int texto_sombra(int x, int y, int tam, u32 cor, int alfa, const char *s) {
    texto(x + 2, y + 3, tam, 0x000000, alfa * 3 / 4, s);
    return texto(x, y, tam, cor, alfa, s);
}

static void retangulo(int x, int y, int w, int h, u32 cor, int a) {
    for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) mistura(xx, yy, cor, a);
}

#define RGB(r, g, b) ((u32)(r) | (u32)(g) << 8 | (u32)(b) << 16)
static const u32 OURO = RGB(236, 178, 74), OURO_ESC = RGB(150, 92, 30), CINZA = RGB(190, 180, 165),
                 BRANCO = RGB(240, 232, 220), VERMELHO = RGB(230, 80, 60),
                 PLATINA = RGB(165, 220, 255), PRATA = RGB(202, 210, 222), BRONZE = RGB(205, 134, 82),
                 VERDE = RGB(115, 210, 120);

static int texto_largura(int tam, const char *s) {
    if (!f_txt || !s) return 0;
    FT_Set_Pixel_Sizes(f_txt, 0, tam);
    int w = 0;
    while (*s) {
        u32 c = utf8(&s);
        FT_Face f = (c >= 0xE000 && c <= 0xF8FF && f_ext && FT_Get_Char_Index(f_ext, c)) ? f_ext : f_txt;
        FT_Set_Pixel_Sizes(f, 0, tam);
        if (!FT_Load_Char(f, c, FT_LOAD_DEFAULT)) w += f->glyph->advance.x >> 6;
    }
    return w;
}

static void texto_trunc(int x, int y, int tam, u32 cor, int alfa, const char *s, int max_w) {
    if (!s || !*s) return;
    if (texto_largura(tam, s) <= max_w) {
        texto_sombra(x, y, tam, cor, alfa, s);
        return;
    }
    char buf[384];
    int ew = texto_largura(tam, "...");
    int cur = 0;
    size_t out = 0;
    const char *p = s;
    while (*p && out + 8 < sizeof buf) {
        const char *prev = p;
        u32 c = utf8(&p);
        int adv = 0;
        if (f_txt) {
            FT_Set_Pixel_Sizes(f_txt, 0, tam);
            if (!FT_Load_Char(f_txt, c, FT_LOAD_DEFAULT)) adv = f_txt->glyph->advance.x >> 6;
        }
        if (cur + adv + ew > max_w) break;
        size_t clen = (size_t)(p - prev);
        if (out + clen + 4 >= sizeof buf) break;
        memcpy(buf + out, prev, clen);
        out += clen;
        cur += adv;
    }
    while (out > 0 && buf[out - 1] == ' ') out--;
    memcpy(buf + out, "...", 4);
    texto_sombra(x, y, tam, cor, alfa, buf);
}

static void texto_wrap2(int x, int y, int tam, int dy, u32 cor, int alfa, const char *s, int max_w) {
    if (!s || !*s) return;
    if (texto_largura(tam, s) <= max_w) {
        texto_sombra(x, y, tam, cor, alfa, s);
        return;
    }
    char l1[256];
    const char *p = s, *last_sp = NULL, *cut = s;
    int cur = 0;
    while (*p) {
        const char *prev = p;
        u32 c = utf8(&p);
        int adv = 0;
        if (f_txt) {
            FT_Set_Pixel_Sizes(f_txt, 0, tam);
            if (!FT_Load_Char(f_txt, c, FT_LOAD_DEFAULT)) adv = f_txt->glyph->advance.x >> 6;
        }
        if (cur + adv > max_w) {
            cut = last_sp ? last_sp : prev;
            break;
        }
        cur += adv;
        if (c == ' ') last_sp = prev;
        cut = p;
    }
    size_t n1 = (size_t)(cut - s);
    if (n1 >= sizeof l1) n1 = sizeof l1 - 1;
    memcpy(l1, s, n1);
    l1[n1] = 0;
    texto_sombra(x, y, tam, cor, alfa, l1);
    while (*cut == ' ') cut++;
    if (*cut) texto_trunc(x, y + dy, tam, cor, alfa, cut, max_w);
}

static void barra_progresso(int x, int y, int w, int h, float frac, int indet, u64 tick) {
    retangulo(x, y, w, h, 0x000000, 170);
    retangulo(x, y, w, 2, OURO_ESC, 220);
    retangulo(x, y + h - 2, w, 2, OURO_ESC, 220);
    retangulo(x, y, 2, h, OURO_ESC, 220);
    retangulo(x + w - 2, y, 2, h, OURO_ESC, 220);
    int iw = w - 6, ih = h - 6;
    if (indet) {
        int bw = iw / 4;
        int ox = (int)((tick * 6) % (u64)(iw + bw)) - bw;
        int x0 = ox < 0 ? 0 : ox;
        int x1 = ox + bw > iw ? iw : ox + bw;
        if (x1 > x0) retangulo(x + 3 + x0, y + 3, x1 - x0, ih, OURO, 230);
    } else {
        if (frac < 0.f) frac = 0.f;
        if (frac > 1.f) frac = 1.f;
        int fw = (int)(iw * frac + 0.5f);
        if (fw > 0) retangulo(x + 3, y + 3, fw, ih, OURO, 235);
    }
}

static u32 cor_grau(char g) {
    switch (g) {
    case 'P': return PLATINA;
    case 'G': return OURO;
    case 'S': return PRATA;
    default:  return BRONZE;
    }
}

static int idx_grau(char g) {
    switch (g) {
    case 'P': return 0;
    case 'G': return 1;
    case 'S': return 2;
    default:  return 3;
    }
}

static void desenhar_icone_trofeu(int x, int y, const TrophyItem *it, int mostrar_ocultos) {
    int oculto = it->hidden && !it->unlocked && !mostrar_ocultos;
    u32 borda = it->unlocked ? cor_grau(it->grade) : RGB(80, 74, 68);
    retangulo(x - 1, y - 1, TRP_ICON_SZ + 2, TRP_ICON_SZ + 2, borda, it->unlocked ? 235 : 140);
    retangulo(x, y, TRP_ICON_SZ, TRP_ICON_SZ, RGB(18, 15, 13), 235);
    if (oculto || !it->icon) {
        const char *m = oculto ? "?" : (it->grade == 'P' ? "P" : it->grade == 'G' ? "G" : it->grade == 'S' ? "S" : "B");
        int mw = texto_largura(24, m);
        texto_sombra(x + (TRP_ICON_SZ - mw) / 2, y + 34, 24, oculto ? CINZA : borda, 210, m);
        return;
    }
    for (int yy = 0; yy < TRP_ICON_SZ; yy++) {
        for (int xx = 0; xx < TRP_ICON_SZ; xx++) {
            u32 c = it->icon[yy * TRP_ICON_SZ + xx];
            int a = (int)(c >> 24);
            if (!a) continue;
            int r = (int)(c & 0xff), g = (int)((c >> 8) & 0xff), b = (int)((c >> 16) & 0xff);
            if (!it->unlocked) {
                int lum = (r * 77 + g * 150 + b * 29) >> 8;
                lum = lum * 45 / 100;
                r = g = b = lum;
            }
            mistura(x + xx, y + yy, RGB(r, g, b), a);
        }
    }
}

static int filtrar_trofeus(const TrophySet *set, int filtro, int *vis) {
    int n = 0;
    for (int i = 0; i < set->count; i++) {
        if (filtro == 1 && !set->items[i].unlocked) continue;
        if (filtro == 2 && set->items[i].unlocked) continue;
        vis[n++] = i;
    }
    return n;
}

static void assinatura(int yb);

static void desenhar_trofeus(const TrophySet *set, int jogo_idx, int filtro, int mostrar_ocultos,
                             int sel_idx, int scroll, u64 tick) {
    const Textos *t = txt();
    if (bg) { for (int y = 0; y < H; y++) memcpy(&px[y * pitch], &bg[y * W], W * 4); }
    else { for (int y = 0; y < H; y++) memset(&px[y * pitch], 0, W * 4); }
    retangulo(0, 0, W, H, 0x000000, 150);
    float pulso = 0.5f + 0.5f * sinf(tick * 0.06f);

    // Cabeçalho com abas dos 2 jogos e resumo de progresso
    texto_sombra(72, 68, 26, OURO, 255, t->trp_titulo);
    int tx = 260;
    for (int i = 0; i < N; i++) {
        int a = (i == jogo_idx);
        int tw = texto_largura(22, jogos[i].titulo);
        if (a) {
            retangulo(tx - 12, 42, tw + 24, 34, OURO_ESC, 120);
            retangulo(tx - 12, 74, tw + 24, 3, OURO, 240);
        }
        texto_sombra(tx, 66, 22, a ? BRANCO : CINZA, a ? 255 : 170, jogos[i].titulo);
        tx += tw + 42;
    }

    // Filtro atual no canto superior direito
    char fbuf[64];
    snprintf(fbuf, sizeof fbuf, "[ %s ]", t->trp_filtros[filtro]);
    texto_sombra(W - 72 - texto_largura(20, fbuf), 66, 20, OURO, 235, fbuf);
    retangulo(72, 84, W - 144, 2, OURO_ESC, 180);

    if (!set->loaded) {
        texto_sombra(72, 220, 24, VERMELHO, 255, t->trp_sem_trp);
        char ex[256];
        snprintf(ex, sizeof ex, "game/PCSA00126/sce_sys/trophy/%s/TROPHY.TRP", jogos[jogo_idx].trp_comm);
        texto_sombra(72, 262, 19, CINZA, 210, ex);
    } else {
        int pct = trp_percent(set);
        char sum[128];
        snprintf(sum, sizeof sum, "%d / %d  (%d%%)", set->unlocked_count, set->count, pct);
        texto_sombra(72, 118, 22, BRANCO, 250, sum);
        barra_progresso(280, 100, 400, 20, (float)pct / 100.f, 0, tick);

        // Contadores por grau (P / G / S / B)
        static const char *const gsym[4] = { "P", "G", "S", "B" };
        static const char gchr[4] = { 'P', 'G', 'S', 'B' };
        int gx = 710;
        for (int g = 0; g < 4; g++) {
            char gb[32];
            snprintf(gb, sizeof gb, "%s %d/%d", gsym[g], set->grade_have[g], set->grade_total[g]);
            gx += texto_sombra(gx, 118, 19, cor_grau(gchr[g]), 240, gb) + 26;
        }

        int vis[TRP_MAX_TROPHIES];
        int nvis = filtrar_trofeus(set, filtro, vis);
        if (nvis == 0) {
            texto_sombra(72, 220, 24, CINZA, 230, t->trp_vazio);
        } else {
            const int vis_rows = 6, row_h = 68, y0 = 144;
            for (int r = 0; r < vis_rows && scroll + r < nvis; r++) {
                int vi = scroll + r;
                const TrophyItem *it = &set->items[vis[vi]];
                int y = y0 + r * row_h;
                int s = (vi == sel_idx);
                if (s) {
                    retangulo(72, y, W - 160, row_h - 6, OURO_ESC, 95);
                    retangulo(72, y, 5, row_h - 6, OURO, 160 + (int)(95 * pulso));
                } else {
                    retangulo(72, y, W - 160, row_h - 6, RGB(18, 15, 13), 135);
                }

                desenhar_icone_trofeu(88, y + 5, it, mostrar_ocultos);

                int oculto = it->hidden && !it->unlocked && !mostrar_ocultos;
                const char *nm = oculto ? t->trp_oculto : it->name;
                const char *ds = oculto ? t->trp_oculto_desc : it->detail;
                u32 nc = it->unlocked ? (s ? OURO : BRANCO) : (s ? BRANCO : CINZA);
                texto_trunc(156, y + 26, 22, nc, it->unlocked || s ? 255 : 195, nm, 760);
                texto_trunc(156, y + 50, 17, CINZA, s ? 230 : 175, ds, 760);

                // Grau + estado à direita
                const char *gname = t->trp_graus[idx_grau(it->grade)];
                int gw = texto_largura(18, gname);
                texto_sombra(W - 104 - gw, y + 26, 18, cor_grau(it->grade), it->unlocked ? 255 : 170, gname);
                if (it->unlocked) {
                    int dw = texto_largura(16, t->trp_desbloq);
                    texto_sombra(W - 104 - dw, y + 48, 16, VERDE, 235, t->trp_desbloq);
                }
            }

            // Barra de rolagem lateral
            if (nvis > vis_rows) {
                int sx = W - 78, sy = y0, sh = vis_rows * row_h - 6;
                retangulo(sx, sy, 6, sh, RGB(60, 54, 48), 160);
                int bh = sh * vis_rows / nvis;
                if (bh < 24) bh = 24;
                int by = sy + (sh - bh) * scroll / (nvis - vis_rows);
                retangulo(sx, by, 6, bh, OURO, 220);
            }

            // Cartão inferior com a descrição completa do troféu selecionado
            if (sel_idx >= 0 && sel_idx < nvis) {
                const TrophyItem *it = &set->items[vis[sel_idx]];
                int oculto = it->hidden && !it->unlocked && !mostrar_ocultos;
                retangulo(72, 560, W - 144, 78, RGB(14, 11, 10), 200);
                retangulo(72, 560, W - 144, 2, OURO_ESC, 180);
                texto_wrap2(90, 588, 19, 25, BRANCO, 235, oculto ? t->trp_oculto_desc : it->detail, W - 180);
            }
        }
    }

    int x = 72, yb = 676;
    x += texto_sombra(x, yb, 26, BRANCO, 255, "\xEE\x83\xA4/\xEE\x83\xA5"); // L/R
    x += texto_sombra(x + 4, yb, 20, BRANCO, 230, t->trp_btn_jogo) + 28;
    x += texto_sombra(x, yb, 26, BRANCO, 255, "\xEE\x83\xA2");             // X
    x += texto_sombra(x + 4, yb, 20, BRANCO, 230, t->trp_btn_filtro) + 28;
    x += texto_sombra(x, yb, 26, BRANCO, 255, "\xEE\x83\xA3");             // Y
    x += texto_sombra(x + 4, yb, 20, BRANCO, 230, t->trp_btn_spoil) + 28;
    x += texto_sombra(x, yb, 26, BRANCO, 255, "\xEE\x83\xA1");             // B
    texto_sombra(x + 4, yb, 20, BRANCO, 230, t->btn_voltar);
    assinatura(yb);
}

static void assinatura(int yb) {
    if (!f_txt) return;
    const char *ass = "Guto Recomp", *ver = "v" VERSAO;
    FT_Set_Pixel_Sizes(f_txt, 0, 22);
    int wa = 0, wv = 0;
    { int sx = 0; const char *s = ass; while (*s) { u32 c = utf8(&s); if (!FT_Load_Char(f_txt, c, FT_LOAD_DEFAULT)) sx += f_txt->glyph->advance.x >> 6; } wa = sx; }
    FT_Set_Pixel_Sizes(f_txt, 0, 18);
    { int sx = 0; const char *s = ver; while (*s) { u32 c = utf8(&s); if (!FT_Load_Char(f_txt, c, FT_LOAD_DEFAULT)) sx += f_txt->glyph->advance.x >> 6; } wv = sx; }
    texto_sombra(W - 60 - wa - 12 - wv, yb, 22, OURO, 235, ass);
    texto_sombra(W - 60 - wv, yb, 18, CINZA, 220, ver);
}

static void desenhar_opcoes(int sel, int msg, u64 tick) {
    const Textos *t = txt();
    if (bg) { for (int y = 0; y < H; y++) memcpy(&px[y * pitch], &bg[y * W], W * 4); }
    else { for (int y = 0; y < H; y++) memset(&px[y * pitch], 0, W * 4); }
    float pulso = 0.5f + 0.5f * sinf(tick * 0.06f);
    texto_sombra(110, 174, 26, CINZA, 255, t->opcoes_titulo);
    retangulo(110, 188, 300, 2, OURO_ESC, 200);
    for (int i = 0; i < NOP; i++) {
        int y = 228 + i * 44, s = i == sel;
        if (s) {
            for (int x = 96; x < 900; x++) retangulo(x, y - 30, 1, 40, OURO_ESC, 80 * (900 - x) / 804);
            retangulo(90, y - 30, 5, 40, OURO, 150 + (int)(105 * pulso));
        }
        texto_sombra(116, y, 24, s ? OURO : CINZA, s ? 255 : 210, t->op_nomes[i]);
        char raw[80], v[96];
        opcao_valor_texto(i, raw, sizeof raw);
        snprintf(v, sizeof v, s ? "<  %s  >" : "%s", raw);
        texto_sombra(520, y, 24, s ? BRANCO : CINZA, s ? 255 : 200, v);
    }
    texto_sombra(116, 228 + NOP * 44 + 8, 18, CINZA, 200, t->nota_opcoes);
    const char *ms = msg_texto(msg);
    if (ms) texto_sombra(110, 640, 22, VERMELHO, 255, ms);
    int x = 110, yb = 676;
    x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA0");   // U+E0E0 botão A
    x += texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_mudar) + 44;
    x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA1");   // U+E0E1 botão B
    texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_salvar);
    assinatura(yb);
}

#ifdef ENABLE_UPDATER
static void desenhar_atualizador(const UpdStatus *st, u64 tick) {
    const Textos *t = txt();
    if (bg) { for (int y = 0; y < H; y++) memcpy(&px[y * pitch], &bg[y * W], W * 4); }
    else { for (int y = 0; y < H; y++) memset(&px[y * pitch], 0, W * 4); }

    texto_sombra(110, 174, 26, CINZA, 255, t->upd_titulo);
    retangulo(110, 188, 300, 2, OURO_ESC, 200);
    texto_sombra(110, 224, 18, CINZA, 195, "GitHub: " UPDATE_REPO);

    char linha[256];
    snprintf(linha, sizeof linha, t->upd_ver_atual, VERSAO);
    texto_sombra(110, 272, 24, BRANCO, 240, linha);
    if (st->latest_tag[0]) {
        snprintf(linha, sizeof linha, t->upd_ver_nova, st->latest_tag);
        texto_sombra(110, 312, 24, OURO, 255, linha);
    }

    if (st->state == UPD_CHECKING) {
        texto_sombra(110, 374, 24, BRANCO, 240, t->upd_buscando);
        barra_progresso(110, 412, 680, 22, 0.f, 1, tick);
    } else if (st->state == UPD_UP_TO_DATE) {
        texto_sombra(110, 374, 24, OURO, 255, t->upd_atualizado);
        if (st->asset_name[0]) {
            if (st->asset_size > 0)
                snprintf(linha, sizeof linha, "%s (%.1f MB)", st->asset_name, (double)st->asset_size / (1024.0 * 1024.0));
            else
                snprintf(linha, sizeof linha, "%s", st->asset_name);
            texto_sombra(110, 416, 20, CINZA, 210, linha);
        }
    } else if (st->state == UPD_AVAILABLE) {
        texto_sombra(110, 374, 24, OURO, 255, t->upd_disponivel);
        if (st->asset_name[0]) {
            if (st->asset_size > 0)
                snprintf(linha, sizeof linha, "%s (%.1f MB)", st->asset_name, (double)st->asset_size / (1024.0 * 1024.0));
            else
                snprintf(linha, sizeof linha, "%s", st->asset_name);
            texto_sombra(110, 416, 20, BRANCO, 220, linha);
        }
    } else if (st->state == UPD_DOWNLOADING) {
        texto_sombra(110, 374, 24, BRANCO, 245, t->upd_baixando);
        float frac = st->dl_total > 0 ? (float)((double)st->dl_now / (double)st->dl_total) : 0.f;
        barra_progresso(110, 412, 680, 24, frac, st->dl_total == 0, tick);
        if (st->dl_total > 0) {
            int pct = (int)(frac * 100.f + 0.5f);
            if (pct > 100) pct = 100;
            snprintf(linha, sizeof linha, "%d%%  (%.1f / %.1f MB)", pct,
                     (double)st->dl_now / (1024.0 * 1024.0), (double)st->dl_total / (1024.0 * 1024.0));
        } else {
            snprintf(linha, sizeof linha, "%.1f MB", (double)st->dl_now / (1024.0 * 1024.0));
        }
        texto_sombra(110, 470, 20, CINZA, 230, linha);
    } else if (st->state == UPD_EXTRACTING) {
        texto_sombra(110, 374, 24, BRANCO, 245, t->upd_extraindo);
        float frac = st->files_total > 0 ? (float)st->files_done / (float)st->files_total : 0.f;
        barra_progresso(110, 412, 680, 24, frac, st->files_total <= 0, tick);
        if (st->files_total > 0)
            snprintf(linha, sizeof linha, "%d / %d  %s", st->files_done, st->files_total, st->current_file);
        else
            snprintf(linha, sizeof linha, "%s", st->current_file);
        texto_sombra(110, 470, 20, CINZA, 230, linha);
    } else if (st->state == UPD_DONE) {
        texto_sombra(110, 374, 24, OURO, 255, t->upd_pronto);
        barra_progresso(110, 412, 680, 24, 1.f, 0, tick);
    } else if (st->state == UPD_ERROR) {
        const char *em = t->upd_err_net;
        if (st->err == UPD_ERR_HTTP) { snprintf(linha, sizeof linha, t->upd_err_http, st->http_code); em = linha; }
        else if (st->err == UPD_ERR_NO_ASSET) em = t->upd_err_asset;
        else if (st->err == UPD_ERR_DISK) em = t->upd_err_disk;
        else if (st->err == UPD_ERR_ZIP) em = t->upd_err_zip;
        else if (st->err == UPD_ERR_CANCELLED) em = t->upd_err_cancel;
        texto_sombra(110, 374, 24, VERMELHO, 255, em);
    }

    int x = 110, yb = 676;
    if (st->state == UPD_AVAILABLE) {
        x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA0");
        x += texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_instalar) + 44;
        x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA1");
        texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_voltar);
    } else if (st->state == UPD_UP_TO_DATE) {
        if (st->asset_url[0]) {
            x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA0");
            x += texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_reinstalar) + 44;
        }
        x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA1");
        texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_voltar);
    } else if (st->state == UPD_DONE) {
        x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA0");
        texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_reiniciar);
    } else if (st->state == UPD_CHECKING || st->state == UPD_DOWNLOADING || st->state == UPD_EXTRACTING) {
        x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA1");
        texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_cancelar);
    } else {
        x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA1");
        texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_voltar);
    }
    assinatura(yb);
}
#endif

static void desenhar(int sel, int msg, u64 tick) {
    const Textos *t = txt();
    if (bg) { for (int y = 0; y < H; y++) memcpy(&px[y * pitch], &bg[y * W], W * 4); }
    else { for (int y = 0; y < H; y++) memset(&px[y * pitch], 0, W * 4); }
    float pulso = 0.5f + 0.5f * sinf(tick * 0.06f);

    texto_sombra(110, 178, 26, CINZA, 255, t->escolher);
    retangulo(110, 192, 260, 2, OURO_ESC, 200);

    for (int i = 0; i < N; i++) {
        int y = 260 + i * 98;
        int ok = jogo_ok[i], s = i == sel;
        if (s) {
            for (int x = 96; x < 560; x++) retangulo(x, y - 48, 1, 80, OURO_ESC, 80 * (560 - x) / 464);
            retangulo(90, y - 48, 6, 80, OURO, 150 + (int)(105 * pulso));
        }
        texto_sombra(120, y, s ? 46 : 40, s ? OURO : CINZA, s ? 255 : 200, jogos[i].titulo);
        texto_sombra(122, y + 26, 20, ok ? (s ? BRANCO : CINZA) : VERMELHO, s ? 230 : 160,
                     ok ? t->sub_jogos[i] : t->falta_arq);
    }

    {   // Itens secundários (Troféus, Opções, Atualizador, Sair)
        const char *rotulos[4];
        int ids[4], nsec = 0;
        rotulos[nsec] = t->trp_menu;    ids[nsec++] = ITEM_TROFEUS;
        rotulos[nsec] = t->opcoes_menu; ids[nsec++] = ITEM_OPCOES;
#ifdef ENABLE_UPDATER
        rotulos[nsec] = t->upd_menu;    ids[nsec++] = ITEM_UPD;
#endif
        rotulos[nsec] = t->menu_sair;   ids[nsec++] = ITEM_SAIR;
        for (int i = 0; i < nsec; i++) {
            int y = 432 + i * 46, s = sel == ids[i];
            if (s) {
                for (int x = 96; x < 560; x++) retangulo(x, y - 30, 1, 42, OURO_ESC, 80 * (560 - x) / 464);
                retangulo(90, y - 30, 5, 42, OURO, 150 + (int)(105 * pulso));
            }
            texto_sombra(120, y, s ? 28 : 24, s ? OURO : CINZA, s ? 255 : 200, rotulos[i]);
        }
    }

    const char *ms = msg_texto(msg);
    if (ms) texto_sombra(110, 640, 22, VERMELHO, 255, ms);

    int x = 110, yb = 676;
    const char *lbl_a = sel == ITEM_SAIR ? t->btn_sair : sel >= N ? t->btn_abrir : t->btn_jogar;
    x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xA0");   // U+E0E0 botão A
    x += texto_sombra(x + 4, yb, 22, BRANCO, 230, lbl_a) + 44;
    x += texto_sombra(x, yb, 28, BRANCO, 255, "\xEE\x83\xAF");   // U+E0EF botão +
    texto_sombra(x + 4, yb, 22, BRANCO, 230, t->btn_sair);

    assinatura(yb); // canto inferior direito
}

static void carregar_capa(void) {
    bg = malloc(W * H * 4);
    if (!bg) return;
    FILE *f = romfs_ativo ? fopen("romfs:/bg.bgra", "rb") : NULL;
    if (!f) {
        char p[FS_MAX_PATH]; snprintf(p, sizeof p, "%.700sbg.bgra", dir);
        f = fopen(p, "rb");
    }
    size_t n = 0;
    if (f) { n = fread(bg, 4, W * H, f); fclose(f); }
    if (n != W * H) memset(bg, 0, W * H * 4);
    for (int i = 0; i < W * H; i++) { u32 v = bg[i]; bg[i] = (v >> 16 & 0xff) | (v & 0xff00) | (v & 0xff) << 16 | 0xff000000u; }
    // escurece o lado esquerdo (já é escuro na arte) uma vez só, para o menu ler bem
    px = bg; pitch = W;
    for (int x = 0; x < 640; x++) {
        int a = x < 420 ? 110 : 110 * (640 - x) / 220;
        for (int y = 0; y < H; y++) mistura(x, y, 0, a);
    }
}

int main(int argc, char **argv) {
    // Pasta do launcher (onde também ficam gow1.nro / gow2.nro), tirada do argv[0].
    if (argc > 0 && argv[0] && strchr(argv[0], '/')) {
        const char *a = argv[0];
        if (strncmp(a, "sdmc:", 5) != 0 && a[0] == '/') snprintf(self_nro, sizeof self_nro, "sdmc:%.700s", a);
        else snprintf(self_nro, sizeof self_nro, "%.700s", a);
        snprintf(dir, sizeof dir, "%s", self_nro);
        char *b = strrchr(dir, '/'); if (b) b[1] = 0;
    }
    if (R_SUCCEEDED(romfsInit())) romfs_ativo = 1;
    carregar_capa();

    plInitialize(PlServiceType_User);
    FT_Init_FreeType(&ft);
    PlFontData fd;
    if (R_SUCCEEDED(plGetSharedFontByType(&fd, PlSharedFontType_Standard)))
        FT_New_Memory_Face(ft, fd.address, fd.size, 0, &f_txt);
    if (R_SUCCEEDED(plGetSharedFontByType(&fd, PlSharedFontType_NintendoExt)))
        FT_New_Memory_Face(ft, fd.address, fd.size, 0, &f_ext);

    Framebuffer fb;
    framebufferCreate(&fb, nwindowGetDefault(), W, H, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad; padInitializeDefault(&pad);

    musica_iniciar();

    idioma_sistema = detectar_idioma_switch();
    opcoes_ler();
    atualizar_jogos();
    int sel = ler_ultimo(), lancar = 0, tela_op = 0, sel_op = 0;
    int tela_trp = 0, trp_jogo = 0, trp_filtro = 0, trp_ocultos = 0, trp_sel = 0, trp_scroll = 0;
    int trp_hold = 0, trp_hold_dir = 0;
    TrophySet trp_set;
    memset(&trp_set, 0, sizeof trp_set);
#ifdef ENABLE_UPDATER
    int tela_upd = 0;
#endif
    int msg = msg_inicial();
    u64 tick = 0;
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 k = padGetButtonsDown(&pad);
        u64 h = padGetButtons(&pad);
        if (tela_trp) {
            int vis[TRP_MAX_TROPHIES];
            int nvis = trp_set.loaded ? filtrar_trofeus(&trp_set, trp_filtro, vis) : 0;
            if (k & (HidNpadButton_B | HidNpadButton_Plus)) {
                trp_free(&trp_set);
                tela_trp = 0;
                msg = msg_inicial();
                px = framebufferBegin(&fb, &pitch); pitch /= 4;
                desenhar(sel, msg, tick++);
                framebufferEnd(&fb);
                continue;
            }
            {
                int mudou_jogo = 0;
                if (k & (HidNpadButton_L | HidNpadButton_AnyLeft)) { trp_jogo = (trp_jogo + N - 1) % N; mudou_jogo = 1; }
                if (k & (HidNpadButton_R | HidNpadButton_AnyRight)) { trp_jogo = (trp_jogo + 1) % N; mudou_jogo = 1; }
                if (mudou_jogo) {
                    trp_load(&trp_set, dir, jogos[trp_jogo].trp_comm, l_val[idioma_atual() + 1]);
                    trp_sel = trp_scroll = 0;
                    nvis = trp_set.loaded ? filtrar_trofeus(&trp_set, trp_filtro, vis) : 0;
                }
                if (k & HidNpadButton_X) {
                    trp_filtro = (trp_filtro + 1) % 3;
                    trp_sel = trp_scroll = 0;
                    nvis = trp_set.loaded ? filtrar_trofeus(&trp_set, trp_filtro, vis) : 0;
                }
                if (k & HidNpadButton_Y) trp_ocultos = !trp_ocultos;
                int dir_v = (h & HidNpadButton_AnyUp) ? -1 : (h & HidNpadButton_AnyDown) ? 1 : 0;
                if (dir_v != trp_hold_dir) { trp_hold_dir = dir_v; trp_hold = 0; }
                if (nvis > 0) {
                    if (dir_v && (trp_hold == 0 || (trp_hold >= 18 && trp_hold % 4 == 0)))
                        trp_sel = (trp_sel + dir_v + nvis) % nvis;
                    if (dir_v) trp_hold++;
                    if (k & HidNpadButton_ZL) { trp_sel -= 6; if (trp_sel < 0) trp_sel = 0; }
                    if (k & HidNpadButton_ZR) { trp_sel += 6; if (trp_sel >= nvis) trp_sel = nvis - 1; }
                    if (trp_sel < trp_scroll) trp_scroll = trp_sel;
                    if (trp_sel >= trp_scroll + 6) trp_scroll = trp_sel - 5;
                    if (trp_scroll > nvis - 6) trp_scroll = nvis > 6 ? nvis - 6 : 0;
                    if (trp_scroll < 0) trp_scroll = 0;
                }
            }
            px = framebufferBegin(&fb, &pitch); pitch /= 4;
            desenhar_trofeus(&trp_set, trp_jogo, trp_filtro, trp_ocultos, trp_sel, trp_scroll, tick++);
            framebufferEnd(&fb);
            continue;
        }
#ifdef ENABLE_UPDATER
        if (tela_upd) {
            UpdStatus st;
            upd_get_status(&st);
            if (st.state == UPD_DONE) {
                if (k & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_Plus)) {
                    upd_shutdown();
                    if (envHasNextLoad()) {
                        char args[FS_MAX_PATH + 4]; snprintf(args, sizeof args, "\"%.760s\"", self_nro);
                        envSetNextLoad(self_nro, args);
                    }
                    lancar = 1;
                }
            } else if (st.state == UPD_AVAILABLE || (st.state == UPD_UP_TO_DATE && st.asset_url[0])) {
                if (k & HidNpadButton_A) {
                    // Solta romfs:/ antes de substituir o próprio gow-nx.nro no cartão SD.
                    musica_parar();
                    if (romfs_ativo) {
                        salvar_asset_no_sd("bg.bgra");
                        salvar_asset_no_sd("music.mp3");
                        romfsExit(); romfs_ativo = 0;
                    }
                    upd_start_install(dir, self_nro);
                } else if (k & (HidNpadButton_B | HidNpadButton_Plus)) {
                    upd_cancel(1);
                    tela_upd = 0;
                    msg = msg_inicial();
                }
            } else if (st.state == UPD_CHECKING || st.state == UPD_DOWNLOADING || st.state == UPD_EXTRACTING) {
                if (k & HidNpadButton_B) upd_cancel(0);
            } else {
                if (k & (HidNpadButton_B | HidNpadButton_Plus)) {
                    upd_cancel(1);
                    tela_upd = 0;
                    atualizar_jogos();
                    if (!romfs_ativo && R_SUCCEEDED(romfsInit())) romfs_ativo = 1;
                    musica_iniciar();
                    msg = msg_inicial();
                }
            }
            px = framebufferBegin(&fb, &pitch); pitch /= 4;
            desenhar_atualizador(&st, tick++);
            if (lancar) retangulo(0, 0, W, H, 0, 140);
            framebufferEnd(&fb);
            if (lancar) break;
            continue;
        }
#endif
        if (tela_op) {
            Opcao *o = &opcoes[sel_op];
            if (k & HidNpadButton_AnyUp) sel_op = (sel_op + NOP - 1) % NOP;
            if (k & HidNpadButton_AnyDown) sel_op = (sel_op + 1) % NOP;
            if (k & (HidNpadButton_AnyRight | HidNpadButton_A)) o->atual = (o->atual + 1) % o->n;
            if (k & HidNpadButton_AnyLeft) o->atual = (o->atual + o->n - 1) % o->n;
            if (k & (HidNpadButton_B | HidNpadButton_Plus)) {
                msg = !opcoes_gravar() ? MSG_FALHA_INI : msg_inicial();
                tela_op = 0;
            }
            px = framebufferBegin(&fb, &pitch); pitch /= 4;
            desenhar_opcoes(sel_op, msg, tick++);
            framebufferEnd(&fb);
            continue;
        }
        if (k & HidNpadButton_Plus) break;
        if (k & HidNpadButton_AnyUp) sel = (sel + NMENU - 1) % NMENU;
        if (k & HidNpadButton_AnyDown) sel = (sel + 1) % NMENU;
        if ((k & HidNpadButton_A) && sel == ITEM_TROFEUS) {
            tela_trp = 1;
            trp_jogo = ler_ultimo();
            trp_filtro = 0;
            trp_sel = trp_scroll = 0;
            trp_hold = trp_hold_dir = 0;
            trp_load(&trp_set, dir, jogos[trp_jogo].trp_comm, l_val[idioma_atual() + 1]);
            msg = MSG_NONE;
        } else if ((k & HidNpadButton_A) && sel == ITEM_OPCOES) { tela_op = 1; msg = MSG_NONE; }
#ifdef ENABLE_UPDATER
        else if ((k & HidNpadButton_A) && sel == ITEM_UPD) {
            tela_upd = 1;
            msg = MSG_NONE;
            upd_start_check(VERSAO);
        }
#endif
        else if ((k & HidNpadButton_A) && sel == ITEM_SAIR) break;
        else if (k & HidNpadButton_A) {
            char p[FS_MAX_PATH]; caminho(p, sizeof p, sel);
            jogo_ok[sel] = existe(p);
            if (!jogo_ok[sel]) msg = MSG_SEM_JOGO;
            else if (envHasNextLoad()) {
                if (opcoes[OP_IDIOMA].atual == 0) opcoes_gravar();
                char args[FS_MAX_PATH + 4]; snprintf(args, sizeof args, "\"%.760s\"", p);
                if (R_SUCCEEDED(envSetNextLoad(p, args))) { gravar_ultimo(sel); lancar = 1; }
                else msg = MSG_FALHA_PREP;
            }
        }
        px = framebufferBegin(&fb, &pitch); pitch /= 4;
        desenhar(sel, msg, tick++);
        if (lancar) retangulo(0, 0, W, H, 0, 140);
        framebufferEnd(&fb);
        if (lancar) break;
    }

    trp_free(&trp_set);
#ifdef ENABLE_UPDATER
    upd_shutdown();
#endif
    musica_parar();
    framebufferClose(&fb);
    if (f_txt) FT_Done_Face(f_txt);
    if (f_ext) FT_Done_Face(f_ext);
    FT_Done_FreeType(ft);
    plExit();
    if (romfs_ativo) romfsExit();
    free(bg);
    return 0;
}
