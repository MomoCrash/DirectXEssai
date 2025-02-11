#include "RenderObject.h"

RenderItem::RenderItem(MeshGeometry* geometry) : Geo(geometry)
{
    transform.Identity();
}
