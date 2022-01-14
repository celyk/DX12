#include"stdafx.h"

#include"Platform.h"
#include"Renderer.h"

#include<iostream>

// this will only call release if an object exists (prevents exceptions calling release on non existant objects)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

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

	bool waitForPreviousFrame(){
		HRESULT hr;

		// swap the current rtv buffer index so we draw on the correct buffer
		frameIndex = swapChain->GetCurrentBackBufferIndex();

		// if the current fence value is still less than "fenceValue", then we know the GPU has not finished executing
		// the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
		if( fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex] ){
			// we have the fence create an event which is signaled once the fence's current value is "fenceValue"
			hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
			if( FAILED(hr) )
				return false;

			// We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
			// has reached "fenceValue", we know the command queue has finished executing
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		// increment fenceValue for next frame
		fenceValue[frameIndex]++;
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

		// -- Create a direct command queue -- //
		D3D12_COMMAND_QUEUE_DESC cqDesc = {}; // we will be using all the default values
		cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // direct means the gpu can directly execute this command queue

		hr = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue)); // create the command queue
		if( FAILED(hr) )
			return false;

		if( !createSwapChain(dxgiFactory, window) )
			return false;
		
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = frameBufferCount; // number of descriptors for this heap.
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // this heap is a render target view heap
		// This heap will not be directly referenced by the shaders (not shader visible), as this will store the output from the pipeline
		// otherwise we would set the heap's flag to D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
		if( FAILED(hr) )
			return false;


		// get a handle to the first descriptor in the descriptor heap. a handle is basically a pointer
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); //starting address
		// get the size of a descriptor in this heap (this is a rtv heap, so only rtv descriptors should be stored in it.
		// descriptor sizes may vary from device to device, which is why there is no set size and we must ask the 
		// device to give us the size. we will use this size to increment a descriptor handle offset
		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); //stride

		// Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
		for( int n = 0; n < frameBufferCount; n++ ){
			// first we get the n'th buffer in the swap chain and store it in the n'th
			// position of our ID3D12Resource array
			hr = swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n]));
			if( FAILED(hr) )
				return false;

			// then we "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
			device->CreateRenderTargetView(renderTargets[n], nullptr, rtvHandle);
			// we increment the rtv handle by the rtv descriptor size we got above
			rtvHandle.Offset(1, rtvDescriptorSize);

			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[n]));
			if( FAILED(hr) )
				return false;

			// create the fences
			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[n]));
			if( FAILED(hr) )
				return false;
			fenceValue[n] = 0; // initialize the fence value array
		}

		// create a handle to a fence event
		fenceEvent = CreateEvent(0, FALSE, FALSE, nullptr); //manual resetting
		if( !fenceEvent )
			return false;

		//DIRECT specifies commands which the gpu directly executes, BUNDLE specifies a group of commands
		hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], nullptr, IID_PPV_ARGS(&commandList));
		if( FAILED(hr) )
			return false;
		// command lists are created in the recording state. our main loop will set it up for recording again so close it now
		commandList->Close();

		return true;
	}

	bool Renderer::render(){
		HRESULT hr;

		// We have to wait for the gpu to finish with the command allocator before we reset it
		waitForPreviousFrame();

		hr = commandAllocator[frameIndex]->Reset();
		if( FAILED(hr) )
			return false;

		// reset the command list. by resetting the command list we are putting it into
		// a recording state so we can start recording commands into the command allocator.
		// the command allocator that we reference here may have multiple command lists
		// associated with it, but only one can be recording at any time. Make sure
		// that any other command lists associated to this command allocator are in
		// the closed state (not recording).
		// Here you will pass an initial pipeline state object as the second parameter,
		// but in this tutorial we are only clearing the rtv, and do not actually need
		// anything but an initial default pipeline, which is what we get by setting
		// the second parameter to NULL
		hr = commandList->Reset(commandAllocator[frameIndex], nullptr); //commandAllocator must be respecified as if this is a new commandlist
		if( FAILED(hr) )
			return false;


		// here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

		// transition the "frameIndex" render target from the PRESENT state to the RENDER_TARGET state so the command list draws to it starting from here
		CD3DX12_RESOURCE_BARRIER rb_desc = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &rb_desc);

		// here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
		// handle/pointer to current frame's rtv
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

		// set the render target for the output merger stage (the output of the pipeline)
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr); //final argument is to set DepthStencil buffer

		// Clear the render target by using the ClearRenderTargetView command
		const float clearColor[] = {1.0f, 0.2f, 1.0f, 1.0f};
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr); //last 2 arguments specify clearing region. null means clear the entire buffer

		// transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
		// warning if present is called on the render target when it's not in the present state
		rb_desc = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &rb_desc);

		hr = commandList->Close();
		if( FAILED(hr) ) 
			return false;

		// create an array of command lists (only one command list here)
		ID3D12CommandList* ppCommandLists[] = { commandList };

		commandQueue->ExecuteCommandLists(1, ppCommandLists);


		// this command goes in at the end of our command queue. we will know when our command queue 
		// has finished because the fence value will be set to "fenceValue" from the GPU since the command
		// queue is being executed on the GPU
		hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
		if( FAILED(hr) )
			return false;

		// present the current backbuffer
		hr = swapChain->Present(0, 0);
		if( FAILED(hr) )
			return false;

		return true;
	}

	void Renderer::shutdown(){
		// wait for the gpu to finish all frames
		for( int i = 0; i < frameBufferCount; ++i ) {
			frameIndex = i;
			waitForPreviousFrame();
		}

		SAFE_RELEASE(device);
		SAFE_RELEASE(swapChain);
		SAFE_RELEASE(commandQueue);
		SAFE_RELEASE(rtvDescriptorHeap);
		SAFE_RELEASE(commandList);

		for( int i = 0; i < frameBufferCount; ++i ){
			SAFE_RELEASE(renderTargets[i]);
			SAFE_RELEASE(commandAllocator[i]);
			SAFE_RELEASE(fence[i]);
		};
	}
}