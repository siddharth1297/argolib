# argolib
argolib libaray for argobots.

#### Compile:
```sh
cd argolib/src
source export.env
make clean; make
```

Default compilation is with RAND_WS.
To use private deque version, enable CFLAGS=-DMODE_PRIVATE_DQ in argolib/src
and then recompile.

#### Test
```sh
cd argolib/test
source export.env
make clean; make
export ARGOLIB_WORKERS=2
```

## TODO:
    Remove commented code.: Done
    Structure code.
    Add documentations.
    Check if working in other directories.: Done
    
### Paper reffered to remove overhead
https://hal.inria.fr/hal-00863028/document
