# OpenMake view UI

The view's UI that every app of the suite shares (GFOpenMake, the slicer,
Burn and CNC), as shared source. GFOpenLayout's UI is a link library
(`libolui.a`), so this code works on its `olui` directly, the same way the
app's own code does, rather than through a jump table.

| File | What |
|---|---|
| `omview_ui.h`, `view_ui.c` | The status line, a spinner's number, the layer slider's 1 to 100 along the layers and back, and the pen drawer (a picture as runs of one pen each, one `RectFill` per run). Portable; `core/tests/test_view_ui.c` runs it on the host. |
