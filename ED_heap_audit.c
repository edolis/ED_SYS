#include "ED_heap_audit.h"
#include "esp_heap_caps.h"

void heap_audit_take_snapshot(heap_audit_snapshot_t *snap) {
    if (!snap) return;

    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);   // Main DRAM only

    snap->total_free_bytes   = info.total_free_bytes;
    snap->largest_free_block = info.largest_free_block;
    snap->total_allocated_bytes = info.total_allocated_bytes;
    snap->allocated_blocks      = info.allocated_blocks;
    snap->free_blocks           = info.free_blocks;

    // Compute fragmentation ratio (stack only)
    float frag = 0.0f;
    if (info.total_free_bytes > 0) {
        frag = (1.0f - ((float)info.largest_free_block / (float)info.total_free_bytes)) * 100.0f;
    }
    snap->fragmentation_percent = frag;

    // Historical low water marks
    snap->min_free_8bit  = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    snap->min_free_32bit = heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT);

    // Integrity check – does NOT allocate, only inspects
    snap->heap_integrity_ok = heap_caps_check_integrity_all(false); // false = no print
}