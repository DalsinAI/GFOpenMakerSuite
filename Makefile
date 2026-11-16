# Host build of the GFOpenMake suite. core/ is OpenMakeCore, shared by the
# apps: the mesh and booleans, STL, the layer cut, the plate and the view.
# The Amiga windows are slicer/src/amiga.c (OpenMakeSlicer) and cad/src/make.c (OpenMakeCad, the editor).
CC ?= gcc
CFLAGS ?= -O2
WARN = -std=c99 -Wall -Wextra -Werror -pedantic
BUILD = build
SDK ?= /tmp/sdk/Include
LAYOUT ?= ../gfopenlayout

.PHONY: all test preview clean

all: $(BUILD)/gfopenslicer $(BUILD)/test_slice $(BUILD)/test_model $(BUILD)/test_gpu $(BUILD)/test_edit $(BUILD)/test_layer $(BUILD)/test_make_ui $(BUILD)/test_machine

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/gfopenslicer: slicer/src/cli.c core/src/stl.c slicer/src/slice.c core/include/gs.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Islicer/include -o $@ slicer/src/cli.c core/src/stl.c slicer/src/slice.c -lm

$(BUILD)/test_slice: slicer/tests/test_slice.c core/src/stl.c slicer/src/slice.c core/src/view.c core/src/solid.c core/include/gs.h core/include/gs_view.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Islicer/include -o $@ slicer/tests/test_slice.c core/src/stl.c slicer/src/slice.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_model: core/tests/test_model.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_model.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_model.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_gpu: core/tests/test_gpu.c core/src/gpu.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_gpu.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -I$(SDK) -o $@ core/tests/test_gpu.c core/src/gpu.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/preview: slicer/tools/preview.c slicer/src/job.c core/src/stl.c slicer/src/slice.c core/src/view.c core/src/solid.c slicer/src/markup.h core/include/gs.h slicer/include/gs_job.h core/include/gs_view.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Islicer/include -Islicer/src -I$(LAYOUT)/include -o $@ slicer/tools/preview.c slicer/src/job.c core/src/stl.c slicer/src/slice.c core/src/view.c core/src/solid.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

$(BUILD)/test_edit: cad/tests/test_edit.c cad/src/edit.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c cad/include/gs_edit.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icad/include -o $@ cad/tests/test_edit.c cad/src/edit.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_layer: core/tests/test_layer.c core/src/layer.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_layer.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_layer.c core/src/layer.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

# GFOpenMake's window, laid out by GFOpenLayout (LAYOUT, built with make there).
$(BUILD)/test_make_ui: cad/tests/test_make_ui.c cad/src/make_ui.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icad/src -I$(LAYOUT)/include -o $@ cad/tests/test_make_ui.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

# The machine drivers every app shares (machines/), to be openmakemachine.library.
$(BUILD)/test_machine: machines/tests/test_machine.c machines/src/machine.c machines/include/om_machine.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Imachines/include -o $@ machines/tests/test_machine.c machines/src/machine.c

test: $(BUILD)/test_slice $(BUILD)/test_model $(BUILD)/test_gpu $(BUILD)/test_edit $(BUILD)/test_layer $(BUILD)/test_make_ui $(BUILD)/test_machine
	$(BUILD)/test_slice
	$(BUILD)/test_model
	$(BUILD)/test_gpu
	$(BUILD)/test_edit
	$(BUILD)/test_layer
	$(BUILD)/test_make_ui
	$(BUILD)/test_machine

preview: $(BUILD)/preview

clean:
	rm -rf $(BUILD)
