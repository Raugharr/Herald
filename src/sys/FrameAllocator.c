/**
 * File: FrameAllocator.c
 * Author: David Brotz
 */

#include "FrameAllocator.h"

#include "Log.h"
#include "Memory.h"

struct FrameAllocator {
	void* Arena;
	void* ArenaTop; //FIXME: Make atomic.
	size_t Size;
};

static struct FrameAllocator g_FrameAlloc = {NULL, NULL, 0};

__attribute__((constructor))
void FrameAllocatorCtor() {
	g_FrameAlloc.Size = 1024 * 4;
	g_FrameAlloc.Arena = PageAlloc(g_FrameAlloc.Size);
	g_FrameAlloc.ArenaTop = g_FrameAlloc.Arena;
}

__attribute__((destructor))
void FrameAllocatorDtor() {
	PageFree(g_FrameAlloc.Arena);
}

void* FrameAllocGuard(size_t _Block, size_t _MemGuard) {
	size_t _Size = _Block + (2 * _MemGuard);
	Assert((g_FrameAlloc.ArenaTop + _Size) <= (g_FrameAlloc.Arena + g_FrameAlloc.Size));
	g_FrameAlloc.ArenaTop += _Size;
	//assert(g_FrameAlloc.ArenaTop - g_FrameAlloc.Arena < g_FrameAlloc.Size);
	return g_FrameAlloc.ArenaTop - _Size;
}

void FrameFree() {
	g_FrameAlloc.ArenaTop = g_FrameAlloc.Arena;
}

void FrameReduce(uint32_t _Size) {
	if(_Size >= g_FrameAlloc.Size) {
		FrameFree();
		return;
	}
	g_FrameAlloc.ArenaTop -= _Size;
	Assert(g_FrameAlloc.ArenaTop);
}

uint32_t FrameSizeRemain() {
	return (g_FrameAlloc.Arena + g_FrameAlloc.Size) - g_FrameAlloc.ArenaTop;
}

void FrameSet(void* Ptr) {
	if(Ptr >= g_FrameAlloc.Arena && Ptr < (g_FrameAlloc.Arena + g_FrameAlloc.Size)) g_FrameAlloc.ArenaTop = Ptr;
}
