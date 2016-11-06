/*
This file present all abstraction needed to port LiteNES.
  (The current working implementation uses allegro library.)

To port this project, replace the following functions by your own:
1) nes_hal_init()
    Do essential initialization work, including starting a FPS HZ timer.

2) nes_set_bg_color(c)
    Set the back ground color to be the NES internal color code c.

3) nes_flush_buf(*buf)
    Flush the entire pixel buf's data to frame buffer.

4) nes_flip_display()
    Fill the screen with previously set background color, and
    display all contents in the frame buffer.

5) wait_for_frame()
    Implement it to make the following code is executed FPS times a second:
        while (1) {
            wait_for_frame();
            do_something();
        }

6) int nes_key_state(int b) 
    Query button b's state (1 to be pressed, otherwise 0).
    The correspondence of b and the buttons:
      1 - A
      2 - B
      3 - SELECT
      4 - START
      5 - UP
      6 - DOWN
      7 - LEFT
      8 - RIGHT
      9 - Power
*/
#include "hal.h"
#include "fce.h"
#include "common.h"
#include <sdl2/sdl.h>

#define SCALE 6 

SDL_TimerID fps_timer;
SDL_Surface *surface;
SDL_Surface *screen_surface;
SDL_Window  *window;
SDL_GameController *controller = NULL;
Sint32 controller_id;

typedef enum
{
    A = 1,
    B,
    SELECT,
    START,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    POWER
} button_t;

