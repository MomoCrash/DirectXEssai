#pragma once

#include "lib/Maths.h"

struct TRANSFORM
{
private:
    bool mDirty;

    DirectX::XMFLOAT4X4 mMatrix;
    DirectX::XMFLOAT4X4 mRotation;

public:
    TRANSFORM();

    DirectX::XMFLOAT3 scale;
    
    DirectX::XMFLOAT3 forward;
    DirectX::XMFLOAT3 right;
    DirectX::XMFLOAT3 up;
    
    DirectX::XMFLOAT4 rotation;

    DirectX::XMFLOAT3 position;

    void Identity();
    void XM_CALLCONV FromMatrix(DirectX::FXMMATRIX pMat);
    void UpdateMatrix();
    DirectX::XMMATRIX GetMatrix() const;

    void XM_CALLCONV SetPosition(DirectX::FXMVECTOR pVec);

    void Rotate(float pitch, float yaw, float roll);
    void RotatePitch(float angle);
    void RotateYaw(float angle);
    void RotateRoll(float angle);

    void XM_CALLCONV LookAt(DirectX::FXMVECTOR vector);
};
