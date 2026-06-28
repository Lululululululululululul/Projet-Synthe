/* =====================================================================
 *  menu.h  —  MENU + ENCODEUR (couche commande)
 * ---------------------------------------------------------------------
 *  - lecture de l'encodeur rotatif (rotation + clic)
 *  - machine à états du menu (menu principal / sous-menus / édition)
 *  - dispatch de l'édition des paramètres vers le moteur audio
 *
 *  L'état du menu est exposé en lecture pour le module d'affichage.
 * ===================================================================== */
#ifndef MENU_H
#define MENU_H

#include "config.h"

// ---------- État du menu exposé (lecture, pour l'affichage) ----------
extern int  menuIndex;     // entrée sélectionnée dans le menu principal
extern int  menuTop;       // première ligne visible (menu déroulant)
extern bool inSubMenu;     // dans un sous-menu ?
extern int  subCursor;     // ligne sélectionnée dans le sous-menu
extern bool editing;       // édition de la valeur courante active ?
extern int  presetIndex;   // preset sélectionné

extern const char* menuItems[MENU_SIZE]; // libellés du menu principal

// ---------- Initialisation ----------
void menuSetup();          // configure l'encodeur et son bouton

// ---------- Boucle (appelées depuis loop()) ----------
void readEncoder();        // rotation de l'encodeur
void readMenuButton();     // clic du bouton de l'encodeur

#endif // MENU_H
