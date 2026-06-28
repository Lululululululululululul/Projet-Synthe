/* =====================================================================
 *  audio_engine.h  —  LE SON (couche DSP + gestion de voix)
 * ---------------------------------------------------------------------
 *  Tout le « moteur sonore » de MONOSTATION :
 *    - la chaîne audio Teensy : OSC1 + OSC2 -> Mixer -> Filtre -> ADSR -> I2S
 *    - la gestion de voix monophonique (pile de notes, priorité dernière note)
 *    - la conversion note -> fréquence et le décalage d'octave
 *    - les presets et la lecture des potentiomètres vers le filtre
 *
 *  Ce header n'expose que ce dont les autres modules ont besoin :
 *  l'état à afficher (lecture) et les fonctions de réglage (écriture).
 *  Les objets audio eux-mêmes restent privés à audio_engine.cpp.
 * ===================================================================== */
#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include "config.h"

// ---------- État audio exposé (lecture, pour l'affichage) ----------
extern int    waveIndex;                 // forme d'onde courante (0..NUM_WAVES-1)
extern int    octaveShift;               // décalage d'octave (-2..+2)
extern float  detuneCents;               // désaccord OSC2 (-50..+50)
extern float  osc2Mix;                   // niveau OSC2 (0..1)
extern float  envAttack, envDecay, envSustain, envRelease; // ADSR
extern float  volumeLevel;               // volume général (0..1)
extern float  filterCutoff, filterReso;  // valeurs filtre (Hz / Q)
extern float  filterCutoffFrac, filterResoFrac; // positions potards (0..1)
extern float  actualFreq;                // fréquence en cours de jeu
extern int8_t activeKey;                 // touche active (-1 si silence)

extern const char* waveNames[NUM_WAVES]; // noms des formes d'onde
extern const char* noteNames[NUM_KEYS];  // noms des notes (DO..SI)
extern const Preset presets[NUM_PRESETS];// table des presets

// ---------- Initialisation ----------
void audioSetup();      // démarre le codec, la chaîne audio et les entrées de jeu

// ---------- Boucle de jeu (appelées depuis loop()) ----------
void readNotes();       // lit le clavier, gère la pile de notes et déclenche l'ADSR
void readPots();        // lit les potards et met à jour le filtre

// ---------- Réglages de paramètres (appelés par le menu) ----------
void audioCycleWaveform(int dir);
void audioAdjustDetune(int dir);
void audioAdjustMix(int dir);
void audioAdjustEnv(int row, int dir);  // row 0=A 1=D 2=S 3=R
void audioAdjustOctave(int dir);
void audioAdjustVolume(int dir);
void audioLoadPreset(int index);

// ---------- Utilisé par le séquenceur ----------
void audioSeqPlay(int8_t key, float freq); // joue une note (note on)
void audioSeqStop();                        // coupe la note (note off)

#endif // AUDIO_ENGINE_H
