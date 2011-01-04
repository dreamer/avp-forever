
#ifndef _mempool_h_
#define _mempool_h_ 1

#define NEW_DEALLOCATION_ORDER 1

#define USE_LEVEL_MEMORY_POOL 1

#if USE_LEVEL_MEMORY_POOL
void* PoolAllocateMem(/*unsigned int*/size_t amount);
void ClearMemoryPool();
#else
#define PoolAllocateMem AllocateMem
#define ClearMemoryPool() ((void)0)
#endif

#endif
