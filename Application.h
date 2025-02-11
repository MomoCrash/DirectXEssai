#pragma once

#include "Shader.h"
#include "lib/d3dUtils.h"

class Application
{
public:
    static Application* GetInstance();
    
    Application(HINSTANCE instance);
    ~Application();

    virtual bool Initialize();
    float AspectRatio()const;

    int Run();
    virtual LRESULT InputHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    
    bool InitializeWindow();
    bool InitializeDirectX();

    virtual void Update() = 0;
    virtual void Draw();
    virtual void UpdateTimer();
    virtual void OnResize();

    void Set4xMsaaState(bool value);

    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateRtvAndDsvDescriptorHeaps();

    void FlushCommandQueue();

    ID3D12Resource* GetCurrentBackBuffer();
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;


    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y) = 0;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)   = 0;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) = 0;
    virtual void OnKeyPressed(WPARAM btnState, int x, int y) = 0;

    static Application* instance;
    
    GameTimer mTimer;
    
    HINSTANCE mwInstance;
    HWND mWindow = nullptr; // main window handle
    bool      mAppPaused = false;  // is the application paused?
    bool      mMinimized = false;  // is the application minimized?
    bool      mMaximized = false;  // is the application maximized?
    bool      mResizing = false;   // are the resize bars being dragged?
    bool      mFullscreenState = false;// fullscreen enabled

    ObjectConstants WVP;
    
    ID3D12Device* mDevice;
    IDXGISwapChain* mSwapChain;
    IDXGIFactory4* mdxgiFactory;

    ID3D12Fence* mFence;
    UINT64 mCurrentFence = 0;

    ID3D12CommandQueue* mCommandQueue;
    ID3D12CommandAllocator* mDirectCmdListAlloc;
    ID3D12GraphicsCommandList* mCommandList;

    ID3D12DescriptorHeap* mRtvHeap;
    ID3D12DescriptorHeap* mDsvHeap;

    static const int SwapChainBufferCount = 2;
    int mCurrBackBuffer = 0;
    ID3D12Resource* mSwapChainBuffer[SwapChainBufferCount];
    ID3D12Resource* mDepthStencilBuffer;

    D3D12_VIEWPORT mScreenViewport; 
    D3D12_RECT mScissorRect;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    // Set true to use 4X MSAA (§4.1.8).  The default is false.
    bool      m4xMsaaState = false;    // 4X MSAA enabled
    UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

    // Derived class should set these in derived constructor to customize starting values.
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    int mClientWidth = 800;
    int mClientHeight = 600;
};
