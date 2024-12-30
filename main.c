#include <SDL2/SDL.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdbool.h>

// Definición de colores RGB
SDL_Color colors[] = {
    {255, 0, 0},   // Rojo
    {0, 255, 0},   // Verde
    {0, 0, 255},   // Azul
    {255, 255, 255}, // Blanco
    {0, 0, 0}      // Negro
};

// Función para obtener la resolución desde fbdev
bool getFramebufferResolution(const char *fbdev, int *width, int *height) {
    int fb = open(fbdev, O_RDWR);
    if (fb < 0) {
        perror("Error al abrir el framebuffer");
        return false;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("Error al obtener la información del framebuffer");
        close(fb);
        return false;
    }

    *width = vinfo.xres;
    *height = vinfo.yres;
    close(fb);
    return true;
}

void fillScreen(SDL_Renderer *renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {
    // Obtener resolución de fbdev
    int screenWidth = 640, screenHeight = 480; // Valores por defecto
    if (!getFramebufferResolution("/dev/fb0", &screenWidth, &screenHeight)) {
        fprintf(stderr, "No se pudo obtener la resolución de /dev/fb0. Usando valores predeterminados.\n");
    } else {
        printf("Resolución detectada: %dx%d\n", screenWidth, screenHeight);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "Error al inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Reparar Píxeles Muertos",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screenWidth, screenHeight,
        SDL_WINDOW_FULLSCREEN
    );

    if (!window) {
        fprintf(stderr, "Error al crear la ventana: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Error al crear el renderizador: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Iniciar el joystick
    if (SDL_NumJoysticks() < 1) {
        fprintf(stderr, "No se detectó ningún gamepad.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Joystick *gamepad = SDL_JoystickOpen(0);
    if (!gamepad) {
        fprintf(stderr, "Error al abrir el gamepad: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    int colorIndex = 0;
    SDL_Event event;

    Uint32 lastTime = SDL_GetTicks();
    const Uint32 interval = 50; // Tiempo entre cambios de color en ms

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_MENU))) {
                running = false;
            }

            // Verificar si el botón "Guide" es presionado
            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 8) {
                    printf("Botón Guide presionado, cerrando...\n");
                    running = false;
                }
            }
        }

        Uint32 currentTime = SDL_GetTicks();
        if (currentTime > lastTime + interval) {
            fillScreen(renderer, colors[colorIndex]);
            colorIndex = (colorIndex + 1) % (sizeof(colors) / sizeof(colors[0]));
            lastTime = currentTime;
        }
    }

    // Cerrar el gamepad y SDL
    SDL_JoystickClose(gamepad);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
