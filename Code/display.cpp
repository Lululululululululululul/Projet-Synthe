/* =====================================================================
 *  display.cpp  —  Implémentation de l'affichage OLED
 * ===================================================================== */
#include "display.h"
#include "audio_engine.h"   // état sonore à afficher
#include "menu.h"           // état du menu
#include "sequencer.h"      // état du séquenceur

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static bool oledOK = false;

// =====================================================================
//  INITIALISATION
// =====================================================================
void displaySetup() {
  oledOK = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (oledOK) { display.clearDisplay(); display.display(); }
}

// =====================================================================
//  HELPERS DE DESSIN
// =====================================================================

// Petite icône de forme d'onde (0=saw, 1=square, 2=triangle, 3=sine)
static void drawWaveIcon(int type, int ix, int iy) {
  int top = iy + 1, bot = iy + 9, mid = iy + 5;
  if (type == 0) {
    display.drawLine(ix, bot, ix + 10, top, SSD1306_WHITE); display.drawLine(ix + 10, top, ix + 10, bot, SSD1306_WHITE);
    display.drawLine(ix + 11, bot, ix + 21, top, SSD1306_WHITE); display.drawLine(ix + 21, top, ix + 21, bot, SSD1306_WHITE);
  } else if (type == 1) {
    display.drawLine(ix, top, ix + 5, top, SSD1306_WHITE); display.drawLine(ix + 5, top, ix + 5, bot, SSD1306_WHITE);
    display.drawLine(ix + 5, bot, ix + 11, bot, SSD1306_WHITE); display.drawLine(ix + 11, bot, ix + 11, top, SSD1306_WHITE);
    display.drawLine(ix + 11, top, ix + 16, top, SSD1306_WHITE); display.drawLine(ix + 16, top, ix + 16, bot, SSD1306_WHITE);
    display.drawLine(ix + 16, bot, ix + 22, bot, SSD1306_WHITE);
  } else if (type == 2) {
    display.drawLine(ix, mid, ix + 5, top, SSD1306_WHITE); display.drawLine(ix + 5, top, ix + 11, bot, SSD1306_WHITE);
    display.drawLine(ix + 11, bot, ix + 16, top, SSD1306_WHITE); display.drawLine(ix + 16, top, ix + 22, mid, SSD1306_WHITE);
  } else {
    int prevY = mid;
    for (int px = 0; px <= 22; px++) {
      float ph = (float)px / 22.0 * 2.0 * PI * 1.5;
      int py = mid - (int)round(4.0 * sinf(ph));
      if (px > 0) display.drawLine(ix + px - 1, prevY, ix + px, py, SSD1306_WHITE);
      prevY = py;
    }
  }
}

// Barre de progression / niveau (frac 0..1)
static void drawBar(int x, int y, int w, int h, float frac) {
  if (frac < 0) frac = 0; else if (frac > 1) frac = 1;
  display.drawRect(x, y, w, h, SSD1306_WHITE);
  int fillw = (int)((w - 2) * frac);
  if (fillw > 0) display.fillRect(x + 1, y + 1, fillw, h - 2, SSD1306_WHITE);
}

// Fond de ligne en surbrillance (inverse le texte)
static void rowStart(int ry, int rowH, bool hi) {
  if (hi) { display.fillRect(0, ry, 128, rowH, SSD1306_WHITE); display.setTextColor(SSD1306_BLACK); }
  else display.setTextColor(SSD1306_WHITE);
}

// Repère "édition active" (petit triangle à droite de la ligne)
static void editMark(int ry, int rowH, bool show) {
  if (show) display.fillTriangle(119, ry + 1, 119, ry + rowH - 2, 124, ry + rowH / 2, SSD1306_BLACK);
}

// Ligne "< Back" générique
static void drawBackRow(int ry, int rowH, bool hi) {
  rowStart(ry, rowH, hi);
  display.setCursor(4, ry); display.print("< Back");
  display.setTextColor(SSD1306_WHITE);
}

