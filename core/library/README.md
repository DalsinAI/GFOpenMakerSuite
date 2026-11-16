# The OpenMake libraries

The suite's shared code is built as Amiga shared libraries, each loaded once
and shared by every app: `openmakemachine.library` first, then
`openmakepath`, `openmakemesh` and `openmakegcode`.

| file | what it is |
|---|---|
| `runtime.c`, `runtime.h` | The C runtime a library is built with: memory over `AllocVec`, strings, `qsort`, `snprintf`, `strtod` and the maths, on exec.library alone (a library has no start-up code). `test_runtime` holds it to the host's. |
| `omlib.h` | What a library and its callers share: `REG()`, and a float carried in a data register as its bits. |
| `gen-library.py` | Writes a library's glue from its `.sfd`: the inline calls and proto header for programs, the library's side of each call and its jump table, and a link library with the C API through the library. `make test` checks the files are as it writes them. |

Each library's `.sfd` is the list of its calls, in order. Never reorder or
take one out; add new calls at the end. The C function is the call's name
in snake case (`OM_ClampSpeed` is `om_clamp_speed`). Run
`python3 core/library/gen-library.py <app>/library/<name>_lib.sfd` after
changing it.

Each library is built twice under one name, with the FPU and without it; the
installer picks one. The FPU build will not open on a machine without one.
