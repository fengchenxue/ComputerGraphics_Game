#include "Renderer.h"
#include <d3dcompiler.h>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
void Renderer::CreateVertexBuffer()
{
	Vertex vertices[] = {
		{ { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};
	//create vertex buffer description
	D3D11_BUFFER_DESC bd = { 0 };
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.ByteWidth = sizeof(vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	//create vertex buffer data
	D3D11_SUBRESOURCE_DATA initData = { 0 };
	initData.pSysMem = vertices;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	//create vertex buffer
	device->CreateBuffer(&bd, &initData, &vertexBuffer);
}

void Renderer::CreateShaders()
{
	//compile vertex shader and store it in vsBlob,then create vertex shader
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
	D3DCompileFromFile(L"VertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", 0, 0, vsBlob.GetAddressOf(), nullptr);
	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);

	//create input layout
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);


	//compile pixel shader and store it in psBlob,then create pixel shader
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
	D3DCompileFromFile(L"PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", 0, 0, psBlob.GetAddressOf(), nullptr);
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

	//set the vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	//set the primitive topology
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//set the input layout
	context->IASetInputLayout(inputLayout.Get());
	//set the shaders
	context->VSSetShader(vertexShader.Get(), NULL, 0);
	context->PSSetShader(pixelShader.Get(), NULL, 0);

	//create shader reflection
	Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
	D3DReflect(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void**)reflection.GetAddressOf());
	//get the shader description
	D3D11_SHADER_DESC sdc;
	reflection->GetDesc(&sdc);
	for (UINT i = 0; i < sdc.ConstantBuffers; i++) {
		//get the constant buffer
		ID3D11ShaderReflectionConstantBuffer* reflectionConstantBuffer = reflection->GetConstantBufferByIndex(i);
		//get the constant buffer description
		D3D11_SHADER_BUFFER_DESC sbd;
		reflectionConstantBuffer->GetDesc(&sbd);

		//create a constant buffer manager for each constant buffer
		ConstantBufferManager_single constantBufferManager;
		constantBufferManager.name = sbd.Name;

		//Store total size of the constant buffer
		UINT totalSize = 0;

		//iterate through all the variables in the constant buffer
		for (UINT j = 0; j < sbd.Variables; j++) {
			ID3D11ShaderReflectionVariable * variable = reflectionConstantBuffer->GetVariableByIndex(j);

			D3D11_SHADER_VARIABLE_DESC svd;
			variable->GetDesc(&svd);

			ConstantBufferVariableInfo bufferVariable;
			bufferVariable.offset = svd.StartOffset;
			bufferVariable.size = svd.Size;

			constantBufferManager.constantBufferVariableInfoMap.insert({ svd.Name,bufferVariable });

			totalSize += svd.Size;
		}
		//align the size of the constant buffer to 16 bytes
		UINT allignedSize = (totalSize + 15) & ~15;

		//create constant buffer
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.ByteWidth = allignedSize;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
		device->CreateBuffer(&bd, nullptr, constantBuffer.GetAddressOf());

		//store the constant buffer in the map
		constantBufferMap.insert({ constantBufferManager.name, constantBuffer });
		//store the constant buffer manager in the vector
		constantBufferManager_collection.push_back(constantBufferManager);
	}


	//send the data to the GPU
	//context->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

}

