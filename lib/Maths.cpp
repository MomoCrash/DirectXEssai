#include "Maths.h"

void Maths::Identity4X4(DirectX::XMFLOAT4X4* matrix)
{
    matrix->_11 = 1.0f;
    matrix->_12 = 0.0f;
    matrix->_13 = 0.0f;
    matrix->_14 = 0.0f;
    matrix->_21 = 0.0f;
    matrix->_22 = 1.0f;
    matrix->_23 = 0.0f;
    matrix->_24 = 0.0f;
    matrix->_31 = 0.0f;
    matrix->_32 = 0.0f;
    matrix->_33 = 1.0f;
    matrix->_34 = 0.0f;
    matrix->_41 = 0.0f;
    matrix->_42 = 0.0f;
    matrix->_43 = 0.0f;
    matrix->_44 = 1.0f;
}