#pragma once
#include "Transform.h"

class Camera
{
public:
    Camera();
    ~Camera();
    TRANSFORM mView;

    void Update();
    TRANSFORM& GetTransform();
    
};
