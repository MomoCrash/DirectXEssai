#pragma once
#include <map>

#include "Application.h"
#include "lib/d3dUtils.h"
#include "Camera.h"
#include "RenderObject.h"
#include "Shader.h"
#include "Transform.h"
#include "UploadBuffer.h"
#include "lib/GeometryFactory.h"

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

    void UpdatePassBC(); // Used as global update for global world data
    void UpdatePerObjectBC(); // Used as update for each object
    
    void OnResize() override;

    void AddRenderItem(RenderItem* item);
    void BuildRenderableItem(); // Add RenderItem who will be used
    void BuildDescriptorHeaps(); // Build RenderItem & Pass descriptor
    void BuildConstantBuffer(); // Build Constant Buffer for Pass & perObject
    void BuildPSO();

    void DrawRenderItems();

    GeometryFactory* mFactory;
    
    ID3D12RootSignature* mRootSignature;
    ID3D12DescriptorHeap* mCbvHeap;
    ID3D12PipelineState* mPSO;
    
    Shader shader;
    Camera camera;
    XMFLOAT4X4 mProj;

    std::vector<RenderItem*> mRendersItems;
    
    std::vector<UploadBuffer<ObjectConstants>*> mObjectsCB;
    UploadBuffer<PassConstants>* mPassCB;
    
    UINT mPassCbvOffset = 0;
    
    POINT mLastMousePosition;
    float mYaw = 0.0f;
    float mPitch = 0.0f;

    bool mTurn;
};
