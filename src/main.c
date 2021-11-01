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

#include <fleance_sheet_png.h>
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

#define BUFFERSIZE 480
#define SAMPLERATE 48000

static int16_t left[BUFFERSIZE];
static int16_t right[BUFFERSIZE];
static int16_t * const buffers[2] = { left, right };
static int16_t interleaved_buffer[BUFFERSIZE * 2];
static int is_interleaved = 0;



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

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf( stderr, "Error initializing SDL: %s\n", SDL_GetError() );
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Run Fleance Run", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
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
    unsigned char *fleance_data = stbi_load_from_memory(fleance_sheet_png_data, fleance_sheet_png_size, &w, &h, &n, 0);
    SDL_Texture* fleance_texture = STBIMG_CreateTexture(renderer, fleance_data, w, h, n);
    Animation* fleance_crying = CreateAnimation(fleance_texture, 30, 21, w, h);
    Sprite* fleance_sprite = CreateSprite(fleance_crying);
    STBTTF_Font* font = STBTTF_OpenFontMem(renderer, ChronoTrigger_ttf_data, ChronoTrigger_ttf_size, 32);

    free(fleance_data);

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
        return 1;
    }
    openmpt_module_set_error_func( mod, NULL, NULL );

    pa_error = Pa_Initialize();
    if ( pa_error != paNoError ) {
        fprintf( stderr, "Error: %s\n", "Pa_Initialize() failed." );
        return 1;
    }

    pa_error = Pa_OpenDefaultStream( &stream, 0, 2, paInt16 | paNonInterleaved, SAMPLERATE, paFramesPerBufferUnspecified, NULL, NULL );
    if ( pa_error == paSampleFormatNotSupported ) {
        is_interleaved = 1;
        pa_error = Pa_OpenDefaultStream( &stream, 0, 2, paInt16, SAMPLERATE, paFramesPerBufferUnspecified, NULL, NULL );
    }
    if ( pa_error != paNoError ) {
        fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
        return 1;
    }
    if ( !stream ) {
        fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
        return 1;
    }

    pa_error = Pa_StartStream( stream );
    if ( pa_error != paNoError ) {
        fprintf( stderr, "Error: %s\n", "Pa_StartStream() failed." );
        return 1;
    }

    uint8_t  running = 1;
    while (running) {
        SDL_Event e;
        if (SDL_PollEvent(&e)) {
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
                            running = 0;
                            break;

                        case SDLK_r:
                            openmpt_module_set_position_seconds(mod, 0);
                            break;
                    }
                    break;
            }
        }

        fleance_sprite->animation->frame += 1;
        if (fleance_sprite->animation->frame >= fleance_sprite->animation->frame_count) fleance_sprite->animation->frame = 0;

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

        RenderSprite(renderer, fleance_sprite);

        SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
        STBTTF_RenderText(renderer, font, 50, 50, "Press any key to begin");

        SDL_RenderPresent(renderer);

        openmpt_module_error_clear( mod );
        count = is_interleaved ? openmpt_module_read_interleaved_stereo( mod, SAMPLERATE, BUFFERSIZE, interleaved_buffer ) : openmpt_module_read_stereo( mod, SAMPLERATE, BUFFERSIZE, left, right );
        mod_err = openmpt_module_error_get_last( mod );
        mod_err_str = openmpt_module_error_get_last_message( mod );
        if ( mod_err != OPENMPT_ERROR_OK ) {
            libopenmpt_example_print_error( "openmpt_module_read_stereo()", mod_err, mod_err_str );
            openmpt_free_string( mod_err_str );
            mod_err_str = NULL;
        }
        if ( count == 0 ) {
            break;
        }

        pa_error = is_interleaved ? Pa_WriteStream( stream, interleaved_buffer, (unsigned long)count ) : Pa_WriteStream( stream, buffers, (unsigned long)count );
        if ( pa_error == paOutputUnderflowed ) {
            pa_error = paNoError;
        }
        if ( pa_error != paNoError ) {
            fprintf( stderr, "Error: %s\n", "Pa_WriteStream() failed." );
            return 1;
        }
    }

    SDL_DestroyTexture(fleance_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

//    stbir_resize();
    return 0;
}