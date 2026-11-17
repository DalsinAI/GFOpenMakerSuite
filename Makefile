# Host build of the GFOpenMake suite. core/ is OpenMakeCore, shared by the
# apps: the mesh and booleans, STL, the layer cut, the plate and the view.
# The Amiga windows are slicer/src/amiga.c (OpenMakeSlicer), burn/src/amiga.c (OpenMakeBurn),
# and cad/src/make.c (OpenMakeCad, the editor).
CC ?= gcc
CFLAGS ?= -O2
WARN = -std=c99 -Wall -Wextra -Werror -pedantic
BUILD = build
SDK ?= /tmp/sdk/Include
LAYOUT ?= ../gfopenlayout

.PHONY: all test preview clean

# OpenMakeCore's slice: a part's layers as a toolpath.
PLAN = core/src/plan.c core/src/fill.c core/src/inset.c core/src/union.c core/src/toolpath.c core/src/layer.c core/src/loops.c
SL_JOB = slicer/src/sl_job.c slicer/src/sl_args.c $(PLAN) core/src/plan_par.c core/src/par_posix.c \
	core/src/gcode.c core/src/settings.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c machines/src/machine.c

all: $(BUILD)/gfopenslicer $(BUILD)/test_stl $(BUILD)/test_model $(BUILD)/test_solids $(BUILD)/test_parts $(BUILD)/test_gpu $(BUILD)/test_edit $(BUILD)/test_make_rexx $(BUILD)/test_layer $(BUILD)/test_make_ui $(BUILD)/test_slicer_ui $(BUILD)/test_sl_job $(BUILD)/test_view_ui $(BUILD)/test_view_gpu $(BUILD)/test_machine_ui $(BUILD)/test_machine $(BUILD)/test_runtime $(BUILD)/test_machine_lib $(BUILD)/test_path_lib $(BUILD)/test_mesh_lib $(BUILD)/test_gcode_lib $(BUILD)/test_view_lib $(BUILD)/test_task_lib $(BUILD)/test_vec $(BUILD)/test_vec_lib $(BUILD)/test_project $(BUILD)/test_project_lib $(BUILD)/test_rexx $(BUILD)/test_rexx_lib $(BUILD)/test_dxf $(BUILD)/test_toolpath $(BUILD)/test_inset $(BUILD)/test_fill $(BUILD)/test_gcode $(BUILD)/test_plan $(BUILD)/test_plan_par $(BUILD)/test_plan_one $(BUILD)/test_toolpath_paint $(BUILD)/test_settings $(BUILD)/test_settings_ui $(BUILD)/test_sl_args $(BUILD)/test_profiles $(BUILD)/test_burn $(BUILD)/test_burn_ui $(BUILD)/test_workshop_ui $(BUILD)/openmake-slice $(BUILD)/openmake-burn $(BUILD)/openmake-workshop

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/gfopenslicer: slicer/src/cli.c slicer/include/sl_job.h $(SL_JOB) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -pthread -Icore/include -Islicer/include -Imachines/include -o $@ slicer/src/cli.c $(sort $(SL_JOB)) -lm

# The bench's other solids (core/src/model.c).
$(BUILD)/test_parts: core/tests/test_parts.c core/src/parts.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_parts.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_parts.c core/src/parts.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_solids: core/tests/test_solids.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_model.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_solids.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_stl: core/tests/test_stl.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs.h core/include/gs_view.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_stl.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_model: core/tests/test_model.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_model.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_model.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_gpu: core/tests/test_gpu.c core/src/gpu.c $(PLAN) core/src/toolpath_paint.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_gpu.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -I$(SDK) -o $@ core/tests/test_gpu.c core/src/gpu.c $(PLAN) core/src/toolpath_paint.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/preview: slicer/tools/preview.c slicer/src/markup.h slicer/src/slicer_ui.c core/ui/settings_ui.c core/ui/view_ui.c core/ui/view_gpu.c core/ui/machines_ui.c $(SL_JOB) core/src/toolpath_paint.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -pthread -Icore/include -Icore/ui -Islicer/include -Islicer/src -Imachines/include -I$(LAYOUT)/include -I$(SDK) -o $@ \
	    slicer/tools/preview.c slicer/src/slicer_ui.c core/ui/settings_ui.c core/ui/view_ui.c core/ui/view_gpu.c core/src/gpu.c core/ui/machines_ui.c $(sort $(SL_JOB)) core/src/toolpath_paint.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

$(BUILD)/test_edit: cad/tests/test_edit.c cad/src/edit.c core/src/parts.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c cad/include/gs_edit.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icad/include -o $@ cad/tests/test_edit.c cad/src/edit.c core/src/parts.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

# GFOpenMake's ARexx commands (cad/src/make_rexx.c), on the host stub.
$(BUILD)/test_make_rexx: cad/tests/test_make_rexx.c cad/src/make_rexx.c cad/src/edit.c core/src/rexx.c $(PLAN) core/src/gcode.c core/src/settings.c core/src/parts.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c machines/src/machine.c cad/src/make_rexx.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/src -Icad/include -Icad/src -Imachines/include -o $@ \
	    cad/tests/test_make_rexx.c cad/src/make_rexx.c cad/src/edit.c core/src/rexx.c $(PLAN) \
	    core/src/gcode.c core/src/settings.c core/src/parts.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c machines/src/machine.c -lm

