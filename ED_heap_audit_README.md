# ESP32 Heap Fragmentation Audit Module – User Guide

This guide explains how to integrate the **heap audit module** (provided as a submodule) into your ESP‑IDF project, enable it via `sdkconfig.defaults`, and use its zero‑allocation snapshot functions to diagnose heap fragmentation.

> The module source files (`heap_audit.h`, `heap_audit.c`, and `Kconfig.projbuild`) are part of the submodule and **not** repeated here.

---

## 1. Submodule Setup

Assume your audit module lives in `components/heap_audit/` as a Git submodule.

**Add the submodule:**

```bash
git submodule add https://github.com/your-org/esp32-heap-audit.git components/heap_audit
git submodule update --init --recursive
```

**Ensure the component is registered:**
ESP‑IDF automatically scans `components/`. The submodule must contain a minimal `CMakeLists.txt`:

```cmake
idf_component_register(SRCS "heap_audit.c"
                       INCLUDE_DIRS ".")
```

If missing, create it inside `components/heap_audit/`.

---

## 2. Enabling the Audit via `sdkconfig.defaults` (No `menuconfig`)

The audit code is guarded by `CONFIG_HEAP_AUDIT_ENABLE`. To enable it **without opening `menuconfig`**, add the following lines to your **main project’s** `sdkconfig.defaults` file (create it if it doesn't exist):

```
# Enable the heap audit module (your custom component)
CONFIG_HEAP_AUDIT_ENABLE=y

# Lightweight heap poisoning – catches memory corruption, low overhead
CONFIG_HEAP_POISONING_LIGHT=y

# Tracks which task allocated each heap block – adds 4 bytes per allocation
CONFIG_HEAP_TASK_TRACKING=y
```

**How it works:**
- The submodule contains a `Kconfig.projbuild` file that defines `CONFIG_HEAP_AUDIT_ENABLE`.
- When you build your project, ESP‑IDF merges all `Kconfig.projbuild` files from all components and your `sdkconfig.defaults`.
- The macro is then available in C code as `#ifdef CONFIG_HEAP_AUDIT_ENABLE`.

> **Important:** After changing `sdkconfig.defaults`, run `idf.py reconfigure` or simply `idf.py build` to regenerate `sdkconfig`.

### Why no heap tracing?
`CONFIG_HEAP_TRACING_STANDALONE` and related options are **not required** for the audit module. They record every malloc/free with stack traces, adding significant overhead and memory usage. Enable them only if the audit shows severe fragmentation and you need to pinpoint the exact lines of code causing it.

---

## 3. Using the Audit in Your Main Program

Include the header and wrap your audit calls with the config macro.

### Step 1: Include the header

```c
#include "heap_audit.h"
```

### Step 2: Take a snapshot (zero allocation)

```c
#ifdef CONFIG_HEAP_AUDIT_ENABLE

void my_debug_function(void) {
    heap_audit_snapshot_t snap;
    heap_audit_take_snapshot(&snap);   // No heap allocation

    // Now use the data: snap.total_free_bytes, snap.fragmentation_percent, etc.
    // Example: log to console
    ESP_LOGI("HEAP", "Free: %zu, Largest block: %zu, Frag: %.1f%%",
             snap.total_free_bytes,
             snap.largest_free_block,
             snap.fragmentation_percent);
}

#endif
```

### Step 3: Publish to MQTT (optional, with JSON)

If you want to send the snapshot via MQTT, convert it to JSON *outside* the audit module. Example using cJSON:

```c
#ifdef CONFIG_HEAP_AUDIT_ENABLE

void publish_heap_metrics(void) {
    heap_audit_snapshot_t snap;
    heap_audit_take_snapshot(&snap);

    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddNumberToObject(root, "total_free_bytes", snap.total_free_bytes);
    cJSON_AddNumberToObject(root, "largest_free_block", snap.largest_free_block);
    cJSON_AddNumberToObject(root, "fragmentation_percent", snap.fragmentation_percent);
    cJSON_AddNumberToObject(root, "min_free_8bit", snap.min_free_8bit);
    cJSON_AddNumberToObject(root, "min_free_32bit", snap.min_free_32bit);
    cJSON_AddNumberToObject(root, "total_allocated_bytes", snap.total_allocated_bytes);
    cJSON_AddNumberToObject(root, "allocated_blocks", snap.allocated_blocks);
    cJSON_AddNumberToObject(root, "free_blocks", snap.free_blocks);
    cJSON_AddBoolToObject(root, "heap_integrity_ok", snap.heap_integrity_ok);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        // Replace with your MQTT publish function
        mqtt_publish("tele/esp32/heap", json_str);
        free(json_str);
    }
    cJSON_Delete(root);
}

#endif
```

### Step 4: Trigger periodically (example timer)

```c
void app_main(void) {
    // ... your WiFi, MQTT init ...

#ifdef CONFIG_HEAP_AUDIT_ENABLE
    const esp_timer_create_args_t timer_args = {
        .callback = [](void* arg) { publish_heap_metrics(); },
        .name = "heap_timer"
    };
    esp_timer_handle_t timer;
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_periodic(timer, 60 * 1000 * 1000); // every 60 seconds
#endif

    // ... rest of your app ...
}
```

---

## 4. Interpreting the Snapshot Data

| Field | Meaning | Danger signal |
|-------|---------|----------------|
| `fragmentation_percent` | % of free memory that is not contiguous | > 70% critical |
| `largest_free_block` | Biggest single block you can allocate | Less than your typical allocation size |
| `heap_integrity_ok` | `false` means corruption detected | Must be `true` for reliable operation |
| `min_free_8bit` | Historical low water mark | Dropping over time = leak or fragmentation |

---

## 5. Disabling the Audit for Production

In your main project’s `sdkconfig.defaults`, change or add:

```
# CONFIG_HEAP_AUDIT_ENABLE is not set
```

Then run `idf.py reconfigure && idf.py build`. The audit code will be completely compiled out.

---

## 6. Troubleshooting

**`CONFIG_HEAP_AUDIT_ENABLE` is not defined**
- Ensure `components/heap_audit/Kconfig.projbuild` exists.
- Run `idf.py fullclean && idf.py reconfigure`.

**Component not found**
- Verify `components/heap_audit/CMakeLists.txt` exists with the `idf_component_register` line.

**Compilation errors about `multi_heap_info_t`**
- Your ESP‑IDF version must be ≥5.0. This module targets ESP‑IDF 5.5.

```