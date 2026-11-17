# OpenMake view UI

The view's UI that every app of the suite shares (GFOpenMake, the slicer,
Burn and CNC), as shared source. GFOpenLayout's UI is a link library
(`libolui.a`), so this code works on its `olui` directly, the same way the
app's own code does, rather than through a jump table.

| File | What |
|---|---|
| `omview_ui.h`, `view_ui.c` | The status line, a spinner's number, the layer slider's 1 to 100 along the layers and back, and the pen drawer (a picture as runs of one pen each, one `RectFill` per run). Portable; `core/tests/test_view_ui.c` runs it on the host. |
| `omview_gpu.h`, `view_gpu_amiga.c` | The view on OpenGPU, on the Amiga: gfopengpu.library opened and asked whether it draws 3D triangles with depth, a retained picture and depth buffer the view's size, room for the vertices and the stream, and the stream submitted and the picture shown in the window. When any of it is missing the app draws with the pens. Built by `build-amiga.sh`; there is no gfopengpu.library on the host. |
| `omview_machines.h`, `machines_ui.c` | The machine picker: the machines of one family under `machines/<family>/` (a driver with its `driver.om`, each `profiles/*.om` read over it), sorted by name, the ones that do not load skipped and the first reason kept; a select filled with them, and the chosen one loaded. Portable (opendir: libnix's on the Amiga); `core/tests/test_machine_ui.c` runs it on the test machines in `core/tests/data/machines`. |
| `omview_window.h`, `window_amiga.c` | Every OpenMake window on the Amiga: opened on the default public screen at `omv_window_fit`'s size (640 by 480 inside on a 640 by 512 PAL screen, the whole of a bigger RTG screen below its title bar, no cap), resizable up to the screen's size, and never smaller than `OMV_MIN_W` by `OMV_MIN_H` inside. GFOpenLayout lays the window out again at each `IDCMP_NEWSIZE` and the app's view is drawn at the size it is given, so it grows with the window. `omv_window_fit` is portable (`view_ui.c`) and tested on the host. |
