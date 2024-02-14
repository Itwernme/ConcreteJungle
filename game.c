#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "global.h"

// Local function definitions
static void drawLevel();
static Quad rectToQuad(Rectangle rect);
static Vector2 V2Round(Vector2 in);
static Quad projectQuad(Quad baseQuad, float height);
static void quicksort(IntFloat *array, int low, int high);
static float rectPointDist(Vector2 point, Rectangle rect);
static uint indexShit(uint x, int width, int height);
static bool readFromLevel(uint index);
static void writeToLevel(uint index, bool state);

// Constants
const Vector2 screenCentre = (Vector2){viewportWidth / 2.0f, viewportHeight / 2.0f};
const struct {
  float speed;
  float size;
} playerConsts = {80, 6};

// Variables
static PlayerData player = {(Vector2){0, 0}};
static bool paused = false;
static WorldData world;
static Camera2D camera;
static Vector2 viewportPos;

static Texture2D backgroundTex;
static Texture2D vignetteTex;

static float flicker = 0;

static Rectangle collidable[4];

void initGame() {
  writeToLevel(5, true);

  camera = (Camera2D){Vector2Zero(), Vector2Zero(), 0.0f, 1.0f};

  Image img = GenImageChecked(64, 64, 32, 32, (Color){30, 30, 30, 255}, (Color){15, 15, 15, 255});
  backgroundTex = LoadTextureFromImage(img);
  UnloadImage(img);

  img = GenImageGradientRadial(viewportWidth, viewportHeight, 0.1f, (Color){0, 0, 0, 0}, (Color){0, 0, 0, 255});
  vignetteTex = LoadTextureFromImage(img);
  UnloadImage(img);
}

void updateGame(float delta) {
  if (paused) delta *= 0.01;

  Vector2 rawIn = Vector2Zero();
  static Vector2 rawPos = (Vector2){0, 0};

  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) rawIn.y--;
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) rawIn.y++;
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) rawIn.x--;
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) rawIn.x++;

  rawIn = Vector2Scale(Vector2Normalize(rawIn), playerConsts.speed * delta * GetRandomValue(30, 100) / 100.0f);
  rawPos = Vector2Add(rawPos, rawIn);

  // Collisions
  for (int i = 0; i < 4; i++) {
    Rectangle rect = collidable[i];
    if (rect.y + rect.height > rawPos.y && rawPos.y > rect.y) {
      if (rawPos.x > rect.x + rect.width && rawPos.x < rect.x + rect.width + playerConsts.size) {
        rawPos.x -= rawPos.x - (rect.x + rect.width + playerConsts.size);
      } else if (rawPos.x < rect.x && rawPos.x > rect.x - playerConsts.size) {
        rawPos.x -= rawPos.x - (rect.x - playerConsts.size);
      }
    } else if (rect.x + rect.width > rawPos.x && rawPos.x > rect.x) {
      if (rawPos.y > rect.y + rect.height && rawPos.y < rect.y + rect.height + playerConsts.size) {
        rawPos.y -= rawPos.y - (rect.y + rect.height + playerConsts.size);
      } else if (rawPos.y < rect.y && rawPos.y > rect.y - playerConsts.size) {
        rawPos.y -= rawPos.y - (rect.y - playerConsts.size);
      }
    } else {
      Quad quad = rectToQuad(rect);

      float dist = FLT_MAX;
      int corner;
      for (int i = 0; i < 4; i++) {
        float temp = Vector2Distance(rawPos, quad.verts[i]);
        dist = fminf(dist, temp);
        if (temp == dist) corner = i;
      }

      if (dist < playerConsts.size) {
        Vector2 change = Vector2Scale(Vector2Normalize(Vector2Subtract(rawPos, quad.verts[corner])), dist - playerConsts.size);
        rawPos = Vector2Subtract(rawPos, change);
      }
    }
  }

  player.pos = V2Round(rawPos);
  camera.offset = Vector2Subtract(screenCentre, player.pos);
  viewportPos = Vector2Subtract(player.pos, screenCentre);
}

