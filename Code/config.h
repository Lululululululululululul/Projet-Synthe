/* =====================================================================
 *  config.h  —  En-tête COMMUN du projet MONOSTATION
 * ---------------------------------------------------------------------
 *  Regroupe tout ce qui est partagé par les autres modules :
 *    - le brochage (broches Teensy 4.1)
 *    - les constantes de réglage (plages, tailles, durées)
 *    - les structures de données communes (Preset, SeqEvent, Track)
 *
 *  Ce fichier ne contient AUCUNE variable d'état ni aucun code : que des
 *  #define, des constantes et des définitions de type. Il peut donc être
 *  inclus sans risque par tous les modules.
 * ===================================================================== */
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =================== ÉCRAN OLED ===================
constexpr int SCREEN_WIDTH  = 128;
constexpr int SCREEN_HEIGHT = 64;

// =================== BROCHES — ENCODEUR ===================
#define PIN_ENC_CLK 33   // signal A (rotation)
#define PIN_ENC_DT  34   // signal B (rotation)
#define PIN_ENC_SW  35   // bouton-poussoir (clic)

// =================== BROCHES — CLAVIER (do … si) ===================
#define BTN_DO  36
#define BTN_RE  37
#define BTN_MI  32
#define BTN_FA  31
#define BTN_SOL 30
#define BTN_LA  29
#define BTN_SI  28

// =================== BROCHES — POTENTIOMÈTRES ===================
#define POT_FREQ A10   // coupure du filtre (cutoff)
#define POT_AMP  A11   // résonance du filtre

// =================== PLAGES DU FILTRE ===================
constexpr float CUTOFF_MIN = 80.0f;    // Hz
constexpr float CUTOFF_MAX = 9000.0f;  // Hz
constexpr float RESO_MIN   = 0.7f;
constexpr float RESO_MAX   = 4.5f;

// =================== OSCILLATEURS ===================
constexpr int   NUM_WAVES = 4;     // sinus, dent de scie, carré, triangle
constexpr float OSC_AMP   = 0.8f;  // amplitude de chaque oscillateur

// =================== ANTI-REBOND (boutons) ===================
constexpr unsigned long DEBOUNCE_MS = 8;

// =================== ENCODEUR ===================
constexpr int STEPS_PER_DETENT = 4; // pas codeur par cran physique

// =================== RAFRAÎCHISSEMENT OLED ===================
constexpr int DISPLAY_INTERVAL_MS = 50; // ~20 images/s

// =================== CLAVIER ===================
constexpr int NUM_KEYS = 7;
// Fréquences (Hz) des notes do … si, octave de référence
constexpr float FREQ_DO  = 261.63f;
constexpr float FREQ_RE  = 293.66f;
constexpr float FREQ_MI  = 329.63f;
constexpr float FREQ_FA  = 349.23f;
constexpr float FREQ_SOL = 392.00f;
constexpr float FREQ_LA  = 440.00f;
constexpr float FREQ_SI  = 493.88f;

// =================== MENU — index des entrées ===================
#define IDX_PRESET  0
#define IDX_OSC     1
#define IDX_DETUNE  2
#define IDX_FILTER  3
#define IDX_ENV     4
#define IDX_OCTAVE  5
#define IDX_VOLUME  6
#define IDX_SEQ     7
constexpr int MENU_SIZE = 8;

// =================== PRESETS ===================
constexpr int NUM_PRESETS = 6;

// =================== SÉQUENCEUR (FS6) ===================
constexpr int      MAX_TRACKS   = 4;
constexpr int      MAX_EVENTS   = 64;
constexpr uint32_t TRACK_LEN_MS = 3000; // durée d'une piste (ms)

// =====================================================================
//  STRUCTURES DE DONNÉES COMMUNES
// =====================================================================

// Un preset = un jeu complet de paramètres sonores rappelable (FS4).
struct Preset {
  const char* name;
  int   wave;        // index de forme d'onde
  int   octave;      // décalage d'octave
  float a, d, s, r;  // enveloppe ADSR (A, D en ms ; S 0..1 ; R en ms)
  float det, mix;    // désaccord (cents) et niveau de l'OSC2
  float cutoffRef;   // coupure indicative (affichage)
  float resoRef;     // résonance indicative (affichage)
};

// Un événement du séquenceur = un note on/off horodaté.
struct SeqEvent {
  uint16_t t;     // instant depuis le début de la piste (ms)
  bool     on;    // true = note on, false = note off
  int8_t   key;   // touche concernée (-1 si silence)
  float    freq;  // fréquence à jouer
};

// Une piste = une suite d'événements enregistrés.
struct Track {
  SeqEvent ev[MAX_EVENTS];
  int      count;
  bool     used;
};

#endif // CONFIG_H
