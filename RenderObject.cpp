#include "RenderObject.h"

RenderItem::RenderItem(MeshGeometry* geometry) : meshGeometry(geometry)
{
    transform.Identity();
}
