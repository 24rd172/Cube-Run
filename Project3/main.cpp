#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

// 画面解像度の設定
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// DirectX 11 関連のグローバル変数
ID3D11Device* g_pDevice = nullptr;
ID3D11DeviceContext* g_pContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

ID3D11Texture2D* g_pPixelTexture = nullptr;
std::vector<uint32_t>   g_PixelBuffer(SCREEN_WIDTH* SCREEN_HEIGHT);

// --- 色定義 (ABGR形式) ---
const uint32_t COLOR_BG_GRAY = 0xFF222222; // 背景の暗いグレー
const uint32_t COLOR_BLUE = 0xFFFF0000; // 青い地面 (B=FF)
const uint32_t COLOR_RED = 0xFF0000FF; // 赤い地面 (R=FF)
const uint32_t COLOR_GREEN = 0xFF00FF00; // 緑の地面 (G=FF)
const uint32_t COLOR_WHITE = 0xFFFFFFFF; // スコア・文字用の白

// ゲーム状態
bool g_IsGameOver = false;
int g_Score = 0;
int g_HighScore = 0;
float g_SubScore = 0.0f;

// プレイヤー情報
const int PLAYER_X = 200;
int g_PlayerY = 360;
int g_TargetY = 360; // プレイヤーが目指す目標Y座標
const int PLAYER_SIZE = 40;
uint32_t g_PlayerColor = COLOR_BLUE; // 初期色

// 正確な色変更検知のための、前フレームのキー状態保持用変数
bool g_PrevKeyState1 = false;
bool g_PrevKeyState2 = false;
bool g_PrevKeyState3 = false;

// 色維持コンボ用変数
int g_ComboCount = 0;                // 現在のコンボ数
float g_ColorMaintainDistance = 0.0f;// 現在の色を維持して進んだ距離
uint32_t g_PrevPlayerColor = COLOR_BLUE; // 前フレームのプレイヤーの色

// プレイヤーの張り付き状態
enum PlayerState { STATE_BOTTOM, STATE_TOP };
PlayerState g_PlayerState = STATE_BOTTOM;

// --- オートスクロール・動的ステージ管理 ---
float g_ScrollX = 0.0f;
const float SCROLL_SPEED = 4.0f;

// 上下の地面をそれぞれ構造体ベクターで独立管理
struct Block {
    uint32_t color;
    int width;
    int startX; // ワールド座標での開始位置
};
std::vector<Block> g_StageBlocksBottom;
std::vector<Block> g_StageBlocksTop;

int g_TotalBlockCountBottom = 0; // 下の通算生成ブロック数
int g_TotalBlockCountTop = 0; // 上の通算生成ブロック数

// --- 空中アイテム（カラー・クリスタル）の定義 ---
struct Item {
    float worldX;   // ワールド座標X
    float worldY;   // 画面座標Y（空中）
    uint32_t color; // クリスタルの色
    bool active;    // まだ獲得されていないか
    bool processed; // すでに当たり判定の合否を出したか（多段ヒット防止）
    int size;       // アイテムの大きさ
};
std::vector<Item> g_Items;

// --- 3x5 ドットのピクセルフォントデータ (0〜9) ---
const int NUMBER_FONTS[10][5][3] = {
    {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}, // 0
    {{0,1,0},{0,1,0},{0,1,0},{0,1,0},{0,1,0}}, // 1
    {{1,1,1},{0,0,1},{1,1,1},{1,0,0},{1,1,1}}, // 2
    {{1,1,1},{0,0,1},{1,1,1},{0,0,1},{1,1,1}}, // 3
    {{1,0,1},{1,0,1},{1,1,1},{0,0,1},{0,0,1}}, // 4
    {{1,1,1},{1,0,0},{1,1,1},{0,0,1},{1,1,1}}, // 5
    {{1,1,1},{1,0,0},{1,1,1},{1,0,1},{1,1,1}}, // 6
    {{1,1,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1}}, // 7
    {{1,1,1},{1,0,1},{1,1,1},{1,0,1},{1,1,1}}, // 8
    {{1,1,1},{1,0,1},{1,1,1},{0,0,1},{1,1,1}}  // 9
};