$(BUILD)/test_layer: core/tests/test_layer.c core/src/layer.c core/src/layer_paint.c core/src/loops.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c core/include/gs_layer.h core/tests/data/nasa-wrench.stl | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_layer.c core/src/layer.c core/src/layer_paint.c core/src/loops.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

# GFOpenMake's window, laid out by GFOpenLayout (LAYOUT, built with make there).
$(BUILD)/test_make_ui: cad/tests/test_make_ui.c cad/src/make_ui.h core/ui/settings_ui.c core/src/settings.c core/src/parts.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icad/src -Icore/include -Icore/ui -Imachines/include -I$(LAYOUT)/include -o $@ cad/tests/test_make_ui.c core/src/parts.c \
	    core/ui/settings_ui.c core/ui/view_ui.c core/src/settings.c $(PLAN) core/src/gcode.c machines/src/machine.c core/src/view.c core/src/solid.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

# GFOpenSlicer's window at 640 by 480, its status and numbers through core/ui.
$(BUILD)/test_slicer_ui: slicer/tests/test_slicer_ui.c slicer/src/markup.h slicer/src/slicer_ui.c core/ui/settings_ui.c core/ui/view_ui.c core/ui/omview_ui.h core/src/view.c core/src/solid.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -Islicer/src -Imachines/include -I$(LAYOUT)/include -o $@ slicer/tests/test_slicer_ui.c \
	    slicer/src/slicer_ui.c $(PLAN) core/src/gcode.c core/src/settings.c machines/src/machine.c core/src/model.c core/src/stl.c \
	    core/ui/view_ui.c core/ui/settings_ui.c core/src/view.c core/src/solid.c $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

# The print settings panel both windows share, in a side column at 640 by 480.
$(BUILD)/test_settings_ui: core/tests/test_settings_ui.c core/ui/settings_ui.c core/ui/omview_settings.h core/ui/view_ui.c core/src/settings.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -Imachines/include -I$(LAYOUT)/include -o $@ core/tests/test_settings_ui.c \
	    core/ui/settings_ui.c core/ui/view_ui.c core/src/settings.c $(PLAN) core/src/gcode.c machines/src/machine.c core/src/view.c core/src/solid.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

# OpenMakeSlicer's job: the toolpath engine on tasks, checked and written.
$(BUILD)/test_sl_job: slicer/tests/test_sl_job.c slicer/include/sl_job.h $(SL_JOB) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -pthread -Icore/include -Islicer/include -Imachines/include -o $@ slicer/tests/test_sl_job.c \
	    $(sort $(SL_JOB)) -lm

# The machine picker the apps share (core/ui), on the test machines.
$(BUILD)/test_machine_ui: core/tests/test_machine_ui.c core/ui/machines_ui.c core/ui/omview_machines.h machines/src/machine.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/ui -Imachines/include -I$(LAYOUT)/include -o $@ core/tests/test_machine_ui.c \
	    core/ui/machines_ui.c machines/src/machine.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

# The view's UI the apps share (core/ui), in a GFOpenLayout window.
$(BUILD)/test_view_gpu: core/tests/test_view_gpu.c core/ui/view_gpu.c core/ui/omview_scene.h core/src/gpu.c $(PLAN) core/src/toolpath_paint.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -I$(SDK) -o $@ core/tests/test_view_gpu.c core/ui/view_gpu.c core/src/gpu.c $(PLAN) \
	    core/src/toolpath_paint.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_view_ui: core/tests/test_view_ui.c core/ui/view_ui.c core/ui/omview_ui.h core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -I$(LAYOUT)/include -o $@ core/tests/test_view_ui.c core/ui/view_ui.c \
	    core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

# The machine drivers every app shares (machines/), to be openmakemachine.library.
$(BUILD)/test_machine: machines/tests/test_machine.c machines/src/machine.c machines/include/om_machine.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Imachines/include -o $@ machines/tests/test_machine.c machines/src/machine.c

