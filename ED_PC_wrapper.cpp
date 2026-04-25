#include <cstddef>
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "ED_PC_wrapper.h"

// ed_heaptrace_* functions are defined in components/toolbox/diag/heap_tracer.cpp
// which owns the control variables. ED_PC_wrapper.h declares them for inclusion
// by call sites (e.g. main.cpp). No definitions here.