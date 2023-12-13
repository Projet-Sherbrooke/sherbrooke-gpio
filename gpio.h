#pragma once

struct gpio_priv;

enum gpio_dir {
    GPIO_DIR_IN,
    GPIO_DIR_OUT
};

struct gpio {
    /* Direction for the GPIO line */
    enum gpio_dir dir;

    /* Value to write back to the GPIO line on close */
    int init_value;

    int gpio;
    /* GPIO line */

    struct gpio_priv *priv;
};

#define GPIO_VALUE_UP   1
#define GPIO_VALUE_DOWN 0

int gpio_open_write(int gpioId, struct gpio *gpio);

int gpio_open_read(int gpioId, struct gpio *gpio);

int gpio_wait_single(struct gpio *gpio);

int gpio_read(struct gpio *gpio, int *val);

int gpio_write(struct gpio *gpio, int val);

int gpio_close(struct gpio *gpio);

