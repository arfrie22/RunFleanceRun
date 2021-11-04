#ifndef SPRITE_H
#define SPRITE_H
#include <SDL2/SDL_render.h>

typedef struct Animation {
    SDL_Texture* texture_sheet;
    uint8_t frametime;
    uint8_t frame_count;
    uint8_t frame;
    SDL_Rect rect;
} Animation;

typedef struct Sprite {
    SDL_Rect rect;
    uint8_t timer;
    Animation* animation;
    uint8_t speed;
} Sprite;

Animation* CreateAnimation(SDL_Texture* texture, uint8_t frametime, uint8_t frame_count, int w, int h);
Sprite* CreateSprite(Animation* animation, int w, int h);

void RenderSprite(SDL_Renderer* renderer, Sprite* sprite);
uint8_t TickSprite(Sprite* sprite);
uint8_t IncFrame(Sprite* sprite);
uint8_t IncFrameTo(Sprite* sprite, uint8_t frame);


void DestroyAnimation(Animation* animation);
void DestroySprite(Sprite* sprite);


#endif //SPRITE_H
