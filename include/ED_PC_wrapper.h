/**
 * @file ED_PC_wrapper.h
 * @author Emanuele Dolis (edoliscom@gmail.com)
 * @brief wrapper for dynamic allocationm tracking at dedug time
 * Used to diagnose memeory allocation fragmenting heap on repetitive tasks
 * How to use:
 *
 * in Platformio.ini
 build_flags=
    -D DEBUG_BUILD
    -Wl,--wrap=heap_caps_malloc #dynamic allocation tracing
    -Wl,--wrap=heap_caps_calloc #dynamic allocation tracing
    -Wl,--wrap=heap_caps_realloc #dynamic allocation tracing
    -Wl,--wrap=heap_caps_free #dynamic allocation tracing
    -I${sysenv.ESP_HEADERS} ; required to pull in the compilation the secrets.h saved in the centralized credential storage

 * in main:

    ed_heaptrace_start();
    ed_heaptrace_set_min_size(64);                  // optional filter
    ed_heaptrace_set_caps_mask(MALLOC_CAP_DEFAULT); // optional filter
    ed_heaptrace_enable(true);

 * in modules which have system level timing sensitive allocation, the wrapping might
 * disrupt the operations. in such case, try adding the task name in is_critical_task
 * if that does not solve, disable wrapping before the code affected and re-enable afterwards
 * using ed_heaptrace_pause([true|false])
 *
 *
 * @version 0.1
 * @date 2025-09-22
 *
 *
 */

#pragma once
#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Control API
void ed_heaptrace_start();
void ed_heaptrace_enable(bool on);
void ed_heaptrace_pause(bool on);
void ed_heaptrace_set_min_size(size_t bytes);
void ed_heaptrace_set_caps_mask(uint32_t mask_any);
void ed_heaptrace_set_print_bt(bool on); // prints short backtrace in dumper task

#ifdef __cplusplus
}
#endif
