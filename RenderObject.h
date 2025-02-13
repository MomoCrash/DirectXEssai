#pragma once

#include "Transform.h"
#include "lib/d3dUtils.h"

struct RenderItem
{
    RenderItem(RenderMesh* geometry);

    TRANSFORM Transform;
    DirectX::XMFLOAT4 Color;
    
    RenderMesh* Mesh;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    UINT ObjCBIndex = -1;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // cmdList->DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
    
};
