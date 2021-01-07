#include "sdl_player.h"

#include <iostream>

SDLPlayer::SDLPlayer()
{
	Init();
}

SDLPlayer::~SDLPlayer()
{
	Destroy();
}

void SDLPlayer::SetWindow(void* handle)
{
	_hwnd = (HWND)handle;
	window_ = SDL_CreateWindowFrom(handle);
	if (window_ == nullptr) {
		std::cout << SDL_GetError() << std::endl;
	}
	InitRender();
}

void SDLPlayer::Render(uint8_t* data, uint32_t width, uint32_t height)
{
	if (width != width_ || height != height_ || format_change_)
	{
		width_ = width;
		height_ = height;
		InitTexture();
	}
	SDL_UpdateTexture(texture_, NULL, data, width_);
	RECT rect;
	SDL_Rect sdlRect;
	// 只获得窗口客户区的大小
	GetClientRect(_hwnd, &rect);
	uint32_t window_width = rect.right - rect.left;
	uint32_t window_height = rect.bottom - rect.top;
	int rate = width * window_height - height * window_width;
	if (rate < 0)
	{
		uint32_t render_width = width * window_height / height;
		sdlRect.x = rect.left + (window_width - render_width) / 2;
		sdlRect.y = rect.top;
		sdlRect.w = render_width;
		sdlRect.h = window_height;
	}
	else if (rate > 0)
	{
		uint32_t render_height = height * window_width / width;
		sdlRect.x = rect.left;
		sdlRect.y = rect.top + +(window_height - render_height) / 2;
		sdlRect.w = window_width;
		sdlRect.h = render_height;
	}
	else
	{
		sdlRect.x = rect.left;
		sdlRect.y = rect.top;
		sdlRect.w = rect.right - rect.left;
		sdlRect.h = rect.bottom - rect.top;
	}
	SDL_RenderClear(renderer_);
	SDL_RenderCopy(renderer_, texture_, NULL, &sdlRect);
	SDL_RenderPresent(renderer_);
}

void SDLPlayer::SetPixFormat(PixFormat format)
{
	if (pix_format_ != format) {
		pix_format_ = format;
		format_change_ = true;
	}
}

void SDLPlayer::Init()
{
	pixel_format_map_[I420] = SDL_PIXELFORMAT_IYUV;
	pixel_format_map_[NV12] = SDL_PIXELFORMAT_NV12;
	pixel_format_map_[NV21] = SDL_PIXELFORMAT_NV21;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1) {
		std::cout << "SDL2 init failed" << std::endl;
	}
}

void SDLPlayer::Destroy()
{
	if (window_) {
		SDL_DestroyWindow(window_);
	}
	if (renderer_) {
		SDL_DestroyRenderer(renderer_);
	}
	if (texture_) {
		SDL_DestroyTexture(texture_);
	}
	SDL_Quit();

}

void SDLPlayer::InitRender()
{
	if (renderer_) {
		SDL_DestroyRenderer(renderer_);
	}
	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

}

void SDLPlayer::InitTexture()
{
	if (texture_) {
		SDL_DestroyTexture(texture_);
	}
	texture_ = SDL_CreateTexture(renderer_, pixel_format_map_[pix_format_], SDL_TEXTUREACCESS_STREAMING, width_, height_);
}