// --- 3x5 ドットのピクセルフォントデータ (A〜Z) ---
const int ALPHA_FONTS[26][5][3] = {
    {{1,1,1},{1,0,1},{1,1,1},{1,0,1},{1,0,1}}, // A
    {{1,1,0},{1,0,1},{1,1,0},{1,0,1},{1,1,0}}, // B
    {{1,1,1},{1,0,0},{1,0,0},{1,0,0},{1,1,1}}, // C
    {{1,1,0},{1,0,1},{1,0,1},{1,0,1},{1,1,0}}, // D
    {{1,1,1},{1,0,0},{1,1,1},{1,0,0},{1,1,1}}, // E
    {{1,1,1},{1,0,0},{1,1,1},{1,0,0},{1,0,0}}, // F
    {{1,1,1},{1,0,0},{1,0,1},{1,0,1},{1,1,1}}, // G
    {{1,0,1},{1,0,1},{1,1,1},{1,0,1},{1,0,1}}, // H
    {{1,1,1},{0,1,0},{0,1,0},{0,1,0},{1,1,1}}, // I
    {{0,0,1},{0,0,1},{0,0,1},{1,0,1},{1,1,1}}, // J
    {{1,0,1},{1,0,1},{1,1,0},{1,0,1},{1,0,1}}, // K
    {{1,0,0},{1,0,0},{1,0,0},{1,0,0},{1,1,1}}, // L
    {{1,0,1},{1,1,1},{1,0,1},{1,0,1},{1,0,1}}, // M
    {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,0,1}}, // N
    {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}, // O
    {{1,1,1},{1,0,1},{1,1,1},{1,0,0},{1,0,0}}, // P
    {{1,1,1},{1,0,1},{1,0,1},{1,1,1},{0,0,1}}, // Q
    {{1,1,1},{1,0,1},{1,1,1},{1,1,0},{1,0,1}}, // R
    {{1,1,1},{1,0,0},{1,1,1},{0,0,1},{1,1,1}}, // S
    {{1,1,1},{0,1,0},{0,1,0},{0,1,0},{0,1,0}}, // T
    {{1,0,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}, // U
    {{1,0,1},{1,0,1},{1,0,1},{1,0,1},{0,1,0}}, // V
    {{1,0,1},{1,0,1},{1,0,1},{1,1,1},{1,0,1}}, // W
    {{1,0,1},{1,0,1},{0,1,0},{1,0,1},{1,0,1}}, // X
    {{1,0,1},{1,0,1},{1,1,1},{0,1,0},{0,1,0}}, // Y
    {{1,1,1},{0,0,1},{0,1,0},{1,0,0},{1,1,1}}  // Z
};

int GetCurrentBlockWidth(int blockCount) {
    int baseWidth = 250;
    int reduction = (blockCount / 5) * 30;
    int currentWidth = baseWidth - reduction;
    if (currentWidth < 60) currentWidth = 60;
    return currentWidth;
}

