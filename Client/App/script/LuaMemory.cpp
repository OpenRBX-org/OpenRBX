#include "script/LuaMemory.h"

#define NOMINMAX

#include <algorithm>
#include <mutex>
#include <new>
#include <stdlib.h>
#include <string.h>

template <size_t chunkSize>
class ChunkPool
{
public:
	static void* malloc()
	{
		ChunkPool& pool = instance();
		std::lock_guard<std::mutex> lock(pool.mutex);

		if (pool.freeList == NULL && !pool.grow()) {
			return NULL;
		}

		void* chunk = pool.freeList;
		pool.freeList = *static_cast<void**>(chunk);
		return chunk;
	}

	static void free(void* chunk)
	{
		ChunkPool& pool = instance();
		std::lock_guard<std::mutex> lock(pool.mutex);

		*static_cast<void**>(chunk) = pool.freeList;
		pool.freeList = chunk;
	}

private:
	static_assert(chunkSize % sizeof(void*) == 0, "chunks must hold and align a free-list pointer");

	std::mutex mutex;
	void* freeList = NULL;
	size_t nextSize = 32;

	static ChunkPool& instance()
	{
		static ChunkPool* pool = new ChunkPool();
		return *pool;
	}

	bool grow()
	{
		char* block = new (std::nothrow) char[nextSize * chunkSize];

		if (block == NULL) {
			return false;
		}

		for (size_t i = nextSize; i-- > 0;) {
			void* chunk = block + i * chunkSize;
			*static_cast<void**>(chunk) = freeList;
			freeList = chunk;
		}

		nextSize <<= 1;
		return true;
	}
};

LuaAllocator::LuaAllocator() : heapSize(0), heapCount(0), maxHeapSize(0), maxHeapCount(0)
{
}

void LuaAllocator::clearHeapMax()
{
	maxHeapSize = 0;
	maxHeapCount = 0;
}

void LuaAllocator::getHeapStats(unsigned int& heapSize, unsigned int& heapCount) const
{
	heapSize = this->heapSize;
	heapCount = this->heapCount;
}

void LuaAllocator::getHeapStats(
	unsigned int& heapSize, unsigned int& heapCount, unsigned int& maxHeapSize, unsigned int& maxHeapCount
) const
{
	heapSize = this->heapSize;
	heapCount = this->heapCount;
	maxHeapSize = this->maxHeapSize;
	maxHeapCount = this->maxHeapCount;
}

template <size_t smallSize, size_t largeSize>
void* bucketRealloc(void* ptr, size_t osize, size_t nsize)
{
	void* result;

	if (nsize > largeSize) {
		if (osize > largeSize || osize == 0) {
			result = realloc(ptr, nsize);
		}
		else {
			result = malloc(nsize);
			memcpy(result, ptr, osize);
			if (osize > smallSize) {
				ChunkPool<largeSize>::free(ptr);
			}
			else {
				ChunkPool<smallSize>::free(ptr);
			}
		}
	}
	else if (nsize > smallSize) {
		if (osize > largeSize) {
			result = ChunkPool<largeSize>::malloc();
			memcpy(result, ptr, nsize);
			free(ptr);
		}
		else if (osize > smallSize) {
			result = ptr;
		}
		else {
			result = ChunkPool<largeSize>::malloc();
			if (osize > 0) {
				memcpy(result, ptr, osize);
				ChunkPool<smallSize>::free(ptr);
			}
		}
	}
	else if (nsize > 0) {
		if (osize > largeSize) {
			result = ChunkPool<smallSize>::malloc();
			memcpy(result, ptr, nsize);
			free(ptr);
		}
		else if (osize > smallSize) {
			result = ChunkPool<smallSize>::malloc();
			memcpy(result, ptr, nsize);
			ChunkPool<largeSize>::free(ptr);
		}
		else if (osize > 0) {
			result = ptr;
		}
		else {
			result = ChunkPool<smallSize>::malloc();
		}
	}
	else {
		if (osize > largeSize) {
			free(ptr);
		}
		else if (osize > smallSize) {
			ChunkPool<largeSize>::free(ptr);
		}
		else if (osize > 0) {
			ChunkPool<smallSize>::free(ptr);
		}
		result = NULL;
	}

	return result;
}

void* LuaAllocator::alloc(void* ptr, size_t osize, size_t nsize)
{
	void* result = bucketRealloc<16, 32>(ptr, osize, nsize);

	heapSize += nsize - osize;

	if (osize == 0) {
		heapCount++;
	}

	if (nsize == 0) {
		heapCount--;
	}

	maxHeapSize = std::max(maxHeapSize, heapSize);
	maxHeapCount = std::max(maxHeapCount, heapCount);

	return result;
}

void* LuaAllocator::alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	return static_cast<LuaAllocator*>(ud)->alloc(ptr, osize, nsize);
}
