# OpenMake view UI

The view's UI that every app of the suite shares (GFOpenMake, the slicer,
Burn and CNC), as shared source. GFOpenLayout's UI is a link library
(`libolui.a`), so this code works on its `olui` directly, the same way the
app's own code does, rather than through a jump table.

| File | What |
|---|---|
| `omview_ui.h`, `view_ui.c` | The status line, a spinner's number, the layer slider's 1 to 100 along the layers and back, and the pen drawer (a picture as runs of one pen each, one `RectFill` per run). Portable; `core/tests/test_view_ui.c` runs it on the host. |
| `omview_gpu.h`, `view_gpu_amiga.c` | The view on OpenGPU, on the Amiga: gfopengpu.library opened and asked whether it draws 3D triangles with depth, a retained picture and depth buffer the view's size, room for the vertices and the stream, and the stream submitted and the picture shown in the window. When any of it is missing the app draws with the pens. Built by `build-amiga.sh`; there is no gfopengpu.library on the host. |
