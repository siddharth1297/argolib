# https://stackoverflow.com/questions/14884126/build-so-file-from-c-file-using-gcc-command-line
mkdir ../lib
gcc -I../include -I/usr/local/argobots-install/include -shared -o ../lib/libargolib.so -fPIC argolib.c
