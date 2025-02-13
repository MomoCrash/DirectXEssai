#include "RenderObject.h"

RenderItem::RenderItem(RenderMesh* geoMesh) : Mesh(geoMesh)
{
    Transform.Identity();
    Color.x = 0.0f;
    Color.y = 0.0f;
    Color.z = 0.0f;
    Color.w = 0.0f;
    IndexCount = (UINT)Mesh->MeshData.Indices32.size();
}