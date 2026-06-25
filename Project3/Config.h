#pragma once
#include <cstdint>

// 画面解像度の設定
inline constexpr int SCREEN_WIDTH = 640;
inline constexpr int SCREEN_HEIGHT = 480;

// 色定義 (ABGR形式)
inline constexpr uint32_t COLOR_BG_GRAY = 0xFF222222; // 背景の暗いグレー
inline constexpr uint32_t COLOR_BLUE = 0xFFFF0000; // 青い地面 (B=FF)
inline constexpr uint32_t COLOR_RED = 0xFF0000FF; // 赤い地面 (R=FF)
inline constexpr uint32_t COLOR_GREEN = 0xFF00FF00; // 緑の地面 (G=FF)
inline constexpr uint32_t COLOR_WHITE = 0xFFFFFFFF; // スコア・文字用の白

// ゲームバランスに関する定数
inline constexpr int   PLAYER_X = 200;
inline constexpr int   PLAYER_SIZE = 40;
inline constexpr float SCROLL_SPEED = 4.0f;
inline constexpr int   MOVE_SPEED = 24;