void drawGame(RenderTexture2D *output) {
  flicker += GetRandomValue(-150, 150) / 100.0f;
  flicker = Clamp(flicker, 0, 64);

  BeginTextureMode(*output);
    ClearBackground(PURPLE);

    Vector2 modPos = (Vector2){(float)((int)viewportPos.x % 64), (float)((int)viewportPos.y % 64)};
    DrawTexturePro(backgroundTex, (Rectangle){modPos.x, modPos.y, (float)viewportWidth, (float)viewportHeight}, (Rectangle){0, 0, (float)viewportWidth, (float)viewportHeight}, Vector2Zero(), 0.0f, WHITE);

    BeginMode2D(camera);
      double startTime = GetTime(); // Start timing
      drawLevel();
      debugStats.levelDrawT = (GetTime() - startTime + debugStats.levelDrawT*19) / 20.0f; // End timing
    EndMode2D();

    DrawTexturePro(vignetteTex, (Rectangle){flicker / 2.0f, flicker / 2.0f, viewportWidth - flicker, viewportHeight - flicker}, (Rectangle){0, 0, viewportWidth, viewportHeight}, Vector2Zero(), 0.0f, WHITE);

    DrawCircleV(screenCentre, playerConsts.size, RED);

    DrawFPS(viewportWidth - 80, 10);
  EndTextureMode();
}

void unloadGame() {
  UnloadTexture(backgroundTex);
}

