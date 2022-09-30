gcc -O3 -I../include -I/usr/local/argobots-install/include \
	-L/usr/local/argobots-install/lib -L../lib \
	-o fib.out fibonacci.c -largolib -labt
