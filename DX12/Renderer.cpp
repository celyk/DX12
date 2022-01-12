#include"stdafx.h"

#include"Platform.h"
#include"Renderer.h"

#include<iostream>

namespace gfx{

	// direct3d stuff
	const int frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering
	ID3D12Device* device; // direct3d device
	IDXGISwapChain3* swapChain; // swapchain used to switch between render targets
	ID3D12CommandQueue* commandQueue; // container for command lists
	ID3D12DescriptorHeap* rtvDescriptorHeap; // a descriptor heap to hold resources like the render targets
	ID3D12Resource* renderTargets[frameBufferCount]; // number of render targets equal to buffer count
	ID3D12CommandAllocator* commandAllocator[frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)
	ID3D12GraphicsCommandList* commandList; // a command list we can record commands into, then execute them to render the frame
	ID3D12Fence* fence[frameBufferCount];    // an object that is locked while our command list is being executed by the gpu. We need as many 
											 //as we have allocators (more if we want to know when the gpu is finished with an asset)
	HANDLE fenceEvent; // a handle to an event when our fence is unlocked by the gpu
	UINT64 fenceValue[frameBufferCount]; // this value is incremented each frame. each fence will have its own value

	int frameIndex; // current rtv we are on

	int rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

	IDXGIAdapter1* findAdapter(IDXGIFactory4* dxgiFactory){
		HRESULT hr;
		IDXGIAdapter1* adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
		int adapterIndex = 0; // we'll start looking for directx 12  compatible graphics devices starting at index 0
		bool adapterFound = false; // set this to true when a good one was found

		// find first hardware gpu that supports d3d 12
		while( dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND ){
			adapterIndex++;

			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) 
				continue;

			// we want a device that is compatible with direct3d 12 (feature level 11 or higher)
			hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
			if( SUCCEEDED(hr) )
				std::wcout << "GPU FOUND: " << desc.Description << std::endl;
				return adapter;
		}

		//if not found
		return nullptr;
	}
	bool createSwapChain(IDXGIFactory4* dxgiFactory, platform::Window* window){
		// -- Create the Swap Chain (double/tripple buffering) -- //
		DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
		backBufferDesc.Width = window->width; // buffer width
		backBufferDesc.Height = window->height; // buffer height
		backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the buffer (rgba 32 bits, 8 bits for each chanel)

		// describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least one sample of course)
		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = frameBufferCount; // number of buffers we have
		swapChainDesc.BufferDesc = backBufferDesc; // our back buffer description
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this says the pipeline will render to this swap chain
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // dxgi will discard the buffer (data) after we call present
		swapChainDesc.OutputWindow = window->hwnd; // handle to our window
		swapChainDesc.SampleDesc = sampleDesc; // our multi-sampling description
		swapChainDesc.Windowed = true; // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps

		IDXGISwapChain* tempSwapChain;

		dxgiFactory->CreateSwapChain(
			commandQueue, // the queue will be flushed once the swap chain is created
			&swapChainDesc, // give it the swap chain description we created above
			&tempSwapChain // store the created swap chain in a temp IDXGISwapChain interface
		);
		swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

		frameIndex = swapChain->GetCurrentBackBufferIndex();

		return true;
	}

	bool Renderer::init(platform::Window* w){
		window = w;

		HRESULT hr;

		// -- Create the Device -- //
		IDXGIFactory4* dxgiFactory;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
		if( FAILED(hr) ) 
			return false;

		IDXGIAdapter1* adapter = findAdapter(dxgiFactory);
		if( !adapter )
			return false;

		// Create the device
		hr = D3D12CreateDevice(
			adapter,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		);
		if( FAILED(hr) )
			return false;

		// -- Create the Command Queue -- //
		D3D12_COMMAND_QUEUE_DESC cqDesc ={}; // we will be using all the default values
		hr = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue)); // create the command queue
		if( FAILED(hr) )
			return false;

		if( !createSwapChain(dxgiFactory, window) )
			return false;
	}
	void Renderer::render(){
	}
	void Renderer::shutdown()
	{
	}
}