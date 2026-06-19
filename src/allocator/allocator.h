#include <cstddef>
#include <cstdint>
#include <cstring>
#include <atomic>
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
  size_t m_capacity;
  std::byte* heapStart;
  std::byte* heapEnd;
  std::atomic<bool> lock {false};
  uint32_t blockNum {0};
  uint16_t pageNum {0};
  
  memBlock* findLastBlock();
  memBlock* findSmallestBlock(size_t size);
  memBlock* allocateBestFit(size_t size);
  memBlock* findPrevUsedBlock(memBlock* block);
  void reduceSizeIfPossible();

public:
  explicit Allocator(size_t capacity);

  // Obey rule of 5, don't want to copy or move the allocator
  Allocator(const Allocator&) = delete;
  Allocator& operator=(const Allocator&) = delete;
  Allocator(Allocator&&) = delete;
  Allocator& operator=(Allocator&&) = delete;

  std::byte* allocate(size_t size);
  void deallocate(std::byte* ptr);
};
