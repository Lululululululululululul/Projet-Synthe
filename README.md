# MONOSTATION — Firmware

Mini-synthétiseur monophonique numérique à synthèse soustractive.
Projet pluridisciplinaire ISEN2 (2025/2026) — groupe **Synthé**.

**Plateforme :** Teensy 4.1 + Teensy Audio Adapter Board Rev D (codec SGTL5000).

## Membres du groupe

- **Mathieu Gattone** — chef de projet
- **Louis Laurent** — développeur (logiciel / DSP)
- **Doryan Le Coadou** — conception mécanique / CAO, responsable Fablab
- **Lucas Desfougeres** — conception électronique, responsable Fablab

Encadrant : Amaury Auguste — Promotion ISEN2, année 2025/2026.

## Démonstration vidéo

▶️ https://youtu.be/DSE_qQ8myZg

---

## Chaîne de traitement du son

```
OSC1 ┐
     ├─► Mixer ─► Filtre passe-bas résonant ─► Enveloppe ADSR ─► Sortie I²S
OSC2 ┘            (State Variable)
```

Deux oscillateurs (le second désaccordable), un mélangeur, un filtre passe-bas
résonant piloté par deux potentiomètres (coupure + résonance), une enveloppe
ADSR, puis la sortie audio I²S vers le codec.

---

## Organisation du code

Le firmware est découpé en **modules**, suivant la séparation
*interface → logique → traitement du signal* présentée dans le rapport. Chaque
module expose son état et ses fonctions dans un en-tête `.h`, et les implémente
dans le `.cpp` correspondant. Le fichier **commun** `config.h` regroupe tout ce
qui est partagé (brochage, constantes, structures).

| Fichier | Rôle |
|---|---|
| `MONOSTATION.ino` | Point d'entrée : `setup()` et `loop()`. Ne fait qu'orchestrer les modules. |
| `config.h` | **En-tête commun** : brochage Teensy, constantes de réglage, structures `Preset` / `SeqEvent` / `Track`. Que des `#define`, `constexpr` et types — aucune variable d'état. |
| `audio_engine.h` / `.cpp` | **Le son** : chaîne audio Teensy, voix monophonique (pile de notes, priorité à la dernière note), note → fréquence + octave, presets, lecture des potentiomètres vers le filtre. |
| `menu.h` / `.cpp` | **Menu + encodeur** : lecture de l'encodeur rotatif (rotation + clic), machine à états du menu, édition des paramètres (qui délègue le travail sonore au moteur audio). |
| `display.h` / `.cpp` | **Affichage OLED** : barre d'état, menu déroulant et sous-menus. Ne fait que *lire* l'état des autres modules. |
| `sequencer.h` / `.cpp` | **Séquenceur** : enregistrement et relecture de courtes pistes (note on/off horodatés). |

### Comment les modules communiquent

- Chaque variable d'état globale est **définie une seule fois** dans son module
  (état sonore dans `audio_engine.cpp`, état du menu dans `menu.cpp`, état du
  séquenceur dans `sequencer.cpp`) et **déclarée `extern`** dans son en-tête pour
  que les autres modules puissent la lire.
- Le menu ne touche jamais directement aux objets audio : il appelle des
  fonctions du moteur (`audioCycleWaveform`, `audioAdjustEnv`, `audioLoadPreset`…).
- Le séquenceur joue les notes via `audioSeqPlay()` / `audioSeqStop()`.
- L'affichage est purement passif : il lit l'état et le dessine.

---

## Brochage (Teensy 4.1)

| Élément | Broche(s) |
|---|---|
| Encodeur rotatif | CLK = 33, DT = 34, bouton = 35 |
| Clavier do → si | 36, 37, 32, 31, 30, 29, 28 |
| Potentiomètre coupure filtre | A10 |
| Potentiomètre résonance filtre | A11 |
| Écran OLED SSD1306 | I²C (SDA/SCL), adresse `0x3C` |
| Sortie audio | I²S (via l'Audio Adapter Board) |

---

## Dépendances (bibliothèques)

Fournies avec **Teensyduino** (rien à installer) :
- Teensy Audio Library (`Audio.h`)
- `Wire` (I²C), `SPI`
- `Encoder`

À installer via le gestionnaire de bibliothèques de l'IDE Arduino :
- **Adafruit GFX Library**
- **Adafruit SSD1306**
- **Bounce2**

---

## Compilation et téléversement

1. Installer l'**IDE Arduino** puis l'add-on **Teensyduino**.
2. Installer les trois bibliothèques ci-dessus (Croquis → Inclure une
   bibliothèque → Gérer les bibliothèques).
3. Ouvrir `MONOSTATION.ino`. ⚠️ Le **dossier** doit porter le même nom que le
   `.ino` (`MONOSTATION/`) — c'est déjà le cas ici. Les autres fichiers
   (`.h` / `.cpp`) apparaissent alors automatiquement en onglets.
4. Outils → Type de carte → **Teensy 4.1**.
5. Sélectionner le port, puis **Téléverser**.

Le son sort par la prise casque de l'Audio Adapter Board.

---

## Pourquoi des fichiers `.cpp` (C++) et non `.c` (C) ?

Le projet s'appuie sur des **classes C++** : la bibliothèque audio Teensy
(`AudioSynthWaveform`, `AudioFilterStateVariable`…), `Adafruit_SSD1306`,
`Bounce`, `Encoder`. Ces objets n'existent pas en langage C : un fichier `.c`
compilé en C ne pourrait pas les utiliser. Les fichiers d'implémentation sont
donc en `.cpp`. En revanche, le « fichier commun » demandé est bien là : c'est
l'en-tête **`config.h`**, inclus par tous les modules.
