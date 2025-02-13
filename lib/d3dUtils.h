#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#define D3D_DEBUG_INFO
#include <crtdbg.h>
#endif
#include <windows.h>
#include <WindowsX.h>
#include <iostream>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "d3dx12.h"
#include <DirectXColors.h>

#include "GameTimer.h"
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using uint16 = std::uint16_t;
using uint32 = std::uint32_t;

struct Vertex
{
    Vertex(){}
    Vertex(
        const DirectX::XMFLOAT3& p, 
        const DirectX::XMFLOAT3& n, 
        const DirectX::XMFLOAT3& t, 
        const DirectX::XMFLOAT2& uv) :
        Position(p), 
        Normal(n), 
        TangentU(t), 
        TexC(uv){}
    Vertex(
        float px, float py, float pz, 
        float nx, float ny, float nz,
        float tx, float ty, float tz,
        float u, float v) : 
        Position(px,py,pz), 
        Normal(nx,ny,nz),
        TangentU(tx, ty, tz), 
        TexC(u,v){}

    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 TangentU;
    DirectX::XMFLOAT2 TexC;
};

struct MeshData
{
    std::vector<Vertex> Vertices;
    std::vector<uint32> Indices32;

    std::vector<uint16>& GetIndices16()
    {
        if(mIndices16.empty())
        {
            mIndices16.resize(Indices32.size());
            for(size_t i = 0; i < Indices32.size(); ++i)
                mIndices16[i] = static_cast<uint16>(Indices32[i]);
        }

        return mIndices16;
    }

private:
    std::vector<uint16> mIndices16;
};

struct RenderMesh
{

    ID3DBlob* VertexBufferCPU = nullptr; 
    ID3DBlob* IndexBufferCPU  = nullptr;

    // LE buffer dans le GPU
    ID3D12Resource* VertexBufferGPU = nullptr;
    ID3D12Resource* IndexBufferGPU = nullptr;

    ID3D12Resource* VertexBufferUploader = nullptr;
    ID3D12Resource* IndexBufferUploader = nullptr;

    // Les donnes pour dessiner a l'eran
    UINT VertexByteStride = 0;
    UINT VertexBufferByteSize = 0;
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
    UINT IndexBufferByteSize = 0;

    // Toutes les geometrie qui sont dans le vectex buffer 
    MeshData MeshData;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = VertexByteStride;
        vbv.SizeInBytes = VertexBufferByteSize;

        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }

    // We can free this memory after we finish upload to the GPU.
    void DisposeUploaders()
    {
        VertexBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
};


class d3dUtils
{
public:
    static UINT CalcConstantBufferByteSize(UINT byteSize);
    
    static bool IsKeyDown(int vkeyCode);

    static ID3D12Resource* CreateBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const void* initData,
        UINT64 byteSize,
        ID3D12Resource* uploadBuffer);
    
};
