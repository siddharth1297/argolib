default: libargolib.so

ARGOLIB_INSTALL_DIR=$(dir $(CURDIR))
ARGOLIB_LIB=$(ARGOLIB_INSTALL_DIR)lib
PCM_ROOT=$(PCMROOT)

# Create lib directory
$(shell mkdir -p $(ARGOLIB_LIB))

#CFLAGS=-DMODE_PRIVATE_DQ
#CFLAGS+=-DDEBUG

libargolib.so: argolib.cpp
	g++ -std=c++11 $(CFLAGS) -Werror -shared -O0 -ggdb -fPIC \
		-I../include -I${ARGOBOTS_INSTALL_DIR}/include -I${PCM_ROOT}/src \
		-L${PCM_ROOT}/build/lib \
		-o ../lib/libargolib.so argolib.cpp -lpcm
format:
	clang-format -i ../include/*
	clang-format -i *.cpp
clean:
	rm ../lib/*

