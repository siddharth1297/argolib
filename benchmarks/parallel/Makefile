TARGETS := fib array_sum qsort iterative

all: clean $(TARGETS) clean-obj

ARGOLIB_CXXFLAGS=-I../../include/
ARGOLIB_CXXFLAGS+=-I${ARGOBOTS_INSTALL_DIR}/include

ARGOLIB_LDFLAGS+=-L../../lib/
ARGOLIB_LDFLAGS+=-L${ARGOBOTS_INSTALL_DIR}/lib

ARGOLIB_LDLIBS+=-largolib -labt
ARGOLIB_LDLIBS+=-labt

%: %.cpp
	$(CXX) -O3 $(ARGOLIB_CXXFLAGS) $(ARGOLIB_LDFLAGS) -o $@ $^ $(ARGOLIB_LDLIBS) -lm

clean-obj:
	rm -rf *.o

clean:
	rm -rf *.o $(TARGETS)
