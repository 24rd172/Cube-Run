#include "Stage.h"
#include "Graphics.h"
#include <cstdlib>
#include <cmath>

extern Graphics g_Graphics;

Stage::Stage() {
    Reset();
}

void Stage::Reset() {
    m_ScrollX = 0.0f;
    m_StageBlocksBottom.clear();
    m_StageBlocksTop.clear();
    m_Items.clear();
    m_TotalBlockCountBottom = 0;
    m_TotalBlockCountTop = 0;
    Update(); // 初回ブロック生成
}

int Stage::GetCurrentBlockWidth(int blockCount) const {
    int baseWidth = 250;
    int reduction = (blockCount / 5) * 30;
    int currentWidth = baseWidth - reduction;
    if (currentWidth < 60) currentWidth = 60;
    return currentWidth;
}

void Stage::Update() {
    m_ScrollX += SCROLL_SPEED;

    // 下の地面の生成・破棄
    if (m_StageBlocksBottom.empty()) {
        int w = GetCurrentBlockWidth(m_TotalBlockCountBottom);
        m_StageBlocksBottom.push_back({ COLOR_BLUE, w, 0 });
        m_TotalBlockCountBottom++;
    }
    while (m_StageBlocksBottom.back().startX + m_StageBlocksBottom.back().width < static_cast<int>(m_ScrollX) + SCREEN_WIDTH) {
        int w = GetCurrentBlockWidth(m_TotalBlockCountBottom);
        int nextStartX = m_StageBlocksBottom.back().startX + m_StageBlocksBottom.back().width;
        uint32_t nextColor = COLOR_BLUE;
        if (m_TotalBlockCountBottom >= 2) {
            uint32_t colors[] = { COLOR_BLUE, COLOR_RED, COLOR_GREEN };
            nextColor = colors[rand() % 3];
            if (m_StageBlocksBottom.back().color == nextColor && (rand() % 2 == 0)) {
                nextColor = colors[rand() % 3];
            }
        }
        m_StageBlocksBottom.push_back({ nextColor, w, nextStartX });
        m_TotalBlockCountBottom++;

        if (m_TotalBlockCountBottom > 2 && (rand() % 100 < 35)) {
            Item item;
            item.worldX = static_cast<float>(nextStartX + w / 2 - 10);
            item.worldY = static_cast<float>(150 + (rand() % 140));
            uint32_t colors[] = { COLOR_RED, COLOR_GREEN, COLOR_BLUE };
            item.color = colors[rand() % 3];
            item.active = true;
            item.processed = false;
            item.size = 24;
            m_Items.push_back(item);
        }
    }
    while (!m_StageBlocksBottom.empty() && m_StageBlocksBottom.front().startX + m_StageBlocksBottom.front().width < static_cast<int>(m_ScrollX)) {
        m_StageBlocksBottom.erase(m_StageBlocksBottom.begin());
    }

    // 上の地面の生成・破棄
    if (m_StageBlocksTop.empty()) {
        int w = GetCurrentBlockWidth(m_TotalBlockCountTop) / 2;
        m_StageBlocksTop.push_back({ COLOR_BLUE, w, 0 });
        m_TotalBlockCountTop++;
    }
    while (m_StageBlocksTop.back().startX + m_StageBlocksTop.back().width < static_cast<int>(m_ScrollX) + SCREEN_WIDTH) {
        int w = GetCurrentBlockWidth(m_TotalBlockCountTop);
        int nextStartX = m_StageBlocksTop.back().startX + m_StageBlocksTop.back().width;
        uint32_t nextColor = COLOR_BLUE;
        if (m_TotalBlockCountTop >= 2) {
            uint32_t colors[] = { COLOR_BLUE, COLOR_RED, COLOR_GREEN };
            nextColor = colors[rand() % 3];
            if (m_StageBlocksTop.back().color == nextColor && (rand() % 2 == 0)) {
                nextColor = colors[rand() % 3];
            }
        }
        m_StageBlocksTop.push_back({ nextColor, w, nextStartX });
        m_TotalBlockCountTop++;
    }
    while (!m_StageBlocksTop.empty() && m_StageBlocksTop.front().startX + m_StageBlocksTop.front().width < static_cast<int>(m_ScrollX)) {
        m_StageBlocksTop.erase(m_StageBlocksTop.begin());
    }

    while (!m_Items.empty() && m_Items.front().worldX + m_Items.front().size < m_ScrollX) {
        m_Items.erase(m_Items.begin());
    }
}

void Stage::Draw() const {
    // 地面の描画
    for (int x = 0; x < SCREEN_WIDTH; ++x) {
        int worldX = x + (int)m_ScrollX;
        uint32_t bottomColor = GetBottomGroundColorAt(worldX);
        uint32_t topColor = GetTopGroundColorAt(worldX);

        for (int y = 0; y < 80; ++y) {
            g_Graphics.SetPixel(x, y, topColor);
        }
        for (int y = 400; y < SCREEN_HEIGHT; ++y) {
            g_Graphics.SetPixel(x, y, bottomColor);
        }
    }

    // アイテムの描画
    for (const auto& item : m_Items) {
        if (!item.active) continue;

        int screenX = static_cast<int>(item.worldX - m_ScrollX);
        if (screenX + item.size < 0 || screenX >= SCREEN_WIDTH) continue;

        for (int dy = 0; dy < item.size; ++dy) {
            for (int dx = 0; dx < item.size; ++dx) {
                int tx = screenX + dx;
                int ty = static_cast<int>(item.worldY) + dy;

                int cx = item.size / 2;
                int cy = item.size / 2;
                if (std::abs(dx - cx) + std::abs(dy - cy) <= item.size / 2) {
                    g_Graphics.SetPixel(tx, ty, item.color);
                }
            }
        }
    }
}

uint32_t Stage::GetBottomGroundColorAt(int worldX) const {
    if (worldX < 0) return COLOR_BLUE;
    for (const auto& block : m_StageBlocksBottom) {
        if (worldX >= block.startX && worldX < block.startX + block.width) {
            return block.color;
        }
    }
    return COLOR_BLUE;
}

uint32_t Stage::GetTopGroundColorAt(int worldX) const {
    if (worldX < 0) return COLOR_BLUE;
    for (const auto& block : m_StageBlocksTop) {
        if (worldX >= block.startX && worldX < block.startX + block.width) {
            return block.color;
        }
    }
    return COLOR_BLUE;
}