all: gpio_sysfs

clean:
	-rm *.o gpio_sysfs

gpio_sysfs.o: gpio_sysfs.c
	gcc -c -O0 -g -ogpio_sysfs.o gpio_sysfs.c

main.o: main.c
	gcc -c -O0 -g -omain.o main.c

gpio_sysfs: gpio_sysfs.o main.o
	gcc -ogpio_sysfs gpio_sysfs.o main.o
