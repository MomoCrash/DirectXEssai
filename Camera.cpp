#include "Camera.h"

#include "lib/Maths.h"

Camera::Camera()
{
    mView.Identity();
}

Camera::~Camera() { }

void Camera::UpdateMatrix()
{
    mView.UpdateMatrix();
}

TRANSFORM& Camera::GetTransform()
{
    return mView;
}