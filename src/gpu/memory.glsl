#ifndef MEMORY_GLSL
#define MEMORY_GLSL

#include "../../src/shared/push.inl"

// TODO: This is technically UB, replace with less UB version.
u32 atomicMalloc(u64 allocator) {
  // Check allocator lock status until it is free,
  // in which case this thread while lock it and
  // allocate a page.
  Allocator pAllocator = Allocator(allocator);
  while( bool(atomicCompSwap(pAllocator.locked, UNLOCKED, LOCKED)) );

  // Look for free page
  u32 i = 0;
  while(pAllocator.free[i] != 0) { i++; }

  // Mark page as used
  pAllocator.free[i] = 1;
  // Unlock allocator
  atomicExchange(pAllocator.locked, UNLOCKED);

  // Base of allocation
  return i*ALLOCATOR_PAGE_SIZE;
}

u32 debugMalloc(u64 pAllocator, u32 size) {
  return atomicAdd(Allocator(pAllocator).locked, size);
}

void atomicFree(u64 pAllocator, u32 global_index) {
  u32 page = global_index/ALLOCATOR_PAGE_SIZE;
  atomicExchange(Allocator(pAllocator).free[page], 0);
}

#endif
