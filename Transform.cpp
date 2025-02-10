#include "Transform.h"

TRANSFORM::TRANSFORM() : mDirty(true)
{
    Identity();
}

void TRANSFORM::Identity()
{
    mMatrix._11 = 1.0f;
    mMatrix._12 = 0.0f;
    mMatrix._13 = 0.0f;
    mMatrix._14 = 0.0f;
    mMatrix._21 = 0.0f;
    mMatrix._22 = 1.0f;
    mMatrix._23 = 0.0f;
    mMatrix._24 = 0.0f;
    mMatrix._31 = 0.0f;
    mMatrix._32 = 0.0f;
    mMatrix._33 = 1.0f;
    mMatrix._34 = 0.0f;
    mMatrix._41 = 0.0f;
    mMatrix._42 = 0.0f;
    mMatrix._43 = 0.0f;
    mMatrix._44 = 1.0f;
    
    mRotation._11 = 1.0f;
    mRotation._12 = 0.0f;
    mRotation._13 = 0.0f;
    mRotation._14 = 0.0f;
    mRotation._21 = 0.0f;
    mRotation._22 = 1.0f;
    mRotation._23 = 0.0f;
    mRotation._24 = 0.0f;
    mRotation._31 = 0.0f;
    mRotation._32 = 0.0f;
    mRotation._33 = 1.0f;
    mRotation._34 = 0.0f;
    mRotation._41 = 0.0f;
    mRotation._42 = 0.0f;
    mRotation._43 = 0.0f;
    mRotation._44 = 1.0f;

    scale.x = 1.0f;
    scale.y = 1.0f;
    scale.z = 1.0f;

    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 0.0f;


    rotation.x = 0.0f;
    rotation.y = 0.0f;
    rotation.z = 0.0f;
    rotation.w = 1.0f;

    forward.x = 0.0f;
    forward.y = 0.0f;
    forward.z = 1.0f;
    
    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;

    right.x = 1.0f;
    right.y = 0.0f;
    right.z = 0.0f;
}

void TRANSFORM::FromMatrix(DirectX::XMMATRIX& pMat)
{
    XMStoreFloat4x4(&mMatrix, pMat);
    mDirty = true;
}

void TRANSFORM::UpdateMatrix()
{

    if (mDirty == false) return;
    mDirty = false;
    
    DirectX::XMMATRIX scalingMatrix = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&scale));
    DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&position));
    DirectX::XMMATRIX rotation = DirectX::XMLoadFloat4x4(&mRotation);

    DirectX::XMMATRIX matrix = DirectX::XMLoadFloat4x4(&mMatrix);
    matrix = DirectX::XMMatrixMultiply(matrix, rotation);
    matrix = DirectX::XMMatrixMultiply(matrix, scalingMatrix);
    matrix = DirectX::XMMatrixMultiply(matrix, translationMatrix);
    
    DirectX::XMStoreFloat4x4(&mMatrix, matrix);
    
}

DirectX::XMMATRIX TRANSFORM::GetMatrix() const
{
    return XMLoadFloat4x4(&mMatrix);
}

void TRANSFORM::SetPosition(DirectX::XMVECTOR& pVec)
{
    XMStoreFloat3(&position, pVec);
    mDirty = true;
}

void TRANSFORM::Rotate(float pitch, float yaw, float roll)
{
    mDirty = true;
    pitch *= Maths::PI/180;
    yaw *= Maths::PI/180;
    roll *= Maths::PI/180;
    DirectX::XMVECTOR currentRotation = DirectX::XMQuaternionIdentity();
    DirectX::XMVECTOR qTemp;
    
    // Pitch
    qTemp = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(&right), pitch);
    currentRotation = DirectX::XMQuaternionMultiply(currentRotation, qTemp);

    // Yaw
    qTemp = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(&forward), roll);
    currentRotation = DirectX::XMQuaternionMultiply(currentRotation, qTemp);
    
    // Roll
    qTemp = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(&up), yaw);
    currentRotation = DirectX::XMQuaternionMultiply(currentRotation, qTemp);

    // Multiply quaternion with rotation 
    DirectX::XMVECTOR objectRotation = DirectX::XMLoadFloat4(&rotation);
    objectRotation = DirectX::XMQuaternionMultiply(objectRotation, currentRotation);
    DirectX::XMStoreFloat4(&rotation, objectRotation);

    // Rotate matrix
    DirectX::XMStoreFloat4x4(&mRotation, DirectX::XMMatrixRotationQuaternion(objectRotation));

    right.x = mRotation._11;
    right.y = mRotation._12;
    right.z = mRotation._13;

    up.x = mRotation._21;
    up.y = mRotation._22;
    up.z = mRotation._23;
    
    forward.x = mRotation._31;
    forward.y = mRotation._32;
    forward.z = mRotation._33;
}

void TRANSFORM::RotateYaw(float angle)
{
}

void TRANSFORM::RotatePitch(float angle)
{
}

void TRANSFORM::RotateRoll(float angle)
{
}

void TRANSFORM::LookAt(DirectX::XMVECTOR vector)
{
    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&position), vector, XMLoadFloat3(&up));
    XMStoreFloat4x4(&mMatrix, view);
}