# Every machine profile, loaded over its driver; the test target slices with each printer.
$(BUILD)/test_profiles: machines/tests/test_profiles.c machines/src/machine.c machines/include/om_machine.h $(wildcard machines/printer/*/driver.om machines/printer/*/profiles/*.om machines/laser/*/driver.om machines/laser/*/profiles/*.om) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Imachines/include -o $@ machines/tests/test_profiles.c machines/src/machine.c

# OpenMakeBurn: an SVG to Grbl laser G-code (line mode).
BURN_JOB = burn/src/burn_job.c burn/src/material.c burn/src/image.c core/src/vec.c core/src/dxf.c core/src/gcode.c core/src/toolpath.c core/src/fill.c core/src/loops.c core/src/inset.c core/src/union.c machines/src/machine.c
$(BUILD)/test_burn: burn/tests/test_burn.c burn/include/burn_job.h burn/include/burn_material.h $(BURN_JOB) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Imachines/include -Iburn/include -o $@ burn/tests/test_burn.c $(BURN_JOB) -lm

$(BUILD)/openmake-burn: burn/src/cli.c burn/include/burn_job.h burn/include/burn_material.h $(BURN_JOB) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Imachines/include -Iburn/include -o $@ burn/src/cli.c $(BURN_JOB) -lm

# OpenMakeWorkshop, from the shell: a project drawer and its project.om.
$(BUILD)/openmake-workshop: workshop/src/cli.c core/src/project.c core/include/gs_project.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ workshop/src/cli.c core/src/project.c

WS_UI = workshop/src/workshop_ui.c core/src/project.c core/src/stl.c core/src/view.c core/src/solid.c core/src/vec.c core/src/dxf.c core/ui/view_ui.c

$(BUILD)/test_workshop_ui: workshop/tests/test_workshop_ui.c workshop/src/markup.h workshop/src/workshop_ui.h workshop/src/workshop_ui.c $(WS_UI) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -Iworkshop/src -I$(LAYOUT)/include -o $@ \
	    workshop/tests/test_workshop_ui.c workshop/src/workshop_ui.c core/src/project.c core/src/stl.c core/src/view.c core/src/solid.c core/src/vec.c core/src/dxf.c core/ui/view_ui.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

$(BUILD)/workshop_preview: workshop/tools/preview.c workshop/src/markup.h workshop/src/workshop_ui.h workshop/src/workshop_ui.c $(WS_UI) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -Iworkshop/src -I$(LAYOUT)/include -o $@ \
	    workshop/tools/preview.c workshop/src/workshop_ui.c core/src/project.c core/src/stl.c core/src/view.c core/src/solid.c core/src/vec.c core/src/dxf.c core/ui/view_ui.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm


# OpenMakeBurn's window at 640 by 480, its status and layer list.
$(BUILD)/test_burn_ui: burn/tests/test_burn_ui.c burn/src/markup.h burn/src/burn_ui.c burn/src/burn_ui.h $(BURN_JOB) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -Iburn/include -Iburn/src -Imachines/include -I$(LAYOUT)/include -o $@ \
	    burn/tests/test_burn_ui.c burn/src/burn_ui.c $(BURN_JOB) core/ui/view_ui.c core/ui/machines_ui.c \
	    core/src/view.c core/src/solid.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

$(BUILD)/burn_preview: burn/tools/preview.c burn/src/markup.h burn/src/burn_ui.c $(BURN_JOB) core/ui/view_gpu.c core/src/gpu.c core/src/toolpath_paint.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -Iburn/include -Iburn/src -Imachines/include -I$(LAYOUT)/include -I$(SDK) -o $@ \
	    burn/tools/preview.c burn/src/burn_ui.c $(BURN_JOB) core/ui/view_ui.c core/ui/view_gpu.c core/src/gpu.c \
	    core/ui/machines_ui.c core/src/toolpath_paint.c core/src/view.c core/src/solid.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

# openmakemachine.library's calls through its jump table, on the host; the
# glue is written by core/library/gen-library.py, and must be as it writes it.
MACHINE_LIB = machines/library/openmakemachine_lib.sfd
$(BUILD)/test_machine_lib: machines/tests/test_machine_lib.c machines/src/machine.c $(MACHINE_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(MACHINE_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Imachines/include -Icore/library -o $@ machines/tests/test_machine_lib.c machines/src/machine.c

# openmakepath.library's calls, the same way.
PATH_LIB = core/library/openmakepath_lib.sfd
# What the library is built from.
PATH_SRC = core/src/layer.c core/src/loops.c core/src/inset.c core/src/union.c core/src/toolpath.c core/src/fill.c core/src/plan.c
$(BUILD)/test_path_lib: core/tests/test_path_lib.c $(PATH_SRC) core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c $(PATH_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(PATH_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Icore/library -o $@ core/tests/test_path_lib.c $(PATH_SRC) core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c -lm

$(BUILD)/test_toolpath: core/tests/test_toolpath.c core/src/toolpath.c core/include/gs_toolpath.h core/include/gs_layer.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_toolpath.c core/src/toolpath.c -lm

$(BUILD)/test_inset: core/tests/test_inset.c core/src/inset.c core/src/union.c core/src/layer.c core/src/loops.c core/src/model.c core/src/stl.c core/include/gs_layer.h core/tests/data/nasa-wrench.stl | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_inset.c core/src/inset.c core/src/union.c core/src/layer.c core/src/loops.c core/src/model.c core/src/stl.c -lm

$(BUILD)/test_fill: core/tests/test_fill.c core/src/fill.c core/src/toolpath.c core/include/gs_toolpath.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_fill.c core/src/fill.c core/src/toolpath.c -lm

$(BUILD)/test_gcode: core/tests/test_gcode.c core/src/gcode.c core/src/toolpath.c machines/src/machine.c core/include/gs_gcode.h core/include/gs_toolpath.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -DGC_CALLOC=no_room_calloc -Icore/include -Imachines/include -o $@ core/tests/test_gcode.c core/src/gcode.c core/src/toolpath.c machines/src/machine.c -lm

$(BUILD)/test_settings: core/tests/test_settings.c core/src/settings.c core/src/gcode.c $(PLAN) machines/src/machine.c core/include/gs_settings.h core/include/gs_gcode.h core/include/gs_plan.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Imachines/include -o $@ core/tests/test_settings.c core/src/settings.c core/src/gcode.c $(PLAN) machines/src/machine.c -lm

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

# The slicing commands' settings arguments (slicer/src/sl_args.c).
$(BUILD)/test_sl_args: slicer/tests/test_sl_args.c slicer/src/sl_args.c slicer/include/sl_args.h core/src/settings.c settings/standard | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Islicer/include -Imachines/include -o $@ slicer/tests/test_sl_args.c \
	    slicer/src/sl_args.c core/src/settings.c $(PLAN) core/src/gcode.c machines/src/machine.c core/src/view.c core/src/solid.c -lm

# A part to G-code for a machine, from the shell.
$(BUILD)/openmake-slice: slicer/src/openmake_slice.c slicer/include/sl_job.h $(SL_JOB) core/src/toolpath_paint.c core/include/gs_plan.h core/include/gs_gcode.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -pthread -Icore/include -Islicer/include -Imachines/include -o $@ slicer/src/openmake_slice.c $(sort $(SL_JOB)) core/src/toolpath_paint.c -lm

# openmakemesh.library's calls, the same way.
MESH_LIB = core/library/openmakemesh_lib.sfd
$(BUILD)/test_mesh_lib: core/tests/test_mesh_lib.c core/src/model.c core/src/parts.c core/src/stl.c $(MESH_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(MESH_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Icore/library -o $@ core/tests/test_mesh_lib.c core/src/model.c core/src/parts.c core/src/stl.c -lm

# openmakeview.library's calls, the same way.
VIEW_LIB = core/library/openmakeview_lib.sfd
VIEW_SRC = core/src/solid.c core/src/view.c core/src/layer_paint.c core/src/toolpath_paint.c core/src/toolpath.c core/src/gpu.c
$(BUILD)/test_view_lib: core/tests/test_view_lib.c $(VIEW_SRC) $(PLAN) core/src/model.c core/src/stl.c $(VIEW_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(VIEW_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Icore/library -I$(SDK) -o $@ core/tests/test_view_lib.c $(sort $(VIEW_SRC) $(PLAN)) core/src/model.c core/src/stl.c -lm

# openmakegcode.library's calls, the same way.
GCODE_LIB = core/library/openmakegcode_lib.sfd
$(BUILD)/test_gcode_lib: core/tests/test_gcode_lib.c core/src/gcode.c core/src/toolpath.c machines/src/machine.c $(GCODE_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(GCODE_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Imachines/include -Icore/library -o $@ core/tests/test_gcode_lib.c core/src/gcode.c core/src/toolpath.c machines/src/machine.c -lm

# openmaketask.library's calls, the same way: on threads here.
TASK_LIB = core/library/openmaketask_lib.sfd
$(BUILD)/test_task_lib: core/tests/test_task_lib.c core/src/par_posix.c core/include/gs_par.h $(TASK_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(TASK_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -pthread -DOM_LIB_HOST -Icore/include -Icore/library -o $@ core/tests/test_task_lib.c core/src/par_posix.c

# 2D vector drawings, the SVG reader and the ASCII DXF reader (openmakevec.library).
$(BUILD)/test_vec: core/tests/test_vec.c core/src/vec.c core/src/dxf.c core/include/gs_vec.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_vec.c core/src/vec.c core/src/dxf.c -lm

$(BUILD)/test_dxf: core/tests/test_dxf.c core/src/vec.c core/src/dxf.c core/include/gs_vec.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_dxf.c core/src/vec.c core/src/dxf.c -lm

VEC_LIB = core/library/openmakevec_lib.sfd
$(BUILD)/test_vec_lib: core/tests/test_vec_lib.c core/src/vec.c core/src/dxf.c $(VEC_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(VEC_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Icore/library -o $@ core/tests/test_vec_lib.c core/src/vec.c core/src/dxf.c -lm

# A GFOpenMakerWorkshop project (openmakeproject.library).
$(BUILD)/test_project: core/tests/test_project.c core/src/project.c core/include/gs_project.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -o $@ core/tests/test_project.c core/src/project.c

PROJ_LIB = core/library/openmakeproject_lib.sfd
$(BUILD)/test_project_lib: core/tests/test_project_lib.c core/src/project.c $(PROJ_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(PROJ_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Icore/library -o $@ core/tests/test_project_lib.c core/src/project.c

# ARexx parser, dispatch, and the host stub for a port and a client
# (openmakerexx.library). The Amiga port is core/src/rexx_amiga.c.
$(BUILD)/test_rexx: core/tests/test_rexx.c core/src/rexx.c core/include/gs_rexx.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/src -o $@ core/tests/test_rexx.c core/src/rexx.c

REXX_LIB = core/library/openmakerexx_lib.sfd
$(BUILD)/test_rexx_lib: core/tests/test_rexx_lib.c core/src/rexx.c $(REXX_LIB) core/library/gen-library.py core/library/omlib.h | $(BUILD)
	python3 core/library/gen-library.py $(REXX_LIB) --check
	$(CC) $(CFLAGS) $(WARN) -DOM_LIB_HOST -Icore/include -Icore/library -Icore/src -o $@ core/tests/test_rexx_lib.c core/src/rexx.c

# The C runtime the OpenMake libraries are built with, under rt_ names.
$(BUILD)/test_runtime: core/tests/test_runtime.c core/library/runtime.c core/library/runtime.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -DOM_RT_HOST -o $@ core/tests/test_runtime.c core/library/runtime.c -lm

test: $(BUILD)/test_stl $(BUILD)/test_model $(BUILD)/test_solids $(BUILD)/test_parts $(BUILD)/test_gpu $(BUILD)/test_edit $(BUILD)/test_make_rexx $(BUILD)/test_layer $(BUILD)/test_make_ui $(BUILD)/test_slicer_ui $(BUILD)/test_sl_job $(BUILD)/test_view_ui $(BUILD)/test_view_gpu $(BUILD)/test_machine_ui $(BUILD)/test_machine $(BUILD)/test_runtime $(BUILD)/test_machine_lib $(BUILD)/test_path_lib $(BUILD)/test_mesh_lib $(BUILD)/test_gcode_lib $(BUILD)/test_view_lib $(BUILD)/test_task_lib $(BUILD)/test_vec $(BUILD)/test_vec_lib $(BUILD)/test_project $(BUILD)/test_project_lib $(BUILD)/test_rexx $(BUILD)/test_rexx_lib $(BUILD)/test_dxf $(BUILD)/test_toolpath $(BUILD)/test_inset $(BUILD)/test_fill $(BUILD)/test_gcode $(BUILD)/test_plan $(BUILD)/test_plan_par $(BUILD)/test_plan_one $(BUILD)/test_toolpath_paint $(BUILD)/test_settings $(BUILD)/test_settings_ui $(BUILD)/test_sl_args $(BUILD)/test_profiles $(BUILD)/test_burn $(BUILD)/test_burn_ui $(BUILD)/test_workshop_ui $(BUILD)/openmake-slice $(BUILD)/openmake-burn $(BUILD)/openmake-workshop $(BUILD)/gfopenslicer
	$(BUILD)/test_stl
	$(BUILD)/test_model
	$(BUILD)/test_solids
	$(BUILD)/test_parts
	$(BUILD)/test_gpu
	$(BUILD)/test_edit
	$(BUILD)/test_make_rexx
	$(BUILD)/test_layer
	$(BUILD)/test_make_ui
	$(BUILD)/test_slicer_ui
	$(BUILD)/test_sl_job
	$(BUILD)/test_view_ui
	$(BUILD)/test_view_gpu
	$(BUILD)/test_machine_ui
	$(BUILD)/test_machine
	$(BUILD)/test_runtime
	$(BUILD)/test_machine_lib
	$(BUILD)/test_path_lib
	$(BUILD)/test_mesh_lib
	$(BUILD)/test_gcode_lib
	$(BUILD)/test_view_lib
	$(BUILD)/test_task_lib
	$(BUILD)/test_vec
	$(BUILD)/test_vec_lib
	$(BUILD)/test_project
	$(BUILD)/test_project_lib
	$(BUILD)/test_rexx
	$(BUILD)/test_rexx_lib
	$(BUILD)/test_dxf
	$(BUILD)/test_toolpath
	$(BUILD)/test_inset
	$(BUILD)/test_fill
	$(BUILD)/test_gcode
	$(BUILD)/test_plan
	$(BUILD)/test_plan_par
	$(BUILD)/test_plan_one
	$(BUILD)/test_toolpath_paint
	$(BUILD)/test_settings
	$(BUILD)/test_settings_ui
	$(BUILD)/test_sl_args
	$(BUILD)/openmake-slice core/tests/data/nasa-wrench.stl $(BUILD)/wrench.gcode \
	    DRIVER=machines/printer/marlin/driver.om PROFILE=machines/printer/marlin/profiles/generic-220.om \
	    PREVIEW=$(BUILD)/wrench.ppm SIZE=320x240
	head -c 15 $(BUILD)/wrench.ppm | grep -q '^320 240' && test "$$(wc -c < $(BUILD)/wrench.ppm)" = 230415
	test "$$(grep -c '^; LAYER ' $(BUILD)/wrench.gcode)" = 130 && grep -q '^M109 S200' $(BUILD)/wrench.gcode && tail -n 1 $(BUILD)/wrench.gcode | grep -q M84
	! $(BUILD)/openmake-slice core/tests/data/nasa-wrench.stl $(BUILD)/tiny.gcode \
	    DRIVER=machines/printer/marlin/driver.om PROFILE=slicer/tests/data/tiny-bed.om 2> $(BUILD)/tiny.txt
	grep -q 'Tiny 50 cannot print this: [XY] runs to' $(BUILD)/tiny.txt && test ! -e $(BUILD)/tiny.gcode
	for t in 1 4; do $(BUILD)/openmake-slice core/tests/data/nasa-wrench.stl $(BUILD)/wrench-$$t.gcode TASKS=$$t \
	    DRIVER=machines/printer/marlin/driver.om PROFILE=machines/printer/marlin/profiles/generic-220.om \
	    > $(BUILD)/wrench-$$t.txt && cmp $(BUILD)/wrench.gcode $(BUILD)/wrench-$$t.gcode && \
	    grep -q "layers made on $$t task" $(BUILD)/wrench-$$t.txt || exit 1; done
	@echo "slice command ok"
	# gfopenslicer: the same engine and the same G-code, for the generic
	# Marlin printer unless DRIVER/PROFILE say otherwise. The shipped
	# profile, and SKIRT=2, are the defaults' G-code; key=value changes it;
	# a key that is not a setting is refused.
	$(BUILD)/gfopenslicer core/tests/data/nasa-wrench.stl $(BUILD)/cli.gcode > $(BUILD)/cli.txt 2>&1
	cmp $(BUILD)/wrench.gcode $(BUILD)/cli.gcode && grep -q 'wrote .*cli.gcode' $(BUILD)/cli.txt
	$(BUILD)/gfopenslicer core/tests/data/nasa-wrench.stl $(BUILD)/cli-std.gcode SETTINGS=settings/standard SKIRT=2 > /dev/null
	cmp $(BUILD)/wrench.gcode $(BUILD)/cli-std.gcode
	$(BUILD)/gfopenslicer core/tests/data/nasa-wrench.stl $(BUILD)/cli-kv.gcode pattern=grid around=brim hotend=210 > /dev/null
	! cmp -s $(BUILD)/wrench.gcode $(BUILD)/cli-kv.gcode && grep -q '^M109 S210' $(BUILD)/cli-kv.gcode
	$(BUILD)/openmake-slice core/tests/data/nasa-wrench.stl $(BUILD)/kv.gcode DRIVER=machines/printer/marlin/driver.om \
	    PROFILE=machines/printer/marlin/profiles/generic-220.om pattern=grid around=brim hotend=210 > /dev/null
	cmp $(BUILD)/cli-kv.gcode $(BUILD)/kv.gcode
	! $(BUILD)/gfopenslicer core/tests/data/nasa-wrench.stl $(BUILD)/cli-bad.gcode spped=60 2> $(BUILD)/cli-bad.txt
	grep -q 'spped=60: no such setting' $(BUILD)/cli-bad.txt && test ! -e $(BUILD)/cli-bad.gcode
	! $(BUILD)/gfopenslicer core/tests/data/nasa-wrench.stl $(BUILD)/cli-tiny.gcode PROFILE=slicer/tests/data/tiny-bed.om 2> $(BUILD)/cli-tiny.txt
	grep -q 'Tiny 50 cannot print this' $(BUILD)/cli-tiny.txt && test ! -e $(BUILD)/cli-tiny.gcode
	! $(BUILD)/gfopenslicer 2> $(BUILD)/cli-usage.txt && grep -q 'usage: gfopenslicer' $(BUILD)/cli-usage.txt
	# Every printer profile: loaded, and the wrench sliced for it.
	$(BUILD)/test_profiles
	$(BUILD)/test_burn
	$(BUILD)/test_burn_ui
	$(BUILD)/test_workshop_ui
	for p in machines/printer/*/profiles/*.om; do d=$${p%/profiles/*}; \
	    $(BUILD)/gfopenslicer core/tests/data/nasa-wrench.stl $(BUILD)/profile.gcode DRIVER=$$d/driver.om PROFILE=$$p \
	    > $(BUILD)/profile.txt 2>&1 && grep -q '^; LAYER ' $(BUILD)/profile.gcode && grep -q '^G28' $(BUILD)/profile.gcode && \
	    grep -q '^M109 S200' $(BUILD)/profile.gcode && tail -n 1 $(BUILD)/profile.gcode | grep -q M84 || { cat $(BUILD)/profile.txt; echo "$$p"; exit 1; }; \
	    rm -f $(BUILD)/profile.gcode; done
	@echo "gfopenslicer command ok"
	# openmake-burn: the 10 mm square to Grbl G-code on the generic diode.
	$(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-rect.gcode \
	    > $(BUILD)/burn-rect.txt
	grep -q 'wrote .*burn-rect.gcode' $(BUILD)/burn-rect.txt && grep -q 'Generic Diode 400' $(BUILD)/burn-rect.txt
	grep -q '^M4' $(BUILD)/burn-rect.gcode && grep -q '^S500' $(BUILD)/burn-rect.gcode && \
	    grep -q '^M5' $(BUILD)/burn-rect.gcode && grep -q '^G21' $(BUILD)/burn-rect.gcode && \
	    grep -q '^G0 ' $(BUILD)/burn-rect.gcode && grep -q '^G1 ' $(BUILD)/burn-rect.gcode
	$(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-m3.gcode constant=1 power=100 > /dev/null
	grep -q '^M3' $(BUILD)/burn-m3.gcode && grep -q '^S1000' $(BUILD)/burn-m3.gcode && ! grep -q '^M4' $(BUILD)/burn-m3.gcode
	$(BUILD)/openmake-burn burn/tests/data/two-rects.svg $(BUILD)/burn-layers.gcode \
	    cuts_last=0 layer=#ff0000,power=100,speed=10 layer=#0000ff,power=25,speed=40 > /dev/null
	grep -q '^S1000' $(BUILD)/burn-layers.gcode && grep -q '^S250' $(BUILD)/burn-layers.gcode && \
	    grep -q ' F600' $(BUILD)/burn-layers.gcode && grep -q ' F2400' $(BUILD)/burn-layers.gcode
	test $$(grep -n '^S1000' $(BUILD)/burn-layers.gcode | head -1 | cut -d: -f1) -lt \
	    $$(grep -n '^S250' $(BUILD)/burn-layers.gcode | head -1 | cut -d: -f1)
	$(BUILD)/openmake-burn burn/tests/data/two-rects.svg $(BUILD)/burn-cuts-last.gcode \
	    layer=#ff0000,power=100,speed=10 layer=#0000ff,power=25,speed=40 > /dev/null
	test $$(grep -n '^S250' $(BUILD)/burn-cuts-last.gcode | head -1 | cut -d: -f1) -lt \
	    $$(grep -n '^S1000' $(BUILD)/burn-cuts-last.gcode | head -1 | cut -d: -f1)
	$(BUILD)/openmake-burn burn/tests/data/two-rects.svg $(BUILD)/burn-order.gcode \
	    cuts_last=0 order=2,1 layer=#ff0000,power=100,speed=10 layer=#0000ff,power=25,speed=40 > /dev/null
	test $$(grep -n '^S250' $(BUILD)/burn-order.gcode | head -1 | cut -d: -f1) -lt \
	    $$(grep -n '^S1000' $(BUILD)/burn-order.gcode | head -1 | cut -d: -f1)
	! $(BUILD)/openmake-burn burn/tests/data/two-rects.svg $(BUILD)/burn-orderbad.gcode \
	    cuts_last=0 order=9 2> $(BUILD)/burn-orderbad.txt
	grep -q 'order: no such layer 9' $(BUILD)/burn-orderbad.txt && test ! -e $(BUILD)/burn-orderbad.gcode
	$(BUILD)/openmake-burn burn/tests/data/square-fill.svg $(BUILD)/burn-fill.gcode mode=fill interval=0.2 > /dev/null
	grep -q '^M4' $(BUILD)/burn-fill.gcode && grep -q '^G1 ' $(BUILD)/burn-fill.gcode && \
	    test $$(grep -c '^G1 ' $(BUILD)/burn-fill.gcode) -ge 40
	$(BUILD)/openmake-burn burn/tests/data/square-fill.svg $(BUILD)/burn-offset.gcode mode=offset interval=2 > /dev/null
	grep -q '^M4' $(BUILD)/burn-offset.gcode && grep -q '^G1 ' $(BUILD)/burn-offset.gcode && \
	    test $$(grep -c '^G1 ' $(BUILD)/burn-offset.gcode) -ge 8
	! $(BUILD)/openmake-burn burn/tests/data/square-fill.svg $(BUILD)/burn-fillbad.gcode mode=fil \
	    2> $(BUILD)/burn-fillbad.txt
	grep -q 'mode=fil' $(BUILD)/burn-fillbad.txt && test ! -e $(BUILD)/burn-fillbad.gcode
	! $(BUILD)/openmake-burn burn/tests/data/two-rects.svg $(BUILD)/burn-lybad.gcode layer=#ff0000,spped=10 \
	    2> $(BUILD)/burn-lybad.txt
	grep -q 'spped=10' $(BUILD)/burn-lybad.txt && test ! -e $(BUILD)/burn-lybad.gcode
	! $(BUILD)/openmake-burn 2> $(BUILD)/burn-usage.txt && grep -q 'usage: openmake-burn' $(BUILD)/burn-usage.txt
	! $(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-bad.gcode spped=20 2> $(BUILD)/burn-bad.txt
	grep -q 'spped=20: no such setting' $(BUILD)/burn-bad.txt && test ! -e $(BUILD)/burn-bad.gcode
	! $(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-pr.gcode \
	    DRIVER=machines/printer/marlin/driver.om PROFILE=machines/printer/marlin/profiles/generic-220.om \
	    2> $(BUILD)/burn-pr.txt
	grep -q 'not a laser' $(BUILD)/burn-pr.txt && test ! -e $(BUILD)/burn-pr.gcode
	$(BUILD)/openmake-burn burn/tests/data/square-10mm.dxf $(BUILD)/burn-square-dxf.gcode > /dev/null
	grep -q '^M4' $(BUILD)/burn-square-dxf.gcode && grep -q '^G1 ' $(BUILD)/burn-square-dxf.gcode
	$(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-card.gcode \
	    material=materials/laser/card.om > /dev/null
	grep -q '^S400' $(BUILD)/burn-card.gcode && grep -q ' F2400' $(BUILD)/burn-card.gcode
	$(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-ply.gcode \
	    material=materials/laser/plywood-3mm.om > /dev/null
	grep -q '^S800' $(BUILD)/burn-ply.gcode && grep -q ' F480' $(BUILD)/burn-ply.gcode && \
	    test $$(grep -c '^G1 ' $(BUILD)/burn-ply.gcode) -ge 12
	$(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-mat-over.gcode \
	    material=materials/laser/card.om power=100 > /dev/null
	grep -q '^S1000' $(BUILD)/burn-mat-over.gcode
	! $(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-matbad.gcode \
	    material=materials/laser/no-such.om 2> $(BUILD)/burn-matbad.txt
	grep -q 'cannot open' $(BUILD)/burn-matbad.txt && test ! -e $(BUILD)/burn-matbad.gcode
	$(BUILD)/openmake-burn burn/tests/data/bars-8x2.pgm $(BUILD)/burn-img.gcode \
	    interval=1 power=100 > /dev/null
	grep -q '^M4' $(BUILD)/burn-img.gcode && grep -q '^S1000' $(BUILD)/burn-img.gcode && \
	    grep -q '^G0 ' $(BUILD)/burn-img.gcode && grep -q '^G1 ' $(BUILD)/burn-img.gcode
	$(BUILD)/openmake-burn burn/tests/data/square-10mm-at-5.svg $(BUILD)/burn-kerf.gcode kerf=0.2 > /dev/null
	grep -q 'X4.900' $(BUILD)/burn-kerf.gcode && grep -q 'X15.100' $(BUILD)/burn-kerf.gcode
	! $(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-kerf-off.gcode kerf=0.2 \
	    2> $(BUILD)/burn-kerf-off.txt
	grep -q 'cannot burn this' $(BUILD)/burn-kerf-off.txt && test ! -e $(BUILD)/burn-kerf-off.gcode
	$(BUILD)/openmake-burn burn/tests/data/square-10mm-at-5.svg $(BUILD)/burn-tabs.gcode tabs=4 tabw=1 kerf=0.2 > /dev/null
	grep -q 'X4.900' $(BUILD)/burn-tabs.gcode && grep -q 'X15.100' $(BUILD)/burn-tabs.gcode && \
	    test $$(grep -c '^G0 ' $(BUILD)/burn-tabs.gcode) -ge 4
	! $(BUILD)/openmake-burn core/tests/data/rect-10mm.svg $(BUILD)/burn-tabs-fit.gcode tabs=4 tabw=20 \
	    2> $(BUILD)/burn-tabs-fit.txt
	grep -q 'tabs do not fit' $(BUILD)/burn-tabs-fit.txt && test ! -e $(BUILD)/burn-tabs-fit.gcode
	$(BUILD)/openmake-burn burn/tests/data/two-rects.svg $(BUILD)/burn-frame.gcode frame=1 > /dev/null
	grep -q 'G0 ' $(BUILD)/burn-frame.gcode && ! grep -q 'G1 ' $(BUILD)/burn-frame.gcode && \
	    ! grep -q '^M4' $(BUILD)/burn-frame.gcode && ! grep -q '^M3' $(BUILD)/burn-frame.gcode && \
	    grep -q 'X0' $(BUILD)/burn-frame.gcode && grep -q 'X30' $(BUILD)/burn-frame.gcode && \
	    test $$(grep -c '^G0 ' $(BUILD)/burn-frame.gcode) -ge 5
	$(BUILD)/openmake-burn burn/tests/data/square-10mm-at-5.svg $(BUILD)/burn-frame-kerf.gcode \
	    frame=1 kerf=0.2 constant=1 > /dev/null
	grep -q 'X4.900' $(BUILD)/burn-frame-kerf.gcode && ! grep -q 'G1 ' $(BUILD)/burn-frame-kerf.gcode && \
	    ! grep -q '^M3' $(BUILD)/burn-frame-kerf.gcode && ! grep -q '^M4' $(BUILD)/burn-frame-kerf.gcode
	@echo "openmake-burn command ok"
	# openmake-workshop: new, list, add (including an unknown kind).
	! $(BUILD)/openmake-workshop 2> $(BUILD)/ws-usage.txt && grep -q 'usage: openmake-workshop' $(BUILD)/ws-usage.txt
	! $(BUILD)/openmake-workshop foo 2> $(BUILD)/ws-badcmd.txt && grep -q 'not new, list or add' $(BUILD)/ws-badcmd.txt
	rm -rf $(BUILD)/ws-proj
	$(BUILD)/openmake-workshop new $(BUILD)/ws-proj name=Widget
	test -f $(BUILD)/ws-proj/project.om && test -d $(BUILD)/ws-proj/cad
	$(BUILD)/openmake-workshop list $(BUILD)/ws-proj | grep -q '^Widget$$'
	! $(BUILD)/openmake-workshop new $(BUILD)/ws-proj name=Other 2> $(BUILD)/ws-exists.txt
	grep -q 'already a project' $(BUILD)/ws-exists.txt
	printf 'solid x\nendsolid x\n' > $(BUILD)/ws-body.stl
	$(BUILD)/openmake-workshop add $(BUILD)/ws-proj kind=cad file=$(BUILD)/ws-body.stl name=Body
	test -f $(BUILD)/ws-proj/cad/ws-body.stl
	$(BUILD)/openmake-workshop list $(BUILD)/ws-proj | grep -q '1 cad Body cad/ws-body.stl'
	printf '; gcode\nG28\n' > $(BUILD)/ws-part.gcode
	$(BUILD)/openmake-workshop add $(BUILD)/ws-proj kind=print file=$(BUILD)/ws-part.gcode name='Body for the printer' from=1 settings=settings/standard
	test -f $(BUILD)/ws-proj/print/ws-part.gcode
	$(BUILD)/openmake-workshop list $(BUILD)/ws-proj | grep -q '2 print Body for the printer print/ws-part.gcode from=1 settings=settings/standard'
	printf 'exr\n' > $(BUILD)/ws-shot.exr
	$(BUILD)/openmake-workshop add $(BUILD)/ws-proj kind=render file=$(BUILD)/ws-shot.exr name=Shot from=1
	$(BUILD)/openmake-workshop list $(BUILD)/ws-proj | grep -q '3 render Shot'
	grep -q 'item.3.kind = render' $(BUILD)/ws-proj/project.om
	! $(BUILD)/openmake-workshop add $(BUILD)/ws-proj kind=cad file=$(BUILD)/ws-body.stl foo=1 2> $(BUILD)/ws-badkey.txt
	grep -q 'foo=1: no such setting' $(BUILD)/ws-badkey.txt
	! $(BUILD)/openmake-workshop add $(BUILD)/ws-proj kind=cad file=$(BUILD)/ws-no-such.stl 2> $(BUILD)/ws-nofile.txt
	grep -q 'cannot open' $(BUILD)/ws-nofile.txt
	@echo "openmake-workshop command ok"

preview: $(BUILD)/preview $(BUILD)/make_preview $(BUILD)/burn_preview $(BUILD)/workshop_preview

# GFOpenMake's window after Slice, on the host (cad/tools/make_preview.c).
$(BUILD)/make_preview: cad/tools/make_preview.c cad/src/make_ui.h cad/src/edit.c core/src/parts.c core/ui/view_ui.c core/ui/machines_ui.c core/ui/settings_ui.c core/src/settings.c $(PLAN) core/src/gcode.c | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Icore/include -Icore/ui -Icad/include -Icad/src -Imachines/include -I$(LAYOUT)/include -o $@ \
	    cad/tools/make_preview.c cad/src/edit.c core/src/parts.c core/ui/view_ui.c core/ui/machines_ui.c core/ui/settings_ui.c core/src/settings.c $(PLAN) core/src/toolpath_paint.c core/src/gcode.c \
	    machines/src/machine.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c \
	    $(LAYOUT)/ui/olui.c $(LAYOUT)/ui/olui_css.c $(LAYOUT)/ui/olui_markup.c $(LAYOUT)/ui/olui_theme_none.c \
	    $(LAYOUT)/build/libopenlayout.a -lm

clean:
	rm -rf $(BUILD)
