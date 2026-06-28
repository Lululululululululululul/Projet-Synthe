/* =====================================================================
 *  audio_engine.cpp  —  Implémentation du moteur sonore
 * ===================================================================== */
#include "audio_engine.h"
#include "sequencer.h"   // pour l'enregistrement des notes (seqRecordNote) et seqPlaying

#include <Audio.h>
#include <Wire.h>
#include <Bounce2.h>

// =====================================================================
//  CHAÎNE AUDIO (CdC, Fig.2)
//  OSC1 + OSC2 -> Mixer -> Filtre passe-bas résonant -> Enveloppe ADSR -> I2S
// =====================================================================
static AudioSynthWaveform        waveform1;
static AudioSynthWaveform        waveform2;   // 2e oscillateur désaccordable (FS1)
static AudioMixer4               mixer;
static AudioFilterStateVariable  filter;
static AudioEffectEnvelope       envelope;
static AudioOutputI2S            audioOut;
static AudioControlSGTL5000      audioShield;

static AudioConnection patchCord1 (waveform1, 0, mixer, 0);
static AudioConnection patchCord1b(waveform2, 0, mixer, 1);
static AudioConnection patchCord2 (mixer, 0, filter, 0);
static AudioConnection patchCord3 (filter, 0, envelope, 0);   // sortie 0 = passe-bas
static AudioConnection patchCord4 (envelope, 0, audioOut, 0);
static AudioConnection patchCord5 (envelope, 0, audioOut, 1);

// =====================================================================
//  TABLES DE FORMES D'ONDE (FP2)
// =====================================================================
static const short waveTypes[NUM_WAVES] = {
  WAVEFORM_BANDLIMIT_SAWTOOTH,
  WAVEFORM_BANDLIMIT_SQUARE,
  WAVEFORM_TRIANGLE,
  WAVEFORM_SINE
};
const char* waveNames[NUM_WAVES] = {"Saw", "Square", "Triangle", "Sine"};

// =====================================================================
//  ÉTAT AUDIO (partagé via audio_engine.h)
// =====================================================================
int   waveIndex   = 0;
int   octaveShift = 0;          // FP5
float detuneCents = 0;          // FS1
float osc2Mix     = 0.0;        // FS1
float envAttack   = 10;         // FP4 — ADSR
float envDecay    = 80;
float envSustain  = 0.7;
float envRelease  = 250;
float volumeLevel = 0.5;
float filterCutoff     = 1000.0;
float filterReso       = 0.9;
float filterCutoffFrac = 0.0;
float filterResoFrac   = 0.0;
float actualFreq  = 0;
int8_t activeKey  = -1;

// =====================================================================
//  PRESETS (FS4)  —  définis avec liaison externe pour l'affichage
// =====================================================================
extern const Preset presets[NUM_PRESETS] = {
  // name      wave oct   A     D     S     R    det   mix   cutRef resoRef
  { "Bass",    0,  -1,    2,   60,  0.30,  120,   6,  0.30,   600,  1.2 },
  { "Lead",    0,   0,    5,   40,  0.85,  200,  12,  0.50,  4000,  2.0 },
  { "Pluck",   1,   0,    1,  120,  0.00,  150,   0,  0.00,  2500,  1.5 },
  { "Pad",     0,   0,  600,  300,  0.80, 1200,  18,  0.60,  1800,  1.0 },
  { "Organ",   1,   0,    3,   10,  1.00,   80,   0,  0.00,  5000,  0.9 },
  { "Reso",    0,   0,    4,   90,  0.50,  300,   8,  0.40,  1200,  4.2 }
};

// =====================================================================
//  CLAVIER + PILE DE NOTES (FP1)
// =====================================================================
static const uint8_t keyPins[NUM_KEYS]  = {BTN_DO, BTN_RE, BTN_MI, BTN_FA, BTN_SOL, BTN_LA, BTN_SI};
static const float   keyFreqs[NUM_KEYS] = {FREQ_DO, FREQ_RE, FREQ_MI, FREQ_FA, FREQ_SOL, FREQ_LA, FREQ_SI};
const char*          noteNames[NUM_KEYS] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI"};

static bool   keyHeld[NUM_KEYS] = {false, false, false, false, false, false, false};
static int8_t noteStack[NUM_KEYS];
static int    stackSize = 0;
static Bounce keyBtn[NUM_KEYS];

