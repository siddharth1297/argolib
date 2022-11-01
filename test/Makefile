
SRCC=$(wildcard *.c)
CTARGETS=$(patsubst %.c,%,$(SRCC))

SRCCPP=$(wildcard *.cpp)
CPPTARGETS=$(patsubst %.cpp,%,$(SRCCPP))

default: $(CTARGETS)

# Use for development
CFLAGS+=-DDEBUG

OP=-O3

ifneq (,$(findstring DEBUG,$(CFLAGS)))
        #OP=-g
        OP=-ggdb -O0
endif

%: %.c
	gcc -std=c11 -Werror $(OP) \
		-I${ARGOLIB_INSTALL_DIR}/include -I${ARGOBOTS_INSTALL_DIR}/include \
		-L${ARGOBOTS_INSTALL_DIR}/lib -L${ARGOLIB_INSTALL_DIR}/lib \
	        -o $*.out $< -largolib -labt

default: $(CPPTARGETS)

%: %.cpp
	g++ -std=c++11 -Werror $(OP) \
                -I${ARGOLIB_INSTALL_DIR}/include -I${ARGOBOTS_INSTALL_DIR}/include \
                -L${ARGOBOTS_INSTALL_DIR}/lib -L${ARGOLIB_INSTALL_DIR}/lib \
                -o $*.out $< -largolib -labt

format:
	clang-format -i *.c
	clang-format -i *.cpp

clean:
	rm *.out trace_report.txt
