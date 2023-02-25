#include <stdio.h>

#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    const uint led = PICO_DEFAULT_LED_PIN;
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);

    for (int i = 0; ; ++i) {
        gpio_put(led, !gpio_get(led));
        printf("Hello, MicroLua! (%d)\n", i);
        sleep_ms(500);
    }
    return 0;
}
