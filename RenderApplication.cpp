#include "RenderApplication.h"

#include "UploadBuffer.h"
#include "lib/GeometryFactory.h"
#include "lib/Maths.h"

RenderApplication::RenderApplication(HINSTANCE instance) : Application(instance), mRootSignature(nullptr),
                                                           mCbvHeap(nullptr), mPSO(nullptr),
                                                           shader(L"shader\\default.hlsl"),
                                                           WorldViewProj(), mRenderItem(nullptr)
{
}

bool RenderApplication::Initialize()
{
    
    mWorld.Identity();
    camera = Camera();

	XMVECTOR finalPosition = XMVectorSet(0.0f, 0.0f, -7.0f, 1.0f);
	camera.GetTransform().SetPosition(finalPosition);
	camera.GetTransform().Rotate(0, 0, 180.f);

	Application::Initialize();

	BuildDescriptorHeaps();
	BuildConstantBuffer();
	
	// Notre root signature
	{
    	// Shader programs typically require resources as input (constant buffers,
    	// textures, samplers).  The root signature defines the resources the shader
    	// programs expect.  If we think of the shader programs as a function, and
    	// the input resources as function parameters, then the root signature can be
    	// thought of as defining the function signature.  

    	// Root parameter can be a table, root descriptor or root constants.
    	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    	// Create a single descriptor table of CBVs.
    	CD3DX12_DESCRIPTOR_RANGE cbvTable;
    	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    	// A root signature is an array of root parameters.
    	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    	ID3DBlob* serializedRootSig = nullptr;
    	ID3DBlob* errorBlob = nullptr;
    	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig, &errorBlob);

    	if(errorBlob != nullptr)
    	{
    		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    	}

    	if (FAILED(hr)) std::cout << "Signature creation failed" << std::endl;

    	mDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&mRootSignature));
	}

    CreateMesh();
    BuildPSO();

	OnResize();

    return true;
}

