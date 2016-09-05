/**
 * File: FrameAllocator.h
 * Author: David Brotz
 */

#ifndef __FRAMEALLOCATOR_H
#define __FRAMEALLOCATOR_H

#include <stddef.h>
#include <inttypes.h>

#define FrameAlloc(_Block) FrameAllocGuard(_Block, 0)

/**
 * Allocates a block of memory of size (_Block + (2 * _MemGuard)),
 * giving the caller a block of size _Block.
 */
void* FrameAllocGuard(size_t _Block, size_t _MemGuard);
/**
 * Frees all memory that is allocated on the FrameAllocator.
 */
void FrameFree();
void FrameReduce(uint32_t _Size);
#endif
