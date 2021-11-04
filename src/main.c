#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>
#define SDL_STBIMAGE_IMPLEMENTATION
#include <SDL_stbimage.h>
//#include <stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STBTTF_IMPLEMENTATION
#include <stbttf.h>
#include <pthread.h>

#include <fleance_crying_sheet_png.h>
#include <fleance_running_sheet_png.h>
#include <fleance_ducking_sheet_png.h>
#include <background_sheet_png.h>
#include <rock_png.h>
#include <bird_sheet_png.h>
#include <radix_mountain_king_mod.h>
#include <ChronoTrigger_ttf.h>
#include "Sprite.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>

#include <portaudio.h>

#define FRAMERATE 30
#define FRAMEDELTA (1000/FRAMERATE)

#define BUFFERSIZE 480
#define SAMPLERATE 48000

typedef enum State {
    TITLE,
    PLAY,
    GAMEOVER,
    WIN
} State;

#define WIDTH 800
#define HEIGHT 600

uint8_t quote = 0;

#define QUOTECOUNT 13
static char* quotes[] = {
        "Fair is foul, and foul is fair",
        "For them the gracious Duncan have I murdered,  put rancors in the vessel of my peace",
        "Creeps in this petty pace from day to day to the last syllable of recorded time",
        "The power of man, for none of woman born  Shall harm Macbeth",
        "Thou shalt get kings, though thou be none",
        "Your children shall be kings",
        "O treachery! Fly, good Fleance, fly, fly, fly!",
        "Most royal sir, Fleance is ’scaped",
        "Whom you may say, if ’t please you, Fleance killed,  For Fleance fled",
        "What ’twere to kill a father. So should Fleance",
        "Lesser than Macbeth and greater",
        "Not so happy, yet much happier",
        "Here lay Duncan,  his silver skin laced with his golden blood"
};

static int16_t left[BUFFERSIZE];
static int16_t right[BUFFERSIZE];
static int16_t * const buffers[2] = { left, right };
static int16_t interleaved_buffer[BUFFERSIZE * 2];
static int is_interleaved = 0;
State state;
#define PI 3.141592654

typedef struct Obstacle {
    Sprite * sprite;
    uint8_t lane;
    uint8_t speed;
} Obstacle;

typedef struct QStat {
    uint8_t quote_val;
    char* quote;
    uint8_t side;
    float speed;
    float period;
    float amplitude;
    uint32_t next_quote;
    uint8_t has_quote;
    uint32_t timer;
} QStat;

QStat * qStat;

#define OBSTACLECOUNT 10
static Obstacle obstacles[OBSTACLECOUNT];

static void libopenmpt_example_logfunc( const char * message, void * userdata ) {
    (void)userdata;
    if ( message ) {
        fprintf( stderr, "openmpt: %s\n", message );
    }
}

static int libopenmpt_example_errfunc( int error, void * userdata ) {
    (void)userdata;
    (void)error;
    return OPENMPT_ERROR_FUNC_RESULT_DEFAULT & ~OPENMPT_ERROR_FUNC_RESULT_LOG;
}

static void libopenmpt_example_print_error( const char * func_name, int mod_err, const char * mod_err_str ) {
    if ( !func_name ) {
        func_name = "unknown function";
    }
    if ( mod_err == OPENMPT_ERROR_OUT_OF_MEMORY ) {
        mod_err_str = openmpt_error_string( mod_err );
        if ( !mod_err_str ) {
            fprintf( stderr, "Error: %s\n", "OPENMPT_ERROR_OUT_OF_MEMORY" );
        } else {
            fprintf( stderr, "Error: %s\n", mod_err_str );
            openmpt_free_string( mod_err_str );
            mod_err_str = NULL;
        }
    } else {
        if ( !mod_err_str ) {
            mod_err_str = openmpt_error_string( mod_err );
            if ( !mod_err_str ) {
                fprintf( stderr, "Error: %s failed.\n", func_name );
            } else {
                fprintf( stderr, "Error: %s failed: %s\n", func_name, mod_err_str );
            }
            openmpt_free_string( mod_err_str );
            mod_err_str = NULL;
        }
        fprintf( stderr, "Error: %s failed: %s\n", func_name, mod_err_str );
    }
}

