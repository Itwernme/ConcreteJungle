#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "global.h"

// Local function definitions
static Quad rectToQuad(Rectangle rect);
static float rectPointDist(Vector2 point, Rectangle rect);
static Int2 indexShit(uint x);
static bool readFromLevel(Int2 pos);
static void writeToLevel(Int2 pos, bool state);
static void drawTile(Vector2 pos);
static uint myMod(int a, int b);

// Constants
const Vector2 screenCentre = (Vector2){viewportWidth / 2.0f, viewportHeight / 2.0f};
const struct {
  float speed;
  float size;
} playerConsts = {200, 6};

// Variables
static PlayerData player = {(Vector2){0, 0}};
static bool paused = false;
static WorldData world;
//static Camera2D camera;
static Vector2 globalOffset;
static Vector2 viewportPos;

static Texture2D backgroundTex;
static Texture2D vignetteTex;

static float flicker = 0;

void initGame() {
  for (int i = 0; i < 21; i++) {
    writeToLevel((Int2){i, i}, true);
  }

  //camera = (Camera2D){Vector2Zero(), Vector2Zero(), 0.0f, 1.0f};

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
  Int2 gridPos = (Int2){(rawPos.x > 0 ? (int)rawPos.x / 32 : floor(rawPos.x / 32.0f)), (rawPos.y > 0 ? (int)rawPos.y / 32 : floor(rawPos.y / 32.0f))};
  for (int i = 0; i < 8; i++) {
    Int2 relPos = indexShit(i+1);
    if (readFromLevel((Int2){gridPos.x + relPos.x, gridPos.y + relPos.y})) {
      Rectangle rect = (Rectangle){(gridPos.x + relPos.x) * 32, (gridPos.y + relPos.y) * 32, 32, 32};

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
  }

  player.pos = (Vector2){roundf(rawPos.x), roundf(rawPos.y)};
  globalOffset = Vector2Subtract(screenCentre, player.pos);
  viewportPos = Vector2Subtract(player.pos, screenCentre);
}

void drawGame(RenderTexture2D *output) {
  flicker += GetRandomValue(-150, 150) / 100.0f;
  flicker = Clamp(flicker, 0, 64);

  BeginTextureMode(*output);
    ClearBackground(PURPLE);

    DrawTexturePro(backgroundTex, (Rectangle){(int)viewportPos.x % 64, (int)viewportPos.y % 64, (float)viewportWidth, (float)viewportHeight}, (Rectangle){0, 0, (float)viewportWidth, (float)viewportHeight}, Vector2Zero(), 0.0f, WHITE);

    double startTime = GetTime(); // Start timing
    Int2 gridPos = (Int2){(player.pos.x > 0 ? (int)player.pos.x / 32 : floor(player.pos.x / 32.0f)), (player.pos.y > 0 ? (int)player.pos.y / 32 : floor(player.pos.y / 32.0f))};
    Int2 subGridPos = (Int2){myMod(player.pos.x, 32), myMod(player.pos.y, 32)};

    for (int i = 440; i > 0; i--) {
      Int2 relPos = indexShit(i);

      if (readFromLevel((Int2){gridPos.x + relPos.x, gridPos.y + relPos.y})) {
        drawTile((Vector2){relPos.x * 32 - subGridPos.x + screenCentre.x, relPos.y * 32 - subGridPos.y + screenCentre.y});
      }
    }
    if (readFromLevel(gridPos)) {
      drawTile((Vector2){-subGridPos.x + screenCentre.x, -subGridPos.y + screenCentre.y});
    }

    debugStats.levelDrawT = (GetTime() - startTime + debugStats.levelDrawT*19) / 20.0f; // End timing

    DrawTexturePro(vignetteTex, (Rectangle){flicker / 2.0f, flicker / 2.0f, viewportWidth - flicker, viewportHeight - flicker}, (Rectangle){0, 0, viewportWidth, viewportHeight}, Vector2Zero(), 0.0f, WHITE);

    DrawCircleV(screenCentre, playerConsts.size, RED);
  EndTextureMode();
}

void unloadGame() {
  UnloadTexture(backgroundTex);
}

static Quad rectToQuad(Rectangle rect) {
  Quad quad = {(Vector2){rect.x, rect.y + rect.height}, (Vector2){rect.x + rect.width, rect.y + rect.height}, (Vector2){rect.x + rect.width, rect.y}, (Vector2){rect.x, rect.y}};
  return quad;
}

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

static Int2 indexShit(uint x) {
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

  //return (outputx + outputy * width) + width * height * 0.5f;// + width * height * 0.5f;
  return (Int2){outputx, outputy};
}

static bool readFromLevel(Int2 pos) {
  pos = (Int2){myMod(pos.x, 21), myMod(pos.y, 21)};
  int byte = pos.x / 8;
  int p = pos.x % 8;
  return (world.level[pos.y][byte] >> p) % 2;
}

static void writeToLevel(Int2 pos, bool state) {
  pos = (Int2){myMod(pos.x, 21), myMod(pos.y, 21)};
  int byte = pos.x / 8;
  int p = pos.x % 8;
  char mask = 1 << p;
  world.level[pos.y][byte] = ((world.level[pos.y][byte] & ~mask) | (state << p));
}

static void drawTile(Vector2 pos) {
  Rectangle rect = (Rectangle){pos.x, pos.y, 32, 32};
  Quad quad = rectToQuad(rect);
  Quad projQuad;
  for (int i = 0; i < 4; i++) {
    projQuad.verts[i] = Vector2Add(Vector2Scale(Vector2Subtract(quad.verts[i], screenCentre), 1.0f / 1.0f), quad.verts[i]);
  }

  Vector2 middle = (Vector2){rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
  float brightness = (100 / (rectPointDist(screenCentre, rect) * 0.08 + 1)) * (flicker / 256.0f + 0.875);

  DrawTriangleFan(projQuad.verts, 4, (Color){20, 20, 20, 255});

  if (quad.verts[3].y < screenCentre.y) {
    Vector2 face[4] = {quad.verts[0], quad.verts[1], projQuad.verts[1], projQuad.verts[0]};
    unsigned char faceBr = brightness * Vector2DotProduct((Vector2){0, 1}, Vector2Normalize(Vector2Subtract(screenCentre, middle)));
    DrawTriangleFan(face, 4, (Color){faceBr, faceBr, faceBr, 255});
  } else if (quad.verts[2].y > screenCentre.y) {
    Vector2 face[4] = {quad.verts[2], quad.verts[3], projQuad.verts[3], projQuad.verts[2]};
    unsigned char faceBr = brightness * Vector2DotProduct((Vector2){0, -1}, Vector2Normalize(Vector2Subtract(screenCentre, middle)));
    DrawTriangleFan(face, 4, (Color){faceBr, faceBr, faceBr, 255});
  }
  if (quad.verts[1].x < screenCentre.x) {
    Vector2 face[4] = {quad.verts[1], quad.verts[2], projQuad.verts[2], projQuad.verts[1]};
    unsigned char faceBr = brightness * Vector2DotProduct((Vector2){1, 0}, Vector2Normalize(Vector2Subtract(screenCentre, middle)));
    DrawTriangleFan(face, 4, (Color){faceBr, faceBr, faceBr, 255});
  } else if (quad.verts[3].x > screenCentre.x) {
    Vector2 face[4] = {quad.verts[3], quad.verts[0], projQuad.verts[0], projQuad.verts[3]};
    unsigned char faceBr = brightness * Vector2DotProduct((Vector2){-1, 0}, Vector2Normalize(Vector2Subtract(screenCentre, middle)));
    DrawTriangleFan(face, 4, (Color){faceBr, faceBr, faceBr, 255});
  }
}

static uint myMod(int a, int b) {
  int r = a % b;
  if (r < 0) return (r + b);
  else return r;
}
