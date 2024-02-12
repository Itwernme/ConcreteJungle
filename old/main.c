#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "myutils.h"
#include <stdio.h>

// Custom Blend Modes
#define RLGL_SRC_ALPHA 0x0302
#define RLGL_MIN 0x8007
#define RLGL_MAX 0x8008

#define MAX_BOXES     20
#define MAX_SHADOWS   MAX_BOXES*4         // MAX_BOXES *3. Each box can cast up to two shadow volumes for the edges it is away from, and one for the box itself

// Shadow geometry type
typedef struct ShadowGeometry {
    Vector2 vertices[4];
} ShadowGeometry;

// player info type
typedef struct PlayerInfo {
    bool dirty;                 // Does this player need to be updated?

    Vector2 position;           // player position
    RenderTexture mask;         // Alpha mask for the player
    float outerRadius;          // The distance the player touches
    Rectangle bounds;           // A cached rectangle of the player bounds to help with culling

    ShadowGeometry shadows[MAX_SHADOWS];
    int shadowCount;
} PlayerInfo;

PlayerInfo player;

// Move a player and mark it as dirty so that we update it's mask next frame
void Moveplayer(float x, float y)
{
    player.dirty = true;
    player.position.x = x;
    player.position.y = y;

    // update the cached bounds
    player.bounds.x = x - player.outerRadius;
    player.bounds.y = y - player.outerRadius;
}

// Compute a shadow volume for the edge
// It takes the edge and projects it back by the player radius and turns it into a quad
void ComputeShadowVolumeForEdge(Vector2 sp, Vector2 ep)
{
    if (player.shadowCount >= MAX_SHADOWS) return;

    Vector2 spProjection = Vector2Add(Vector2Scale(Vector2Subtract(sp, player.position), 4.0f), sp);
    Vector2 epProjection = Vector2Add(Vector2Scale(Vector2Subtract(ep, player.position), 4.0f), ep);

    player.shadows[player.shadowCount].vertices[0] = sp;
    player.shadows[player.shadowCount].vertices[1] = ep;
    player.shadows[player.shadowCount].vertices[2] = epProjection;
    player.shadows[player.shadowCount].vertices[3] = spProjection;

    player.shadowCount++;
}

// Draw the player and shadows to the mask for a player
void DrawplayerMask()
{
    // Use the player mask
    BeginTextureMode(player.mask);

        ClearBackground(BLACK);

        // Force the blend mode to only set the alpha of the destination
        rlSetBlendFactors(RLGL_SRC_ALPHA, RLGL_SRC_ALPHA, RLGL_MIN);
        rlSetBlendMode(BLEND_CUSTOM);

        // If we are valid, then draw the player radius to the alpha mask
        DrawCircleGradient((int)player.position.x, (int)player.position.y, player.outerRadius, ColorAlpha(BLACK, 0), BLACK);

        rlDrawRenderBatchActive();

        // Cut out the shadows from the player radius by forcing the alpha to maximum
        rlSetBlendMode(BLEND_ALPHA);
        rlSetBlendFactors(RLGL_SRC_ALPHA, RLGL_SRC_ALPHA, RLGL_MAX);
        rlSetBlendMode(BLEND_CUSTOM);

        // Draw the shadows to the alpha mask
        for (int i = 0; i < player.shadowCount; i++)
        {
            DrawTriangleFan(player.shadows[i].vertices, 4, WHITE);
        }

        rlDrawRenderBatchActive();

        // Go back to normal blend mode
        rlSetBlendMode(BLEND_ALPHA);

    EndTextureMode();
}

// Setup a player
void Setupplayer(float x, float y, float radius)
{
    player.mask = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    player.outerRadius = radius;

    player.bounds.width = radius * 2;
    player.bounds.height = radius * 2;

    Moveplayer(x, y);

    // Force the render texture to have something in it
    DrawplayerMask();
}

// See if a player needs to update it's mask
bool Updateplayer(Rectangle* boxes, int count)
{
    if (!player.dirty) return false;

    player.dirty = false;
    player.shadowCount = 0;

    for (int i = 0; i < count; i++)
    {
        // Are we in a box? if so we are not valid
        //if (CheckCollisionPointRec(player.position, boxes[i])) continue;

        // If this box is outside our bounds, we can skip it
        //if (!CheckCollisionRecs(player.bounds, boxes[i])) continue;

        // Check the edges that are on the same side we are, and cast shadow volumes out from them

        // Top
        Vector2 sp = (Vector2){ boxes[i].x, boxes[i].y };
        Vector2 ep = (Vector2){ boxes[i].x + boxes[i].width, boxes[i].y };

        if (player.position.y > ep.y) ComputeShadowVolumeForEdge(sp, ep);

        // Right
        sp = ep;
        ep.y += boxes[i].height;
        if (player.position.x < ep.x) ComputeShadowVolumeForEdge(sp, ep);

        // Bottom
        sp = ep;
        ep.x -= boxes[i].width;
        if (player.position.y < ep.y) ComputeShadowVolumeForEdge(sp, ep);

        // Left
        sp = ep;
        ep.y -= boxes[i].height;
        if (player.position.x > ep.x) ComputeShadowVolumeForEdge(sp, ep);

        // The box itself
        player.shadows[player.shadowCount].vertices[0] = (Vector2){ boxes[i].x, boxes[i].y };
        player.shadows[player.shadowCount].vertices[1] = (Vector2){ boxes[i].x, boxes[i].y + boxes[i].height };
        player.shadows[player.shadowCount].vertices[2] = (Vector2){ boxes[i].x + boxes[i].width, boxes[i].y + boxes[i].height };
        player.shadows[player.shadowCount].vertices[3] = (Vector2){ boxes[i].x + boxes[i].width, boxes[i].y };
        player.shadowCount++;
    }

    DrawplayerMask();

    return true;
}

