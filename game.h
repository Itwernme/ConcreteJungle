#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "raymath.h"
#include "global.h"

// Typedefs
typedef struct Quad {
    Vector2 verts[4];
} Quad;

typedef struct Int2 {
    int x, y;
} Int2;

typedef struct PlayerData {
  Vector2 pos;
} PlayerData;

typedef struct WorldData {
  char level[21][3];
} WorldData;

// Function definitions
void initGame();
void updateGame(float delta);
void drawGame(RenderTexture2D *output);
void unloadGame();

#endif
