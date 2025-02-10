#pragma once
#include "Application.h"
#include "lib/d3dUtils.h"
#include "Camera.h"
#include "Shader.h"
#include "Transform.h"
#include "UploadBuffer.h"

using namespace DirectX;

class RenderApplication : public Application
{
public:
    RenderApplication(HINSTANCE instance);
    
    bool Initialize() override;
    
    virtual void Update() override;
    virtual void Draw() override;

private:
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnMouseUp(WPARAM btnState, int x, int y) override;
    void OnMouseMove(WPARAM btnState, int x, int y) override;
    void OnKeyPressed(WPARAM btnState, int x, int y) override;
    
    void OnResize() override;

    void BuildPSO();
    void BuildConstantBuffer();
    void BuildDescriptorHeaps();
    void CreateTriangle();

    UploadBuffer<ObjectConstants>* mObjectCB = nullptr;

    ID3D12RootSignature* mRootSignature;
    ID3D12DescriptorHeap* mCbvHeap;
    ID3D12PipelineState* mPSO;
    
    Shader shader;
    Camera camera;
    
    TRANSFORM mWorld;
    XMFLOAT4X4 mProj;

    XMFLOAT4X4 WorldViewProj;

    // App resources.
    ID3D12Resource* mVertexBuffer;
    ID3D12Resource* mIndicesBuffer;
    
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndicesBufferView;

    POINT mLastMousePosition;
    float mYaw = 0.0f;
    float mPitch = 0.0f;

    bool mTurn;
};