// Set up some boxes
void SetupBoxes(Rectangle *boxes, int *count)
{
    boxes[0] = (Rectangle){ 150,80, 40, 40 };
    boxes[1] = (Rectangle){ 1200, 700, 40, 40 };
    boxes[2] = (Rectangle){ 200, 600, 40, 40 };
    boxes[3] = (Rectangle){ 1000, 50, 40, 40 };
    boxes[4] = (Rectangle){ 500, 350, 40, 40 };

    for (int i = 5; i < MAX_BOXES; i++)
    {
        boxes[i] = (Rectangle){(float)GetRandomValue(0,GetScreenWidth()), (float)GetRandomValue(0,GetScreenHeight()), (float)GetRandomValue(10,100), (float)GetRandomValue(10,100) };
    }

    *count = MAX_BOXES;
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    const float moveSpeed = 100.0f;

    Camera2D cam;
    cam.offset = Vector2Zero();
    cam.target = Vector2Zero();
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "raylib [shapes] example - top down players");

    // Initialize our 'world' of boxes
    int boxCount = 0;
    Rectangle boxes[MAX_BOXES] = { 0 };
    SetupBoxes(boxes, &boxCount);

    // Create a checkerboard ground texture
    Image img = GenImageChecked(64, 64, 32, 32, DARKBROWN, DARKGRAY);
    Texture2D backgroundTexture = LoadTextureFromImage(img);
    UnloadImage(img);

    // Create a global player mask to hold all the blended players
    RenderTexture playerMask = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    // Setup initial player
    Setupplayer(600, 400, 300);

    bool showLines = false;

    SetWindowOpacity(0.1f);
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Drag player 0
        //if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) Moveplayer(GetMousePosition().x, GetMousePosition().y);
        Vector2 input = Vector2Zero();
        float delta = GetFrameTime();

        if (IsKeyDown(KEY_W)) input.y -= moveSpeed * delta;
        if (IsKeyDown(KEY_S)) input.y += moveSpeed * delta;
        if (IsKeyDown(KEY_A)) input.x -= moveSpeed * delta;
        if (IsKeyDown(KEY_D)) input.x += moveSpeed * delta;

        Vector2 newPos = Vector2Add(player.position, input);

        Moveplayer(newPos.x, newPos.y);

        // Toggle debug info
        if (IsKeyPressed(KEY_F1)) showLines = !showLines;

        bool dirtyplayer = false;
        if (Updateplayer(boxes, boxCount)) dirtyplayer = true;

        // Update the player mask
        if (dirtyplayer)
        {
            // Build up the player mask
            BeginTextureMode(playerMask);

                ClearBackground(BLACK);

                // Force the blend mode to only set the alpha of the destination
                rlSetBlendFactors(RLGL_SRC_ALPHA, RLGL_SRC_ALPHA, RLGL_MIN);
                rlSetBlendMode(BLEND_CUSTOM);

                // Merge in all the player masks
                DrawTextureRec(player.mask.texture, (Rectangle){ 0, 0, (float)GetScreenWidth(), -(float)GetScreenHeight() }, Vector2Zero(), WHITE);

                rlDrawRenderBatchActive();

                // Go back to normal blend
                rlSetBlendMode(BLEND_ALPHA);
            EndTextureMode();
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(BLACK);
            cam.offset = Vector2Subtract(screenCentre(), player.position);
            Vector2 modPos = (Vector2){(float)((int)cam.offset.x % 64), (float)((int)cam.offset.y % 64)};
            DrawTextureRec(backgroundTexture, (Rectangle){ 64, 64, (float)GetScreenWidth()-64, (float)GetScreenHeight()-64 }, modPos, WHITE);

            BeginMode2D(cam);

                // Draw the tile background
                //Vector2 snappedPos = (Vector2){round(-cam.offset.x / 64) * 64, round(-cam.offset.y / 64) * 64};
                //Vector2 modPos = (Vector2){mod(-cam.offset.x, 64), mod(-cam.offset.y, 64)};

                //DrawTextureRec(backgroundTexture, (Rectangle){ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }, snappedPos, WHITE);

                // Overlay the shadows from all the players
                DrawTextureRec(player.mask.texture, (Rectangle){ 0, 0, (float)GetScreenWidth(), -(float)GetScreenHeight() }, Vector2Zero(), ColorAlpha(WHITE, showLines? 0.75f : 1.0f));

                if (showLines)
                {
                    for (int s = 0; s < player.shadowCount; s++)
                    {
                        DrawTriangleFan(player.shadows[s].vertices, 4, DARKPURPLE);
                    }

                    for (int b = 0; b < boxCount; b++)
                    {
                        if (CheckCollisionRecs(boxes[b],player.bounds)) DrawRectangleRec(boxes[b], PURPLE);

                        DrawRectangleLines((int)boxes[b].x, (int)boxes[b].y, (int)boxes[b].width, (int)boxes[b].height, DARKBLUE);
                    }
                }

            EndMode2D();

            DrawCircle(screenCentre().x, screenCentre().y, 10, YELLOW);

            DrawFPS(screenWidth - 80, 10);
            DrawText("(F1) to toggle Shadow Volumes", 10, 20, 10, GREEN);
            DrawText("(WASD) to move player", 10, 10, 10, GREEN);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(backgroundTexture);
    UnloadRenderTexture(playerMask);
    UnloadRenderTexture(player.mask);

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
