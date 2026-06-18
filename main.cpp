#include <cstddef>
#include <cstdint>
#include <new>
#include <unistd.h>

const int BLOCKMARKER {0xBEEF};

struct memBlock {
  uint32_t marker;
  memBlock* prev;
  bool inUse;
  size_t length;
  memBlock* next;
};

class Allocator {
private:
  std::byte* heapStart;
  std::byte* heapEnd;
  bool lock;
  uint32_t blockNum {0};
  uint16_t pageNum {0};

public:
  explicit Allocator(size_t capacity) {
    if (capacity > sizeof(memBlock)) throw std::bad_alloc();

    heapStart = reinterpret_cast<std::byte*>(sbrk(0));
    sbrk(capacity);
    heapEnd = reinterpret_cast<std::byte*>(sbrk(0));
    
    ++blockNum;
    ++pageNum;

    memBlock* firstBlock = reinterpret_cast<memBlock*>(heapStart);
    firstBlock->marker = BLOCKMARKER;
    firstBlock->inUse = false;
    firstBlock->length = capacity - sizeof(memBlock);
    firstBlock->next = nullptr;
    firstBlock->prev = nullptr;
  }
  
  std::byte* allocate(size_t size) {
    while (lock) sleep(1);
    lock = true;

    
  }

};
