default: libargolib.so

ARGOLIB_INSTALL_DIR=$(dir $(CURDIR))
ARGOLIB_LIB=$(ARGOLIB_INSTALL_DIR)lib
PCM_ROOT=$(PCMROOT)

# Create lib directory
$(shell mkdir -p $(ARGOLIB_LIB))

#CFLAGS=-DMODE_PRIVATE_DQ

libargolib.so: argolib.c
	g++ -std=c11 $(CFLAGS) -Werror -shared -O3 -fPIC \
		-I../include -I${ARGOBOTS_INSTALL_DIR}/include -I${PCM_ROOT}/src \
		-o ../lib/libargolib.so argolib.cpp
format:
	clang-format -i ../include/*
	clang-format -i *.cpp
clean:
	rm ../lib/*

