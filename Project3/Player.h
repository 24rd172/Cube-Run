#pragma once
#include <cstdint>
#include "Config.h"

// プレイヤーの張り付き状態
enum PlayerState { STATE_BOTTOM, STATE_TOP };

class Player {
public:
    Player();

    void Reset();
    void Update(float scrollSpeed);
    void Draw(bool isGameOver) const;

    // ゲッター
    int GetY() const { return m_Y; }
    uint32_t GetColor() const { return m_Color; }
    PlayerState GetState() const { return m_State; }
    int GetComboCount() const { return m_ComboCount; }
    bool IsMoving() const { return m_Y != m_TargetY; }

    // 外部から状態を変化させる関数
    void AddScoreBonus(); // クリスタル獲得時のコンボ加算
    void ResetCombo();    // ミス時のコンボリセット

private:
    int m_Y = 360;
    int m_TargetY = 360;
    uint32_t m_Color = COLOR_BLUE;
    uint32_t m_PrevColor = COLOR_BLUE;

    // キーの押し下げ判定用
    bool m_PrevKey1 = false;
    bool m_PrevKey2 = false;
    bool m_PrevKey3 = false;

    int m_ComboCount = 0;
    float m_ColorMaintainDistance = 0.0f;

    PlayerState m_State = STATE_BOTTOM;
};