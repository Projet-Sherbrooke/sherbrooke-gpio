#pragma once

#include <limits.h>

struct gpio_priv {
    /* File description for the open value file */
    int fd;

    /* Path to the direction file for the GPIO */
    char dir_path[PATH_MAX];

    /* Path to the value file for the GPIO */
    char value_path[PATH_MAX];

    /* Path to the 'edge' file for the GPIO */
    char edge_path[PATH_MAX];
};
