#include <cstddef>
#include <cstdint>
#include <cstring>
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
  size_t m_capacity;
  std::byte* heapStart;
  std::byte* heapEnd;
  bool lock;
  uint32_t blockNum {0};
  uint16_t pageNum {0};
  
  memBlock* findLastBlock() {
    memBlock* block = reinterpret_cast<memBlock*>(heapStart);
    while (block->next) block = block->next;
    return block;
  }

  memBlock* findSmallestBlock(size_t size) {
    memBlock* currentBlock = reinterpret_cast<memBlock*>(heapStart);
    memBlock* smallestBlock {nullptr};
    memBlock* lastBlock = currentBlock;
    
    while (currentBlock) {
      if (currentBlock->marker != BLOCKMARKER) throw std::bad_alloc();

      if (currentBlock->length + sizeof(memBlock) >= size && currentBlock->inUse == false) {
        if (smallestBlock == nullptr || currentBlock->length < smallestBlock->length) {
          smallestBlock = currentBlock;
        }
      }
      lastBlock = currentBlock;
      currentBlock = currentBlock->next;
    }

    if (!smallestBlock) {
      memBlock* block = findLastBlock();
      while (block->length < size) {
        sbrk(m_capacity);
        block->length += m_capacity;
        ++pageNum;
      }
      smallestBlock = block;
    }
    
    smallestBlock->inUse = true;

    return smallestBlock;
  }

  memBlock* allocateBestFit(size_t size) {
    memBlock* smallestBlock = findSmallestBlock(size);
    int leftoverSpace = smallestBlock->length - size - sizeof(memBlock);
    
    if (smallestBlock->next && leftoverSpace < 1) return smallestBlock; // Smallest block not at end and too small to split

    while (leftoverSpace < 1) {  // Must have at least 1 byte space to allocate something in a new block
       sbrk(m_capacity);
      smallestBlock->length += m_capacity;
      ++pageNum;
      leftoverSpace = smallestBlock->length - size - sizeof(memBlock);
    }
    
    std::byte* newBlockLocation = reinterpret_cast<std::byte*>(smallestBlock) + sizeof(memBlock) + size;
    memBlock* newBlock = reinterpret_cast<memBlock*>(newBlockLocation);
    newBlock->marker = BLOCKMARKER;
    newBlock->inUse = false;
    newBlock->length = leftoverSpace;
    newBlock->next = smallestBlock->next;
    newBlock->prev = smallestBlock;
    
    if (newBlock->next) newBlock->next->prev = newBlock;
    smallestBlock->next = newBlock;

    smallestBlock->length = size;
    ++blockNum;
    
    return smallestBlock;
  }

public:
  explicit Allocator(size_t capacity) : m_capacity(capacity) {
    if (capacity <= sizeof(memBlock)) throw std::bad_alloc();

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
  
  // Obey rule of 5, don't want to copy or move the allocator
  Allocator(const Allocator&) = delete;
  Allocator& operator=(const Allocator&) = delete;
  Allocator(Allocator&&) = delete;
  Allocator& operator=(Allocator&&) = delete;
  
  std::byte* allocate(size_t size) {
    while (lock) sleep(1);
    lock = true;

    memBlock* block = allocateBestFit(size);
    
    lock = false;

    return reinterpret_cast<std::byte*>(block) + sizeof(memBlock);
  }

  bool deallocate(std::byte* ptr) {
    if (!ptr) return false;
    while (lock) sleep(1);

    lock = true;
    memBlock* block = reinterpret_cast<memBlock*>(ptr - sizeof(memBlock));
    if (block->marker != BLOCKMARKER) return false;

    block->inUse = false;
    
    if (block->next && !(block->next->inUse)){
      memBlock* nextUnusedBlock = block->next;
      block->length += sizeof(memBlock) + nextUnusedBlock->length;
      block->next = nextUnusedBlock->next;
      if (block->next) block->next->prev = block;
      --blockNum;
    }

    if (block->prev && !(block->prev->inUse)) {
      memBlock* prevUnusedBlock = block->prev;
      prevUnusedBlock->length += sizeof(memBlock) + block->length;
      prevUnusedBlock->next = block->next;
      if (prevUnusedBlock->next) prevUnusedBlock->next->prev = prevUnusedBlock;
      --blockNum;
    }

    lock = false;
    return true;
  }
};
