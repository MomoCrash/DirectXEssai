#include "RenderApplication.h"

#include "UploadBuffer.h"
#include "lib/GeometryFactory.h"
#include "lib/Maths.h"

RenderApplication::RenderApplication(HINSTANCE instance) : Application(instance), mFactory(nullptr),
                                                           mRootSignature(nullptr),
                                                           mCbvHeap(nullptr), mPSO(nullptr),
                                                           shader(L"shader\\default.hlsl"), mProj(),
                                                           mPassCB(nullptr),
                                                           mLastMousePosition(),
                                                           mTurn(false)
{
}

bool RenderApplication::Initialize()
{
	
    camera = Camera();

	XMVECTOR finalPosition = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
	camera.GetTransform().SetPosition(finalPosition);
	camera.GetTransform().Rotate(0.0f, 0.0f, 0.0f);

	Application::Initialize();

	mCommandList->Reset(mDirectCmdListAlloc, nullptr);
	
	// Notre root signature
	{

    	// Root parameter can be a table, root descriptor or root constants.
    	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
    	
    	// Create root CBVs.
    	slotRootParameter[0].InitAsConstantBufferView(0);
    	slotRootParameter[1].InitAsConstantBufferView(1);

    	// A root signature is an array of root parameters.
    	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, 
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


    BuildDescriptorHeaps();
    BuildConstantBuffer();

    BuildPSO();

    BuildRenderableItem();
    // Execute the initialization commands.
	mCommandList->Close();
	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	OnResize();

    return true;
}

void RenderApplication::BuildRenderableItem()
{
	mFactory = new GeometryFactory(mDevice, mCommandList);
	
	RenderMesh* boxMesh = mFactory->CreateBox(1.0f, 1.0f, 1.0f, 3);
	RenderMesh* circleMesh = mFactory->CreateGeosphere(2.0f, 5.0f);
	RenderMesh* customMesh = mFactory->LoadGeometryFromFile("objects/FinalBaseMesh.obj");
	
	RenderItem* box = new RenderItem(boxMesh);
	box->Transform.SetPosition(XMVectorSet(5, 0, 1.0f, 1));
	XMStoreFloat4(&box->Color, XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f));
	box->ObjCBIndex = 0;
	AddRenderItem(box);

	RenderItem* box1 = new RenderItem(boxMesh);
	box1->Transform.SetPosition(XMVectorSet(0, 0, 0, 1));
	XMStoreFloat4(&box1->Color, XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));
	box1->ObjCBIndex = 0;
	AddRenderItem(box1);

	RenderItem* circle = new RenderItem(customMesh);
	circle->Transform.SetPosition(XMVectorSet(10, 0, 0, 1));
	XMStoreFloat4(&circle->Color, XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f));
	circle->ObjCBIndex = 0;
	AddRenderItem(circle);
}


void RenderApplication::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)mRendersItems.size();

	// Need a CBV descriptor for each object
	UINT numDescriptors = (objCount+1);

	// Save an offset to the start of the pass CBVs.
	mPassCbvOffset = objCount;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	mDevice->CreateDescriptorHeap(&cbvHeapDesc,
	                              IID_PPV_ARGS(&mCbvHeap));
}

void RenderApplication::AddRenderItem(RenderItem* item)
{
	mObjectsCB.push_back(new UploadBuffer<ObjectConstants>(mDevice, 1, true));
	mRendersItems.push_back(item);
}

void RenderApplication::BuildConstantBuffer()
{

	mPassCB = new UploadBuffer<PassConstants>(mDevice, 1, true);
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

	// Le shaders qui a des CONSTANT BUFFER pas pris en compte dans la signature
	// ou dans le mInputLayout du Shader
	
	if (FAILED(result)) { std::cerr << "Failed to create render pipeline !\n"; }
}

void RenderApplication::DrawRenderItems()
{
	
	// For each render item...
	for(size_t i = 0; i < mRendersItems.size(); ++i)
	{
		auto objectCB = mObjectsCB[i]->Resource();
		auto ri = mRendersItems[i];

		D3D12_VERTEX_BUFFER_VIEW vertexBuffer = ri->Mesh->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW indexBuffer = ri->Mesh->IndexBufferView();
		
		mCommandList->IASetVertexBuffers(0, 1, &vertexBuffer);
		mCommandList->IASetIndexBuffer(&indexBuffer);
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		mCommandList->SetGraphicsRootConstantBufferView(0, objectCB->GetGPUVirtualAddress());

		mCommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
	
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

	// Bind per-pass constant buffer.  We only need to do this once per-pass.
	auto passCB = mPassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	DrawRenderItems();

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

	if (d3dUtils::IsKeyDown('M'))
	{
		RenderItem* box1 = new RenderItem(mFactory->CreateBox(1.0f, 1.0f, 1.0f, 3));
		box1->Transform.SetPosition(XMVectorSet(0, 2, 0, 1));
		XMStoreFloat4(&box1->Color, XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));
		box1->ObjCBIndex = 0;
		AddRenderItem(box1);
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
	
	camera.UpdateMatrix();
	
	UpdatePassBC();
	UpdatePerObjectBC();
    
}

void RenderApplication::UpdatePassBC()
{

	PassConstants mMainPassCB;
	
	XMMATRIX view = camera.GetTransform().GetMatrix();
	view = XMMatrixInverse(nullptr, view);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMVECTOR projDet = XMMatrixDeterminant(viewProj);
	XMVECTOR viewProjDet = XMMatrixDeterminant(proj);
	XMMATRIX invView = XMMatrixInverse(&viewDet, view);
	XMMATRIX invProj = XMMatrixInverse(&projDet, proj);
	XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = camera.mView.position;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = mTimer.TotalTime();
	mMainPassCB.DeltaTime = mTimer.DeltaTime();
	
	mPassCB->CopyData(0, mMainPassCB);
}

void RenderApplication::UpdatePerObjectBC()
{
	for(int i = 0; i < mRendersItems.size(); i++)
	{
		auto& e = mRendersItems[i];

		e->Transform.UpdateMatrix();
		
		XMMATRIX world = e->Transform.GetMatrix();

		ObjectConstants objConstants;
		
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		objConstants.Color = e->Color;
		
		mObjectsCB[i]->CopyData(e->ObjCBIndex, objConstants);
		
	}
}

void RenderApplication::OnResize()
{
    Application::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX ProjMatrix = XMMatrixPerspectiveFovLH(0.25f*Maths::PI, AspectRatio(), 0.1f, 1000);
	XMStoreFloat4x4(&mProj, ProjMatrix);
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
void RenderApplication::OnKeyPressed(WPARAM btnState, int x, int y){}