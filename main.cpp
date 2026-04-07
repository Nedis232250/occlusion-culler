#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <chrono>
#include <iostream>
#include <vector>
#include "utils.hpp"
#include <unordered_map>
#include <fstream>
#include "fastnoiselite.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#define rx 0.0f
#define ry 0.0f

bool running = true;
unsigned int width = 1920;
unsigned int height = 1080;
unsigned int occluder_width = 480;
unsigned int occluder_height = 270;
HWND hwnd;
HRESULT hr;
int hr_code = 0;
unsigned int vertex_count = 3;

std::vector<unsigned int> screen(width* height); // tightly packed BGRA
std::vector<unsigned int> screen2(occluder_width* occluder_height); // tightly packed BGRA
void* data_gpu_copy;

using namespace Microsoft::WRL;


PAINTSTRUCT ps_;
BITMAPINFO bmi = {};

void attach_console() {
	if (AllocConsole()) {
		FILE* file;
		freopen_s(&file, "CONOUT$", "w", stdout);
	}
}

LRESULT w_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_QUIT: {
		running = false;
		PostQuitMessage(0);
		break;
	} case WM_DESTROY: {
		running = false;
		PostQuitMessage(0);
		break;
	} default: {
		return DefWindowProc(wnd, msg, wparam, lparam);
	}
	}
}

