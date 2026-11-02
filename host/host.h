// Interfaz del juego para un front end: la ventana de Mac hoy, un canvas de WebAssembly manana.
// Sin dependencias de plataforma y con enlace C. Todo se llama desde un mismo hilo salvo
// hostSerialPush() y hostAudioRender(), que pueden venir de otro.
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HOST_CANVAS_SIZE 466  // lienzo logico del juego, cuadrado

// Arranca el juego. resources contiene mons/ y fonts/; saveDir se crea si no existe.
void hostInit(const char *resources, const char *saveDir);
// Un pase del bucle del juego; llamarlo a 50 Hz o mas. Nunca bloquea.
void hostTick(void);
// Guarda ya lo pendiente: al salir o antes de que el sistema se suspenda.
void hostFlushSave(void);
// El juego ha borrado la partida y quiere reiniciarse: guarda y vuelve a lanzar el proceso.
bool hostRestartRequested(void);

// true si hay un frame nuevo desde la ultima llamada a hostFrameRGBA()
bool hostFrameDirty(void);
// Copia el lienzo a out (HOST_CANVAS_SIZE^2 pixeles RGBA de 8 bits, en orden de bytes R G B A),
// con el brillo del panel aplicado.
void hostFrameRGBA(uint8_t *out);

// Puntero en coordenadas del lienzo; se simula un dedo. Los toques mas cortos que el sondeo del
// juego se alargan, para que un tap de trackpad no se pierda.
void hostPointer(int x, int y, bool down);
// El raton pasa por encima: despierta la pantalla atenuada, como lo haria acercarse al aparato.
void hostPointerHover(void);
// Teclas de inclinacion; el juego ve una rampa, no un salto, para poder afinar
void hostTiltKeys(bool left, bool right);
// Boton fisico del Knomi: toque = rival salvaje, 1-3 s = pantalla, 10 s = borrar partida
void hostButton(bool down);

// Linea para la consola serie del juego (los mismos comandos que en el Knomi)
void hostSerialPush(const char *line);

// Audio: PCM mono de 16 bits a HOST_AUDIO_RATE. Rellena siempre frames muestras (silencio si no suena nada).
#define HOST_AUDIO_RATE 16000
void hostAudioRender(int16_t *out, int frames);

#ifdef __cplusplus
}
#endif
