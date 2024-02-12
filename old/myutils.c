#include "raylib.h"
#include "raymath.h"
#include "myutils.h"

Vector2 screenCentre()
{
    Vector2 screenSize = (Vector2){(float)GetScreenWidth(), (float)GetScreenHeight()};
    return Vector2Scale(screenSize, 0.5f);
}
