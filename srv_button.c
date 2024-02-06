#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "gpio.h"
#include "gpio_sysfs.h"

struct gpio yellow_button;
struct gpio green_button;
struct gpio red_led;
struct gpio yellow_led;
struct gpio green_led;

#define SOCK_PATH "srv_button_sock"

/* Maximum # of events received at the same time. */
#define MAX_EVENTS 10

/* Maximum number of concurrent connections. */
#define MAX_CLIENTS 256

/* Client file descriptors. */
int cli_fds[MAX_CLIENTS];

/* Last allocated client FD. */
int cli_max;

/* Number of client FD currently active. */
int cli_nb;

static int add_fd(int fd) {
    int i;

    for (i = 0; i < cli_max; i++)
        if (cli_fds[i] == -1)
            cli_fds[i] = fd;
    if (i <= cli_max) 
        cli_fds[cli_max++] = fd;

    cli_nb++;
}

static int rm_fd(int fd) {
    int i;

    for (i = 0; i < cli_max; i++)
        if (cli_fds[i] == fd)
            cli_fds[i] = -1;
    if (i == cli_max)
        cli_max--;

    cli_nb--;
}

static int setup_gpios() {
    if (gpio_open_read(19, &yellow_button) < 0) {
        fprintf(stderr, "Unable to open GPIO line for yellow button\n");
        return -1;
    }

    if (gpio_open_read(20, &green_button) < 0) {
        fprintf(stderr, "Unable to open GPIO line for red button\n");
        return -1;
    }

    return 0;
}

static void close_gpios() {
    gpio_close(&yellow_button);
    gpio_close(&green_button);
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

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        return -1;
    }

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

int main() {
    int ep_fd, srv_sock, cli_sock;
    struct epoll_event ev_srv;
    char status_fmt[] = "G%dY%d\n";
    char status_buf[sizeof(status_fmt)];
    size_t status_sz, status_nb;    

    if (setup_gpios() < 0)
        exit(1);

    if ((srv_sock = setup_server_socket()) < 0)
        exit(1);

    /* Create an epoll instance */
    if ((ep_fd = epoll_create1(0)) < 0) {
        perror("epoll_create1");
        exit(1);
    }

    ev_srv.events = EPOLLIN;
    ev_srv.data.fd = srv_sock;
        
    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, srv_sock, &ev_srv) == -1) {
        perror("epoll_ctl: listen_sock");
        close_gpios();
        exit(1);
    }

    /* Add the GPIOs into the epoll set. */
    ev_srv.events = EPOLLPRI | EPOLLET;
    ev_srv.data.fd = gpio_fd(&yellow_button);

    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, gpio_fd(&yellow_button), &ev_srv) < 0) {
        perror("epoll_ctl");
        close_gpios();
        exit(1);
    }

    ev_srv.events = EPOLLPRI | EPOLLET;
    ev_srv.data.fd = gpio_fd(&green_button);

    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, gpio_fd(&green_button), &ev_srv) < 0) {
        perror("epoll_ctl");
        close_gpios();
        exit(1);
    }

    /* Loop forever... */
    for(;;) {
        int nfds;
        struct epoll_event events[MAX_EVENTS];
        struct epoll_event ev_new;

        /* Wait for the GPIOs to change state. */
        if ((nfds = epoll_wait(ep_fd, events, MAX_EVENTS, -1)) < 0) {
            perror("epoll_wait");
            //done = 1;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == srv_sock) {
                int cli_sock;

                if ((cli_sock = accept(srv_sock, NULL, NULL)) == -1) {
                    perror("accept");
                    break;
                }

                if (add_fd(cli_sock) < 0) {

                }

                ev_new.data.fd = cli_sock;

                /* Add the new client descriptor to the epoll set. */
                if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, cli_sock, &ev_new) == -1) {
                    perror("epoll_ctl");
                    break;   
                }
            } else if (events[i].data.fd == gpio_fd(&yellow_button) ||
                       events[i].data.fd == gpio_fd(&green_button)) {
                int val_green, val_yellow;

                /* Change the GPIO set. */
                for (int i = 0; i < cli_max; i++) {                    
                    if (cli_fds[i] != -1) {
                        ev_new.data.fd = cli_fds[i];
                        ev_new.events = EPOLLHUP | EPOLLOUT | EPOLLONESHOT; 

                        epoll_ctl(ep_fd, EPOLL_CTL_MOD, cli_fds[i], &ev_new);
                    }

                    if (gpio_read(&green_button, &val_green) < 0) {

                    }
                    if (gpio_read(&yellow_button, &val_yellow) < 0) {

                    }

                    /* Format the data to send. */
                    status_sz = sprintf(status_buf, status_fmt, val_green, val_yellow);
                    status_nb = cli_nb;
                }
            } else {
                /* Send whatever is waiting to be sent to all the connected clients. */
                if (write(events[i].data.fd, status_buf, status_sz) < 0) {

                    cli_nb--;
                }

                /* Check for client hangups. */
                if ((events[i].events & EPOLLHUP) > 0) {
                    if (epoll_ctl(ep_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
                        
                    }

                    rm_fd(events[i].data.fd);
                }
            }
        }
    }

    exit(0);
}
