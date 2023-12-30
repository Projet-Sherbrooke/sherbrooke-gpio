all: gpio_sysfs

CFLAGS=-c -O0 -g -Wall

clean:
	-rm *.o gpio_sysfs

gpio_sysfs.o: gpio_sysfs.c
	gcc $(CFLAGS) -ogpio_sysfs.o gpio_sysfs.c

main.o: main.c
	gcc $(CFLAGS) -omain.o main.c

gpio_sysfs: gpio_sysfs.o main.o
	gcc -ogpio_sysfs gpio_sysfs.o main.o