void Renderer::updateConstantBuffer(std::string bufferName, std::string variableName, void* data, size_t dataSize)
{

	for (size_t i = 0; i < constantBufferManager_collection.size(); i++)
	{
		//find the constant buffer manager with the given name
		if (constantBufferManager_collection[i].name == bufferName)
		{
			ConstantBufferVariableInfo cbInfo = constantBufferManager_collection[i].constantBufferVariableInfoMap[variableName];

			//map the constant buffer
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			//constantBufferMap stores the constant buffer and is initialized in CreateShaders
			context->Map(constantBufferMap[bufferName].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			unsigned char* dataPtr = static_cast<unsigned char*>(mappedResource.pData);
			memcpy(dataPtr + cbInfo.offset, data, dataSize);
			context->Unmap(constantBufferMap[bufferName].Get(), 0);

			break;
		}
	}


	////update constant buffer data
	//constantBufferData.time =_time;
	//for (int i = 0; i < 4; i++)
	//{
	//	float angle = _time + (i * M_PI / 2.0f);
 //       constantBufferData.lights[i] = DirectX::XMFLOAT4(400.f + (cosf(angle) * 120.f), 300.f + (sinf(angle) * 90.f), 0.f, 0.f);
	//}


	////send the data to the GPU
	//D3D11_MAPPED_SUBRESOURCE msr;
	//context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	//memcpy(msr.pData, &constantBufferData, sizeof(ConstantBuffer));
	//context->Unmap(constantBuffer.Get(), 0);


}


void Renderer::Initialize(Window& window)
{
	//find the best adapter
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapterf;
	std::vector<IDXGIAdapter1*> adapters;
	Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)factory.GetAddressOf());
	for (int i = 0; factory->EnumAdapters1(i, adapterf.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++)
	{
		adapters.push_back(adapterf.Get());
	}
	size_t maxVideoMemory = 0;
	for (auto local_adapter : adapters)
	{
		DXGI_ADAPTER_DESC1 desc;
		local_adapter->GetDesc1(&desc);
		if (desc.DedicatedVideoMemory > maxVideoMemory)
		{
			maxVideoMemory = desc.DedicatedVideoMemory;
			adapter = local_adapter;
		}
	}

	//Create configuration for SwapChain
	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount = 1;
	scd.BufferDesc.Width = window.width;
	scd.BufferDesc.Height = window.height;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = window.hwnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed = window.fullscreen ? FALSE : TRUE;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//Create Device, SwapChain and DeviceContext according to DXGI_SWAP_CHAIN_DESC
	D3D11CreateDeviceAndSwapChain(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL, D3D11_CREATE_DEVICE_DEBUG,
		NULL, 0, D3D11_SDK_VERSION, &scd, &swapChain, &device, NULL, &context);

	swapChain->SetFullscreenState(window.fullscreen, NULL);

	//get the back buffer and its render target view
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuffer.GetAddressOf());
	device->CreateRenderTargetView(backbuffer.Get(), NULL, backbufferRenderTargetView.GetAddressOf());

	//create configuration for depth buffer
	D3D11_TEXTURE2D_DESC dbd = {};
	dbd.Width = window.width;
	dbd.Height = window.height;
	dbd.MipLevels = 1;
	dbd.ArraySize = 1;
	dbd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dbd.SampleDesc.Count = 1;
	dbd.SampleDesc.Quality = 0;
	dbd.Usage = D3D11_USAGE_DEFAULT;
	dbd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dbd.CPUAccessFlags = 0;
	dbd.MiscFlags = 0;


	//create depth buffer and its depth stencil view
	device->CreateTexture2D(&dbd, NULL, depthbuffer.GetAddressOf());
	device->CreateDepthStencilView(depthbuffer.Get(), NULL, depthStencilView.GetAddressOf());

	//send command to GPU for rendering
	context->OMSetRenderTargets(1, backbufferRenderTargetView.GetAddressOf(), depthStencilView.Get());

	//create configuration for Viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(window.width);
	viewport.Height = static_cast<float>(window.height);
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	//apply the configuration to the context
	context->RSSetViewports(1, &viewport);



	//create configuration for rasterizer state
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	device->CreateRasterizerState(&rd, rasterizerState.GetAddressOf());
	//apply the configuration to the context
	context->RSSetState(rasterizerState.Get());

	//create configuration for depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	device->CreateDepthStencilState(&dsd, depthStencilState.GetAddressOf());
	//apply the configuration to the context
	context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	////create configuration for blend state
	//D3D11_BLEND_DESC bd = {};
	//device->CreateBlendState(&bd, blendState.GetAddressOf());
	////apply the configuration to the context
	//context->OMSetBlendState(blendState.Get(), 0, 0xffffffff);

	CreateVertexBuffer();
	CreateShaders();


}

void Renderer::Render()
{


	//draw the vertex buffer
	context->Draw(3, 0);

}

void Renderer::cleanup()
{
	float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	context->ClearRenderTargetView(backbufferRenderTargetView.Get(), clearColor);
	context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Renderer::present()
{
	swapChain->Present(0, 0);
}