// Potentiomètres (valeurs lissées)
static int potFiltered = 0,    potValue = 0;
static int potAmpFiltered = 0, potAmpValue = 0;

// ---- Pile de notes : on garde la priorité à la dernière note jouée ----
static void removeNote(int8_t k) {
  for (int i = 0; i < stackSize; i++) {
    if (noteStack[i] == k) {
      for (int j = i; j < stackSize - 1; j++) noteStack[j] = noteStack[j + 1];
      stackSize--;
      break;
    }
  }
}
static void pushNote(int8_t k) {
  removeNote(k);
  if (stackSize < NUM_KEYS) noteStack[stackSize++] = k;
}

// Fréquence d'une touche en tenant compte de l'octave (FP5)
static float noteFreq(int8_t k) { return keyFreqs[k] * powf(2.0, octaveShift); }

// =====================================================================
//  APPLICATION DES PARAMÈTRES À LA CHAÎNE AUDIO
// =====================================================================
static void applyOscFreq() {
  float f2 = actualFreq * powf(2.0, detuneCents / 1200.0);
  waveform1.frequency(actualFreq);
  waveform2.frequency(f2);
}
static void applyOscMix() {
  float g = 1.0 + osc2Mix;
  mixer.gain(0, 1.0 / g);
  mixer.gain(1, osc2Mix / g);
}

// Pas d'incrément adaptatif pour les durées d'enveloppe
static float adjustTime(float v, int dir, float vmax) {
  float step;
  if      (v < 20)  step = 1;
  else if (v < 100) step = 5;
  else if (v < 500) step = 20;
  else              step = 50;
  v += dir * step;
  if (v < 1)    v = 1;
  if (v > vmax) v = vmax;
  return v;
}

// =====================================================================
//  RÉGLAGES APPELÉS PAR LE MENU
// =====================================================================
void audioCycleWaveform(int dir) {
  waveIndex += dir;
  if (waveIndex < 0) waveIndex = NUM_WAVES - 1;
  if (waveIndex >= NUM_WAVES) waveIndex = 0;
  AudioNoInterrupts();
  waveform1.begin(waveTypes[waveIndex]); waveform1.amplitude(OSC_AMP);
  waveform2.begin(waveTypes[waveIndex]); waveform2.amplitude(OSC_AMP);
  if (activeKey >= 0) applyOscFreq();
  AudioInterrupts();
}
void audioAdjustDetune(int dir) {
  detuneCents = constrain(detuneCents + dir * 1.0, -50.0, 50.0);
  if (activeKey >= 0) { AudioNoInterrupts(); applyOscFreq(); AudioInterrupts(); }
}
void audioAdjustMix(int dir) {
  osc2Mix = constrain(osc2Mix + dir * 0.05, 0.0, 1.0);
  AudioNoInterrupts(); applyOscMix(); AudioInterrupts();
}
void audioAdjustEnv(int row, int dir) {
  if      (row == 0) { envAttack  = adjustTime(envAttack,  dir, 2000); envelope.attack(envAttack); }
  else if (row == 1) { envDecay   = adjustTime(envDecay,   dir, 2000); envelope.decay(envDecay); }
  else if (row == 2) { envSustain = constrain(envSustain + dir * 0.05, 0.0, 1.0); envelope.sustain(envSustain); }
  else               { envRelease = adjustTime(envRelease, dir, 3000); envelope.release(envRelease); }
}
void audioAdjustOctave(int dir) {
  octaveShift = constrain(octaveShift + dir, -2, 2);
  if (activeKey >= 0) { actualFreq = noteFreq(activeKey); AudioNoInterrupts(); applyOscFreq(); AudioInterrupts(); }
}
void audioAdjustVolume(int dir) {
  volumeLevel = constrain(volumeLevel + dir * 0.05, 0.0, 1.0);
  audioShield.volume(volumeLevel);
}

