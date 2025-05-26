#include <project.h>
#include <stdlib.h>
#include "C:\Users\Ronen\Downloads\ssd1306.h"
#include "math.h"

#define DISPLAY_ADDRESS 0x3C

void draw_sine_wave_with_axes_labels() {
    display_clear();

    // === Axes ===
    // Y-axis at x = 0
    for (int y = 0; y < 64; y++) {
        gfx_drawPixel(0, y, WHITE);
    }

    // X-axis at y = 32
    for (int x = 0; x < 128; x++) {
        gfx_drawPixel(x, 32, WHITE);
    }

    // === Sine Wave (2 cycles) ===
    for (int x = 0; x < 128; x++) {
        float radians = ((float)x / 128.0f) * 4.0f * M_PI;  // 2 cycles
        int y = (int)(16.0f * sinf(radians)) + 32;
        if (y >= 0 && y < 64) {
            gfx_drawPixel(x, y, WHITE);
        }
    }

    // === Labels on X-axis ===
    gfx_setTextSize(1);
    gfx_setTextColor(WHITE);

    gfx_setCursor(0, 33);    gfx_println("0");
    gfx_setCursor(31, 33);   gfx_println("1");
    gfx_setCursor(63, 33);   gfx_println("2");
    gfx_setCursor(95, 33);   gfx_println("3");
    gfx_setCursor(112, 33);  gfx_println("4pi");

    // === Labels on Y-axis ===
    gfx_setCursor(2, 0);     gfx_println("+8");
    gfx_setCursor(2, 30);    gfx_println("0");
    gfx_setCursor(2, 56);    gfx_println("-8");

    display_update();
}

int main() {
    I2C_Start();
    CyGlobalIntEnable;
    display_init(DISPLAY_ADDRESS);
    display_clear();

    for(;;) {
        draw_sine_wave_with_axes_labels();
        CyDelay(1000);
    }
}