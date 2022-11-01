default: libargolib.so

ARGOLIB_INSTALL_DIR=$(dir $(CURDIR))
ARGOLIB_LIB=$(ARGOLIB_INSTALL_DIR)lib

# Create lib directory
$(shell mkdir -p $(ARGOLIB_LIB))

#CFLAGS=-DMODE_PRIVATE_DQ
CFLAGS=-DTRACE_REPLAY

# Comment it in live
CFLAGS+=-DDUMP_TRACE

# Use for development
CFLAGS+=-DDEBUG

OP=-O3

ifneq (,$(findstring DEBUG,$(CFLAGS)))
        #OP=-g
        OP=-ggdb -O0
endif

libargolib.so: argolib.c
	gcc -std=c11 $(CFLAGS) -Werror -shared $(OP) -fPIC \
		-I../include -I${ARGOBOTS_INSTALL_DIR}/include \
		-o ../lib/libargolib.so argolib.c
format:
	clang-format -i ../include/*
	clang-format -i *.c
clean:
	rm ../lib/*

