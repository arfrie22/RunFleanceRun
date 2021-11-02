#include "Sprite.h"

Animation* CreateAnimation(SDL_Texture* texture, uint8_t frametime, uint8_t frame_count, int w, int h) {
    Animation* animation = malloc(sizeof(Animation));
    animation->texture_sheet = texture;
    animation->frametime = frametime;
    animation->frame_count = frame_count;
    animation->frame = 0;
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = w;
    rect.h = h/frame_count;

    animation->rect = rect;

    return animation;
}

Sprite* CreateSprite(Animation* animation, int w, int h) {
    Sprite* sprite = malloc(sizeof(Sprite));
    sprite->animation = animation;
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = w;
    rect.h = h;
    sprite->rect = rect;
    sprite->timer = 0;
    sprite->speed = 0;

    return sprite;
}

void RenderSprite(SDL_Renderer* renderer, Sprite* sprite) {
    sprite->animation->rect.y = sprite->animation->rect.h*sprite->animation->frame;
    SDL_RenderCopy(renderer, sprite->animation->texture_sheet, &sprite->animation->rect, &sprite->rect);
}

uint8_t TickSprite(Sprite* sprite) {
    uint8_t result = 0;
    sprite->timer += 1;
    if (sprite->timer >= sprite->animation->frametime) {
        result = 1;
        sprite->timer = 0;
    }

    return result;
}

uint8_t IncFrame(Sprite* sprite) {
    uint8_t result = 0;
    sprite->animation->frame += 1;
    if (sprite->animation->frame >= sprite->animation->frame_count) {
        result = 1;
        sprite->animation->frame = 0;
    }

    return result;
}

void DestroyAnimation(Animation* animation) {
    SDL_DestroyTexture(animation->texture_sheet);
    free(animation);
}

void DestroySprite(Sprite* sprite) {
    free(sprite);
}