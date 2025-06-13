#include <SDL2/SDL.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define PIXEL_FIXER_SIZE 64  // Size of the pixel fixer square

// Function to get resolution from fbdev
bool getFramebufferResolution(const char *fbdev, int *width, int *height) {
    int fb = open(fbdev, O_RDWR);
    if (fb < 0) {
        perror("Error opening framebuffer");
        return false;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("Error getting framebuffer information");
        close(fb);
        return false;
    }

    *width = vinfo.xres;
    *height = vinfo.yres;
    close(fb);
    return true;
}

// Function to generate a random color
SDL_Color getRandomColor() {
    SDL_Color color;
    color.r = rand() % 256;
    color.g = rand() % 256;
    color.b = rand() % 256;
    return color;
}

// Function to draw the pixel fixer square with random pixel pattern
void drawPixelFixer(SDL_Renderer *renderer, int x, int y) {
    // Draw individual pixels with random colors
    for (int i = 0; i < PIXEL_FIXER_SIZE; i++) {
        for (int j = 0; j < PIXEL_FIXER_SIZE; j++) {
            SDL_Color color = getRandomColor();
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            SDL_RenderDrawPoint(renderer, x + i, y + j);
        }
    }
}

// Function to draw black background
void drawBackground(SDL_Renderer *renderer, int screenWidth, int screenHeight) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

int main(int argc, char *argv[]) {
    // Initialize random number generator
    srand(time(NULL));

    // Get resolution from fbdev
    int screenWidth = 640, screenHeight = 480; // Default values
    if (!getFramebufferResolution("/dev/fb0", &screenWidth, &screenHeight)) {
        fprintf(stderr, "Could not get resolution from /dev/fb0. Using default values.\n");
    } else {
        printf("Detected resolution: %dx%d\n", screenWidth, screenHeight);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Dead Pixel Repair - JScreenFix Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screenWidth, screenHeight,
        SDL_WINDOW_FULLSCREEN
    );

    if (!window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize joystick
    if (SDL_NumJoysticks() < 1) {
        fprintf(stderr, "No gamepad detected.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Joystick *gamepad = SDL_JoystickOpen(0);
    if (!gamepad) {
        fprintf(stderr, "Error opening gamepad: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;
    
    // Initial position of the fixer square in the center
    int fixerX = (screenWidth - PIXEL_FIXER_SIZE) / 2;
    int fixerY = (screenHeight - PIXEL_FIXER_SIZE) / 2;
    
    // Variables for movement control
    const int moveSpeed = 5;
    
    // Time for fast flashing
    Uint32 lastTime = SDL_GetTicks();
    const Uint32 flashInterval = 16; // ~60 FPS flashing

    printf("Instructions:\n");
    printf("- Use the joystick to move the fixer square over dead pixels\n");
    printf("- Press the Guide button (button 8) or ESC/MENU to exit\n");

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || 
                (event.type == SDL_KEYDOWN && 
                 (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_MENU))) {
                running = false;
            }

            // Check if the "Guide" button is pressed
            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 8) {
                    printf("Guide button pressed, closing...\n");
                    running = false;
                }
            }
        }

        // Read joystick state for movement
        int xAxis = SDL_JoystickGetAxis(gamepad, 0);
        int yAxis = SDL_JoystickGetAxis(gamepad, 1);
        
        // Convert axis values (-32768 to 32767) to movement
        if (abs(xAxis) > 8000) { // Dead zone
            fixerX += (xAxis > 0) ? moveSpeed : -moveSpeed;
        }
        if (abs(yAxis) > 8000) { // Dead zone
            fixerY += (yAxis > 0) ? moveSpeed : -moveSpeed;
        }

        // Keep the square within screen bounds
        if (fixerX < 0) fixerX = 0;
        if (fixerY < 0) fixerY = 0;
        if (fixerX > screenWidth - PIXEL_FIXER_SIZE) fixerX = screenWidth - PIXEL_FIXER_SIZE;
        if (fixerY > screenHeight - PIXEL_FIXER_SIZE) fixerY = screenHeight - PIXEL_FIXER_SIZE;

        Uint32 currentTime = SDL_GetTicks();
        if (currentTime > lastTime + flashInterval) {
            // Draw black background
            drawBackground(renderer, screenWidth, screenHeight);
            
            // Draw the fixer square with flashing pixels
            drawPixelFixer(renderer, fixerX, fixerY);
            
            // Present to screen
            SDL_RenderPresent(renderer);
            
            lastTime = currentTime;
        }
    }

    // Close gamepad and SDL
    SDL_JoystickClose(gamepad);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
