#pragma once
#include <Arduino.h>

// Idiomas soportados. La fuente 5x7 de Arduino_GFX es una tabla CP437 completa
// (256 glifos), asi que SI hay acentos, dieresis y enes: van como byte suelto en
// octal (\240 a, \244 n~, \202 e, \204 a"...), nunca en UTF-8, porque la UI centra
// con strlen(t)*6 y un caracter de dos bytes descuadraria las etiquetas.
// No estan en CP437: las mayusculas acentuadas salvo E (\220), ni a~/o~ del portugues.
enum Lang : uint8_t { LANG_ES = 0, LANG_EN, LANG_FR, LANG_DE, LANG_IT, LANG_PT,
                     LANG_JA, LANG_KO, LANG_COUNT };
// idiomas que necesitan una fuente U8g2 y texto UTF-8 en vez de la tabla CP437
#define LANG_IS_CJK(l) ((l) == LANG_JA || (l) == LANG_KO)
#define LANG_DEFAULT LANG_EN  // idioma por defecto: ingles

extern Lang gLang;  // idioma activo (definido en i18n.cpp)

// IDs de cadena. El orden debe coincidir con la tabla STRINGS de i18n.cpp.
enum StrId : uint8_t {
  // estado del bicho (statusMsg)
  S_EVOLVING, S_EATING, S_LIKES, S_HUNGRY, S_NEEDS_BATH,
  S_EXHAUSTED, S_SAD, S_CHUBBY, S_IS_SHINY, S_HAPPY,
  // ceremonias de despedida
  S_FAREWELL, S_RUNAWAY, S_GOODBYE,
  // huevo
  S_EGG_HDR, S_EGG_LEGEND, S_EGG_RARE, S_EGG_TOUCH, S_EGG_MOVES, S_EGG_ALMOST,
  // formatos compartidos
  S_POKEDEX_FMT,   // "POKEDEX %u/151"
  S_NAME_FMT,      // "%s%s Nv.%u"
  // dialogo soltar
  S_RELEASE_FMT, S_YES, S_NO,
  // minijuego y saco
  S_WINS_FMT, S_STR_GAIN_FMT, S_NEW_RECORD, S_RECORD_FMT, S_BTL_RUN,
  S_SCORE_FMT, S_GREAT_JOY, S_PLUS_JOY,
  // reloj / ajustes
  S_SET_TIME, S_HOUR, S_MIN, S_CLOCK_CANCEL, S_LANG_LABEL,
  // celebracion
  S_MEDAL_BANNER, S_GREAT, S_STREAK_DAYS_FMT,
  // ficha: perfil
  S_STREAK_FMT, S_VIN, S_BERRY_UNK, S_BERRY_RED, S_BERRY_BLUE, S_BERRY_GREEN,
  S_INFO_FMT, S_RENAME_HINT,
  // ficha: combate
  S_BATTLE, S_STAT_ATK, S_STAT_DEF, S_STAT_SPE, S_STAT_WGT, S_BTL_START,
  // ficha: medallas
  S_MEDALS_FMT, S_BACK,
  // teclado y galeria
  S_NAME, S_DETAIL_BACK,
  // barras
  S_BAR_FOOD, S_BAR_JOY, S_BAR_ENE, S_BAR_HYG,
  // marcador en vivo del minijuego
  S_REC_FMT,
  // ficha: pagina de progreso
  S_PROGRESS, S_LVL_FMT, S_NEXT_LVL_FMT, S_EVO_LABEL, S_FINAL_FORM,
  S_EVO_READY, S_EVO_BLOCKED, S_EVO_IN_FMT, S_MISTAKES_FMT,
  // interruptor de sonido (ajustes)
  S_SND_ON, S_SND_OFF,
  S_EVO_TAP,        // texto del boton de evolucion
  S_FAREWELL_BTN,   // texto del boton de despedida (lleva el nombre: "%s ...")
  S_RUNAWAY_BTN,    // texto del boton de escapada por abandono (final triste)
  // dialogos de decision (evolucionar/mantener, despedirse/quedaros)
  S_EVO_Q, S_EVO_KEEP, S_FAR_Q, S_FAR_GO, S_FAR_STAY,
  S_CHOOSE_STARTER,  // titulo de la eleccion del inicial (primera vez)
  S_NO_SPRITES, S_LOAD_SPRITES,  // aviso cuando falta el sprite en la SD
  // combate
  S_BTL_WILD, S_BTL_SUPER, S_BTL_WEAK, S_BTL_IMMUNE, S_BTL_WON, S_BTL_LOST, S_BTL_RAN,
  S_BTL_NEWDEX, S_BTL_LEARNED, S_BTL_TIRED,
  // borrar la partida (boton del Knomi)
  S_RESET_Q, S_RESET_GO, S_RESET_KEEP,
  // ajustes y cambio de wifi (Knomi)
  S_SETTINGS, S_WIFI_CHANGE, S_WIFI_SYNCED, S_WIFI_NOCONN, S_WIFI_SEARCH, S_WIFI_TRYING, S_WIFI_SAVED,
  S_WIFI_KEPT, S_CANCEL,
  // juegos, captura y visita
  S_GAMES, S_GAME_TILT, S_GAME_WHO, S_WHO_Q, S_ITS_FMT, S_TILT_HINT, S_THROW_HINT,
  S_CAUGHT_FMT, S_MISSED, S_FLED_FMT, S_VISITOR, S_VISIT_BYE_FMT,
  S_GAME_RUN, S_TAP_JUMP, S_SWIPE_QUIT,
  STR_COUNT
};

const char *T(StrId id);       // texto en el idioma activo
const char *medalName(int i);  // banner de medalla (MED_COUNT)
const char *medalLabel(int i); // etiqueta corta de medalla
const char *medalDesc(int i);  // descripcion larga de medalla

void loadLang();             // lee el idioma de NVS (llamar en setup)
void setLang(Lang l);        // cambia y persiste el idioma
