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

# OpenMakeCore's slice: a part's layers as a toolpath.
PLAN = core/src/plan.c core/src/fill.c core/src/inset.c core/src/toolpath.c core/src/layer.c core/src/loops.c

all: $(BUILD)/gfopenslicer $(BUILD)/test_slice $(BUILD)/test_model $(BUILD)/test_gpu $(BUILD)/test_edit $(BUILD)/test_layer $(BUILD)/test_make_ui $(BUILD)/test_machine $(BUILD)/test_runtime $(BUILD)/test_machine_lib $(BUILD)/test_path_lib $(BUILD)/test_mesh_lib $(BUILD)/test_gcode_lib $(BUILD)/test_toolpath $(BUILD)/test_inset $(BUILD)/test_fill $(BUILD)/test_gcode $(BUILD)/test_plan $(BUILD)/test_plan_par $(BUILD)/test_plan_one $(BUILD)/test_toolpath_paint $(BUILD)/openmake-slice

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/gfopenslicer: slicer/src/cli.c core/src/stl.c slicer/src/slice.c core/include/gs.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Islicer/include -o $@ slicer/src/cli.c core/src/stl.c slicer/src/slice.c -lm

$(BUILD)/test_slice: slicer/tests/test_slice.c core/src/stl.c slicer/src/slice.c core/src/view.c core/src/solid.c core/include/gs.h core/include/gs_view.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Islicer/include -o $@ slicer/tests/test_slice.c core/src/stl.c slicer/src/slice.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_model: core/tests/test_model.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_model.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_model.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_gpu: core/tests/test_gpu.c core/src/gpu.c $(PLAN) core/src/toolpath_paint.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_gpu.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -I$(SDK) -o $@ core/tests/test_gpu.c core/src/gpu.c $(PLAN) core/src/toolpath_paint.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/preview: slicer/tools/preview.c slicer/src/job.c core/src/stl.c slicer/src/slice.c core/src/view.c core/src/solid.c slicer/src/markup.h core/include/gs.h slicer/include/gs_job.h core/include/gs_view.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Islicer/include -Islicer/src -I$(LAYOUT)/include -o $@ slicer/tools/preview.c slicer/src/job.c core/src/stl.c slicer/src/slice.c core/src/view.c core/src/solid.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

$(BUILD)/test_edit: cad/tests/test_edit.c cad/src/edit.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c cad/include/gs_edit.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icad/include -o $@ cad/tests/test_edit.c cad/src/edit.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_layer: core/tests/test_layer.c core/src/layer.c core/src/layer_paint.c core/src/loops.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_layer.h core/tests/data/nasa-wrench.stl | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_layer.c core/src/layer.c core/src/layer_paint.c core/src/loops.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

# GFOpenMake's window, laid out by GFOpenLayout (LAYOUT, built with make there).
$(BUILD)/test_make_ui: cad/tests/test_make_ui.c cad/src/make_ui.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icad/src -I$(LAYOUT)/include -o $@ cad/tests/test_make_ui.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

# The machine drivers every app shares (machines/), to be openmakemachine.library.
$(BUILD)/test_machine: machines/tests/test_machine.c machines/src/machine.c machines/include/om_machine.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Imachines/include -o $@ machines/tests/test_machine.c machines/src/machine.c