// Charge un preset complet (FS4)
void audioLoadPreset(int p) {
  const Preset &pr = presets[p];
  waveIndex = pr.wave; octaveShift = pr.octave;
  envAttack = pr.a; envDecay = pr.d; envSustain = pr.s; envRelease = pr.r;
  detuneCents = pr.det; osc2Mix = pr.mix;
  AudioNoInterrupts();
  waveform1.begin(waveTypes[waveIndex]); waveform1.amplitude(OSC_AMP);
  waveform2.begin(waveTypes[waveIndex]); waveform2.amplitude(OSC_AMP);
  envelope.attack(envAttack); envelope.decay(envDecay);
  envelope.sustain(envSustain); envelope.release(envRelease);
  applyOscMix();
  if (activeKey >= 0) { actualFreq = noteFreq(activeKey); applyOscFreq(); }
  AudioInterrupts();
}

// =====================================================================
//  INTERFACE DU SÉQUENCEUR (note on/off pilotée par la lecture)
// =====================================================================
void audioSeqPlay(int8_t key, float freq) {
  AudioNoInterrupts();
  actualFreq = freq; applyOscFreq(); envelope.noteOn();
  AudioInterrupts();
  activeKey = key;
}
void audioSeqStop() {
  AudioNoInterrupts(); envelope.noteOff(); AudioInterrupts();
  activeKey = -1;
}

// =====================================================================
//  INITIALISATION
// =====================================================================
void audioSetup() {
  audioShield.enable();
  audioShield.volume(volumeLevel);

  waveform1.begin(waveTypes[waveIndex]); waveform1.amplitude(OSC_AMP);
  waveform2.begin(waveTypes[waveIndex]); waveform2.amplitude(OSC_AMP);
  mixer.gain(2, 0.0); mixer.gain(3, 0.0);
  applyOscMix();

  filter.frequency(filterCutoff);
  filter.resonance(filterReso);
  envelope.attack(envAttack);   envelope.decay(envDecay);
  envelope.sustain(envSustain); envelope.release(envRelease);

  // Entrées de jeu : boutons du clavier (anti-rebond) + potentiomètres
  for (int i = 0; i < NUM_KEYS; i++) {
    keyBtn[i].attach(keyPins[i], INPUT_PULLUP);
    keyBtn[i].interval(DEBOUNCE_MS);
  }
  pinMode(POT_FREQ, INPUT);
  pinMode(POT_AMP, INPUT);
}

// =====================================================================
//  LECTURE DU CLAVIER + DÉCLENCHEMENT DE LA VOIX (FP1)
// =====================================================================
void readNotes() {
  for (int i = 0; i < NUM_KEYS; i++) {
    keyBtn[i].update();
    if (keyBtn[i].fell())      { pushNote(i);   keyHeld[i] = true; }
    else if (keyBtn[i].rose()) { removeNote(i); keyHeld[i] = false; }
  }
  if (seqPlaying) return;   // en lecture séquenceur, le clavier ne déclenche pas

  int8_t top = (stackSize > 0) ? noteStack[stackSize - 1] : -1;
  if (top != activeKey) {
    AudioNoInterrupts();
    if (top >= 0) { actualFreq = noteFreq(top); applyOscFreq(); envelope.noteOn(); }
    else          { envelope.noteOff(); }
    AudioInterrupts();
    activeKey = top;

    // Enregistrement éventuel de ce changement de note (FS6)
    seqRecordNote(top, actualFreq);
  }
}

// =====================================================================
//  LECTURE DES POTENTIOMÈTRES -> FILTRE (FP3)
//  Lissage par moyenne glissante (7/8 ancien + 1/8 nouveau)
// =====================================================================
void readPots() {
  int raw = analogRead(POT_FREQ); potFiltered    = (potFiltered    * 7 + raw) / 8; potValue    = potFiltered;
  raw     = analogRead(POT_AMP);  potAmpFiltered = (potAmpFiltered * 7 + raw) / 8; potAmpValue = potAmpFiltered;

  // Coupure : échelle logarithmique sur toute la course du potard
  float x = potValue / 1023.0; if (x < 0) x = 0; else if (x > 1) x = 1;
  filterCutoff = CUTOFF_MIN * powf(CUTOFF_MAX / CUTOFF_MIN, x); filterCutoffFrac = x;
  // Résonance : échelle linéaire
  float y = potAmpValue / 1023.0; if (y < 0) y = 0; else if (y > 1) y = 1;
  filterReso = RESO_MIN + y * (RESO_MAX - RESO_MIN); filterResoFrac = y;

  filter.frequency(filterCutoff); filter.resonance(filterReso);
}
