/* =====================================================================
 *  sequencer.cpp  —  Implémentation du séquenceur (FS6)
 * ===================================================================== */
#include "sequencer.h"
#include "audio_engine.h"   // audioSeqPlay / audioSeqStop, et l'état activeKey / actualFreq

// ---------- État du séquenceur ----------
Track tracks[MAX_TRACKS];
int   seqSelection = 0;
bool  seqRecording = false;
bool  seqPlaying   = false;
int   recTrack = 0, playTrack = 0;
unsigned long seqRecStart = 0, seqPlayStart = 0;

static int playIdx = 0;

// Première piste libre (sinon 0)
static int findFreeTrack() {
  for (int i = 0; i < MAX_TRACKS; i++) if (!tracks[i].used) return i;
  return 0;
}

// =====================================================================
//  ENREGISTREMENT
// =====================================================================
void startRecording() {
  recTrack = findFreeTrack();
  tracks[recTrack].count = 0; tracks[recTrack].used = false;
  seqRecStart = millis(); seqRecording = true;
  // Si une note est déjà tenue, on l'enregistre à t = 0
  if (activeKey >= 0) {
    Track &t = tracks[recTrack];
    t.ev[t.count].t = 0; t.ev[t.count].on = true;
    t.ev[t.count].key = activeKey; t.ev[t.count].freq = actualFreq; t.count++;
  }
}
void stopRecording() {
  Track &t = tracks[recTrack];
  // Clôture : si une note est encore tenue, on pose un note off en fin de piste
  if (activeKey >= 0 && t.count < MAX_EVENTS) {
    t.ev[t.count].t = (uint16_t)TRACK_LEN_MS; t.ev[t.count].on = false;
    t.ev[t.count].key = -1; t.ev[t.count].freq = 0; t.count++;
  }
  t.used = (t.count > 0); seqRecording = false;
  seqSelection = recTrack + 1;
}

// Crochet appelé par readNotes() à chaque changement de note
void seqRecordNote(int8_t top, float freq) {
  if (!seqRecording) return;
  Track &t = tracks[recTrack];
  if (t.count < MAX_EVENTS) {
    uint32_t el = millis() - seqRecStart; if (el > TRACK_LEN_MS) el = TRACK_LEN_MS;
    t.ev[t.count].t   = (uint16_t)el;
    t.ev[t.count].on  = (top >= 0);
    t.ev[t.count].key = top;
    t.ev[t.count].freq = (top >= 0) ? freq : 0;
    t.count++;
  }
}

// =====================================================================
//  LECTURE
// =====================================================================
void startPlayback(int ti) { playTrack = ti; playIdx = 0; seqPlayStart = millis(); seqPlaying = true; }
void stopPlayback()        { audioSeqStop(); seqPlaying = false; }

void sequencerAction() {
  if (seqSelection == 0) startRecording();
  else { int ti = seqSelection - 1; if (tracks[ti].used) startPlayback(ti); }
}

// =====================================================================
//  MISE À JOUR (boucle)
// =====================================================================
void seqUpdate() {
  if (seqRecording) {
    if (millis() - seqRecStart >= TRACK_LEN_MS) stopRecording();
  }
  else if (seqPlaying) {
    uint32_t el = millis() - seqPlayStart;
    Track &t = tracks[playTrack];
    while (playIdx < t.count && t.ev[playIdx].t <= el) {
      if (t.ev[playIdx].on) audioSeqPlay(t.ev[playIdx].key, t.ev[playIdx].freq);
      else                  audioSeqStop();
      playIdx++;
    }
    if (el >= TRACK_LEN_MS) stopPlayback();
  }
}
