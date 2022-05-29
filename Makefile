obj-m += ssa.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

ssa_rwtest: ssa_rwtest.o

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf *.o
