#pragma once

#include "UploadBuffer.h"
#include "lib/d3dUtils.h"
#include "lib/Maths.h"

using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

// C'est les constantes globales du monde pour chaque frame
// Y'a tout les parametres du livre mais c'est pas forcement tout utile
struct PassConstants
{
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 InvView;
    DirectX::XMFLOAT4X4 Proj;
    DirectX::XMFLOAT4X4 InvProj;
    DirectX::XMFLOAT4X4 ViewProj;
    DirectX::XMFLOAT4X4 InvViewProj;
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    PassConstants();
};

// C'est les constante pour chaque objet individuel
struct ObjectConstants
{
    XMFLOAT4X4 World;
};

class Shader
{
public:
    Shader(std::wstring path);

    ID3DBlob* GetVertexShader();
    ID3DBlob* GetPixelShader();
    std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputLayout();
private:

    ID3DBlob* mVertexShader;
    ID3DBlob* mPixelShader;
    
    ID3DBlob* CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target);

private:
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
};