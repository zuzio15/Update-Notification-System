#include <windows.h>

#include <sys/stat.h>
// Compatibility wrapper for stat (Windows toolchain handling)
extern "C" int stat64i32(const char* path, struct _stat64* buffer) {
    return _stat64(path, buffer);
}

#include <shellapi.h>
#include <thread>
#include <atomic>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <filesystem>
#include <sstream>

#include "notification.h"

#define ID_TRAY_ICON 1001
#define ID_MENU_EXIT  2001
#define WM_TRAYICON   (WM_USER + 1)

HINSTANCE g_hInstance = nullptr;
HWND g_hWnd = nullptr;
NOTIFYICONDATA g_nid = {};

std::atomic<bool> g_Running(true);
std::thread g_LogThread;

// Holds configuration for each monitored platform/log source.
struct PlatformConfig {
    std::string name;
    std::string logPath;
    std::string searchPhrase;
    std::streampos lastPos = 0;
    bool firstRun = true;
};

std::vector<PlatformConfig> g_Platforms;


//  Loads config file or creates default one if missing.

void LoadOrCreateConfig()
{
    const std::string configFilename = "config.txt";

    // Create default config if file does not exist - needs changing on every computer
    if (!std::filesystem::exists(configFilename))
    {
        std::ofstream outFile(configFilename);
        if (outFile.is_open())
        {
            outFile << "# Format: Name|LogPath|Keyword\n";
            outFile << "Steam|C:/Program Files (x86)/Steam/logs/content_log.txt|update started\n";
            outFile << "Epic Games|C:/Users/Ja/AppData/Local/EpicGamesLauncher/Saved/Logs/EpicGamesLauncher.log|Downloading chunk\n";
        }
    }

    // Parse config entries
    std::ifstream inFile(configFilename);
    if (inFile.is_open())
    {
        std::string line;
        while (std::getline(inFile, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::stringstream ss(line);
            std::string name, path, phrase;

            if (std::getline(ss, name, '|') &&
                std::getline(ss, path, '|') &&
                std::getline(ss, phrase, '|'))
            {
                g_Platforms.push_back({ name, path, phrase });
            }
        }
    }
}


//  Background thread: monitors log files for keyword matches.

void MonitorLogFiles()
{
    // Validate paths once at startup
    for (const auto& platform : g_Platforms)
    {
        if (!std::filesystem::exists(platform.logPath))
        {
            OutputDebugStringA(("Missing log path: " + platform.name + "\n").c_str());
        }
    }

    while (g_Running)
    {
        for (auto& platform : g_Platforms)
        {
            std::ifstream file(platform.logPath);

            if (file.is_open())
            {
                // On first run, skip existing log content
                if (platform.firstRun)
                {
                    file.seekg(0, std::ios::end);
                    platform.lastPos = file.tellg();
                    platform.firstRun = false;
                }
                else
                {

                    file.seekg(0, std::ios::end);
                    std::streampos currentSize = file.tellg();

                    if (currentSize < platform.lastPos)
                    {

                        platform.lastPos = 0;
                    }

                    file.seekg(platform.lastPos);
                }

                std::string line;

                // Scan new log lines for keyword
                while (std::getline(file, line))
                {
                    if (line.find(platform.searchPhrase) != std::string::npos)
                    {
                        TriggerNotification(platform.name);
                    }
                }

                file.clear();
                platform.lastPos = file.tellg();
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}


// Adds system tray icon.

void AddTrayIcon()
{
    ZeroMemory(&g_nid, sizeof(g_nid));

    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = ID_TRAY_ICON;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    lstrcpy(g_nid.szTip, TEXT("Game Update Monitor"));

    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

// Removes tray icon on exit.
void RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}


// Shows tray context menu.

void ShowMenu()
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, ID_MENU_EXIT, TEXT("EXIT"));

    SetForegroundWindow(g_hWnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hWnd, nullptr);
    DestroyMenu(menu);
}


// Window procedure handling tray events and shutdown.

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_MENU_EXIT)
            DestroyWindow(hwnd);
        break;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
            ShowMenu();
        break;

    case WM_DESTROY:
        // Stop monitoring thread and cleanup
        g_Running = false;

        if (g_LogThread.joinable())
            g_LogThread.join();

        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


//Registers hidden window class.

bool RegisterClassWin()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = TEXT("TrayApp");

    return RegisterClass(&wc);
}


//Creates hidden window for message handling.
bool CreateHiddenWindow()
{
    g_hWnd = CreateWindowEx(
        0, TEXT("TrayApp"), TEXT("Tray"), WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, g_hInstance, nullptr
    );

    return g_hWnd != nullptr;
}


// Application entry point.

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    g_hInstance = hInstance;

    LoadOrCreateConfig();

    if (!RegisterClassWin())
        return 0;

    if (!CreateHiddenWindow())
        return 0;

    AddTrayIcon();

    // Start background monitoring thread
    g_LogThread = std::thread(MonitorLogFiles);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}