#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "global.h"

// Local function definitions
static void update(float delta);
static void draw();

// Variables
Screen currentScreen = GAME;
struct DebugStats debugStats = {0, 0, 0, 0};
static RenderTexture2D viewport;

int main(void) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(viewportWidth, viewportHeight, "Generic Game");
  SetTargetFPS(60);
  SetTraceLogLevel(LOG_WARNING);
  viewport = LoadRenderTexture(viewportWidth, viewportHeight);

  initGame();

  while (!WindowShouldClose()) {
    double startTime = GetTime(); // Start timing
    update(GetFrameTime());
    debugStats.mainUpdateT = (GetTime() - startTime + debugStats.mainUpdateT*19) / 20.0f; // End timing

    draw();
  }

  switch (currentScreen) {
    //case MENU: unloadMenu(); break;
    case GAME: unloadGame(); break;
    default: break;
  }

  UnloadRenderTexture(viewport);
  CloseWindow();

  return 0;
}

static void update(float delta) {
  switch (currentScreen)
  {
    //case MENU: updateMenu(); break;
    case GAME: updateGame(delta); break;
    default: break;
  }
}

static void draw() {
  double startTime = GetTime(); // Start timing

  static float scale;
  static Vector2 pos;
  if (IsWindowResized()) {
    scale = fminf(GetScreenWidth() / (float)viewportWidth, GetScreenHeight() / (float)viewportHeight);
    pos = (Vector2){GetScreenWidth() / 2.0f - (viewportWidth / 2.0f) * scale, GetScreenHeight() / 2.0f - (viewportHeight / 2.0f) * scale};
  }

  switch (currentScreen)
  {
    //case MENU: updateMenu(); break;
    case GAME: drawGame(&viewport); break;
    default: break;
  }

  // Calc breakdown
  debugStats.totalT = debugStats.mainDrawT + debugStats.mainUpdateT;
  char buf[1024];
  sprintf(buf, "level draw: %f%%\ntotal draw: %f%%\nupdate: %f%%", debugStats.levelDrawT / debugStats.totalT * 100, debugStats.mainDrawT / debugStats.totalT * 100, debugStats.mainUpdateT / debugStats.totalT * 100);

  debugStats.mainDrawT = (GetTime() - startTime + debugStats.mainDrawT*19) / 20.0f; // End timing
  BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(viewport.texture, (Rectangle){0, 0, viewportWidth, -viewportHeight}, (Rectangle){pos.x, pos.y, viewportWidth * scale, viewportHeight * scale}, Vector2Zero(), 0.0f, WHITE);
    DrawFPS(GetScreenWidth() - 80, 10);
    DrawText(buf, 10, 10, 10, GREEN);
  EndDrawing();
}
