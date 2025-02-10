#pragma once

#include "lib/d3dUtils.h"
#include "lib/Maths.h"

using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj;
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