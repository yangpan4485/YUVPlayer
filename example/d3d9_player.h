#pragma once

#include <cstdint>
#include <d3d9.h>
#include <unordered_map>
#include <fstream>

#include "player_common.h"

class D3D9Player {
public:
    D3D9Player();

    ~D3D9Player();

    void SetWindow(void* handle);

    void Render(uint8_t* data, uint32_t width, uint32_t height);

    void SetPixFormat(PixFormat format);

private:
    void InitRender();

    void CreateSurface();

    void DesrtoySurface();

    void Cleanup();

    void SetViewRect(uint32_t width, uint32_t height);

	void PlayNV12(uint8_t* data, uint32_t width, uint32_t height);
	void PlayNV21(uint8_t* data, uint32_t width, uint32_t height);
	void PlayI420(uint8_t* data, uint32_t width, uint32_t height);
	void Play();

private:
    uint32_t width_{};
    uint32_t height_{};
    PixFormat pix_format_ = I420;

    CRITICAL_SECTION critical_{};
    IDirect3D9* d3d9_{};
    IDirect3DDevice9* d3d9_device_{};
    IDirect3DSurface9* d3d9_surface_{};
    RECT view_rect_{};
    HWND render_window_{};
	bool _formatChange{ false };
	std::unordered_map<PixFormat, D3DFORMAT> _formatMap{};
	uint8_t* _dest{};
	std::ofstream fout;
	RECT _destRect{};
};