int buttons_state[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct
{
    char r;
    char g;
    char b;
} nes_palette[64] = {
    {0x80, 0x80, 0x80}, {0x00, 0x3D, 0xA6}, {0x00, 0x12, 0xB0}, {0x44, 0x00, 0x96},
    {0xA1, 0x00, 0x5E}, {0xC7, 0x00, 0x28}, {0xBA, 0x06, 0x00}, {0x8C, 0x17, 0x00},
    {0x5C, 0x2F, 0x00}, {0x10, 0x45, 0x00}, {0x05, 0x4A, 0x00}, {0x00, 0x47, 0x2E},
    {0x00, 0x41, 0x66}, {0x00, 0x00, 0x00}, {0x05, 0x05, 0x05}, {0x05, 0x05, 0x05},
    {0xC7, 0xC7, 0xC7}, {0x00, 0x77, 0xFF}, {0x21, 0x55, 0xFF}, {0x82, 0x37, 0xFA},
    {0xEB, 0x2F, 0xB5}, {0xFF, 0x29, 0x50}, {0xFF, 0x22, 0x00}, {0xD6, 0x32, 0x00},
    {0xC4, 0x62, 0x00}, {0x35, 0x80, 0x00}, {0x05, 0x8F, 0x00}, {0x00, 0x8A, 0x55},
    {0x00, 0x99, 0xCC}, {0x21, 0x21, 0x21}, {0x09, 0x09, 0x09}, {0x09, 0x09, 0x09},
    {0xFF, 0xFF, 0xFF}, {0x0F, 0xD7, 0xFF}, {0x69, 0xA2, 0xFF}, {0xD4, 0x80, 0xFF},
    {0xFF, 0x45, 0xF3}, {0xFF, 0x61, 0x8B}, {0xFF, 0x88, 0x33}, {0xFF, 0x9C, 0x12},
    {0xFA, 0xBC, 0x20}, {0x9F, 0xE3, 0x0E}, {0x2B, 0xF0, 0x35}, {0x0C, 0xF0, 0xA4},
    {0x05, 0xFB, 0xFF}, {0x5E, 0x5E, 0x5E}, {0x0D, 0x0D, 0x0D}, {0x0D, 0x0D, 0x0D},
    {0xFF, 0xFF, 0xFF}, {0xA6, 0xFC, 0xFF}, {0xB3, 0xEC, 0xFF}, {0xDA, 0xAB, 0xEB},
    {0xFF, 0xA8, 0xF9}, {0xFF, 0xAB, 0xB3}, {0xFF, 0xD2, 0xB0}, {0xFF, 0xEF, 0xA6},
    {0xFF, 0xF7, 0x9C}, {0xD7, 0xE8, 0x95}, {0xA6, 0xED, 0xAF}, {0xA2, 0xF2, 0xDA},
    {0x99, 0xFF, 0xFC}, {0xDD, 0xDD, 0xDD}, {0x11, 0x11, 0x11}, {0x11, 0x11, 0x11}
};

Uint32 timer_callback(Uint32 interval, void *param)
{
    SDL_Event event;
    SDL_UserEvent userevent;

    /* In this example, our callback pushes an SDL_USEREVENT event
     *     into the queue, and causes our callback to be called again at the
     *         same interval: */

    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
    return(interval);
}

/* Wait until next allegro timer event is fired. */
void wait_for_frame()
{
    SDL_Event e;

    
    while (1) {
        SDL_WaitEvent(&e);
        switch(e.type) {
            case SDL_CONTROLLERDEVICEADDED:
                // Open the controller
                controller_id = e.cdevice.which;
                controller = SDL_GameControllerOpen(controller_id);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                // Clear the controller
                if(e.cdevice.which == controller_id)
                    controller = NULL;
                break;
           case SDL_USEREVENT:
                goto end;
                break;
            case SDL_QUIT:
                exit(0);
                break;
        }
    }

end:
    ;
}

/* Set background color. RGB value of c is defined in fce.h */
void nes_set_bg_color(int c)
{
    SDL_FillRect(
            surface,
            NULL,
            SDL_MapRGB(surface->format,
                nes_palette[c].r,
                nes_palette[c].g,
                nes_palette[c].b
                )
            );
}

/* Flush the pixel buffer */
void nes_flush_buf(PixelBuf *buf)
{
    int i;
    for (i = 0; i < buf->size; i++) {

        const Pixel *pixel = &buf->buf[i];
        const int x = pixel->x;
        const int y = pixel->y;
        const unsigned int color = pixel->c;

        if(x < 0 || y < 0) {
            continue;
        }

        int bpp = surface->format->BytesPerPixel;
        /* Here p is the address to the pixel we want to set */
        Uint8 *p_addr = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
        *(Uint32 *)p_addr = 
            SDL_MapRGB(surface->format,
                    nes_palette[color].r,
                    nes_palette[color].g,
                    nes_palette[color].b
                    );
    }
}

/* Initialization:
   (1) start a 1/FPS Hz timer. 
   (2) register fce_timer handle on each timer event */
void nes_hal_init()
{

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    window = SDL_CreateWindow
        (
         "", SDL_WINDOWPOS_UNDEFINED,
         SDL_WINDOWPOS_UNDEFINED,
         SCREEN_WIDTH,
         SCREEN_HEIGHT,
         SDL_WINDOW_SHOWN
        );

    fps_timer = SDL_AddTimer(1/((double)FPS) * 1000, // ms
                             timer_callback,         // callback
                             NULL);                  // no param


    screen_surface = SDL_GetWindowSurface(window);

    int bpp;
    Uint32 rmask, gmask, bmask, amask;
    SDL_PixelFormatEnumToMasks(
            screen_surface->format->format,
            &bpp,
            &rmask,
            &gmask,
            &bmask,
            &amask);
    
    surface = SDL_CreateRGBSurface
        (0,
         screen_surface->w,
         screen_surface->h,
         bpp,
         rmask,
         gmask,
         bmask,
         amask
         );
}

/* Update screen at FPS rate by allegro's drawing function. 
   Timer ensures this function is called FPS times a second. */
void nes_flip_display()
{
    SDL_BlitSurface(surface, NULL, screen_surface, NULL);
    SDL_UpdateWindowSurface(window);
}

/* Query a button's state.
   Returns 1 if button #b is pressed. */
int nes_key_state(int b)
{
    if(controller == NULL) {
        return 0;
    }

    button_t nes_btn = b;
    SDL_GameControllerButton sdl_btn;
    switch(nes_btn) {
       case A:
            sdl_btn = SDL_CONTROLLER_BUTTON_A;
            break;
        case B:
            sdl_btn = SDL_CONTROLLER_BUTTON_B;
            break;
        case SELECT:
            sdl_btn = SDL_CONTROLLER_BUTTON_BACK;
            break;
        case START:
            sdl_btn = SDL_CONTROLLER_BUTTON_START;
            break;
        case UP:
            sdl_btn = SDL_CONTROLLER_BUTTON_DPAD_UP;
            break;
        case DOWN:
            sdl_btn = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
            break;
        case LEFT:
            sdl_btn = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
            break;
        case RIGHT:
            sdl_btn = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
            break;
        case POWER:
            sdl_btn = SDL_CONTROLLER_BUTTON_GUIDE;
            break;
        default:
            break;
    }

    int val = SDL_GameControllerGetButton(controller, sdl_btn);
    return val;
}
