default: libargolib.so

ARGOLIB_INSTALL_DIR=$(dir $(CURDIR))
ARGOLIB_LIB=$(ARGOLIB_INSTALL_DIR)lib

# Create lib directory
$(shell mkdir -p $(ARGOLIB_LIB))

libargolib.so: argolib.c
	gcc -std=c11 $(CFLAGS) -Werror -shared -O3 -fPIC \
		-I../include -I${ARGOBOTS_INSTALL_DIR}/include \
		-o ../lib/libargolib.so argolib.c
format:
	clang-format -i ../include/*
	clang-format -i *.c
clean:
	rm ../lib/*

