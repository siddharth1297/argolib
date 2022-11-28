
SRCC=$(wildcard *.c)
CTARGETS=$(patsubst %.c,%,$(SRCC))


SRCCPP=$(wildcard *.cpp)
CPPTARGETS=$(patsubst %.cpp,%,$(SRCCPP))


#default: $(CTARGETS)

#%: %.c
#	gcc -std=c11 -Werror -O3 \
#		-I${ARGOLIB_INSTALL_DIR}/include -I${ARGOBOTS_INSTALL_DIR}/include   \
#		-L${ARGOBOTS_INSTALL_DIR}/lib -L${ARGOLIB_INSTALL_DIR}/lib  \
#	        -o $*.out $< -largolib -labt 

default: $(CPPTARGETS)

%: %.cpp
	g++ -std=c++11 -Werror -O0 -ggdb \
                -I${ARGOLIB_INSTALL_DIR}/include -I${ARGOBOTS_INSTALL_DIR}/include -I${PCMROOT}/src \
                -L${ARGOBOTS_INSTALL_DIR}/lib -L${ARGOLIB_INSTALL_DIR}/lib -L${PCMROOT}/build/lib \
                -o $*.out $< -largolib -labt -lpcm

format:
	clang-format -i *.c
	clang-format -i *.cpp

clean:
	rm *.out
