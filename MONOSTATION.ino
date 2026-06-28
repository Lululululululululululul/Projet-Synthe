/* =====================================================================
 *  MONOSTATION.ino  —  Point d'entrée du firmware
 * ---------------------------------------------------------------------
 *  Mini-synthétiseur monophonique numérique — Teensy 4.1 + Audio Adapter Rev D
 *  Groupe Synthé — ISEN2 2025/2026
 *
 *  Architecture (séparation interface / logique / DSP) répartie en modules :
 *    config.h          — brochage, constantes, structures communes
 *    audio_engine.*    — LE SON : chaîne DSP, voix monophonique, presets, potards
 *    menu.*            — MENU + ENCODEUR : navigation et édition des paramètres
 *    display.*         — AFFICHAGE OLED : retour visuel
 *    sequencer.*       — SÉQUENCEUR (FS6) : enregistrement / lecture de pistes
 *
 *  Le traitement audio tourne en tâche de fond (moteur audio Teensy / DMA) :
 *  loop() ne fait que lire les entrées, mettre à jour l'état et l'affichage.
 * ===================================================================== */
#include "config.h"
#include "audio_engine.h"
#include "menu.h"
#include "display.h"
#include "sequencer.h"

#include <Audio.h>
#include <Wire.h>

static unsigned long lastDisplay = 0;

// =====================================================================
//  SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);
  AudioMemory(40);     // réserve les blocs mémoire de la chaîne audio
  Wire.begin();        // bus I2C (codec SGTL5000 + écran OLED)

  audioSetup();        // codec, oscillateurs, filtre, enveloppe, entrées de jeu
  displaySetup();      // écran OLED
  menuSetup();         // encodeur + bouton

  audioLoadPreset(presetIndex);  // charge le preset de départ
}

// =====================================================================
//  LOOP
// =====================================================================
void loop() {
  readEncoder();        // rotation de l'encodeur (navigation / édition)
  readMenuButton();     // clic de l'encodeur
  readNotes();          // clavier : pile de notes + déclenchement de la voix
  seqUpdate();          // séquenceur : enregistrement / lecture
  readPots();           // potentiomètres -> filtre (coupure + résonance)

  // Rafraîchissement de l'écran à cadence réduite (~20 img/s)
  if (millis() - lastDisplay > DISPLAY_INTERVAL_MS) {
    drawMenu();
    lastDisplay = millis();
  }
}