void RenderApplication::Update()
{

	float speed = 1.0f;

	XMVECTOR cPosition = XMLoadFloat3(&camera.GetTransform().position);
	XMVECTOR cForward = XMLoadFloat3(&camera.GetTransform().forward);
	XMVECTOR cUp = XMLoadFloat3(&camera.GetTransform().up);
	XMVECTOR cRight = XMLoadFloat3(&camera.GetTransform().right);

	if (d3dUtils::IsKeyDown(VK_SPACE))
	{
		cPosition = XMVectorAdd(cPosition, cUp * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}
	
	if (d3dUtils::IsKeyDown(VK_SHIFT))
	{
		cPosition = XMVectorSubtract(cPosition, cUp * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}
	
	if (d3dUtils::IsKeyDown('Z'))
	{
		cPosition = XMVectorAdd(cPosition, cForward * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}

	if (d3dUtils::IsKeyDown('S'))
	{
		cPosition = XMVectorSubtract(cPosition, cForward * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}
	
	if (d3dUtils::IsKeyDown('Q'))
	{
		cPosition= XMVectorSubtract(cPosition, cRight * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}
	
	if (d3dUtils::IsKeyDown('D'))
	{
		cPosition = XMVectorAdd(cPosition, cRight * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}

	std::cout << "x. " << camera.GetTransform().position.x << " y." << camera.GetTransform().position.y << " z." << camera.GetTransform().position.z << std::endl;
	std::cout << "x. " << camera.GetTransform().forward.x << " y." << camera.GetTransform().forward.y << " z." << camera.GetTransform().forward.z << std::endl;


	mWorld.UpdateMatrix();
	camera.UpdateMatrix();
	
	XMMATRIX worldViewProj = mWorld.GetMatrix() * camera.GetTransform().GetMatrix() * XMLoadFloat4x4(&mProj);
    XMStoreFloat4x4(&WorldViewProj, worldViewProj);

	// Update the constant buffer with the latest worldViewProj matrix.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mObjectCB->CopyData(0, objConstants);
    
}

void RenderApplication::Draw()
{

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
	mDirectCmdListAlloc->Reset();

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    mCommandList->Reset(mDirectCmdListAlloc, mPSO);

	mCommandList->SetGraphicsRootSignature(mRootSignature);
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCommandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
	D3D12_CPU_DESCRIPTOR_HANDLE depthHandle = GetDepthStencilView();
	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1,
		&rtvHandle, true, &depthHandle);


	// ON A TOUT LE PROCESS DE DESSIN A L'ECRAN
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	D3D12_VERTEX_BUFFER_VIEW vertexView = mRenderItem.Geo->VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW indexView = mRenderItem.Geo->IndexBufferView();

    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->IASetVertexBuffers(0, 1, &vertexView);
    mCommandList->IASetIndexBuffer(&indexView);

    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    mCommandList->DrawIndexedInstanced(
		mRenderItem.Geo->MeshData.Indices32.size(), 1, 0, 0, 0);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &barrier);

    // Done recording commands.
	mCommandList->Close();
 
    // Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	
	// swap the back and front buffers
	mSwapChain->Present(0, 0);
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

void RenderApplication::OnMouseDown(WPARAM btnState, int x, int y)
{
	mTurn = true;
	mLastMousePosition.x = x;
	mLastMousePosition.y = y;
}

void RenderApplication::OnMouseUp(WPARAM btnState, int x, int y)
{
	mTurn = false;
}

void RenderApplication::OnMouseMove(WPARAM btnState, int x, int y)
{

	if(!mTurn) return;

	float sensitivity = 0.1f;
		
	// Make each pixel correspond to a degree.
	float dx = static_cast<float>(x - mLastMousePosition.x) * sensitivity;
	float dy = static_cast<float>(y - mLastMousePosition.y) * sensitivity;

	mYaw += dx;
	mPitch += dy;
	
	mLastMousePosition.x = x;
	mLastMousePosition.y = y;
	
	if (mPitch > 89.0f)
	{
		mPitch = 90.0f;
	}
	if (mPitch < -89.0f)
	{
		mPitch = -90.0f;
	}

	// Update angles based on input to orbit camera around box.
	camera.GetTransform().Rotate(dy,  dx, 0.0f);
	
}

void RenderApplication::OnKeyPressed(WPARAM btnState, int x, int y)
{
	
}

void RenderApplication::OnResize()
{
    Application::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX ProjMatrix = XMMatrixPerspectiveFovLH(0.25f*Maths::PI, AspectRatio(), 0.000001f, 1000);
	XMStoreFloat4x4(&mProj, ProjMatrix);
}

void RenderApplication::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { shader.GetInputLayout().data(), (UINT)shader.GetInputLayout().size() };
	psoDesc.pRootSignature = mRootSignature;
	psoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(shader.GetVertexShader()->GetBufferPointer()), 
		shader.GetVertexShader()->GetBufferSize() 
	};
	psoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(shader.GetPixelShader()->GetBufferPointer()), 
		shader.GetPixelShader()->GetBufferSize() 
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	HRESULT result = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
	if (FAILED(result)) { std::cerr << "Failed to create render pipeline !\n"; }
}

void RenderApplication::BuildConstantBuffer()
{
	
	mObjectCB = new UploadBuffer<ObjectConstants>(mDevice, 1, true);

	UINT objCBByteSize = d3dUtils::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	
	// Offset to the ith object constant buffer in the buffer.
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex*objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtils::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	mDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void RenderApplication::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	HRESULT result = mDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mCbvHeap));
	if (FAILED(result)) { std::cerr << "Failed to create heap descriptor !\n"; }
}

void RenderApplication::CreateMesh()
{
	
	MeshGeometry* boxGeometry = new MeshGeometry();
	boxGeometry->MeshData = GeometryFactory::CreateBox(1.5f, 0.5f, 1.5f, 3);

	GenerateGeometryBuffer(boxGeometry);

	mRenderItem = RenderItem(boxGeometry);
	
}

void RenderApplication::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{

}

void RenderApplication::GenerateGeometryBuffer(MeshGeometry* geo)
{

	const UINT vertexBufferSize = (UINT)geo->MeshData.Vertices.size() * sizeof(Vertex);
	const UINT indicesBufferSize = (UINT)geo->MeshData.GetIndices16().size()  * sizeof(std::uint16_t);

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	mDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->VertexBufferGPU));
	
	// Copy the triangle data to the vertex buffer.
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	geo->VertexBufferGPU->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, &geo->MeshData.Vertices, sizeof(geo->MeshData.Vertices));
	geo->VertexBufferGPU->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vertexBufferSize;

	////////////////////// FOR INDICES ////////////////////////////
		
	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap.
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(indicesBufferSize);
	mDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->IndexBufferGPU));

	// Copy the triangle data to the vertex buffer.
	UINT8* pIndicesDataBegin;// We do not intend to read from this resource on the CPU.
	geo->IndexBufferGPU->Map(0, &readRange, reinterpret_cast<void**>(&pIndicesDataBegin));
	memcpy(pIndicesDataBegin, &geo->MeshData.GetIndices16(), sizeof(uint16));
	geo->IndexBufferGPU->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = indicesBufferSize;
	
}