// =====================================================================
//  MENU PRINCIPAL (déroulant, avec ascenseur)
// =====================================================================
static void drawMainMenu() {
  const int rowH = 8, y0 = 15, VIS = 6;
  if (menuIndex < menuTop) menuTop = menuIndex;
  if (menuIndex >= menuTop + VIS) menuTop = menuIndex - VIS + 1;
  if (menuTop > MENU_SIZE - VIS) menuTop = MENU_SIZE - VIS;
  if (menuTop < 0) menuTop = 0;
  for (int row = 0; row < VIS; row++) {
    int i = menuTop + row; if (i >= MENU_SIZE) break;
    int ry = y0 + row * rowH;
    if (i == menuIndex) { display.fillRect(0, ry, 123, rowH, SSD1306_WHITE); display.setTextColor(SSD1306_BLACK); }
    else display.setTextColor(SSD1306_WHITE);
    display.setCursor(4, ry); display.print(menuItems[i]);
  }
  display.setTextColor(SSD1306_WHITE);
  if (MENU_SIZE > VIS) {
    int trackX = 125, trackY = y0, trackH = VIS * rowH;
    display.drawRect(trackX, trackY, 3, trackH, SSD1306_WHITE);
    int thumbH = trackH * VIS / MENU_SIZE; if (thumbH < 4) thumbH = 4;
    int thumbY = trackY + (trackH - thumbH) * menuTop / (MENU_SIZE - VIS);
    display.fillRect(trackX, thumbY, 3, thumbH, SSD1306_WHITE);
  }
}

// =====================================================================
//  SOUS-MENU SÉQUENCEUR
// =====================================================================
static void drawSeqSub() {
  if (seqRecording) {
    uint32_t el = millis() - seqRecStart;
    display.setCursor(0, 16); display.print("** REC **   (clic=Stop)");
    display.setCursor(0, 30);
    display.print(el / 1000.0, 1); display.print(" / "); display.print(TRACK_LEN_MS / 1000.0, 1); display.print(" s");
    drawBar(0, 46, 126, 10, (float)el / TRACK_LEN_MS);
    return;
  }
  if (seqPlaying) {
    uint32_t el = millis() - seqPlayStart;
    display.setCursor(0, 16); display.print("PLAY T"); display.print(playTrack + 1); display.print("  (clic=Stop)");
    display.setCursor(0, 30);
    display.print(el / 1000.0, 1); display.print(" / "); display.print(TRACK_LEN_MS / 1000.0, 1); display.print(" s");
    drawBar(0, 46, 126, 10, (float)el / TRACK_LEN_MS);
    return;
  }
  const int rowH = 8, y0 = 15;
  for (int s = 0; s <= MAX_TRACKS + 1; s++) {     // +New, pistes, Back
    int ry = y0 + s * rowH;
    rowStart(ry, rowH, s == seqSelection);
    display.setCursor(3, ry);
    if (s == 0) display.print("+ New (record)");
    else if (s <= MAX_TRACKS) { display.print("Track "); display.print(s); display.print(tracks[s - 1].used ? " [*]" : " [ ]"); }
    else display.print("< Back");
  }
  display.setTextColor(SSD1306_WHITE);
}

