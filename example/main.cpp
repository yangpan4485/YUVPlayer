#include <iostream>

#include "yuv_player.h"

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

int main(int argc,char** argv) {
    YUVPlayer player;
    player.Init();
    player.CreateDUIWindow();
    player.ShowWindow();
    return 0;
}