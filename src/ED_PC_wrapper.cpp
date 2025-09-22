#include <cstddef>
#include <cstdint>
#include <atomic>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_debug_helpers.h"   // esp_backtrace_print
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "ED_PC_wrapper.h"

// ROM-based, non-allocating print
extern "C" void ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

extern "C" {
// Linker-provided originals (via -Wl,--wrap)
void* __real_heap_caps_malloc(size_t size, uint32_t caps);
void* __real_heap_caps_calloc(size_t n, size_t size, uint32_t caps);
void* __real_heap_caps_realloc(void* ptr, size_t size, uint32_t caps);
void  __real_heap_caps_free(void* ptr);

// Keep these only if you also wrap libc in link flags; otherwise remove.
void* __real_malloc(size_t size);
void* __real_calloc(size_t n, size_t size);
void* __real_realloc(void* ptr, size_t size);
void  __real_free(void* ptr);
}

// ---------------- Controls ----------------
static volatile bool     g_trace_enabled   = false;
static volatile bool     g_trace_paused    = false;
static volatile bool     g_in_trace        = false;
static volatile bool     g_print_bt        = false;  // off by default
static volatile size_t   g_min_size        = 0;      // bytes threshold
static volatile uint32_t g_caps_mask_any   = 0;      // if nonzero, require (caps & mask) != 0

extern "C" void ed_heaptrace_enable(bool on)                 { g_trace_enabled = on; }
extern "C" void ed_heaptrace_pause(bool on)                  { g_trace_paused  = on; }
extern "C" void ed_heaptrace_set_min_size(size_t bytes)      { g_min_size      = bytes; }
extern "C" void ed_heaptrace_set_caps_mask(uint32_t maskAny) { g_caps_mask_any = maskAny; }
extern "C" void ed_heaptrace_set_print_bt(bool on)           { g_print_bt      = on; }

// ---------------- Ring buffer ----------------
struct TraceEntry {
    const char* api;
    void*       ptr;
    size_t      size;
    uint32_t    caps;
    const char* task; // snapshot; treat as read-only
};

static constexpr int TRACE_BUF_SIZE = 128;
static TraceEntry        g_trace_buf[TRACE_BUF_SIZE];
static std::atomic<int>  g_head{0};
static std::atomic<int>  g_tail{0};

// ---------------- Task filter ----------------
static inline bool is_critical_task(const char* name) {
    if (!name) return false;
    // Adjust for your project. We skip common timing-sensitive tasks.
    static const char* skip[] = {
        "wifi", "wifiTask", "ipc0", "ipc1", "Tmr Svc", "IDLE",
        "sys_evt", "tiT",    // lwIP tcpip thread default name often contains "tiT"
        "mqtt", "mqtt_task", "mbedtls", "http", "tls", "event",
    };
    for (const char* s : skip) {
        if (std::strstr(name, s)) return true;
    }
    return false;
}

// ---------------- Gate logic ----------------
static inline bool trace_allowed(uint32_t caps, size_t size) {
    if (!g_trace_enabled || g_trace_paused) return false;
    if (g_in_trace) return false;
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return false;
    if (xPortInIsrContext() != 0) return false;

    if (g_min_size && size < g_min_size) return false;
    if (g_caps_mask_any && (caps & g_caps_mask_any) == 0) return false;

    // Avoid Wi-Fi/driver internal heap (timing critical)
    if (caps & MALLOC_CAP_INTERNAL) return false;

    // Filter by current task name using pcTaskGetName (always available in ESP-IDF)
    TaskHandle_t th = xTaskGetCurrentTaskHandle();
    const char* tname = th ? pcTaskGetName(th) : nullptr;
    if (is_critical_task(tname)) return false;
    
    return true;
}

