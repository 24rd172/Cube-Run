#include "Player.h"
#include "Graphics.h"
#include <windows.h>
#include <cmath>

extern Graphics g_Graphics;

Player::Player() {
    Reset();
}

void Player::Reset() {
    m_Y = 360;
    m_TargetY = 360;
    m_Color = COLOR_BLUE;
    m_PrevColor = COLOR_BLUE;
    m_PrevKey1 = false;
    m_PrevKey2 = false;
    m_PrevKey3 = false;
    m_ComboCount = 0;
    m_ColorMaintainDistance = 0.0f;
    m_State = STATE_BOTTOM;
}

void Player::Update(float scrollSpeed) {
    // キー入力検知
    bool currentKey1 = (GetAsyncKeyState('1') & 0x8000) != 0;
    bool currentKey2 = (GetAsyncKeyState('2') & 0x8000) != 0;
    bool currentKey3 = (GetAsyncKeyState('3') & 0x8000) != 0;

    if (currentKey1 && !m_PrevKey1) m_Color = COLOR_RED;
    if (currentKey2 && !m_PrevKey2) m_Color = COLOR_GREEN;
    if (currentKey3 && !m_PrevKey3) m_Color = COLOR_BLUE;

    m_PrevKey1 = currentKey1;
    m_PrevKey2 = currentKey2;
    m_PrevKey3 = currentKey3;

    // 色維持コンボの判定
    if (m_Color != m_PrevColor) {
        m_ComboCount = 0;
        m_ColorMaintainDistance = 0.0f;
        m_PrevColor = m_Color;
    }
    else {
        m_ColorMaintainDistance += scrollSpeed;
        if (m_ColorMaintainDistance >= 120.0f) {
            m_ComboCount++;
            m_ColorMaintainDistance -= 120.0f;
        }
    }

    // 上下入力
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        m_TargetY = 80;
    }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        m_TargetY = 400 - PLAYER_SIZE;
    }

    // スムーズな移動処理
    if (m_Y > m_TargetY) {
        m_Y -= MOVE_SPEED;
        if (m_Y < m_TargetY) m_Y = m_TargetY;
    }
    else if (m_Y < m_TargetY) {
        m_Y += MOVE_SPEED;
        if (m_Y > m_TargetY) m_Y = m_TargetY;
    }

    m_State = (m_TargetY == 80) ? STATE_TOP : STATE_BOTTOM;
}

void Player::Draw(bool isGameOver) const {
    uint32_t drawColor = isGameOver ? 0xFF555555 : m_Color;

    for (int dy = 0; dy < PLAYER_SIZE; ++dy) {
        for (int dx = 0; dx < PLAYER_SIZE; ++dx) {
            int tx = PLAYER_X + dx;
            int ty = m_Y + dy;
            g_Graphics.SetPixel(tx, ty, drawColor);
        }
    }
}

void Player::AddScoreBonus() {
    m_ComboCount += 5;
}

void Player::ResetCombo() {
    m_ComboCount = 0;
    m_ColorMaintainDistance = 0.0f;
}