#pragma once

#include "Transform.h"
#include "lib/d3dUtils.h"
#include "lib/GeometryFactory.h"

struct MeshGeometry
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
    GeometryFactory::MeshData MeshData;

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

struct RenderItem
{
    RenderItem(MeshGeometry* geometry);

    TRANSFORM transform;
    DirectX::XMFLOAT4 Color;
    
    MeshGeometry* meshGeometry;

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Because we have an object cbuffer for each FrameResource, we have to apply the
    // update to each FrameResource.  Thus, when we modify obect data we should set 
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    int NumFramesDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    UINT ObjCBIndex = -1;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // cmdList->DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
    
};