// ---------------- Buffer write ----------------
static inline void buffer_trace(const char* api, void* ptr, size_t size, uint32_t caps) {
    int h = g_head.load(std::memory_order_relaxed);
    int n = (h + 1) % TRACE_BUF_SIZE;

    // Drop oldest on full buffer
    if (n == g_tail.load(std::memory_order_acquire)) {
        g_tail.store((g_tail.load() + 1) % TRACE_BUF_SIZE, std::memory_order_release);
    }

    const char* tname = nullptr;
    if (TaskHandle_t th = xTaskGetCurrentTaskHandle()) {
        tname = pcTaskGetName(th);
    }

    g_trace_buf[h] = { api, ptr, size, caps, tname };
    g_head.store(n, std::memory_order_release);
}

// ---------------- Dumper task ----------------
static void trace_dumper_task(void*) {
    const uint32_t per_burst = 8;                   // max entries per cycle
    const TickType_t period  = pdMS_TO_TICKS(200);  // flush every 200 ms
    while (true) {
        uint32_t emitted = 0;
        while (g_tail.load(std::memory_order_acquire) != g_head.load(std::memory_order_acquire)) {
            int t = g_tail.load(std::memory_order_relaxed);
            TraceEntry e = g_trace_buf[t];
            g_tail.store((t + 1) % TRACE_BUF_SIZE, std::memory_order_release);

            ets_printf("[HEAPTRACE] %s %u @ %p caps=0x%08x task=%s\n",
                       e.api, (unsigned)e.size, e.ptr, (unsigned)e.caps,
                       e.task ? e.task : "?");

            if (g_print_bt) {
                // Keep shallow for minimal impact
                esp_backtrace_print(4);
            }

            if (++emitted >= per_burst) break;
        }
        vTaskDelay(period);
    }
}

static void start_dumper_task() {
    static bool started = false;
    if (!started) {
        // Lowest priority; pin to core 0 for ESP32-C3 (single-core)
        xTaskCreatePinnedToCore(trace_dumper_task, "heaptrace_dump",
                                4096, nullptr, tskIDLE_PRIORITY, nullptr, 0);
        started = true;
    }
}

extern "C" void ed_heaptrace_start() {
    start_dumper_task();
}

// ---------------- Trace entry point ----------------
static inline void trace_alloc(const char* api, void* ptr, size_t size, uint32_t caps) {
    if (!ptr) return;
    if (!trace_allowed(caps, size)) return;
    g_in_trace = true;
    buffer_trace(api, ptr, size, caps);
    g_in_trace = false;
}

// ---------------- heap_caps_* wrappers ----------------
extern "C" void* __wrap_heap_caps_malloc(size_t size, uint32_t caps) {
    void* p = __real_heap_caps_malloc(size, caps);
    trace_alloc("malloc", p, size, caps);
    return p;
}

extern "C" void* __wrap_heap_caps_calloc(size_t n, size_t size, uint32_t caps) {
    void* p = __real_heap_caps_calloc(n, size, caps);
    size_t total = (size_t)((uint64_t)n * (uint64_t)size);
    trace_alloc("calloc", p, total, caps);
    return p;
}

extern "C" void* __wrap_heap_caps_realloc(void* ptr, size_t size, uint32_t caps) {
    void* p = __real_heap_caps_realloc(ptr, size, caps);
    trace_alloc("realloc", p, size, caps);
    return p;
}

extern "C" void __wrap_heap_caps_free(void* ptr) {
    __real_heap_caps_free(ptr);
}

// ---------------- libc pass-throughs (optional) ----------------
// Keep these only if you also add -Wl,--wrap for libc allocs.
// Otherwise, remove to avoid pulling extra symbols.
extern "C" void* __wrap_malloc(size_t size)        { return __real_malloc(size); }
extern "C" void* __wrap_calloc(size_t n, size_t s) { return __real_calloc(n, s); }
extern "C" void* __wrap_realloc(void* p, size_t s) { return __real_realloc(p, s); }
extern "C" void  __wrap_free(void* p)              { __real_free(p); }
