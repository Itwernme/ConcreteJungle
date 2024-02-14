#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "raymath.h"
#include "global.h"

// Typedefs
typedef struct Quad {
    Vector2 verts[4];
} Quad;

typedef struct IntFloat {
    int i;
    float f;
} IntFloat;

typedef struct PlayerData {
  Vector2 pos;
} PlayerData;

typedef struct WorldData {
  char level[56];
} WorldData;

// Function definitions
void initGame();
void updateGame(float delta);
void drawGame(RenderTexture2D *output);
void unloadGame();

#endif
