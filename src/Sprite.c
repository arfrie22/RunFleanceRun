#include "Sprite.h"

Animation* CreateAnimation(SDL_Texture* texture, uint8_t framerate, uint8_t frame_count, int w, int h) {
    static Animation animation;
    animation.texture_sheet = texture;
    animation.framerate = framerate;
    animation.frame_count = frame_count;
    animation.frame = 0;
    static SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = w;
    rect.h = h/frame_count;

    animation.rect = rect;

    return &animation;
}

Sprite* CreateSprite(Animation* animation) {
    static Sprite sprite;
    sprite.animation = animation;
    static SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = animation->rect.w;
    rect.h = animation->rect.h;
    sprite.rect = rect;
    sprite.timer = 0;

    return &sprite;
}

void RenderSprite(SDL_Renderer* renderer, Sprite* sprite) {
    sprite->animation->rect.y = sprite->animation->rect.h*sprite->animation->frame;
    SDL_RenderCopy(renderer, sprite->animation->texture_sheet, &sprite->animation->rect, &sprite->rect);
}