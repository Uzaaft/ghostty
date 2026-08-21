#include <ghostty/vt.h>

// We only compile and link this test. A real target provides its startup
// code and allocator through the platform SDK.

typedef struct {
  size_t used;
  _Alignas(16) uint8_t bytes[1024 * 1024];
} Heap;

static void* heap_alloc(void* ctx,
                        size_t len,
                        uint8_t alignment,
                        uintptr_t ret_addr) {
  Heap* heap = ctx;
  (void)ret_addr;

  if (alignment >= sizeof(uintptr_t) * 8)
    return NULL;
  const uintptr_t byte_alignment = (uintptr_t)1 << alignment;
  const uintptr_t base = (uintptr_t)heap->bytes;
  const uintptr_t current = base + heap->used;
  const uintptr_t aligned =
      (current + byte_alignment - 1) & ~(byte_alignment - 1);
  if (aligned < current)
    return NULL;

  const size_t offset = (size_t)(aligned - base);
  if (offset > sizeof(heap->bytes) || len > sizeof(heap->bytes) - offset)
    return NULL;

  heap->used = offset + len;
  return (void*)aligned;
}

static bool heap_resize(void* ctx,
                        void* memory,
                        size_t memory_len,
                        uint8_t alignment,
                        size_t new_len,
                        uintptr_t ret_addr) {
  (void)ctx;
  (void)memory;
  (void)memory_len;
  (void)alignment;
  (void)new_len;
  (void)ret_addr;
  return false;
}

static void* heap_remap(void* ctx,
                        void* memory,
                        size_t memory_len,
                        uint8_t alignment,
                        size_t new_len,
                        uintptr_t ret_addr) {
  (void)ctx;
  (void)memory;
  (void)memory_len;
  (void)alignment;
  (void)new_len;
  (void)ret_addr;
  return NULL;
}

static void heap_free(void* ctx,
                      void* memory,
                      size_t memory_len,
                      uint8_t alignment,
                      uintptr_t ret_addr) {
  (void)ctx;
  (void)memory;
  (void)memory_len;
  (void)alignment;
  (void)ret_addr;
}

static Heap heap;
static const GhosttyAllocatorVtable allocator_vtable = {
    .alloc = heap_alloc,
    .resize = heap_resize,
    .remap = heap_remap,
    .free = heap_free,
};
static const GhosttyAllocator allocator = {
    .ctx = &heap,
    .vtable = &allocator_vtable,
};

_Noreturn void _start(void) {
  GhosttyTerminal terminal = NULL;
  if (ghostty_terminal_new(&allocator, &terminal, 80, 24) == GHOSTTY_SUCCESS) {
    static const uint8_t input[] = "hello from bare metal\r\n";
    ghostty_terminal_vt_write(terminal, input, sizeof(input) - 1);
    ghostty_terminal_free(terminal);
  }

  for (;;) {
  }
}
