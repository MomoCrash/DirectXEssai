#pragma once
#include <map>

#include "Application.h"
#include "lib/d3dUtils.h"
#include "Camera.h"
#include "RenderObject.h"
#include "Shader.h"
#include "Transform.h"
#include "UploadBuffer.h"

using namespace DirectX;

class RenderApplication : public Application
{

    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;
    
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
    void CreateMesh();

    void GenerateGeometryBuffer(MeshGeometry* geo);
    void GenerateTriangle();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
    ID3D12Resource* CreateBuffer(const void* initData, UINT64 byteSize, ID3D12Resource* uploadBuffer);
    
    FrameResource* mCurrFrameResource = nullptr;
    
    ID3D12RootSignature* mRootSignature;
    ID3D12DescriptorHeap* mCbvHeap;
    ID3D12PipelineState* mPSO;
    
    Shader shader;
    Camera camera;
    
    TRANSFORM mWorld;
    XMFLOAT4X4 mProj;

    XMFLOAT4X4 WorldViewProj;

    // Geometry resources.
    ID3D12Resource* mVertexBuffer;
    ID3D12Resource* mIndicesBuffer;
    
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndicesBufferView;

    std::vector<RenderItem*> mRendersItems;
    
    UploadBuffer<ObjectConstants>* mObjectCB;
    UploadBuffer<PassConstants>* mPassCB;
    
    UINT mPassCbvOffset = 0;
    
    POINT mLastMousePosition;
    float mYaw = 0.0f;
    float mPitch = 0.0f;

    bool mTurn;
};
