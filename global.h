#ifndef GLOBAL_H
#define GLOBAL_H

#include "raylib.h"
#include "raymath.h"

// Typedefs
typedef enum Screen {UNKNOWN = -1, MENU = 0, GAME} Screen;

// Function definitions

// Variables
extern Screen currentScreen;
static const int viewportWidth = 640;
static const int viewportHeight = 480;

extern struct DebugStats{
  double mainUpdateT;
  double mainDrawT;
  double levelDrawT;
  double totalT;
} debugStats;

#endif
