#include "d3d9_player.h"

#include <iostream>
extern "C" {
#include "libyuv.h"
}

#pragma comment(lib, "d3d9.lib")

D3D9Player::D3D9Player() {
	_formatMap[I420] = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');
	_formatMap[NV12] = (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');
	_formatMap[NV21] = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');

	// fout.open("test_123.yuv", std::ios::binary | std::ios::out);
}

D3D9Player::~D3D9Player() {
    DesrtoySurface();
    Cleanup();
	// fout.close();
}

void D3D9Player::SetWindow(void* handle) {
    render_window_ = (HWND)(handle);
}

void D3D9Player::SetPixFormat(PixFormat format) {
	if (pix_format_ != format) {
		_formatChange = true;
		pix_format_ = format;
	}
}

void D3D9Player::PlayNV12(uint8_t* data, uint32_t width, uint32_t height) {
	D3DLOCKED_RECT d3d_rect;
	if (d3d9_surface_ == nullptr) {
		return;
	}
	auto hr = d3d9_surface_->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(hr)) {
		std::cout << "lock rect error" << std::endl;
		return;
	}

	int y_stride = d3d_rect.Pitch;
	int uv_stride = y_stride / 2;
	
	uint8_t* y_dest = (uint8_t*)d3d_rect.pBits;
	uint8_t* uv_dest = y_dest + height_ * y_stride;

	for (uint32_t i = 0; i < height_; i++) {
		memcpy(y_dest + i * y_stride, data + i * width_, width_);
	}

	for (uint32_t i = 0; i < height_; i++) {
		memcpy(uv_dest + i * uv_stride, data + width_ * height_ + i * width_ / 2, width_);
	}

	hr = d3d9_surface_->UnlockRect();
	if (FAILED(hr)) {
		std::cout << "unlock rect error" << std::endl;
		return;
	}
	Play();
}
void D3D9Player::PlayNV21(uint8_t* data, uint32_t width, uint32_t height) {
	D3DLOCKED_RECT d3d_rect;
	if (d3d9_surface_ == nullptr) {
		return;
	}
	auto hr = d3d9_surface_->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(hr)) {
		std::cout << "lock rect error" << std::endl;
		return;
	}

	int y_stride = d3d_rect.Pitch;
	int v_stride = y_stride / 2;
	int u_stride = y_stride / 2;

	uint8_t* y_dest = (uint8_t*)d3d_rect.pBits;
	uint8_t* v_dest = y_dest + height_ * y_stride;
	uint8_t* u_dest = v_dest + height_ / 2 * v_stride;

	uint8_t* srcY = data;
	uint8_t* srcVU = data + width * height;

	uint8_t* destY = _dest;
	uint8_t* destU = _dest + width * height;
	uint8_t* destV = _dest + width * height * 5 / 4;

	libyuv::NV21ToI420(srcY, width, srcVU, width, destY, width, destU,
		width / 2, destV, width / 2, width, height);
	
	for (uint32_t i = 0; i < height_; i++) {
		memcpy(y_dest + i * y_stride, _dest + i * width_, width_);
	}

	for (uint32_t i = 0; i < height_ / 2; i++) {
		memcpy(u_dest + i * u_stride, _dest + width_ * height_ + i * width_ / 2,
			width_ / 2);
	}

	for (uint32_t i = 0; i < height_ / 2; i++) {
		memcpy(v_dest + i * v_stride, _dest + width_ * height_ * 5 / 4 + i * width_ / 2,
			width_ / 2);
	}

	hr = d3d9_surface_->UnlockRect();
	if (FAILED(hr)) {
		std::cout << "unlock rect error" << std::endl;
		return;
	}
	Play();
}

void D3D9Player::PlayI420(uint8_t* data, uint32_t width, uint32_t height) {
	D3DLOCKED_RECT d3d_rect;
	if (d3d9_surface_ == nullptr) {
		return;
	}
	auto hr = d3d9_surface_->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(hr)) {
		std::cout << "lock rect error" << std::endl;
		return;
	}

	int y_stride = d3d_rect.Pitch;
	int v_stride = y_stride / 2;
	int u_stride = y_stride / 2;

	uint8_t* y_dest = (uint8_t*)d3d_rect.pBits;
	uint8_t* v_dest = y_dest + height_ * y_stride;
	uint8_t* u_dest = v_dest + height_ / 2 * v_stride;

	for (uint32_t i = 0; i < height_; i++) {
		memcpy(y_dest + i * y_stride, data + i * width_, width_);
	}

	for (uint32_t i = 0; i < height_ / 2; i++) {
		memcpy(v_dest + i * v_stride, data + width_ * height_ + i * width_ / 2,
			width_ / 2);
	}

	for (uint32_t i = 0; i < height_ / 2; i++) {
		memcpy(u_dest + i * u_stride, data + width_ * height_ * 5 / 4 + i * width_ / 2,
			width_ / 2);
	}

	hr = d3d9_surface_->UnlockRect();
	if (FAILED(hr)) {
		std::cout << "unlock rect error" << std::endl;
		return;
	}
	Play();
}

