#include <stdlib.h>
#include <stdio.h>

#include "gpio.h"
#include "gpio_sysfs.h"

int main() {
    struct gpio red_led;
    struct gpio yellow_button;

#if 0
    if (gpio_open_write(6, &red_led) < 0) {
        fprintf(stderr, "Unable to open GPIO line\n");
        exit(1);
    }

    if (gpio_write(&red_led, GPIO_VALUE_UP) < 0) {
        fprintf(stderr, "Failed to set GPIO value\n");
    }

    sleep(1);

    if (gpio_write(&red_led, GPIO_VALUE_DOWN) < 0) {
        fprintf(stderr, "Failed to set GPIO value\n");
    }

    if (gpio_close(&red_led) < 0) {
        exit(1);
    }
#endif 

    if (gpio_open_read(19, &yellow_button) < 0) {
        fprintf(stderr, "Unable to open GPIO line for yellow button\n");
        exit(1);
    }

#if 0
    if (gpio_open_read(20, &red_button) < 0) {
        fprintf(stderr, "Unable to open GPIO line for red button\n");
        exit(1);
    }
#endif

    if (gpio_open_write(5, &red_led) < 0) {
        fprintf(stderr, "Unable to open GPIO line for RED led\n");
        exit(1);
    }

    while (1) {
        int n;

        gpio_wait_single(&yellow_button);

        gpio_read(&yellow_button, &n);
        gpio_write(&red_led, n);
    }

    gpio_close(&yellow_button);
    gpio_close(&red_led);

    exit(0);
}
