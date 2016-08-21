/**
 * File: FrameAllocator.c
 * Author: David Brotz
 */

#include "FrameAllocator.h"

#include "Memory.h"

struct FrameAllocator {
	void* Arena;
	void* ArenaTop; //Make atomic.
	size_t Size;
};

static struct FrameAllocator g_FrameAlloc = {NULL, NULL, 0};

__attribute__((constructor))
void FrameAllocatorCtor() {
	g_FrameAlloc.Size = 1024;
	g_FrameAlloc.Arena = PageAlloc(g_FrameAlloc.Size);
	g_FrameAlloc.ArenaTop = g_FrameAlloc.Arena;
}

__attribute__((destructor))
void FrameAllocatorDtor() {
	PageFree(g_FrameAlloc.Arena);
}

void* FrameAllocGuard(size_t _Block, size_t _MemGuard) {
	size_t _Size = _Block + (2 * _MemGuard);
	g_FrameAlloc.ArenaTop += _Size;
	//assert(g_FrameAlloc.ArenaTop - g_FrameAlloc.Arena < g_FrameAlloc.Size);
	return g_FrameAlloc.ArenaTop - _MemGuard - _Block;
}

void FrameFree() {
	g_FrameAlloc.ArenaTop = g_FrameAlloc.Arena;
}
