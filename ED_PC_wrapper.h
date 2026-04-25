/**
 * @file ED_PC_wrapper.h
 * @author Emanuele Dolis (edoliscom@gmail.com)
 * @brief C API for heap allocation tracing at debug time.
 *
 * Implementations live in components/toolbox/diag/heap_tracer.cpp.
 *
 * in CMakeLists.txt (diag component):
 *   target_link_options(${COMPONENT_LIB} INTERFACE
 *       -Wl,--wrap=heap_caps_malloc
 *       -Wl,--wrap=heap_caps_calloc
 *       -Wl,--wrap=heap_caps_realloc
 *       -Wl,--wrap=heap_caps_free
 *   )
 *
 * in app_main:
 *   ed_heaptrace_start();
 *   ed_heaptrace_set_min_size(64);
 *   ed_heaptrace_set_caps_mask(MALLOC_CAP_DEFAULT);
 *   ed_heaptrace_enable(true);
 *
 * To suppress tracing around timing-sensitive sections:
 *   ed_heaptrace_pause(true);
 *   // ... critical code ...
 *   ed_heaptrace_pause(false);
 */

#pragma once
#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

void ed_heaptrace_start();
void ed_heaptrace_enable(bool on);
void ed_heaptrace_pause(bool on);
void ed_heaptrace_set_min_size(size_t bytes);
void ed_heaptrace_set_caps_mask(uint32_t mask_any);
void ed_heaptrace_set_print_bt(bool on);

#ifdef __cplusplus
}
#endif