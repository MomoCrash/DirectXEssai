#include "Shader.h"

PassConstants::PassConstants()
{
    
    Maths::Identity4X4(&View);
    Maths::Identity4X4(&InvView);
    Maths::Identity4X4(&Proj);
    Maths::Identity4X4(&InvProj);
    Maths::Identity4X4(&ViewProj);
    Maths::Identity4X4(&InvViewProj);
    
}

Shader::Shader(std::wstring path)
{
    mVertexShader = CompileShader(path, nullptr, "VS", "vs_5_1");
    mPixelShader = CompileShader(path, nullptr, "PS", "ps_5_1");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

ID3DBlob* Shader::GetVertexShader()
{
    return mVertexShader;
}

ID3DBlob* Shader::GetPixelShader()
{
    return mPixelShader;
}

std::vector<D3D12_INPUT_ELEMENT_DESC>& Shader::GetInputLayout()
{
    return mInputLayout;
}

ID3DBlob* Shader::CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* byteCode = nullptr;
    ID3DBlob* errors;
    HRESULT hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if(errors != nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());

    if (FAILED(hr)) { std::cerr << "Failed to compile shader !\n"; return nullptr; }

    return byteCode;
}
