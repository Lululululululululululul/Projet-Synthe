/* =====================================================================
 *  display.h  —  AFFICHAGE OLED (retour visuel, FP7)
 * ---------------------------------------------------------------------
 *  Dessine la barre d'état, le menu principal déroulant et les sous-menus.
 *  L'affichage ne fait que LIRE l'état des autres modules (son, menu,
 *  séquenceur) : il ne modifie jamais rien.
 * ===================================================================== */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"

void displaySetup();   // initialise l'écran (I2C déjà démarré dans setup())
void drawMenu();       // rafraîchit l'écran complet

#endif // DISPLAY_H