typedef struct MusicArg {
    openmpt_module ** mod;
    uint8_t * paused;
    uint8_t * running;
    uint8_t * initialized;
    uint8_t * seek;
} MusicArg;

void *music_thread(void *vargp) {
    MusicArg args = *(MusicArg*)vargp;

    openmpt_module * mod = 0;
    int mod_err = OPENMPT_ERROR_OK;
    const char * mod_err_str = NULL;
    size_t count = 0;
    PaError pa_error = paNoError;
    PaStream * stream = 0;

    mod = openmpt_module_create_from_memory2( radix_mountain_king_mod_data, radix_mountain_king_mod_size, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
    if (!mod) {
        libopenmpt_example_print_error( "openmpt_module_create2()", mod_err, mod_err_str );
        openmpt_free_string( mod_err_str );
        mod_err_str = NULL;
        *args.running = 0;
    }
    openmpt_module_set_error_func( mod, NULL, NULL );

    pa_error = Pa_Initialize();
    if ( pa_error != paNoError ) {
        fprintf( stderr, "Error: %s\n", "Pa_Initialize() failed." );
        *args.running = 0;
    }

    pa_error = Pa_OpenDefaultStream( &stream, 0, 2, paInt16 | paNonInterleaved, SAMPLERATE, paFramesPerBufferUnspecified, NULL, NULL );
    if ( pa_error == paSampleFormatNotSupported ) {
        is_interleaved = 1;
        pa_error = Pa_OpenDefaultStream( &stream, 0, 2, paInt16, SAMPLERATE, paFramesPerBufferUnspecified, NULL, NULL );
    }
    if ( pa_error != paNoError ) {
        fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
        *args.running = 0;
    }
    if ( !stream ) {
        fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
        *args.running = 0;
    }

    pa_error = Pa_StartStream( stream );
    if ( pa_error != paNoError ) {
        fprintf( stderr, "Error: %s\n", "Pa_StartStream() failed." );
        *args.running = 0;
    }

    *args.mod = mod;
    *args.initialized = 1;

    while (1) {
        if (*args.seek) {
            openmpt_module_set_position_seconds(mod, 0);
            *args.seek = 0;
        }
        if (state == PLAY && !*args.paused) {
            openmpt_module_error_clear(mod);
            count = is_interleaved ? openmpt_module_read_interleaved_stereo(mod, SAMPLERATE, BUFFERSIZE,
                                                                                  interleaved_buffer)
                                         : openmpt_module_read_stereo(mod, SAMPLERATE, BUFFERSIZE, left, right);
            mod_err = openmpt_module_error_get_last(mod);
            mod_err_str = openmpt_module_error_get_last_message(mod);
            if (mod_err != OPENMPT_ERROR_OK) {
                libopenmpt_example_print_error("openmpt_module_read_stereo()", mod_err, mod_err_str);
                openmpt_free_string(mod_err_str);
                mod_err_str = NULL;
            }
//            fprintf(stdout, "Position %f, Length %f\n", openmpt_module_get_position_seconds(mod), openmpt_module_get_duration_seconds(mod));

            if (count == 0) {
                if (openmpt_module_get_position_seconds(mod) >= openmpt_module_get_duration_seconds(mod)) {
                    state = WIN;
                    break;
                } else {
                    openmpt_module_set_position_seconds(mod, 0);
                }
            } else {

                pa_error = is_interleaved ? Pa_WriteStream(stream, interleaved_buffer,
                                                                 (unsigned long) count)
                                                : Pa_WriteStream(stream, buffers, (unsigned long) count);
                if (pa_error == paOutputUnderflowed) {
                    pa_error = paNoError;
                }
                if (pa_error != paNoError) {
                    fprintf(stderr, "Error: %s\n", "Pa_WriteStream() failed.");
                    *args.running = 0;
                    pthread_exit(NULL);
                }
            }
        }
    }
    pthread_exit(NULL);
}

void GenerateQuote() {
    uint8t quote_val = rand() % QUOTECOUNT;
    qStat->quote_val = quote_val;
    qStat->quote = quotes[quote_val];
    qStat->side = rand() % 2;
    qStat->speed = (rand() % 250)/50;
    qStat->period = (rand() % 250)/50;
    qStat->amplitude = (rand() % 250)/50;
    qStat->next_quote = (rand() % 250) + 100;
    qStat->has_quote = 1;
    qStat->timer = 0;
}

void renderQuote(SDL_Renderer * renderer, STBTTF_Font * font, char* quote, float x, float y, float w) {
    char buff[255];
    strcpy (buff, quote);
    float ws;
    float xOffset = 0;
    float yOffset = 0;
    char* token;
    float space = STBTTF_MeasureText(font, " ");



    token = strtok(buff, " ");

    while( token != NULL ) {
        ws = STBTTF_MeasureText(font, token);
        if ((xOffset+ws) > w) {
            yOffset += (9*font->size)/8;
            xOffset = 0;
        }

        STBTTF_RenderText(renderer, font, x+xOffset, y+yOffset, token);
        xOffset += ws;
        xOffset += space;
        token = strtok(NULL, " ");
    }

}


int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf( stderr, "Error initializing SDL: %s\n", SDL_GetError() );
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Run Fleance Run", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT+25, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        fprintf( stderr, "Error creating window: %s\n", SDL_GetError() );
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (window == NULL) {
        fprintf( stderr, "Error creating window: %s\n", SDL_GetError() );
        return 1;
    }

    int w,h,n;
    unsigned char *fleance_crying_data = stbi_load_from_memory(fleance_crying_sheet_png_data, fleance_crying_sheet_png_size, &w, &h, &n, 0);
    SDL_Texture* fleance_crying_texture = STBIMG_CreateTexture(renderer, fleance_crying_data, w, h, n);
    Animation* fleance_crying_animation = CreateAnimation(fleance_crying_texture, 4, 12, w, h);
    unsigned char *fleance_running_data = stbi_load_from_memory(fleance_running_sheet_png_data, fleance_running_sheet_png_size, &w, &h, &n, 0);
    SDL_Texture* fleance_running_texture = STBIMG_CreateTexture(renderer, fleance_running_data, w, h, n);
    Animation* fleance_running_animation = CreateAnimation(fleance_running_texture, 2, 7, w, h);
    unsigned char *fleance_ducking_data = stbi_load_from_memory(fleance_ducking_sheet_png_data, fleance_ducking_sheet_png_size, &w, &h, &n, 0);
    SDL_Texture* fleance_ducking_texture = STBIMG_CreateTexture(renderer, fleance_ducking_data, w, h, n);
    Animation* fleance_ducking_animation = CreateAnimation(fleance_ducking_texture, 2, 7, w, h);
    Sprite* fleance_sprite = CreateSprite(fleance_crying_animation, 64, 128);
    free(fleance_crying_data);
    free(fleance_running_data);
    free(fleance_ducking_data);

    unsigned char *background_data = stbi_load_from_memory(background_sheet_png_data, background_sheet_png_size, &w, &h, &n, 0);
    SDL_Texture* background_texture = STBIMG_CreateTexture(renderer, background_data, w, h, n);
    Animation* background_animation = CreateAnimation(background_texture, 5, 3, w, h);
    Sprite* background_sprite = CreateSprite(background_animation, WIDTH, HEIGHT);
    free(background_data);

    unsigned char *rock_data = stbi_load_from_memory(rock_png_data, rock_png_size, &w, &h, &n, 0);
    SDL_Texture* rock_texture = STBIMG_CreateTexture(renderer, rock_data, w, h, n);
    Animation* rock_animation = CreateAnimation(rock_texture, 5, 1, w, h);
    unsigned char *bird_data = stbi_load_from_memory(bird_sheet_png_data, bird_sheet_png_size, &w, &h, &n, 0);
    SDL_Texture* bird_texture = STBIMG_CreateTexture(renderer, bird_data, w, h, n);
    Animation* bird_animation = CreateAnimation(bird_texture, 5, 3, w, h);


    Sprite* rock_sprite = CreateSprite(rock_animation, 32, 32);
    Sprite* bird_sprite = CreateSprite(bird_animation, 32, 32);
    rock_sprite->speed = 10;
    bird_sprite->speed = 10;
    free(rock_data);
    free(bird_data);

    time_t t;
    srand((unsigned) time(&t));

    STBTTF_Font* font = STBTTF_OpenFontMem(renderer, ChronoTrigger_ttf_data, ChronoTrigger_ttf_size, 32);
    STBTTF_Font* quoteFont = STBTTF_OpenFontMem(renderer, ChronoTrigger_ttf_data, ChronoTrigger_ttf_size, 16);
    openmpt_module * mod = 0;
    uint8_t paused, running, initialized, seek;
    initialized = 0;
    running = 1;
    paused = 0;
    seek = 1;

    MusicArg args = {
            .mod = &mod,
            .paused = &paused,
            .running = &running,
            .initialized = &initialized,
            .seek = &seek
    };

    pthread_t tid;
    pthread_create(&tid, NULL, music_thread, (void *)&args);
    qStat = malloc(sizeof(QStat));


    int prevTime, curTime, deltaTime;
    double pathAngle, pathComp;
    Start:
    // Lol will hang after 43 days
    curTime = SDL_GetTicks();
    prevTime = curTime;
    while (!initialized) {}
    state = TITLE;
    running = 1;
    paused = 0;
    fleance_sprite->animation = fleance_crying_animation;
    fleance_sprite->rect.x = (WIDTH - fleance_sprite->rect.w)/2;
    fleance_sprite->rect.y = (HEIGHT - fleance_sprite->rect.h);
    pathAngle = atan2((7.0*HEIGHT)/8.0, WIDTH/3.0);
    pathComp = (PI/2) - pathAngle;
    GenerateQuote();

    rock_sprite->rect.x = (WIDTH - rock_sprite->rect.w)/2;
    rock_sprite->rect.y = HEIGHT/8;
    bird_sprite->rect.x = (WIDTH - bird_sprite->rect.w)/2;
    bird_sprite->rect.y = HEIGHT/8;

    uint8_t lane = 1;
    uint8_t ducking = 0;

    while (running) {
        curTime = SDL_GetTicks();
        deltaTime = curTime-prevTime;
        if (deltaTime < FRAMEDELTA) {
            SDL_Delay(FRAMEDELTA - deltaTime);
        }
        prevTime = SDL_GetTicks();

        SDL_Event e;
        if (SDL_PollEvent(&e)) {
            rand();
            switch(e.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_WINDOWEVENT:
                    switch (e.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                            break;
                    }
                    break;

                case SDL_KEYDOWN:
                    switch (e.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            if (state == TITLE) {
                                running = 0;
                            } else {
                                paused = !paused;
                            }
                            break;

                        case SDLK_r:
                            goto Start;
                            break;
                    }

                    switch (state) {
                        case TITLE:
                            if (e.key.keysym.sym != SDLK_r) {
                                fleance_sprite->animation = fleance_running_animation;
                                state = PLAY;
                                seek = 1;
                            }
                            break;
                        case PLAY:
                            switch (e.key.keysym.sym) {
                                case SDLK_LEFT:
                                case SDLK_a:
                                    if (lane > 0) {
                                        lane -= 1;
                                        fleance_sprite->rect.x -= WIDTH/3;
                                    }
                                    break;

                                case SDLK_RIGHT:
                                case SDLK_d:
                                    if (lane < 2) {
                                        lane += 1;
                                        fleance_sprite->rect.x += WIDTH/3;
                                    }
                                    break;

                                case SDLK_DOWN:
                                case SDLK_s:
                                    ducking = 1;
                                    fleance_ducking_animation->frame = fleance_running_animation->frame;
                                    fleance_sprite->animation = fleance_ducking_animation;
                                    break;

                                case SDLK_UP:
                                case SDLK_w:
                                    ducking = 0;
                                    fleance_running_animation->frame = fleance_ducking_animation->frame;
                                    fleance_sprite->animation = fleance_running_animation;
                                    break;
                            }
                            break;
                        case GAMEOVER:
                            break;
                        case WIN:
                            break;
                    }
                    break;
            }
        }

        if (!paused) {
            if (state == TITLE) {
                if (TickSprite(fleance_sprite)) {
                    IncFrame(fleance_sprite);
                }
            }

            if (state == PLAY) {
                if (TickSprite(fleance_sprite)) {
                    if (!ducking) {
                        IncFrame(fleance_sprite);
                    } else {
                        IncFrameTo(fleance_sprite, 3);
                    }
                }

                if (TickSprite(background_sprite)) {
                    IncFrame(background_sprite);
                }

                if (TickSprite(rock_sprite)) {
                    double multiplier = 4;
                    multiplier *= ((8.0*rock_sprite->rect.y) - HEIGHT)/(7.0*HEIGHT);
                    multiplier += 1;
                    rock_sprite->rect.h = ceil(multiplier*16);
                    rock_sprite->rect.w = ceil(multiplier*16);

                    rock_sprite->rect.y += (rock_sprite->speed) * multiplier;
                    rock_sprite->rect.x -= (rock_sprite->speed * sin(pathComp) / sin(pathAngle)) * multiplier;
                }

                if (TickSprite(bird_sprite)) {
                    double multiplier = 4;
                    multiplier *= ((8.0*bird_sprite->rect.y) - HEIGHT)/(7.0*HEIGHT);
                    multiplier += 1;
                    bird_sprite->rect.h = ceil(multiplier*16);
                    bird_sprite->rect.w = ceil(multiplier*16);

                    bird_sprite->rect.y += (bird_sprite->speed) * multiplier;
                    bird_sprite->rect.x -= (bird_sprite->speed * sin(pathComp) / sin(pathAngle)) * multiplier;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        RenderSprite(renderer, background_sprite);

        float ws;
        switch (state) {
            case TITLE:
                SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
                ws = STBTTF_MeasureText(font, "Press any key to begin");
                STBTTF_RenderText(renderer, font, 400 - (ws / 2), 300, "Press any key to begin");
                break;
            case PLAY:
                RenderSprite(renderer, rock_sprite);
                RenderSprite(renderer, bird_sprite);

                if (paused) {
                    SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
                    ws = STBTTF_MeasureText(font, "Paused");
                    STBTTF_RenderText(renderer, font, 400 - (ws / 2), 300, "Paused");
                } else {
                    quote = rand() % QUOTECOUNT;
                }

                SDL_FRect progress;
                progress.x = 0;
                progress.y = HEIGHT;
                progress.h = 25;
                progress.w = WIDTH;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderFillRectF(renderer, &progress);
                progress.w = WIDTH * (openmpt_module_get_position_seconds(mod)/openmpt_module_get_duration_seconds(mod));
                SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
                SDL_RenderFillRectF(renderer, &progress);

                // TODO text
//                uint8_t quote = rand() % QUOTECOUNT;
                if (quote < 3) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 127);
                } else {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 63);
                }
                renderQuote(renderer, quoteFont, quotes[quote], 0, 300, 100);
//                ws = STBTTF_MeasureText(quoteFont, quotes[quote]);
//
////                if (qStat)
//                STBTTF_RenderText(renderer, quoteFont, 400 - (ws / 2), 300, quotes[quote]);
                break;
            case GAMEOVER:
                SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
                ws = STBTTF_MeasureText(font, "GAME OVER!");
                STBTTF_RenderText(renderer, font, 400 - (ws / 2), 300, "GAME OVER!");
                break;
            case WIN:
                SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
                ws = STBTTF_MeasureText(font, "YOU WIN!");
                STBTTF_RenderText(renderer, font, 400 - (ws / 2), 300, "YOU WIN!");
                break;
        }

        RenderSprite(renderer, fleance_sprite);

        SDL_RenderPresent(renderer);
    }

    pthread_cancel(tid);
    DestroyAnimation(fleance_crying_animation);
    DestroySprite(fleance_sprite);
    DestroyAnimation(background_animation);
    DestroySprite(background_sprite);

    STBTTF_CloseFont(font);
    STBTTF_CloseFont(quoteFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}