#include "RenderApplication.h"

#include "UploadBuffer.h"
#include "lib/Maths.h"

RenderApplication::RenderApplication(HINSTANCE instance) : Application(instance), mRootSignature(nullptr),
                                                           mCbvHeap(nullptr), mPSO(nullptr),
                                                           shader(L"shader\\default.hlsl"),
                                                           WorldViewProj(), mVertexBuffer(nullptr), mVertexBufferView()
{
}

bool RenderApplication::Initialize()
{
    
    mWorld.Identity();
    mProj.Identity();
    camera = Camera();

	XMVECTOR finalPosition = XMVectorSet(1.0f, -0.0f, -3.0f, 1.0f);
	camera.GetTransform().SetPosition(finalPosition);

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

    CreateTriangle();
    BuildPSO();

    return true;
}

void RenderApplication::Update()
{

	float speed = 10.0f;

	XMVECTOR cPosition = XMLoadFloat3(&camera.GetTransform().position);
	XMVECTOR cForward = XMLoadFloat3(&camera.GetTransform().forward);
	XMVECTOR cRight = XMLoadFloat3(&camera.GetTransform().right);

	if (d3dUtils::IsKeyDown(0x5A))
	{
		cPosition = XMVectorAdd(cPosition, cForward * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}

	if (d3dUtils::IsKeyDown(0x53))
	{
		cPosition = XMVectorSubtract(cPosition, cForward * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}
	
	if (d3dUtils::IsKeyDown(0x44))
	{
		cPosition= XMVectorAdd(cPosition, cRight * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}
	
	if (d3dUtils::IsKeyDown(0x51))
	{
		cPosition = XMVectorSubtract(cPosition, cRight * mTimer.DeltaTime() * speed);
		camera.GetTransform().SetPosition(cPosition);
	}

	
	camera.GetTransform().LookAt(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));

	mWorld.UpdateMatrix();
	camera.Update();
	mProj.UpdateMatrix();


	XMMATRIX worldViewProj = mWorld.GetMatrix() * camera.GetTransform().GetMatrix() * mProj.GetMatrix();
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

    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    mCommandList->IASetIndexBuffer(&mIndicesBufferView);

    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    mCommandList->DrawIndexedInstanced(
		6, 1, 0, 0, 0);

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
}

void RenderApplication::OnMouseUp(WPARAM btnState, int x, int y)
{
}

void RenderApplication::OnMouseMove(WPARAM btnState, int x, int y)
{

	float sensitivity = 0.01f;
		
	// Make each pixel correspond to a degree.
	float dx = static_cast<float>(x - mLastMousePosition.x) * sensitivity;
	float dy = static_cast<float>(y - mLastMousePosition.y) * sensitivity;

	mYaw += dx;
	mPitch += dy;
	
	mLastMousePosition.x = x;
	mLastMousePosition.y = y;
	
	if (mPitch > 89.0f || mPitch < -89.0f)
		return;

	// Update angles based on input to orbit camera around box.
	camera.GetTransform().Rotate(dy, dx, 0.0f);
	
}

void RenderApplication::OnKeyPressed(WPARAM btnState, int x, int y)
{
	
}

void RenderApplication::OnResize()
{
    Application::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*Maths::PI, AspectRatio(), 1.0f, 1000.0f);
	mProj.FromMatrix(P);
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

void RenderApplication::CreateTriangle()
{
	
	{
		// Define the geometry for a triangle.
		std::array<Vertex, 6> vertices {
			Vertex({ XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(Colors::White) }),
			Vertex({ XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(Colors::Black) }),
			Vertex({ XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(Colors::Red) }),
			
			Vertex({ XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(Colors::Blue) }),
			Vertex({ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT4(Colors::LightBlue) }),
			Vertex({ XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(Colors::White) })
		};

		std::array<std::uint16_t, 6> indices {
			0, 2, 1,
			3, 5, 4
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

	
}
