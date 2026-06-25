#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

#include "Config.h"
#include "FontData.h"
#include "Graphics.h"
#include "Player.h"
#include "Stage.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// グローバルマネージャー
Graphics g_Graphics;
Player   g_Player;
Stage    g_Stage;

// ゲーム状態
bool g_IsGameOver = false;
int g_Score = 0;
int g_HighScore = 0;
float g_SubScore = 0.0f;

void DrawTextStr(int startX, int startY, const std::string& str, uint32_t color, int scale) {
    int currentX = startX;
    for (char c : str) {
        if (c >= '0' && c <= '9') {
            int digit = c - '0';
            for (int fontY = 0; fontY < 5; ++fontY) {
                for (int fontX = 0; fontX < 3; ++fontX) {
                    if (NUMBER_FONTS[digit][fontY][fontX] == 1) {
                        for (int dy = 0; dy < scale; ++dy) {
                            for (int dx = 0; dx < scale; ++dx) {
                                int px = currentX + (fontX * scale) + dx;
                                int py = startY + (fontY * scale) + dy;
                                g_Graphics.SetPixel(px, py, color);
                            }
                        }
                    }
                }
            }
        }
        else if (c >= 'A' && c <= 'Z') {
            int index = c - 'A';
            for (int fontY = 0; fontY < 5; ++fontY) {
                for (int fontX = 0; fontX < 3; ++fontX) {
                    if (ALPHA_FONTS[index][fontY][fontX] == 1) {
                        for (int dy = 0; dy < scale; ++dy) {
                            for (int dx = 0; dx < scale; ++dx) {
                                int px = currentX + (fontX * scale) + dx;
                                int py = startY + (fontY * scale) + dy;
                                g_Graphics.SetPixel(px, py, color);
                            }
                        }
                    }
                }
            }
        }
        currentX += (3 + 1) * scale;
    }
}

// 毎フレームのゲーム計算
void UpdateGame() {
    if (g_IsGameOver) {
        if (GetAsyncKeyState('R')) {
            g_Player.Reset();
            g_Stage.Reset();
            g_Score = 0;
            g_SubScore = 0.0f;
            g_IsGameOver = false;
        }
        return;
    }

    // 1. 各オブジェクトを更新
    g_Stage.Update();
    g_Player.Update(SCROLL_SPEED);

    // 2. コンボ倍率を適用したスコア加算
    float baseScoreGain = SCROLL_SPEED / 10.0f;
    float comboMultiplier = 1.0f + (g_Player.GetComboCount() * 0.5f);
    g_SubScore += baseScoreGain * comboMultiplier;

    if (g_SubScore >= 1.0f) {
        int gain = static_cast<int>(g_SubScore);
        g_Score += gain;
        g_SubScore -= gain;
    }
    if (g_Score > g_HighScore) {
        g_HighScore = g_Score;
    }

    // 3. プレイヤーと空中アイテムの当たり判定
    int pLeft = PLAYER_X;
    int pRight = PLAYER_X + PLAYER_SIZE;
    int pTop = g_Player.GetY();
    int pBottom = g_Player.GetY() + PLAYER_SIZE;

    for (auto& item : g_Stage.GetItems()) {
        if (!item.active || item.processed) continue;

        int iLeft = static_cast<int>(item.worldX - g_Stage.GetScrollX());
        int iRight = iLeft + item.size;
        int iTop = static_cast<int>(item.worldY);
        int iBottom = iTop + item.size;

        if (pLeft < iRight && pRight > iLeft && pTop < iBottom && pBottom > iTop) {
            item.processed = true;

            if (g_Player.GetColor() == item.color) {
                item.active = false;
                g_Score += 150;
                g_Player.AddScoreBonus(); // コンボ+5
            }
            else {
                g_Player.ResetCombo();    // コンボ強制リセット
            }
        }
    }

    // 4. 着地・死亡判定のチェック
    bool matchFound = false;

    if (g_Player.IsMoving()) {
        matchFound = true; // 空中移動中は安全
    }
    else {
        // 着地状態の時だけ、その座標の地面の色をステージから取得してチェック
        for (int dx = 0; dx < PLAYER_SIZE; ++dx) {
            int screenX = PLAYER_X + dx;
            int worldX = screenX + (int)g_Stage.GetScrollX();

            uint32_t groundColor = 0;
            if (g_Player.GetState() == STATE_BOTTOM) {
                groundColor = g_Stage.GetBottomGroundColorAt(worldX);
            }
            else {
                groundColor = g_Stage.GetTopGroundColorAt(worldX);
            }

            if (g_Player.GetColor() == groundColor) {
                matchFound = true;
                break;
            }
        }
    }

    if (!matchFound) {
        g_IsGameOver = true;
    }
}

// 描画関数
void RenderGame() {
    g_Graphics.Clear(COLOR_BG_GRAY);

    // 各オブジェクトに描画を命令
    g_Stage.Draw();
    g_Player.Draw(g_IsGameOver);

    // UI：スコア等の表示
    std::string scoreStr = std::to_string(g_Score);
    if (scoreStr.length() < 6) scoreStr = std::string(6 - scoreStr.length(), '0') + scoreStr;

    std::string hiScoreStr = std::to_string(g_HighScore);
    if (hiScoreStr.length() < 6) hiScoreStr = std::string(6 - hiScoreStr.length(), '0') + hiScoreStr;

    DrawTextStr(20, 90, "HI " + hiScoreStr, COLOR_WHITE, 3);
    DrawTextStr(20, 115, "1P " + scoreStr, COLOR_WHITE, 3);

    // UI：コンボ表示
    int combo = g_Player.GetComboCount();
    if (combo > 0 && !g_IsGameOver) {
        std::string comboStr = "COMBO " + std::to_string(combo);
        uint32_t comboColor = COLOR_WHITE;
        if (combo >= 15) comboColor = 0xFF00FFFF;
        else if (combo >= 10) comboColor = COLOR_RED;
        else if (combo >= 5) comboColor = COLOR_GREEN;

        DrawTextStr(20, 140, comboStr, comboColor, 3);
    }

    if (g_IsGameOver) {
        DrawTextStr(212, 180, "GAME OVER", COLOR_RED, 6);
        DrawTextStr(224, 225, "PRESS R TO RETRY", COLOR_WHITE, 3);
    }

    g_Graphics.Present();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    srand(static_cast<unsigned int>(time(nullptr)));

    const wchar_t CLASS_NAME[] = L"PixelGameWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Pixel Camouflage Game",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;
    ShowWindow(hwnd, nCmdShow);

    if (!g_Graphics.Initialize(hwnd)) {
        MessageBox(hwnd, L"DirectX 11 の初期化に失敗しました。", L"エラー", MB_OK);
        return 0;
    }

    LARGE_INTEGER frequency;
    LARGE_INTEGER prevTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevTime);
    const double TARGET_FRAME_TIME = 1.0 / 60.0;

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            double elapsedTime = static_cast<double>(currentTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;

            if (elapsedTime < TARGET_FRAME_TIME) {
                double remainingTime = TARGET_FRAME_TIME - elapsedTime;
                if (remainingTime > 0.002) {
                    Sleep(static_cast<DWORD>(remainingTime * 1000) - 1);
                }
                continue;
            }
            prevTime = currentTime;

            UpdateGame();
            RenderGame();
        }
    }
    return 0;
}