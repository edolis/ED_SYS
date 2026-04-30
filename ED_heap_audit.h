#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief read https:   chat.deepseek.com/a/chat/s/ff222c45-fe45-4b2c-afd1-6f3ba0dd474f
 *
 *
 */

/**
 * @brief Snapshot of heap metrics – all plain old data, no pointers to heap.
 */
typedef struct {
    size_t total_free_bytes;          // Total free memory in main DRAM
    size_t largest_free_block;        // Largest contiguous free block
    float fragmentation_percent;      // 0..100, computed from above
    size_t min_free_8bit;             // Historical low water mark (MALLOC_CAP_8BIT)
    size_t min_free_32bit;            // Historical low water mark (MALLOC_CAP_32BIT)
    size_t total_allocated_bytes;     // Sum of all allocated memory
    size_t allocated_blocks;          // Number of allocated blocks
    size_t free_blocks;               // Number of free blocks
    bool heap_integrity_ok;           // true = no corruption detected
} heap_audit_snapshot_t;

/**
 * @brief Fill the snapshot with current heap data.
 * @param snap Pointer to struct to fill.
 * @note  This function performs NO heap allocation – only stack and direct
 *        calls to ESP-IDF heap inspection functions.
 */
void heap_audit_take_snapshot(heap_audit_snapshot_t *snap);

#ifdef __cplusplus
}
#endif