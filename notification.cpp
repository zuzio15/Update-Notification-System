#include "raylib.h"
#include "notification.h"
#include <thread>
#include <atomic>

// Prevents multiple simultaneous notification windows
static std::atomic<bool> g_active(false);


// Renders a small bottom-right notification window using Raylib.

static void RaylibWindow(std::string platformName)
{
    const int windowWidth = 320;
    const int windowHeight = 110;

    // Create a borderless, topmost hidden window first
    SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
    InitWindow(windowWidth, windowHeight, "Notification");

    // Determine screen resolution
    int screenWidth = GetMonitorWidth(0);
    int screenHeight = GetMonitorHeight(0);

    if (screenWidth <= 0) screenWidth = 1920;
    if (screenHeight <= 0) screenHeight = 1080;

    // Position window in bottom-right corner
    int posX = screenWidth - windowWidth - 20;
    int posY = screenHeight - windowHeight - 60;

    SetWindowPosition(posX, posY);
    ClearWindowState(FLAG_WINDOW_HIDDEN);

    SetTargetFPS(60);

    // Notification lifetime
    float timer = 4.0f;
    std::string displayText = "Game is updating - " + platformName;

    while (!WindowShouldClose() && timer > 0.0f)
    {
        timer -= GetFrameTime();

        BeginDrawing();
        ClearBackground(GetColor(0x1e1e2eff));

        // Border frame
        DrawRectangleLinesEx(Rectangle{0, 0, (float)windowWidth, (float)windowHeight}, 2, SKYBLUE);

        DrawText("NOTIFICATION", 20, 20, 12, LIGHTGRAY);
        DrawText(displayText.c_str(), 20, 45, 16, RAYWHITE);

        // Progress bar (remaining time)
        float progress = timer / 4.0f;
        DrawRectangle(20, 85, (int)((windowWidth - 40) * progress), 4, SKYBLUE);

        EndDrawing();
    }

    CloseWindow();

    // Release lock so next notification can appear
    g_active = false;
}


// triggers notification if none is currently active.

void TriggerNotification(const std::string& platformName)
{
    // Ignore if a notification is already being displayed
    if (g_active)
        return;

    g_active = true;

    // Run UI in detached thread to avoid blocking main logic
    std::thread(RaylibWindow, platformName).detach();
}