static void drawLevel() {
  for (int i = 1; i < 441; i++) {
    uint index = indexShit(i, 21, 21);

    if (readFromLevel(index)) {
      Rectangle rect = (Rectangle){(index % 20)*32, floor(index / 20.0f)*32, 32, 32};
      Quad quad = rectToQuad(rect);
      Quad projQuad = projectQuad(quad, 1);
      Vector2 middle = (Vector2){rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
      float brightness = (100 / (rectPointDist(player.pos, rect) * 0.08 + 1)) * (flicker / 256.0f + 0.875);

      DrawTriangleFan(projQuad.verts, 4, (Color){20, 20, 20, 255});

      if (quad.verts[3].y < player.pos.y) {
        Vector2 face[4] = {quad.verts[0], quad.verts[1], projQuad.verts[1], projQuad.verts[0]};
        unsigned char faceBr = brightness * Vector2DotProduct((Vector2){0, 1}, Vector2Normalize(Vector2Subtract(player.pos, middle)));
        DrawTriangleFan(face, 4, (Color){faceBr, faceBr, faceBr, 255});
      } else if (quad.verts[2].y > player.pos.y) {
        Vector2 face[4] = {quad.verts[2], quad.verts[3], projQuad.verts[3], projQuad.verts[2]};
        unsigned char faceBr = brightness * Vector2DotProduct((Vector2){0, -1}, Vector2Normalize(Vector2Subtract(player.pos, middle)));
        DrawTriangleFan(face, 4, (Color){faceBr, faceBr, faceBr, 255});
      }
      if (quad.verts[1].x < player.pos.x) {
        Vector2 face[4] = {quad.verts[1], quad.verts[2], projQuad.verts[2], projQuad.verts[1]};
        unsigned char faceBr = brightness * Vector2DotProduct((Vector2){1, 0}, Vector2Normalize(Vector2Subtract(player.pos, middle)));
        DrawTriangleFan(face, 4, (Color){faceBr, faceBr, faceBr, 255});
      } else if (quad.verts[3].x > player.pos.x) {
        Vector2 face[4] = {quad.verts[3], quad.verts[0], projQuad.verts[0], projQuad.verts[3]};
        unsigned char faceBr = brightness * Vector2DotProduct((Vector2){-1, 0}, Vector2Normalize(Vector2Subtract(player.pos, middle)));
        DrawTriangleFan(face, 4, (Color){faceBr, faceBr, faceBr, 255});
      }
    }
  }
}

static Quad rectToQuad(Rectangle rect) {
  Quad quad = {(Vector2){rect.x, rect.y + rect.height}, (Vector2){rect.x + rect.width, rect.y + rect.height}, (Vector2){rect.x + rect.width, rect.y}, (Vector2){rect.x, rect.y}};
  return quad;
}

static Vector2 V2Round(Vector2 in) {
  return (Vector2){roundf(in.x), roundf(in.y)};
}

static Quad projectQuad(Quad baseQuad, float height) {
  Quad projQuad;
  for (int i = 0; i < 4; i++) {
    projQuad.verts[i] = Vector2Add(Vector2Scale(Vector2Subtract(baseQuad.verts[i], player.pos), 1.0f / height), baseQuad.verts[i]);
  }
  return projQuad;
}
/*
static void quicksort(IntFloat *array, int low, int high) {
  int x, y, p;
  IntFloat temp;
  if (low < high){
    p = low;
    x = low;
    y = high;
    while(x < y){
      while(array[x].f <= array[p].f && x < high) x++;
      while(array[y].f > array[p].f) y--;
      if (x < y){
        temp = array[x];
        array[x] = array[y];
        array[y] = temp;
      }
    }
    temp = array[p];
    array[p] = array[y];
    array[y] = temp;
    quicksort(array, low, y-1);
    quicksort(array, y+1, high);
  }
}
*/
static float rectPointDist(Vector2 point, Rectangle rect) {
  if (rect.y + rect.height > point.y && point.y > rect.y) return fminf(abs(rect.x - point.x), abs((rect.x + rect.width) - point.x));
  if (rect.x + rect.width > point.x && point.x > rect.x) return fminf(abs(rect.y - point.y), abs((rect.y + rect.height) - point.y));
  Quad quad = rectToQuad(rect);
  float temp = Vector2Distance(point, quad.verts[0]);
  for (int i = 1; i <= 3; i++) {
    temp = fminf(temp, Vector2Distance(point, quad.verts[i]));
  }
  return temp;
}

static uint indexShit(uint x, int width, int height) {
  int outputx, outputy;
  int layer = ceil((sqrt(x+1)-1) / 2.0f);
  int relPos = x - ((layer-1) * (layer-1) + (layer-1)) * 4;
  int section = ceil(relPos / 4.0f);

  if (section == 1 || section == layer * 2) {
    int trough = section / 2;
    switch (relPos % 4) {
      case 0:
        outputx = layer;
        outputy = trough;
        break;
      case 1:
        outputx = -layer;
        outputy = -trough;
        break;
      case 2:
        outputx = trough;
        outputy = -layer;
        break;
      case 3:
        outputx = -trough;
        outputy = layer;
        break;
    }
  } else {
    int stage = ceil((section - 1) / 2.0f);
    if (section % 2 == 0) {
      switch (relPos % 4) {
        case 0:
          outputx = layer;
          outputy = -stage;
          break;
        case 1:
          outputx = layer;
          outputy = stage;
          break;
        case 2:
          outputx = -layer;
          outputy = -stage;
          break;
        case 3:
          outputx = -layer;
          outputy = stage;
          break;
      }
    } else {
      switch (relPos % 4) {
        case 0:
          outputx = stage;
          outputy = -layer;
          break;
        case 1:
          outputx = stage;
          outputy = layer;
          break;
        case 2:
          outputx = -stage;
          outputy = -layer;
          break;
        case 3:
          outputx = -stage;
          outputy = layer;
          break;
      }
    }
  }

  return (outputx + outputy * width) + width * height * 0.5f;// + width * height * 0.5f;
}

static bool readFromLevel(uint index) {
  int byte = floor(index / 8.0f);
  int p = index % 8;
  return (world.level[byte] >> (p-1)) % 2;
}

static void writeToLevel(uint index, bool state) {
  int byte = floor(index / 8.0f);
  int p = index % 8;
  char mask = 1 << p;
  world.level[byte] = ((world.level[byte] & ~mask) | (state << p));
}
