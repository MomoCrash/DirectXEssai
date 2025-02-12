#include "RenderApplication.h"

#include "UploadBuffer.h"
#include "lib/GeometryFactory.h"
#include "lib/Maths.h"

RenderApplication::RenderApplication(HINSTANCE instance) : Application(instance), mRootSignature(nullptr),
                                                           mCbvHeap(nullptr), mPSO(nullptr),
                                                           shader(L"shader\\default.hlsl"),
                                                           WorldViewProj()
{
}

bool RenderApplication::Initialize()
{
    
    mWorld.Identity();
    camera = Camera();

	XMVECTOR finalPosition = XMVectorSet(0.0f, 0.0f, -7.0f, 1.0f);
	camera.GetTransform().SetPosition(finalPosition);
	camera.GetTransform().Rotate(0.0f, 0.0f, 0.0f);

	Application::Initialize();

	mCommandList->Reset(mDirectCmdListAlloc, nullptr);
	
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

	GenerateTriangle();
    CreateMesh();
	
	BuildDescriptorHeaps();
	BuildConstantBuffer();
	
    BuildPSO();

	// Execute the initialization commands.
	mCommandList->Close();
	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

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

	// Merci Zian <3
	XMMATRIX cameraView = camera.GetTransform().GetMatrix();
	cameraView = XMMatrixInverse(nullptr, cameraView);
	
	XMMATRIX worldViewProj = mWorld.GetMatrix() * cameraView * XMLoadFloat4x4(&mProj);
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentBackBufferView(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = GetDepthStencilView();
	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(currentBackBufferView, DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1,
		&currentBackBufferView, true, &depthStencilView);
	
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	D3D12_VERTEX_BUFFER_VIEW vertexView = mRendersItems[0]->meshGeometry->VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW indexView = mRendersItems[0]->meshGeometry->IndexBufferView();

    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->IASetVertexBuffers(0, 1, &vertexView);
    mCommandList->IASetIndexBuffer(&indexView);

    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	
    mCommandList->DrawIndexedInstanced(
		mRendersItems[0]->IndexCount, 1, 0, 0, 0);

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

	mPassCB = new UploadBuffer<PassConstants>(mDevice, 1, true);
	mObjectCB = new UploadBuffer<ObjectConstants>(mDevice, mRendersItems.size(), true);
	
	UINT objCBByteSize = d3dUtils::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT objCount = (UINT)mRendersItems.size();
	
	auto objectCB = mObjectCB->Resource();
	for(UINT i = 0; i < objCount; ++i)
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
		// Offset to the ith object constant buffer in the buffer.
		cbAddress += i*objCBByteSize;

		// Offset to the object cbv in the descriptor heap.
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(i, mCbvSrvUavDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = objCBByteSize;

		mDevice->CreateConstantBufferView(&cbvDesc, cpuHandle);
	}

	UINT passCBByteSize = d3dUtils::CalcConstantBufferByteSize(sizeof(PassConstants));
	auto passCB = mPassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

	// Offset to the pass cbv in the descriptor heap.
	int heapIndex = mPassCbvOffset;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = passCBByteSize;
    
	mDevice->CreateConstantBufferView(&cbvDesc, handle);
	
}

void RenderApplication::BuildDescriptorHeaps()
{
	
	UINT objCount = (UINT)mRendersItems.size();

	// Need a CBV descriptor for each object
	// +1 for the perPass CBV for each frame resource.
	UINT numDescriptors = (objCount+1);

	// Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
	mPassCbvOffset = objCount;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	mDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mCbvHeap));
}

void RenderApplication::CreateMesh()
{
	
	MeshGeometry* boxGeometry = new MeshGeometry();
	boxGeometry->MeshData = 	GeometryFactory::CreateBox(10.0f, 10.0f, 60, 3);

	UINT objCBIndex = 0;
	
	GenerateGeometryBuffer(boxGeometry);

	RenderItem* box = new RenderItem(boxGeometry);
	box->transform.SetPosition(XMVectorSet(5, 0, 5, 1));
	box->IndexCount = (UINT)boxGeometry->MeshData.Indices32.size();
	mRendersItems.push_back(box);

	RenderItem* box1 = new RenderItem(boxGeometry);
	box1->transform.SetPosition(XMVectorSet(0, 0, 0, 1));
	box1->IndexCount = (UINT)boxGeometry->MeshData.Indices32.size();
	mRendersItems.push_back(box1);
}

void RenderApplication::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{

}

ID3D12Resource* RenderApplication::CreateBuffer(const void* initData, UINT64 byteSize, ID3D12Resource* uploadBuffer)
{
	ID3D12Resource* defaultBuffer;
	
	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC descriptor = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	mDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&descriptor,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&defaultBuffer));

	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	descriptor = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	mDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&descriptor,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer));

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mCommandList->ResourceBarrier(1, &barrier);
	UpdateSubresources<1>(mCommandList, defaultBuffer, uploadBuffer, 0, 0, 1, &subResourceData);
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	mCommandList->ResourceBarrier(1, &barrier);

	return defaultBuffer;
}

void RenderApplication::GenerateGeometryBuffer(MeshGeometry* geo)
{

	std::vector<GeometryFactory::Vertex>* vertex = &geo->MeshData.Vertices;
	std::vector<uint16> index = geo->MeshData.GetIndices16();

	const UINT vbByteSize = (UINT)vertex->size() * sizeof(GeometryFactory::Vertex);
	const UINT ibByteSize = (UINT)index.size() * sizeof(std::uint16_t);

	////////////////////// FOR VERTEX ////////////////////////
	D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU);
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertex->data(), vbByteSize);

	D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), index.data(), ibByteSize);

	// Copy the triangle data to the vertex buffer.
	geo->VertexBufferGPU = CreateBuffer(vertex->data(), vbByteSize, geo->VertexBufferUploader);

	////////////////////// FOR INDICES ////////////////////////
	// Copy the triangle data to the vertex buffer.
	geo->IndexBufferGPU = CreateBuffer(index.data(), ibByteSize, geo->IndexBufferUploader);

	// Initialize the vertex buffer view.
	geo->VertexByteStride = sizeof(GeometryFactory::Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	
	// Initialize the vertex buffer view.
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;
	
}

void RenderApplication::GenerateTriangle()
{
	std::array<Vertex, 8> vertices {
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};
		
	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};
	

	const UINT vertexBufferSize = sizeof(vertices);
	const UINT indicesBufferSize = sizeof(indices);

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
		IID_PPV_ARGS(&mVertexBuffer));

	// Copy the triangle data to the vertex buffer.
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, &vertices, sizeof(vertices));
	mVertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = sizeof(Vertex);
	mVertexBufferView.SizeInBytes = vertexBufferSize;

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
		IID_PPV_ARGS(&mIndicesBuffer));

	// Copy the triangle data to the vertex buffer.
	UINT8* pIndicesDataBegin;// We do not intend to read from this resource on the CPU.
	mIndicesBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndicesDataBegin));
	memcpy(pIndicesDataBegin, &indices, sizeof(indices));
	mIndicesBuffer->Unmap(0, nullptr);
	
	// Initialize the vertex buffer view.
	mIndicesBufferView.BufferLocation = mIndicesBuffer->GetGPUVirtualAddress();
	mIndicesBufferView.Format = DXGI_FORMAT_R16_UINT;
	mIndicesBufferView.SizeInBytes = indicesBufferSize;
}
