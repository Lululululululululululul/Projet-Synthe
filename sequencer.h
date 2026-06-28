/* =====================================================================
 *  sequencer.h  —  SÉQUENCEUR (FS6)
 * ---------------------------------------------------------------------
 *  Enregistrement et relecture de courtes pistes (note on/off horodatés).
 *  Le séquenceur s'appuie sur le moteur audio pour jouer les notes et est
 *  piloté par le menu (sélection de piste, démarrage, arrêt).
 * ===================================================================== */
#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "config.h"

// ---------- État exposé (lecture, pour l'affichage et le menu) ----------
extern Track tracks[MAX_TRACKS];
extern int   seqSelection;   // 0=+New, 1..MAX_TRACKS=pistes, MAX_TRACKS+1=Back
extern bool  seqRecording;
extern bool  seqPlaying;
extern int   recTrack, playTrack;
extern unsigned long seqRecStart, seqPlayStart;

// ---------- Boucle (appelée depuis loop()) ----------
void seqUpdate();

// ---------- Actions (appelées par le menu) ----------
void sequencerAction();   // selon seqSelection : enregistre une nouvelle piste ou en relit une
void startRecording();
void stopRecording();
void startPlayback(int trackIndex);
void stopPlayback();

// ---------- Crochet d'enregistrement (appelé par readNotes) ----------
void seqRecordNote(int8_t top, float freq);

#endif // SEQUENCER_H
