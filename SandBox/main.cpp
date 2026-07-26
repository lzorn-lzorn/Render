#include <SDL3/SDL.h>
#include <cstdio>

int main(int argc, char* argv[])
{
    // 初始化 SDL 视频子系统
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    // 创建窗口
    SDL_Window* Window = SDL_CreateWindow("SDL3 Sandbox", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!Window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 创建渲染器
    SDL_Renderer* Renderer = SDL_CreateRenderer(Window, nullptr);
    if (!Renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    // 主循环
    bool Running = true;
    SDL_Event Event;
    while (Running) {
        // 处理事件队列
        while (SDL_PollEvent(&Event)) {
            if (Event.type == SDL_EVENT_QUIT) {
                Running = false;
            }
        }

        // 设置清屏颜色 (深蓝灰)
        SDL_SetRenderDrawColor(Renderer, 30, 40, 50, 255);
        SDL_RenderClear(Renderer);

        // 呈现画面
        SDL_RenderPresent(Renderer);
    }

    // 清理资源
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}