// 上下個別にステージを生成・破棄する関数
void UpdateStageBlocks() {
    if (g_StageBlocksBottom.empty()) {
        int w = GetCurrentBlockWidth(g_TotalBlockCountBottom);
        g_StageBlocksBottom.push_back({ COLOR_BLUE, w, 0 });
        g_TotalBlockCountBottom++;
    }
    while (g_StageBlocksBottom.back().startX + g_StageBlocksBottom.back().width < static_cast<int>(g_ScrollX) + SCREEN_WIDTH) {
        int w = GetCurrentBlockWidth(g_TotalBlockCountBottom);
        int nextStartX = g_StageBlocksBottom.back().startX + g_StageBlocksBottom.back().width;
        uint32_t nextColor = COLOR_BLUE;
        if (g_TotalBlockCountBottom >= 2) {
            uint32_t colors[] = { COLOR_BLUE, COLOR_RED, COLOR_GREEN };
            nextColor = colors[rand() % 3];
            if (g_StageBlocksBottom.back().color == nextColor && (rand() % 2 == 0)) {
                nextColor = colors[rand() % 3];
            }
        }
        g_StageBlocksBottom.push_back({ nextColor, w, nextStartX });
        g_TotalBlockCountBottom++;

        // 下の地面が生成されるタイミングで、35%の確率で空中にカラー・クリスタルを生成
        if (g_TotalBlockCountBottom > 2 && (rand() % 100 < 35)) {
            Item item;
            item.worldX = static_cast<float>(nextStartX + w / 2 - 10); // ブロックの中央付近
            item.worldY = static_cast<float>(150 + (rand() % 140));    // 空中（Y:150〜290）のランダムな高さ
            uint32_t colors[] = { COLOR_RED, COLOR_GREEN, COLOR_BLUE };
            item.color = colors[rand() % 3];
            item.active = true;
            item.processed = false;
            item.size = 24; // 視認しやすいサイズ
            g_Items.push_back(item);
        }
    }
    while (!g_StageBlocksBottom.empty() && g_StageBlocksBottom.front().startX + g_StageBlocksBottom.front().width < static_cast<int>(g_ScrollX)) {
        g_StageBlocksBottom.erase(g_StageBlocksBottom.begin());
    }

    if (g_StageBlocksTop.empty()) {
        int w = GetCurrentBlockWidth(g_TotalBlockCountTop) / 2;
        g_StageBlocksTop.push_back({ COLOR_BLUE, w, 0 });
        g_TotalBlockCountTop++;
    }
    while (g_StageBlocksTop.back().startX + g_StageBlocksTop.back().width < static_cast<int>(g_ScrollX) + SCREEN_WIDTH) {
        int w = GetCurrentBlockWidth(g_TotalBlockCountTop);
        int nextStartX = g_StageBlocksTop.back().startX + g_StageBlocksTop.back().width;
        uint32_t nextColor = COLOR_BLUE;
        if (g_TotalBlockCountTop >= 2) {
            uint32_t colors[] = { COLOR_BLUE, COLOR_RED, COLOR_GREEN };
            nextColor = colors[rand() % 3];
            if (g_StageBlocksTop.back().color == nextColor && (rand() % 2 == 0)) {
                nextColor = colors[rand() % 3];
            }
        }
        g_StageBlocksTop.push_back({ nextColor, w, nextStartX });
        g_TotalBlockCountTop++;
    }
    while (!g_StageBlocksTop.empty() && g_StageBlocksTop.front().startX + g_StageBlocksTop.front().width < static_cast<int>(g_ScrollX)) {
        g_StageBlocksTop.erase(g_StageBlocksTop.begin());
    }

    // 画面の左端から完全に見えなくなった古いアイテムを配列から削除
    while (!g_Items.empty() && g_Items.front().worldX + g_Items.front().size < g_ScrollX) {
        g_Items.erase(g_Items.begin());
    }
}

uint32_t GetBottomGroundColorAt(int worldX) {
    if (worldX < 0) return COLOR_BLUE;
    for (const auto& block : g_StageBlocksBottom) {
        if (worldX >= block.startX && worldX < block.startX + block.width) {
            return block.color;
        }
    }
    return COLOR_BLUE;
}

uint32_t GetTopGroundColorAt(int worldX) {
    if (worldX < 0) return COLOR_BLUE;
    for (const auto& block : g_StageBlocksTop) {
        if (worldX >= block.startX && worldX < block.startX + block.width) {
            return block.color;
        }
    }
    return COLOR_BLUE;
}

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
                                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                                    g_PixelBuffer[py * SCREEN_WIDTH + px] = color;
                                }
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
                                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                                    g_PixelBuffer[py * SCREEN_WIDTH + px] = color;
                                }
                            }
                        }
                    }
                }
            }
        }
        currentX += (3 + 1) * scale;
    }
}