int WinMain(HINSTANCE h_instance, HINSTANCE p_instance, LPSTR cmdln, int n_cmd_show) {
	//SetProcessDPIAware();
	WNDCLASS w_class = {};
	w_class.lpfnWndProc = w_proc;
	w_class.lpszClassName = L"class";
	w_class.style = CS_VREDRAW | CS_HREDRAW;
	w_class.hInstance = h_instance;

	RegisterClassW(&w_class);

	std::vector<float> vertices_loaded;
	load_vertices(vertices_loaded, "hello.world");
	std::vector<float> vertices = vertices_loaded;
	CPU_vertex_transformation(vertices_loaded, vertices, vertices_loaded.size() / 7.0f, rx, ry);

	hwnd = CreateWindowExW(0, L"class", L"hello?", WS_OVERLAPPEDWINDOW, 0, 0, width, height, 0, 0, h_instance, 0);
	ShowWindow(hwnd, n_cmd_show);
	attach_console();

	std::vector<float> non_colored_vertices = load_indices(vertices);

	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> ctx;

	hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &ctx);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11Texture2D> RTV_tex;
	D3D11_TEXTURE2D_DESC RTV_tex_desc = {};
	RTV_tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	RTV_tex_desc.ArraySize = 1;
	RTV_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
	RTV_tex_desc.CPUAccessFlags = 0;
	RTV_tex_desc.Height = height;
	RTV_tex_desc.MipLevels = 1;
	RTV_tex_desc.MiscFlags = 0;
	RTV_tex_desc.SampleDesc.Count = 1;
	RTV_tex_desc.SampleDesc.Quality = 0;
	RTV_tex_desc.Usage = D3D11_USAGE_DEFAULT;
	RTV_tex_desc.Width = width;

	hr = device->CreateTexture2D(&RTV_tex_desc, nullptr, &RTV_tex);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11Texture2D> RTV_occluder;
	D3D11_TEXTURE2D_DESC RTV_occluder_desc = RTV_tex_desc;
	RTV_occluder_desc.Width = occluder_width;
	RTV_occluder_desc.Height = occluder_height;

	hr = device->CreateTexture2D(&RTV_occluder_desc, nullptr, &RTV_occluder);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11Texture2D> DSV_tex;
	D3D11_TEXTURE2D_DESC DSV_tex_desc = RTV_tex_desc;
	DSV_tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DSV_tex_desc.Format = DXGI_FORMAT_D32_FLOAT;
	hr = device->CreateTexture2D(&DSV_tex_desc, nullptr, &DSV_tex);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11Texture2D> DSV_occluder;
	D3D11_TEXTURE2D_DESC DSV_occluder_desc = DSV_tex_desc;
	DSV_occluder_desc.Width = occluder_width;
	DSV_occluder_desc.Height = occluder_height;
	hr = device->CreateTexture2D(&DSV_occluder_desc, nullptr, &DSV_occluder);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;


	//
	// DEBUG
	//

	ComPtr<ID3D11Texture2D> RTV_readback;
	D3D11_TEXTURE2D_DESC RTV_readback_desc = RTV_tex_desc;
	RTV_readback_desc.BindFlags = 0;
	RTV_readback_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	RTV_readback_desc.MiscFlags = 0;
	RTV_readback_desc.Usage = D3D11_USAGE_STAGING;

	hr = device->CreateTexture2D(&RTV_readback_desc, nullptr, &RTV_readback);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11Texture2D> RTV_occluder_readback;
	D3D11_TEXTURE2D_DESC RTV_occluder_readback_desc = RTV_occluder_desc;
	RTV_occluder_readback_desc.BindFlags = 0;
	RTV_occluder_readback_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	RTV_occluder_readback_desc.MiscFlags = 0;
	RTV_occluder_readback_desc.Usage = D3D11_USAGE_STAGING;

	hr = device->CreateTexture2D(&RTV_occluder_readback_desc, nullptr, &RTV_occluder_readback);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	//
	// DEBUG
	//

	ComPtr<ID3D11Buffer> vertex_buffer;
	D3D11_BUFFER_DESC vertex_buffer_desc;
	vertex_buffer_desc.ByteWidth = non_colored_vertices.size() * sizeof(float);
	vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertex_buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	vertex_buffer_desc.CPUAccessFlags = 0;
	vertex_buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	vertex_buffer_desc.StructureByteStride = sizeof(float);

	D3D11_SUBRESOURCE_DATA vertices_upload_struct = {};
	vertices_upload_struct.pSysMem = non_colored_vertices.data();

	hr = device->CreateBuffer(&vertex_buffer_desc, &vertices_upload_struct, &vertex_buffer);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11VertexShader> vs;
	ComPtr<ID3DBlob> vs_blob;
	ComPtr<ID3DBlob> vs_error_blob;
	hr = D3DCompileFromFile(L"vs.hlsl", nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vs_blob, &vs_error_blob);

	if (FAILED(hr)) {
		OutputDebugStringA("SHIT");
		OutputDebugStringA((char*)vs_error_blob->GetBufferPointer());
		return hr_code;
	}
	hr_code++;

	hr = device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vs);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11PixelShader> ps;
	ComPtr<ID3DBlob> ps_blob;
	ComPtr<ID3DBlob> ps_error_blob;
	hr = D3DCompileFromFile(L"ps.hlsl", nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps_blob, &ps_error_blob);

	if (FAILED(hr)) {
		OutputDebugStringA((char*)ps_error_blob->GetBufferPointer());
		return hr_code;
	}
	hr_code++;

	hr = device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &ps);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11VertexShader> vs_final;
	ComPtr<ID3DBlob> vs_blob_final;
	ComPtr<ID3DBlob> vs_error_blob_final;
	hr = D3DCompileFromFile(L"rendervs.hlsl", nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vs_blob_final, &vs_error_blob_final);

	if (FAILED(hr)) {
		OutputDebugStringA("SHIT");
		OutputDebugStringA((char*)vs_error_blob_final->GetBufferPointer());
		return hr_code;
	}
	hr_code++;

	hr = device->CreateVertexShader(vs_blob_final->GetBufferPointer(), vs_blob_final->GetBufferSize(), nullptr, &vs_final);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11PixelShader> ps_final;
	ComPtr<ID3DBlob> ps_blob_final;
	ComPtr<ID3DBlob> ps_error_blob_final;
	hr = D3DCompileFromFile(L"renderps.hlsl", nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps_blob_final, &ps_error_blob_final);

	if (FAILED(hr)) {
		OutputDebugStringA((char*)ps_error_blob_final->GetBufferPointer());
		return hr_code;
	}
	hr_code++;

	hr = device->CreatePixelShader(ps_blob_final->GetBufferPointer(), ps_blob_final->GetBufferSize(), nullptr, &ps_final);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11ShaderResourceView> vertices_SRV;
	D3D11_SHADER_RESOURCE_VIEW_DESC vertices_SRV_desc;
	vertices_SRV_desc.Format = DXGI_FORMAT_UNKNOWN;
	vertices_SRV_desc.Buffer.ElementOffset = 0;
	vertices_SRV_desc.Buffer.ElementWidth = sizeof(float);
	vertices_SRV_desc.Buffer.FirstElement = 0;
	vertices_SRV_desc.Buffer.NumElements = non_colored_vertices.size();
	vertices_SRV_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

	hr = device->CreateShaderResourceView(vertex_buffer.Get(), &vertices_SRV_desc, &vertices_SRV);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ID3D11ShaderResourceView* SRVs[] = { vertices_SRV.Get() };

	ComPtr<ID3D11RenderTargetView> RTV;
	D3D11_RENDER_TARGET_VIEW_DESC RTV_desc = {};
	RTV_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTV_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	RTV_desc.Texture2D.MipSlice = 0;

	hr = device->CreateRenderTargetView(RTV_tex.Get(), &RTV_desc, &RTV);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11DepthStencilView> DSV;
	D3D11_DEPTH_STENCIL_VIEW_DESC DSV_desc = {};
	DSV_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSV_desc.Format = DXGI_FORMAT_D32_FLOAT;
	DSV_desc.Texture2D.MipSlice = 0;
	DSV_desc.Flags = 0;

	hr = device->CreateDepthStencilView(DSV_tex.Get(), &DSV_desc, &DSV);
	if (FAILED(hr)) {
		return hr;
	}
	hr_code++;

	ComPtr<ID3D11RenderTargetView> RTV_occluder_RTV;
	D3D11_RENDER_TARGET_VIEW_DESC RTV_occluder_RTV_desc = {};
	RTV_occluder_RTV_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTV_occluder_RTV_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	RTV_occluder_RTV_desc.Texture2D.MipSlice = 0;

	hr = device->CreateRenderTargetView(RTV_occluder.Get(), &RTV_occluder_RTV_desc, &RTV_occluder_RTV);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11DepthStencilView> DSV_occluder_DSV;
	D3D11_DEPTH_STENCIL_VIEW_DESC DSV_occluder_DSV_desc = {};
	DSV_occluder_DSV_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSV_occluder_DSV_desc.Format = DXGI_FORMAT_D32_FLOAT;
	DSV_occluder_DSV_desc.Texture2D.MipSlice = 0;
	DSV_occluder_DSV_desc.Flags = 0;

	hr = device->CreateDepthStencilView(DSV_occluder.Get(), &DSV_occluder_DSV_desc, &DSV_occluder_DSV);
	if (FAILED(hr)) {
		return hr;
	}
	hr_code++;

	ComPtr<ID3D11RasterizerState> rs;
	D3D11_RASTERIZER_DESC rs_desc = {};
	rs_desc.FillMode = D3D11_FILL_SOLID;
	rs_desc.CullMode = D3D11_CULL_NONE;
	rs_desc.DepthClipEnable = TRUE;
	device->CreateRasterizerState(&rs_desc, &rs);

	float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	D3D11_VIEWPORT vp = {};
	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	D3D11_VIEWPORT ocvp = {};
	ocvp.Width = occluder_width;
	ocvp.Height = occluder_height;
	ocvp.MinDepth = 0.0f;
	ocvp.MaxDepth = 1.0f;

	D3D11_MAPPED_SUBRESOURCE mapped = {};

	ComPtr<ID3D11Buffer> triangle_visibility_buf;
	D3D11_BUFFER_DESC triangle_visibility_buf_desc = {};
	triangle_visibility_buf_desc.ByteWidth = (vertices.size() / 21) * sizeof(unsigned int);
	triangle_visibility_buf_desc.Usage = D3D11_USAGE_DEFAULT;
	triangle_visibility_buf_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	triangle_visibility_buf_desc.CPUAccessFlags = 0;
	triangle_visibility_buf_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	triangle_visibility_buf_desc.StructureByteStride = 0;

	hr = device->CreateBuffer(&triangle_visibility_buf_desc, nullptr, &triangle_visibility_buf);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11UnorderedAccessView> triangle_visibility_UAV;
	D3D11_UNORDERED_ACCESS_VIEW_DESC triangle_visibility_UAV_desc = {};
	triangle_visibility_UAV_desc.Buffer.FirstElement = 0;
	triangle_visibility_UAV_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	triangle_visibility_UAV_desc.Buffer.NumElements = vertices.size() / 21;
	triangle_visibility_UAV_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	triangle_visibility_UAV_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	hr = device->CreateUnorderedAccessView(triangle_visibility_buf.Get(), &triangle_visibility_UAV_desc, &triangle_visibility_UAV);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11ShaderResourceView> triangle_visibility_SRV;
	D3D11_SHADER_RESOURCE_VIEW_DESC triangle_visibility_SRV_desc = {};
	triangle_visibility_SRV_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	triangle_visibility_SRV_desc.BufferEx.FirstElement = 0;
	triangle_visibility_SRV_desc.BufferEx.NumElements = vertices.size() / 21;
	triangle_visibility_SRV_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	triangle_visibility_SRV_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;

	hr = device->CreateShaderResourceView(triangle_visibility_buf.Get(), &triangle_visibility_SRV_desc, &triangle_visibility_SRV);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	// DEBUG
	UINT clear_val_uint[4] = { 0, 0, 0, 0 };
	ID3D11UnorderedAccessView* null_uav[] = { nullptr };

	ComPtr<ID3D11Query> frame_query;
	D3D11_QUERY_DESC query_desc = {};
	query_desc.Query = D3D11_QUERY_EVENT;
	device->CreateQuery(&query_desc, &frame_query);

	ComPtr<IDXGISwapChain> swapchain;
	ComPtr<IDXGIDevice> dxgi_device;
	ComPtr<IDXGIAdapter> adapter;
	ComPtr<IDXGIFactory> factory;

	device.As(&dxgi_device);
	dxgi_device->GetAdapter(&adapter);
	adapter->GetParent(IID_PPV_ARGS(&factory));

	DXGI_SWAP_CHAIN_DESC sc_desc = {};
	sc_desc.BufferCount = 2;
	sc_desc.BufferDesc.Width = width;
	sc_desc.BufferDesc.Height = height;
	sc_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sc_desc.OutputWindow = hwnd;
	sc_desc.SampleDesc.Count = 1;
	sc_desc.Windowed = TRUE;
	sc_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	factory->CreateSwapChain(device.Get(), &sc_desc, &swapchain);

	// get back buffer and make RTV from it
	ComPtr<ID3D11Texture2D> back_buffer;
	swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	device->CreateRenderTargetView(back_buffer.Get(), nullptr, &RTV);

	ComPtr<ID3D11Buffer> color_buffer;
	D3D11_BUFFER_DESC color_buffer_desc = vertex_buffer_desc;
	color_buffer_desc.ByteWidth = vertices.size() * sizeof(float);

	D3D11_SUBRESOURCE_DATA color_data = {};
	color_data.pSysMem = vertices.data();

	hr = device->CreateBuffer(&color_buffer_desc, &color_data, &color_buffer);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11ShaderResourceView> color_SRV;
	D3D11_SHADER_RESOURCE_VIEW_DESC color_SRV_desc = vertices_SRV_desc;
	vertices_SRV_desc.Buffer.NumElements = vertices.size();

	hr = device->CreateShaderResourceView(color_buffer.Get(), &color_SRV_desc, &color_SRV);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	std::vector<unsigned int> culling_avoidance = set_occlusion_viability(vertices, width, height, rx, ry);
	ComPtr<ID3D11Buffer> cull_buffer;
	D3D11_BUFFER_DESC cull_buffer_desc = {};
	cull_buffer_desc.ByteWidth = sizeof(unsigned int) * culling_avoidance.size();
	cull_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	cull_buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	cull_buffer_desc.CPUAccessFlags = 0;
	cull_buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	cull_buffer_desc.StructureByteStride = sizeof(unsigned int);
	
	D3D11_SUBRESOURCE_DATA cull_buffer_data = {};
	cull_buffer_data.pSysMem = culling_avoidance.data();

	hr = device->CreateBuffer(&cull_buffer_desc, &cull_buffer_data, &cull_buffer);
	if (FAILED(hr)) {
		return hr_code;
	}
	hr_code++;

	ComPtr<ID3D11ShaderResourceView> cull_SRV;
	D3D11_SHADER_RESOURCE_VIEW_DESC cull_SRV_desc = {};
	cull_SRV_desc.Format = DXGI_FORMAT_UNKNOWN;
	cull_SRV_desc.Buffer.NumElements = culling_avoidance.size();
	cull_SRV_desc.Buffer.FirstElement = 0;
	cull_SRV_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

	hr = device->CreateShaderResourceView(cull_buffer.Get(), &cull_SRV_desc, &cull_SRV);
	if (FAILED(hr)) {
		return hr;
	}
	hr_code++;

	ID3D11UnorderedAccessView* UAVs[] = { triangle_visibility_UAV.Get() };
	ID3D11ShaderResourceView* FINAL_SRVs[] = { color_SRV.Get(), triangle_visibility_SRV.Get(), cull_SRV.Get() };

	MSG msg = {};
	while (running) {
		while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		auto start = std::chrono::high_resolution_clock::now();

		ctx->ClearUnorderedAccessViewUint(triangle_visibility_UAV.Get(), clear_val_uint);
		ctx->VSSetShader(vs.Get(), nullptr, 0);
		ctx->VSSetShaderResources(0, 1, SRVs);
		ctx->PSSetShader(ps.Get(), nullptr, 0);
		ctx->OMSetRenderTargetsAndUnorderedAccessViews(1, RTV_occluder_RTV.GetAddressOf(), DSV_occluder_DSV.Get(), 1, 1, UAVs, nullptr);
		ctx->ClearRenderTargetView(RTV_occluder_RTV.Get(), clear_color);
		ctx->ClearDepthStencilView(DSV_occluder_DSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		ctx->RSSetViewports(1, &ocvp);
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx->RSSetState(rs.Get());
		ctx->Draw(vertices.size() / 7, 0);
		ctx->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 1, 1, null_uav, nullptr);
		ctx->VSSetShader(vs_final.Get(), nullptr, 0);
		ctx->VSSetShaderResources(0, 3, FINAL_SRVs);
		ctx->PSSetShader(ps_final.Get(), nullptr, 0);
		ctx->OMSetRenderTargets(1, RTV.GetAddressOf(), DSV.Get());
		ctx->ClearRenderTargetView(RTV.Get(), clear_color);
		ctx->ClearDepthStencilView(DSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		ctx->RSSetViewports(1, &vp);
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx->RSSetState(rs.Get());
		ctx->Draw(vertices.size() / 7, 0);
		swapchain->Present(0, 0); // 0,0 = no vsync, no flags

		auto end = std::chrono::high_resolution_clock::now();

		std::cout << "FPS: " << 1000.0f / std::chrono::duration<double, std::milli>(end - start).count() << "\n";
	}

	return 0;
}
