#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <unistd.h>

#include "gpio.h"
#include "gpio_sysfs.h"

#define SOCK_PATH "srv_led_sock"

struct gpio red_led;
struct gpio yellow_led;
struct gpio green_led;

static int setup_gpios() {
    if (gpio_open_write(5, &red_led) < 0) {
        fprintf(stderr, "Unable to open GPIO line for RED led\n");
        return -1;
    }

    if (gpio_open_write(13, &yellow_led) < 0) {
        fprintf(stderr, "Unable to open GPIO line for YELLOW led\n");
        return -1;
    }

    if (gpio_open_write(6, &green_led) < 0) {
        fprintf(stderr, "Unable to open GPIO line for GREEN led\n");
        return -1;
    }

    return 0;
}

static void close_gpios() {
    gpio_close(&red_led);
    gpio_close(&yellow_led);
    gpio_close(&green_led);
}

/* Configure the listening socket for the server. Returns the server 
   socket file descriptor, or -1 in case of failure. */
static int setup_server_socket() {
    int fd;
    size_t len;
    struct sockaddr_un local = {
        .sun_family = AF_UNIX,
    };

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);

    len = strlen(local.sun_path) + sizeof(local.sun_family);

    if (bind(fd, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        return -1;
    }

    if (listen(fd, 1) == -1) {
        perror("listen");
        return -1;
    }

    return fd;
}

static struct gpio* get_led(char c) {
    switch (c) {
        case 'R': return &red_led;
        case 'G': return &green_led;
        case 'Y': return &yellow_led;
        default:
            return NULL;
    }
}

static int parse_cmd(char *cmd, size_t cmdsz) {
    enum cmd_type {
        SET, 
        CLEAR,
        NONE,
    } cur_cmd = NONE;

    for (int i = 0; i < cmdsz; i++) {
        struct gpio *led;

        switch (cur_cmd) {
            case NONE:
                if (cmd[i] == 'S')
                    cur_cmd = SET;
                else if (cmd[i] == 'C')
                    cur_cmd = CLEAR;
                break;
            case SET:
                led = get_led(cmd[i]);
                if (led != NULL)
                    gpio_write(led, 1);
                break;
            case CLEAR:
                led = get_led(cmd[i]);
                if (led != NULL)
                    gpio_write(led, 0);
            default:
                break;
        }
    }
}

int main() {
    int srv_sock, cli_sock;

    if (setup_gpios() < 0)
        exit(1);

    if ((srv_sock = setup_server_socket()) < 0)
        exit(1);

    /* Loop forever... */
    for(;;) {
        int done, n;
        struct pollfd pfd[1];
        char cmd[5];

        /* Prepare the server socket. */

        printf("Waiting for a connection...\n");

        if ((cli_sock = accept(srv_sock, NULL, NULL)) == -1) {
            perror("accept");
            break;
        }

        if (fcntl(cli_sock, F_SETFL, O_NONBLOCK) < 0) {
            perror("fcntl");
            return -1;
        }

        printf("Connected.\n");

        done = 0;

        /* We've accepted a connection, yay! */
        do {
            pfd[0].fd = cli_sock;
            pfd[0].events = POLLIN;
            pfd[0].revents = 0;

            if (poll(pfd, 1, 1000) < 0) {
                /* Poll error */
            }

            if ((pfd[0].revents & POLLHUP) != 0) {
                /* Hang up? */
                done = 1;
            }
            if ((pfd[0].revents & POLLIN) != 0) {
                /* Socket readable! */
                if ((n = read(cli_sock, cmd, sizeof(cmd))) < 0) {
                    /* Read error? done! */
                }
                else if (n == 0)
                    done = 1;
                else 
                    parse_cmd(cmd, n);
            }

        } while (!done);

        close(cli_sock);
    }

    exit(0);
}
