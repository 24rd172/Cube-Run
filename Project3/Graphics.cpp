#include "Graphics.h"

Graphics::Graphics() {
    m_PixelBuffer.resize(SCREEN_WIDTH * SCREEN_HEIGHT);
}

Graphics::~Graphics() {
    Finalize();
}

bool Graphics::Initialize(HWND hwnd) {
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
        D3D11_SDK_VERSION, &scDesc, &m_pSwapChain, &m_pDevice, &featureLevel, &m_pContext
    );
    if (FAILED(hr)) return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
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

    hr = m_pDevice->CreateTexture2D(&texDesc, nullptr, &m_pPixelTexture);
    if (FAILED(hr)) return false;

    D3D11_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT), 0.0f, 1.0f };
    m_pContext->RSSetViewports(1, &vp);

    return true;
}

void Graphics::Finalize() {
    if (m_pPixelTexture) { m_pPixelTexture->Release(); m_pPixelTexture = nullptr; }
    if (m_pRenderTargetView) { m_pRenderTargetView->Release(); m_pRenderTargetView = nullptr; }
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if (m_pContext) { m_pContext->Release(); m_pContext = nullptr; }
    if (m_pDevice) { m_pDevice->Release(); m_pDevice = nullptr; }
}

void Graphics::Clear(uint32_t color) {
    std::fill(m_PixelBuffer.begin(), m_PixelBuffer.end(), color);
}

void Graphics::SetPixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        m_PixelBuffer[y * SCREEN_WIDTH + x] = color;
    }
}

void Graphics::Present() {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = m_pContext->Map(m_pPixelTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
        uint32_t* pDest = (uint32_t*)mappedResource.pData;
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            memcpy(pDest + (y * (mappedResource.RowPitch / 4)), &m_PixelBuffer[y * SCREEN_WIDTH], SCREEN_WIDTH * 4);
        }
        m_pContext->Unmap(m_pPixelTexture, 0);
    }

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);

    ID3D11Resource* pResDest = nullptr;
    ID3D11Resource* pResSrc = nullptr;
    m_pRenderTargetView->GetResource(&pResDest);
    m_pPixelTexture->QueryInterface(__uuidof(ID3D11Resource), (void**)&pResSrc);

    if (pResDest && pResSrc) m_pContext->CopyResource(pResDest, pResSrc);
    if (pResDest) pResDest->Release();
    if (pResSrc) pResSrc->Release();

    m_pSwapChain->Present(0, 0);
}