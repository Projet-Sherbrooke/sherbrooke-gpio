#include <stdlib.h>
#include <stdio.h>

#include "gpio.h"
#include "gpio_sysfs.h"

int main() {
    struct gpio red_led;
    struct gpio yellow_led;
    struct gpio green_led;
    struct gpio yellow_button;
    struct gpio green_button;

    if (gpio_open_read(19, &yellow_button) < 0) {
        fprintf(stderr, "Unable to open GPIO line for yellow button\n");
        exit(1);
    }

    if (gpio_open_read(20, &green_button) < 0) {
        fprintf(stderr, "Unable to open GPIO line for red button\n");
        exit(1);
    }

    if (gpio_open_write(5, &red_led) < 0) {
        fprintf(stderr, "Unable to open GPIO line for RED led\n");
        exit(1);
    }

    if (gpio_open_write(13, &yellow_led) < 0) {
        fprintf(stderr, "Unable to open GPIO line for YELLOW led\n");
        exit(1);
    }

    if (gpio_open_write(6, &green_led) < 0) {
        fprintf(stderr, "Unable to open GPIO line for GREEN led\n");
        exit(1);
    }

    while (1) {
        int n_green, n_yellow;

        gpio_wait(2, &yellow_button, &green_button);

        gpio_read(&yellow_button, &n_yellow);
        gpio_read(&green_button, &n_green);

        if (n_yellow == 1 && n_green == 1)
            gpio_write(&green_led, 1);
        if (n_yellow == 0 || n_green == 0)
            gpio_write(&green_led, 0);

        gpio_write(&red_led, n_green);
        gpio_write(&yellow_led, n_yellow);
    }

    gpio_close(&yellow_button);
    gpio_close(&red_led);

    exit(0);
}
