obj-m += ass3R.o
obj-m += ass3W.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) testW.c -o testW
	$(CC) testR.c -o testR
 
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
