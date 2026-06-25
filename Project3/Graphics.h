#pragma once
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <cstdint>
#include "Config.h"

class Graphics {
public:
    Graphics();
    ~Graphics();

    // コピーや代入を禁止
    Graphics(const Graphics&) = delete;
    Graphics& operator=(const Graphics&) = delete;

    // 初期化と解放
    bool Initialize(HWND hwnd);
    void Finalize();

    // 画面のクリアとピクセル書き込み
    void Clear(uint32_t color);
    void SetPixel(int x, int y, uint32_t color);

    // バックバッファへ転送し、画面に表示する
    void Present();

    // ピクセルバッファに直接アクセスしたい場合のためのアクセサ
    std::vector<uint32_t>& GetPixelBuffer() { return m_PixelBuffer; }

private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    ID3D11RenderTargetView* m_pRenderTargetView = nullptr;

    ID3D11Texture2D* m_pPixelTexture = nullptr;
    std::vector<uint32_t>   m_PixelBuffer;
};