// =====================================================================
//  SOUS-MENUS (un bloc par entrée)
// =====================================================================
static void drawSubMenu() {
  display.setTextColor(SSD1306_WHITE);
  const int rowH = 8;

  if (menuIndex == IDX_PRESET) {
    const Preset &pr = presets[presetIndex];
    int ry0 = 15;
    rowStart(ry0, rowH, subCursor == 0);
    display.setCursor(3, ry0); display.print("Preset: "); display.print(pr.name);
    editMark(ry0, rowH, subCursor == 0 && editing);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 25); display.print(waveNames[pr.wave]);
    display.print(" O:"); if (pr.octave > 0) display.print("+"); display.print(pr.octave);
    display.print(" det"); display.print((int)pr.det);
    display.setCursor(0, 35);
    display.print("A"); display.print((int)pr.a); display.print(" D"); display.print((int)pr.d);
    display.print(" S"); display.print(pr.s, 1); display.print(" R"); display.print((int)pr.r);
    display.setCursor(0, 45);
    display.print("Cut~"); display.print((int)pr.cutoffRef); display.print(" Res~"); display.print(pr.resoRef, 1);
    drawBackRow(55, rowH, subCursor == 1);
  }
  else if (menuIndex == IDX_OSC) {
    int ry0 = 16;
    rowStart(ry0, rowH, subCursor == 0);
    display.setCursor(3, ry0); display.print("Wave: "); display.print(waveNames[waveIndex]);
    editMark(ry0, rowH, subCursor == 0 && editing);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 30); display.print(activeKey >= 0 ? "Playing" : "Silent");
    drawBackRow(48, rowH, subCursor == 1);
  }
  else if (menuIndex == IDX_DETUNE) {
    const int y0 = 16;
    rowStart(y0, rowH, subCursor == 0);
    display.setCursor(3, y0); display.print("Detune: "); display.print((int)detuneCents); display.print(" cents");
    editMark(y0, rowH, subCursor == 0 && editing);
    rowStart(y0 + 10, rowH, subCursor == 1);
    display.setCursor(3, y0 + 10); display.print("Mix: "); display.print((int)(osc2Mix * 100)); display.print(" %");
    editMark(y0 + 10, rowH, subCursor == 1 && editing);
    display.setTextColor(SSD1306_WHITE);
    drawBar(0, y0 + 20, 126, 7, osc2Mix);
    drawBackRow(y0 + 32, rowH, subCursor == 2);
  }
  else if (menuIndex == IDX_FILTER) {
    display.setCursor(0, 16); display.print("Cut "); display.print((int)filterCutoff);
    drawBar(72, 15, 54, 8, filterCutoffFrac);
    display.setCursor(0, 28); display.print("Res "); display.print(filterReso, 1);
    drawBar(72, 27, 54, 8, filterResoFrac);
    display.setCursor(0, 40); display.print("(pots physiques)");
    drawBackRow(52, rowH, subCursor == 0);   // seule ligne sélectionnable
  }
  else if (menuIndex == IDX_ENV) {
    const char* lbl[4] = {"A", "D", "S", "R"};
    const int y0 = 15;
    for (int f = 0; f < 4; f++) {
      int ry = y0 + f * rowH;
      rowStart(ry, rowH, subCursor == f);
      display.setCursor(3, ry); display.print(lbl[f]); display.print(": ");
      if (f == 0)      { display.print((int)envAttack);  display.print(" ms"); }
      else if (f == 1) { display.print((int)envDecay);   display.print(" ms"); }
      else if (f == 2) { display.print(envSustain, 2); }
      else             { display.print((int)envRelease); display.print(" ms"); }
      editMark(ry, rowH, subCursor == f && editing);
    }
    display.setTextColor(SSD1306_WHITE);
    drawBackRow(y0 + 4 * rowH, rowH, subCursor == 4);
  }
  else if (menuIndex == IDX_OCTAVE) {
    int ry0 = 16;
    rowStart(ry0, rowH, subCursor == 0);
    display.setCursor(3, ry0); display.print("Octave: "); if (octaveShift > 0) display.print("+"); display.print(octaveShift);
    editMark(ry0, rowH, subCursor == 0 && editing);
    display.setTextColor(SSD1306_WHITE);
    int yBar = 32, cx = map(octaveShift, -2, 2, 6, 122);
    display.drawLine(6, yBar + 3, 122, yBar + 3, SSD1306_WHITE);
    for (int o = -2; o <= 2; o++) { int gx = map(o, -2, 2, 6, 122); display.drawLine(gx, yBar + 1, gx, yBar + 5, SSD1306_WHITE); }
    display.fillRect(cx - 2, yBar, 5, 7, SSD1306_WHITE);
    drawBackRow(48, rowH, subCursor == 1);
  }
  else if (menuIndex == IDX_VOLUME) {
    int ry0 = 16;
    rowStart(ry0, rowH, subCursor == 0);
    display.setCursor(3, ry0); display.print("Volume: "); display.print((int)(volumeLevel * 100)); display.print(" %");
    editMark(ry0, rowH, subCursor == 0 && editing);
    display.setTextColor(SSD1306_WHITE);
    drawBar(0, 30, 126, 8, volumeLevel);
    drawBackRow(46, rowH, subCursor == 1);
  }
  else if (menuIndex == IDX_SEQ) {
    drawSeqSub();
  }
}

// =====================================================================
//  ÉCRAN COMPLET : barre d'état + contenu
// =====================================================================
void drawMenu() {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Barre d'état : forme d'onde, octave, note jouée
  drawWaveIcon(waveIndex, 0, 1);
  display.setCursor(26, 2);  display.print(waveNames[waveIndex]);
  display.setCursor(82, 2);  display.print("O:"); if (octaveShift > 0) display.print("+"); display.print(octaveShift);
  display.setCursor(110, 2); display.print(activeKey >= 0 ? noteNames[activeKey] : "--");
  display.drawLine(0, 13, 127, 13, SSD1306_WHITE);

  if (!inSubMenu) drawMainMenu();
  else            drawSubMenu();

  display.display();
}
