# OpenMakeCore

The part of the GFOpenMake suite that every app shares: the cad editor,
the slicer, and later the laser (burn) and CNC apps.

- `src/model.c`: meshes and the booleans (union, cut, intersect), sealed
  watertight, and the binary STL writer.
- `src/stl.c`: reading STL, and repair.
- `src/layer.c`: one horizontal cut through a mesh.
- `src/view.c`, `src/solid.c`: the camera, the 220 mm plate and the CPU
  picture.
- `src/gpu.c`: the same picture as an OpenGPU triangle batch.

The headers are in `include/`, the tests in `tests/`. Plain C99, MIT. On
the Amiga these become shared libraries; on the host they build straight
into the tests.
