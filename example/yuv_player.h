#pragma once

#include <memory>

#if 0
#define DISABLE_COPY(Class) \
Class(const Class &); \
Class &operator=(const Class &)

class Widget
{
public:
    int* pi;
private:
    DISABLE_COPY(Widget);
};
#endif

class YUVPlayer {
public:
    YUVPlayer();
    ~YUVPlayer();

    void Init();
    bool CreateDUIWindow();
    void ShowWindow();

private:
    YUVPlayer(const YUVPlayer&) = delete;
    YUVPlayer& operator =(const YUVPlayer&) = delete;

private:
    class YUVPlayerImpl;
    std::unique_ptr<YUVPlayerImpl> _pimpl{};
};