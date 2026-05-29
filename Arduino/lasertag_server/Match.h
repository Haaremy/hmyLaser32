#ifndef LASERTAG_SERVER_MATCH_H
#define LASERTAG_SERVER_MATCH_H

#include <Arduino.h>
#include "Types.h"

void matchLoadSettings();
void matchSaveSettings();
bool matchStart();
bool matchActivate();
bool matchEnd();
bool matchAbort();
void matchLoop();

#endif