bool InitDirectX(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scDesc = {};
    scDesc.BufferCount = 1;
    scDesc.BufferDesc.Width = SCREEN_WIDTH;
    scDesc.BufferDesc.Height = SCREEN_HEIGHT;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferDesc.RefreshRate.Numerator = 60;
    scDesc.BufferDesc.RefreshRate.Denominator = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.OutputWindow = hwnd;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &scDesc, &g_pSwapChain, &g_pDevice, &featureLevel, &g_pContext
    );
    if (FAILED(hr)) return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = SCREEN_WIDTH;
    texDesc.Height = SCREEN_HEIGHT;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DYNAMIC;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = g_pDevice->CreateTexture2D(&texDesc, nullptr, &g_pPixelTexture);
    if (FAILED(hr)) return false;

    D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, 1.0f };
    g_pContext->RSSetViewports(1, &vp);

    return true;
}

void CleanupDirectX() {
    if (g_pPixelTexture) g_pPixelTexture->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pContext) g_pContext->Release();
    if (g_pDevice) g_pDevice->Release();
}

// 毎フレームのゲーム計算
void UpdateGame() {
    if (g_IsGameOver) {
        if (GetAsyncKeyState('R')) {
            g_ScrollX = 0.0f;
            g_Score = 0;
            g_SubScore = 0.0f;
            g_ComboCount = 0;
            g_ColorMaintainDistance = 0.0f;
            g_PlayerColor = COLOR_BLUE;
            g_PrevPlayerColor = COLOR_BLUE;
            g_PlayerState = STATE_BOTTOM;
            g_PlayerY = 360;
            g_TargetY = 360;
            g_PrevKeyState1 = false;
            g_PrevKeyState2 = false;
            g_PrevKeyState3 = false;
            g_StageBlocksBottom.clear();
            g_StageBlocksTop.clear();
            g_Items.clear();
            g_TotalBlockCountBottom = 0;
            g_TotalBlockCountTop = 0;
            UpdateStageBlocks();
            g_IsGameOver = false;
        }
        return;
    }

    g_ScrollX += SCROLL_SPEED;
    UpdateStageBlocks();

    // キー入力検知
    bool currentKey1 = (GetAsyncKeyState('1') & 0x8000) != 0;
    bool currentKey2 = (GetAsyncKeyState('2') & 0x8000) != 0;
    bool currentKey3 = (GetAsyncKeyState('3') & 0x8000) != 0;

    if (currentKey1 && !g_PrevKeyState1) g_PlayerColor = COLOR_RED;
    if (currentKey2 && !g_PrevKeyState2) g_PlayerColor = COLOR_GREEN;
    if (currentKey3 && !g_PrevKeyState3) g_PlayerColor = COLOR_BLUE;

    g_PrevKeyState1 = currentKey1;
    g_PrevKeyState2 = currentKey2;
    g_PrevKeyState3 = currentKey3;

    // 色維持コンボの判定ロジック
    if (g_PlayerColor != g_PrevPlayerColor) {
        g_ComboCount = 0;
        g_ColorMaintainDistance = 0.0f;
        g_PrevPlayerColor = g_PlayerColor;
    }
    else {
        g_ColorMaintainDistance += SCROLL_SPEED;
        if (g_ColorMaintainDistance >= 120.0f) {
            g_ComboCount++;
            g_ColorMaintainDistance -= 120.0f;
        }
    }

    // コンボ倍率を適用したスコア加算
    float baseScoreGain = SCROLL_SPEED / 10.0f;
    float comboMultiplier = 1.0f + (g_ComboCount * 0.5f);
    g_SubScore += baseScoreGain * comboMultiplier;

    if (g_SubScore >= 1.0f) {
        int gain = static_cast<int>(g_SubScore);
        g_Score += gain;
        g_SubScore -= gain;
    }

    if (g_Score > g_HighScore) {
        g_HighScore = g_Score;
    }

    // 上下入力は「目標位置（g_TargetY）」の指定のみを行う
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        g_TargetY = 80;
    }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        g_TargetY = 400 - PLAYER_SIZE;
    }

    // プレイヤーのY座標を目標に向けてスムーズに移動させる
    bool isMoving = false;
    const int MOVE_SPEED = 24;
    if (g_PlayerY > g_TargetY) {
        g_PlayerY -= MOVE_SPEED;
        if (g_PlayerY < g_TargetY) g_PlayerY = g_TargetY;
        isMoving = true;
    }
    else if (g_PlayerY < g_TargetY) {
        g_PlayerY += MOVE_SPEED;
        if (g_PlayerY > g_TargetY) g_PlayerY = g_TargetY;
        isMoving = true;
    }

    // 移動先の目標に応じて状態を確定
    if (g_TargetY == 80) {
        g_PlayerState = STATE_TOP;
    }
    else {
        g_PlayerState = STATE_BOTTOM;
    }

    // --- 空中アイテムとの当たり判定処理 ---
    int pLeft = PLAYER_X;
    int pRight = PLAYER_X + PLAYER_SIZE;
    int pTop = g_PlayerY;
    int pBottom = g_PlayerY + PLAYER_SIZE;

    for (auto& item : g_Items) {
        if (!item.active || item.processed) continue;

        int iLeft = static_cast<int>(item.worldX - g_ScrollX);
        int iRight = iLeft + item.size;
        int iTop = static_cast<int>(item.worldY);
        int iBottom = iTop + item.size;

        // 矩形交差判定 (AABB)
        if (pLeft < iRight && pRight > iLeft && pTop < iBottom && pBottom > iTop) {
            item.processed = true;

            if (g_PlayerColor == item.color) {
                // 【効果】色変色一致：クリスタル獲得成功！
                item.active = false;
                g_Score += 150;      // ボーナススコア加算
                g_ComboCount += 5;   // コンボ数を一気に＋5
            }
            else {
                // 【ペナルティ】色が違うのに接触：コンボ強制終了（リセット）
                g_ComboCount = 0;
                g_ColorMaintainDistance = 0.0f;
            }
        }
    }

    // 着地・死亡判定のチェック
    bool matchFound = false;

    if (isMoving) {
        // 移動中（空中）は地面に触れていないため安全
        matchFound = true;
    }
    else {
        // 完全に着地している時のみ、従来通りの色合わせ判定
        for (int dx = 0; dx < PLAYER_SIZE; ++dx) {
            int screenX = PLAYER_X + dx;
            int worldX = screenX + (int)g_ScrollX;

            uint32_t groundColor = 0;
            if (g_PlayerState == STATE_BOTTOM) {
                groundColor = GetBottomGroundColorAt(worldX);
            }
            else {
                groundColor = GetTopGroundColorAt(worldX);
            }

            if (g_PlayerColor == groundColor) {
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
    std::fill(g_PixelBuffer.begin(), g_PixelBuffer.end(), COLOR_BG_GRAY);

    // オートスクロールする上下の地面の描画
    for (int x = 0; x < SCREEN_WIDTH; ++x) {
        int worldX = x + (int)g_ScrollX;
        uint32_t bottomColor = GetBottomGroundColorAt(worldX);
        uint32_t topColor = GetTopGroundColorAt(worldX);

        // 天井 (0 〜 80px)
        for (int y = 0; y < 80; ++y) {
            g_PixelBuffer[y * SCREEN_WIDTH + x] = topColor;
        }
        // 床 (400 〜 480px)
        for (int y = 400; y < SCREEN_HEIGHT; ++y) {
            g_PixelBuffer[y * SCREEN_WIDTH + x] = bottomColor;
        }
    }

    // --- 空中アイテム（カラー・クリスタル）の描画 ---
    for (const auto& item : g_Items) {
        if (!item.active) continue;

        int screenX = static_cast<int>(item.worldX - g_ScrollX);
        if (screenX + item.size < 0 || screenX >= SCREEN_WIDTH) continue;

        for (int dy = 0; dy < item.size; ++dy) {
            for (int dx = 0; dx < item.size; ++dx) {
                int tx = screenX + dx;
                int ty = static_cast<int>(item.worldY) + dy;

                if (tx >= 0 && tx < SCREEN_WIDTH && ty >= 0 && ty < SCREEN_HEIGHT) {
                    int cx = item.size / 2;
                    int cy = item.size / 2;
                    if (std::abs(dx - cx) + std::abs(dy - cy) <= item.size / 2) {
                        g_PixelBuffer[ty * SCREEN_WIDTH + tx] = item.color;
                    }
                }
            }
        }
    }

    // プレイヤーの矩形を描画
    for (int dy = 0; dy < PLAYER_SIZE; ++dy) {
        for (int dx = 0; dx < PLAYER_SIZE; ++dx) {
            int tx = PLAYER_X + dx;
            int ty = g_PlayerY + dy;

            if (tx >= 0 && tx < SCREEN_WIDTH && ty >= 0 && ty < SCREEN_HEIGHT) {
                if (g_IsGameOver) {
                    g_PixelBuffer[ty * SCREEN_WIDTH + tx] = 0xFF555555;
                }
                else {
                    g_PixelBuffer[ty * SCREEN_WIDTH + tx] = g_PlayerColor;
                }
            }
        }
    }

    // --- UI：スコア・ハイスコアの表示 ---
    std::string scoreStr = std::to_string(g_Score);
    if (scoreStr.length() < 6) scoreStr = std::string(6 - scoreStr.length(), '0') + scoreStr;

    std::string hiScoreStr = std::to_string(g_HighScore);
    if (hiScoreStr.length() < 6) hiScoreStr = std::string(6 - hiScoreStr.length(), '0') + hiScoreStr;

    DrawTextStr(20, 90, "HI " + hiScoreStr, COLOR_WHITE, 3);
    DrawTextStr(20, 115, "1P " + scoreStr, COLOR_WHITE, 3);

    // --- UI：コンボ数のリアルタイム表示 ---
    if (g_ComboCount > 0 && !g_IsGameOver) {
        std::string comboStr = "COMBO " + std::to_string(g_ComboCount);

        uint32_t comboColor = COLOR_WHITE;
        if (g_ComboCount >= 15) {
            comboColor = 0xFF00FFFF;
        }
        else if (g_ComboCount >= 10) {
            comboColor = COLOR_RED;
        }
        else if (g_ComboCount >= 5) {
            comboColor = COLOR_GREEN;
        }

        DrawTextStr(20, 140, comboStr, comboColor, 3);
    }

    // --- ゲームオーバー演出の表示 ---
    if (g_IsGameOver) {
        DrawTextStr(212, 180, "GAME OVER", COLOR_RED, 6);
        DrawTextStr(224, 225, "PRESS R TO RETRY", COLOR_WHITE, 3);
    }

    // GPUへの転送と画面表示
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = g_pContext->Map(g_pPixelTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
        uint32_t* pDest = (uint32_t*)mappedResource.pData;
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            memcpy(pDest + (y * (mappedResource.RowPitch / 4)), &g_PixelBuffer[y * SCREEN_WIDTH], SCREEN_WIDTH * 4);
        }
        g_pContext->Unmap(g_pPixelTexture, 0);
    }

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_pContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

    ID3D11Resource* pResDest = nullptr;
    ID3D11Resource* pResSrc = nullptr;
    g_pRenderTargetView->GetResource(&pResDest);
    g_pPixelTexture->QueryInterface(__uuidof(ID3D11Resource), (void**)&pResSrc);

    if (pResDest && pResSrc) g_pContext->CopyResource(pResDest, pResSrc);
    if (pResDest) pResDest->Release();
    if (pResSrc) pResSrc->Release();

    g_pSwapChain->Present(0, 0);
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

    if (!InitDirectX(hwnd)) {
        MessageBox(hwnd, L"DirectX 11 の初期化に失敗しました。", L"エラー", MB_OK);
        CleanupDirectX();
        return 0;
    }

    LARGE_INTEGER frequency;
    LARGE_INTEGER prevTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevTime);
    const double TARGET_FRAME_TIME = 1.0 / 60.0;

    UpdateStageBlocks();

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

    CleanupDirectX();
    return 0;
}