# openmakemachine.library's calls through its jump table, on the host; the
# glue is written by core/library/gen-library.py, and must be as it writes it.
MACHINE_LIB = machines/library/openmakemachine_lib.sfd
$(BUILD)/test_machine_lib: machines/tests/test_machine_lib.c machines/src/machine.c $(MACHINE_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(MACHINE_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Imachines/include -Icore/library -o $@ machines/tests/test_machine_lib.c machines/src/machine.c

# openmakepath.library's calls, the same way.
PATH_LIB = core/library/openmakepath_lib.sfd
# What the library is built from.
PATH_SRC = core/src/layer.c core/src/loops.c core/src/inset.c core/src/toolpath.c core/src/fill.c core/src/plan.c
$(BUILD)/test_path_lib: core/tests/test_path_lib.c $(PATH_SRC) core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c $(PATH_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(PATH_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Icore/library -o $@ core/tests/test_path_lib.c $(PATH_SRC) core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_toolpath: core/tests/test_toolpath.c core/src/toolpath.c core/include/gs_toolpath.h core/include/gs_layer.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_toolpath.c core/src/toolpath.c -lm

$(BUILD)/test_inset: core/tests/test_inset.c core/src/inset.c core/src/layer.c core/src/loops.c core/src/model.c core/src/stl.c core/include/gs_layer.h core/tests/data/nasa-wrench.stl | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_inset.c core/src/inset.c core/src/layer.c core/src/loops.c core/src/model.c core/src/stl.c -lm

$(BUILD)/test_fill: core/tests/test_fill.c core/src/fill.c core/src/toolpath.c core/include/gs_toolpath.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_fill.c core/src/fill.c core/src/toolpath.c -lm

$(BUILD)/test_gcode: core/tests/test_gcode.c core/src/gcode.c core/src/toolpath.c machines/src/machine.c core/include/gs_gcode.h core/include/gs_toolpath.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Imachines/include -o $@ core/tests/test_gcode.c core/src/gcode.c core/src/toolpath.c machines/src/machine.c -lm

$(BUILD)/test_plan: core/tests/test_plan.c $(PLAN) core/src/model.c core/src/stl.c core/include/gs_plan.h core/include/gs_toolpath.h core/tests/data/nasa-wrench.stl | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_plan.c $(PLAN) core/src/model.c core/src/stl.c -lm

# The layers on several tasks: on threads, and all on the caller.
PAR_TEST = core/tests/test_plan_par.c core/src/plan_par.c $(PLAN) core/src/model.c core/src/stl.c
$(BUILD)/test_plan_par: $(PAR_TEST) core/src/par_posix.c core/include/gs_par.h core/include/gs_plan.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -pthread -Icore/include -o $@ $(PAR_TEST) core/src/par_posix.c -lm

$(BUILD)/test_plan_one: $(PAR_TEST) core/src/par_one.c core/include/gs_par.h core/include/gs_plan.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ $(PAR_TEST) core/src/par_one.c -lm

$(BUILD)/test_toolpath_paint: core/tests/test_toolpath_paint.c core/src/toolpath_paint.c $(PLAN) core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_toolpath.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_toolpath_paint.c core/src/toolpath_paint.c $(PLAN) core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

# A part to G-code for a machine, from the shell.
$(BUILD)/openmake-slice: slicer/src/openmake_slice.c $(PLAN) core/src/toolpath_paint.c core/src/view.c core/src/solid.c core/src/gcode.c core/src/model.c core/src/stl.c machines/src/machine.c core/include/gs_plan.h core/include/gs_gcode.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Imachines/include -o $@ slicer/src/openmake_slice.c $(PLAN) core/src/toolpath_paint.c core/src/view.c core/src/solid.c core/src/gcode.c core/src/model.c core/src/stl.c machines/src/machine.c -lm

# openmakemesh.library's calls, the same way.
MESH_LIB = core/library/openmakemesh_lib.sfd
$(BUILD)/test_mesh_lib: core/tests/test_mesh_lib.c core/src/model.c core/src/stl.c $(MESH_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(MESH_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Icore/library -o $@ core/tests/test_mesh_lib.c core/src/model.c core/src/stl.c -lm

# openmakegcode.library's calls, the same way.
GCODE_LIB = core/library/openmakegcode_lib.sfd
$(BUILD)/test_gcode_lib: core/tests/test_gcode_lib.c core/src/gcode.c core/src/toolpath.c machines/src/machine.c $(GCODE_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(GCODE_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Imachines/include -Icore/library -o $@ core/tests/test_gcode_lib.c core/src/gcode.c core/src/toolpath.c machines/src/machine.c -lm

# The C runtime the OpenMake libraries are built with, under rt_ names.
$(BUILD)/test_runtime: core/tests/test_runtime.c core/library/runtime.c core/library/runtime.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -DOM_RT_HOST -o $@ core/tests/test_runtime.c core/library/runtime.c -lm

test: $(BUILD)/test_slice $(BUILD)/test_model $(BUILD)/test_gpu $(BUILD)/test_edit $(BUILD)/test_layer $(BUILD)/test_make_ui $(BUILD)/test_machine $(BUILD)/test_runtime $(BUILD)/test_machine_lib $(BUILD)/test_path_lib $(BUILD)/test_mesh_lib $(BUILD)/test_gcode_lib $(BUILD)/test_toolpath $(BUILD)/test_inset $(BUILD)/test_fill $(BUILD)/test_gcode $(BUILD)/test_plan $(BUILD)/test_plan_par $(BUILD)/test_plan_one $(BUILD)/test_toolpath_paint $(BUILD)/openmake-slice
	$(BUILD)/test_slice
	$(BUILD)/test_model
	$(BUILD)/test_gpu
	$(BUILD)/test_edit
	$(BUILD)/test_layer
	$(BUILD)/test_make_ui
	$(BUILD)/test_machine
	$(BUILD)/test_runtime
	$(BUILD)/test_machine_lib
	$(BUILD)/test_path_lib
	$(BUILD)/test_mesh_lib
	$(BUILD)/test_gcode_lib
	$(BUILD)/test_toolpath
	$(BUILD)/test_inset
	$(BUILD)/test_fill
	$(BUILD)/test_gcode
	$(BUILD)/test_plan
	$(BUILD)/test_plan_par
	$(BUILD)/test_plan_one
	$(BUILD)/test_toolpath_paint
	$(BUILD)/openmake-slice core/tests/data/nasa-wrench.stl $(BUILD)/wrench.gcode \
	    DRIVER=machines/printer/marlin/driver.om PROFILE=machines/printer/marlin/profiles/generic-220.om \
	    PREVIEW=$(BUILD)/wrench.ppm SIZE=320x240
	head -c 15 $(BUILD)/wrench.ppm | grep -q '^320 240' && test "$$(wc -c < $(BUILD)/wrench.ppm)" = 230415
	test "$$(grep -c '^; LAYER ' $(BUILD)/wrench.gcode)" = 130 && grep -q '^M109 S200' $(BUILD)/wrench.gcode && tail -n 1 $(BUILD)/wrench.gcode | grep -q M84
	! $(BUILD)/openmake-slice core/tests/data/nasa-wrench.stl $(BUILD)/tiny.gcode \
	    DRIVER=machines/printer/marlin/driver.om PROFILE=slicer/tests/data/tiny-bed.om 2> $(BUILD)/tiny.txt
	grep -q 'Tiny 50 cannot print this: [XY] runs to' $(BUILD)/tiny.txt && test ! -e $(BUILD)/tiny.gcode
	@echo "slice command ok"

preview: $(BUILD)/preview

clean:
	rm -rf $(BUILD)
