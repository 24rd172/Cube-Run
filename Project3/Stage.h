#pragma once
#include <vector>
#include <cstdint>
#include "Config.h"

// 構造体定義
struct Block {
    uint32_t color;
    int width;
    int startX;
};

struct Item {
    float worldX;
    float worldY;
    uint32_t color;
    bool active;
    bool processed;
    int size;
};

class Stage {
public:
    Stage();

    void Reset();
    void Update();
    void Draw() const;

    // 特定のワールド座標における地面の色を返す（当たり判定用）
    uint32_t GetBottomGroundColorAt(int worldX) const;
    uint32_t GetTopGroundColorAt(int worldX) const;

    // main.cppの当たり判定ロジックで使用するゲッター
    float GetScrollX() const { return m_ScrollX; }
    std::vector<Item>& GetItems() { return m_Items; }

private:
    int GetCurrentBlockWidth(int blockCount) const;

    float m_ScrollX = 0.0f;
    std::vector<Block> m_StageBlocksBottom;
    std::vector<Block> m_StageBlocksTop;
    std::vector<Item> m_Items;

    int m_TotalBlockCountBottom = 0;
    int m_TotalBlockCountTop = 0;
};