void D3D9Player::Play() {
	d3d9_device_->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0),
		1.0f, 0);
	d3d9_device_->BeginScene();

	IDirect3DSurface9* d3d_surface = NULL;
	d3d9_device_->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
		&d3d_surface);

	d3d9_device_->StretchRect(d3d9_surface_, NULL, d3d_surface, &view_rect_,
		D3DTEXF_LINEAR);
	d3d9_device_->EndScene();
	d3d9_device_->Present(NULL, &_destRect, NULL, NULL);
	d3d_surface->Release();
}

void D3D9Player::Render(uint8_t* data, uint32_t width, uint32_t height) {
    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        SetViewRect(width_, height_);
        InitRender();
        CreateSurface();
		_formatChange = false;
    }
	else if (_formatChange) {
		CreateSurface();
		_formatChange = false;
	}
	switch (pix_format_) {
	case I420:
		PlayI420(data, width, height);
		break;
	case NV12:
		PlayNV12(data, width, height);
		break;
	case NV21:
		PlayNV21(data, width, height);
		break;
	default:
		break;
	}
}

void D3D9Player::InitRender() {
    std::cout << "init render" << std::endl;
    InitializeCriticalSection(&critical_);
    Cleanup();

    d3d9_ = Direct3DCreate9(D3D_SDK_VERSION);
    if (d3d9_ == nullptr) {
        return;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.hDeviceWindow = (HWND)render_window_;
    d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = width_;
    d3dpp.BackBufferHeight = height_;

    HRESULT hr = d3d9_->CreateDevice(
        D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, render_window_,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3d9_device_);
}

void D3D9Player::Cleanup() {
    EnterCriticalSection(&critical_);
    if (d3d9_surface_) {
        d3d9_surface_->Release();
        d3d9_surface_ = nullptr;
    }
    if (d3d9_device_) {
        d3d9_device_->Release();
        d3d9_device_ = nullptr;
    }
    if (d3d9_) {
        d3d9_->Release();
        d3d9_ = nullptr;
    }
    LeaveCriticalSection(&critical_);
}

void D3D9Player::CreateSurface() {
    if (d3d9_device_ == nullptr) {
        return;
    }
    HRESULT hr = d3d9_device_->CreateOffscreenPlainSurface(
        width_, height_, _formatMap[pix_format_],
        D3DPOOL_DEFAULT, &d3d9_surface_, NULL);
    if (FAILED(hr)) {
        std::cout << "create plain surface error" << std::endl;
        return;
    }
    std::cout << "create surface" << std::endl;
	if (_dest) {
		delete[] _dest;
		_dest = nullptr;
	}
	_dest = new uint8_t[width_ * height_ * 3 / 2];
	// _destRect
	RECT rect;
	// 只获得窗口客户区的大小
	GetClientRect(render_window_, &rect);
	uint32_t window_width = rect.right - rect.left;
	uint32_t window_height = rect.bottom - rect.top;
	int rate = width_ * window_height - height_ * window_width;
	if (rate < 0)
	{
		uint32_t render_width = width_ * window_height / height_;
		_destRect.left = rect.left + (window_width - render_width) / 2;
		_destRect.top = rect.top;
		_destRect.right = _destRect.left + render_width;
		_destRect.bottom = _destRect.top + window_height;
	}
	else if (rate > 0)
	{
		uint32_t render_height = height_ * window_width / width_;
		_destRect.left = rect.left;
		_destRect.top = rect.top + +(window_height - render_height) / 2;
		_destRect.right = _destRect.left + window_width;
		_destRect.bottom = _destRect.top + render_height;
	}
	else
	{
		_destRect.left = rect.left;
		_destRect.top = rect.top;
		_destRect.right = rect.right;
		_destRect.bottom = rect.bottom;
	}
}

void D3D9Player::DesrtoySurface() {
    EnterCriticalSection(&critical_);
    if (d3d9_surface_) {
        d3d9_surface_->Release();
        d3d9_surface_ = nullptr;
    }
    LeaveCriticalSection(&critical_);
	if (_dest) {
		delete[] _dest;
		_dest = nullptr;
	}
}

void D3D9Player::SetViewRect(uint32_t width, uint32_t height) {
    view_rect_.left = 0;
    view_rect_.top = 0;
    view_rect_.right = width;
    view_rect_.bottom = height;
}
