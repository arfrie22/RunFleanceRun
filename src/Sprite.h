#ifndef SPRITE_H
#define SPRITE_H
#include <SDL2/SDL_render.h>

typedef struct Animation {
    SDL_Texture* texture_sheet;
    uint8_t framerate;
    uint8_t frame_count;
    uint8_t frame;
    SDL_Rect rect;
} Animation;

typedef struct Sprite {
    SDL_Rect rect;
    uint16_t timer;
    Animation* animation;
} Sprite;

Animation* CreateAnimation(SDL_Texture* texture, uint8_t framerate, uint8_t frame_count, int w, int h);
Sprite* CreateSprite(Animation* animation);

void RenderSprite(SDL_Renderer* renderer, Sprite* sprite);

void DestroyAnimation(Animation* animation);


#endif //SPRITE_H
