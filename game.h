#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "raymath.h"

// Typedefs
typedef struct Quad {
    Vector2 verts[4];
} Quad;

typedef struct Block {
  Rectangle rect;
  float height;
} Block;

typedef struct IntFloat {
    int i;
    float f;
} IntFloat;

typedef struct PlayerData {
  Vector2 pos;
} PlayerData;

typedef struct WorldData {
  Block *blocks;
  int nBlocks;
} LevelData;

// Function definitions
void initGame();
void updateGame(float delta);
void drawGame(RenderTexture2D *output);
void unloadGame();

#endif
