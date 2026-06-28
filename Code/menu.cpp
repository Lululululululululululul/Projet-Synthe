/* =====================================================================
 *  menu.cpp  —  Implémentation du menu et de l'encodeur
 * ===================================================================== */
#include "menu.h"
#include "audio_engine.h"   // réglages sonores déclenchés par le menu
#include "sequencer.h"      // actions du séquenceur

#include <Encoder.h>

// ---------- État du menu ----------
int  menuIndex = 0;
int  menuTop   = 0;
bool inSubMenu = false;
int  subCursor = 0;
bool editing   = false;
int  presetIndex = 0;

const char* menuItems[MENU_SIZE] =
  {"Preset", "Oscillator", "Detune", "Filter", "Envelope", "Octave", "Volume", "Sequencer"};

// ---------- Encodeur ----------
static Encoder enc(PIN_ENC_CLK, PIN_ENC_DT);
static long lastEncPos = 0;

// =====================================================================
//  NAVIGATION : nombre de lignes de valeur par sous-menu
// =====================================================================
static int valueRows(int m) {
  switch (m) {
    case IDX_PRESET: return 1;
    case IDX_OSC:    return 1;
    case IDX_DETUNE: return 2;
    case IDX_FILTER: return 0;   // réglé par les potards physiques
    case IDX_ENV:    return 4;
    case IDX_OCTAVE: return 1;
    case IDX_VOLUME: return 1;
  }
  return 0;
}

// =====================================================================
//  ÉDITION de la valeur à la ligne 'row' du sous-menu 'm'
//  -> délègue le travail sonore au moteur audio
// =====================================================================
static void editValue(int m, int row, int dir) {
  if (m == IDX_PRESET) {
    presetIndex += dir;
    if (presetIndex < 0) presetIndex = NUM_PRESETS - 1;
    if (presetIndex >= NUM_PRESETS) presetIndex = 0;
    audioLoadPreset(presetIndex);
  }
  else if (m == IDX_OSC)    audioCycleWaveform(dir);
  else if (m == IDX_DETUNE) { if (row == 0) audioAdjustDetune(dir); else audioAdjustMix(dir); }
  else if (m == IDX_ENV)    audioAdjustEnv(row, dir);
  else if (m == IDX_OCTAVE) audioAdjustOctave(dir);
  else if (m == IDX_VOLUME) audioAdjustVolume(dir);
}

// =====================================================================
//  ENCODEUR : effet d'un cran de rotation
// =====================================================================
static void applyEncoderStep(int dir) {
  if (seqRecording || seqPlaying) return;   // encodeur figé pendant rec/play

  if (!inSubMenu) {                          // menu principal : on défile
    menuIndex += dir;
    if (menuIndex < 0) menuIndex = MENU_SIZE - 1;
    if (menuIndex >= MENU_SIZE) menuIndex = 0;
    return;
  }
  if (menuIndex == IDX_SEQ) {                 // sous-menu séquenceur : sélection
    int maxSel = MAX_TRACKS + 1;              // +1 = ligne Back
    seqSelection += dir;
    if (seqSelection < 0) seqSelection = maxSel;
    if (seqSelection > maxSel) seqSelection = 0;
    return;
  }
  int rowCount = valueRows(menuIndex) + 1;    // +1 = Back
  if (editing) {
    editValue(menuIndex, subCursor, dir);     // on modifie la valeur
  } else {
    subCursor += dir;                         // on déplace le curseur
    if (subCursor < 0) subCursor = rowCount - 1;
    if (subCursor >= rowCount) subCursor = 0;
  }
}

// =====================================================================
//  CLIC ENCODEUR (clic simple, plus d'appui long)
// =====================================================================
static void onClick() {
  if (!inSubMenu) {                            // menu principal -> entrer
    inSubMenu = true; subCursor = 0; editing = false;
    if (menuIndex == IDX_SEQ) seqSelection = 0;
    return;
  }
  if (menuIndex == IDX_SEQ) {
    if (seqRecording) { stopRecording(); return; }   // clic = Stop
    if (seqPlaying)   { stopPlayback();  return; }
    if (seqSelection <= MAX_TRACKS) sequencerAction();
    else inSubMenu = false;                   // ligne Back
    return;
  }
  int vr = valueRows(menuIndex);
  if (subCursor >= vr) { inSubMenu = false; editing = false; }  // Back
  else                   editing = !editing;                    // active/désactive l'édition
}

// =====================================================================
//  INITIALISATION
// =====================================================================
void menuSetup() {
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  enc.write(0); lastEncPos = 0;
}

// =====================================================================
//  LECTURE ENCODEUR (rotation)
// =====================================================================
void readEncoder() {
  long pos = enc.read() / STEPS_PER_DETENT;
  if (pos != lastEncPos) {
    int dir = (pos > lastEncPos) ? +1 : -1;
    while (lastEncPos != pos) { lastEncPos += dir; applyEncoderStep(dir); }
  }
}

// =====================================================================
//  LECTURE BOUTON ENCODEUR (clic simple)
// =====================================================================
void readMenuButton() {
  static bool lastSW = HIGH;
  static unsigned long pressStart = 0;
  bool sw = digitalRead(PIN_ENC_SW);
  if (lastSW == HIGH && sw == LOW) pressStart = millis();
  if (lastSW == LOW && sw == HIGH) {
    if (millis() - pressStart > 30) onClick();
  }
  lastSW = sw;
}
