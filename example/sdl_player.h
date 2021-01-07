#pragma once
#include <Windows.h>
#include <cstdint>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "player_common.h"

class SDLPlayer {
public:
	SDLPlayer();

	~SDLPlayer();

	void SetWindow(void* handle);

	void Render(uint8_t* data, uint32_t width, uint32_t height);

	void SetPixFormat(PixFormat format);

private:
	void Init();
	void Destroy();
	void InitRender();
	void InitTexture();

private:
	uint32_t width_{};
	uint32_t height_{};

	PixFormat pix_format_{ I420 };
	bool format_change_{ false };
	SDL_Window* window_{}; // 
	SDL_Renderer* renderer_{}; //
	SDL_Texture* texture_{}; // 

	HWND _hwnd;
	std::unordered_map<PixFormat, uint32_t> pixel_format_map_;
};
