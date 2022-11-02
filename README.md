# argolib
argolib libaray for argobots.

#### Compile:
```sh
cd argolib/src
source export.env
make clean; make
```

TO run in debug mode enable CFLAGS+=-DDEBUG in argolib/src
To use Trace Replay enable CFLAGS=-DTRACE_REPLAY in argolib/src
To output trace info enable CFLAGS=-DDUMP_TRACE in argolib/src
and then recompile.

#### Test
```sh
cd argolib/test
source export.env
make clean; make
export ARGOLIB_WORKERS=2
```

## TODO:
    Same task is getting stolen, in case > 2 streams.: Fix
    
### Task
Trace Replay
