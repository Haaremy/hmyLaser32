#ifndef LASERTAG_SERVER_IDENTITY_H
#define LASERTAG_SERVER_IDENTITY_H

#include <Arduino.h>

// Lädt Name + PIN aus NVS; generiert sie neu, falls leer.
void identityLoadOrCreate();

// Neue Identität (z.B. nach Reset-Trigger).
void identityRegenerate();

#endif
