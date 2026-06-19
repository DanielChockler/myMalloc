#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <unistd.h>
#include "allocator.h"

memBlock* Allocator::findLastBlock() {
  memBlock* block = reinterpret_cast<memBlock*>(heapStart);
  while (block->next) block = block->next;
  return block;
}

memBlock* Allocator::findSmallestBlock(size_t size) {
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

memBlock* Allocator::allocateBestFit(size_t size) {
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

memBlock* Allocator::findPrevUsedBlock(memBlock* block) {
  while (block->prev) {
    block = block->prev;
    if (block->inUse) return block;
  }

  return nullptr;
}

void Allocator::reduceSizeIfPossible() {
  memBlock* lastBlock = findLastBlock();
  memBlock* prevUsedBlock = findPrevUsedBlock(lastBlock);

  if (!prevUsedBlock) {
    if (lastBlock->length > m_capacity) lastBlock->length = m_capacity;
    prevUsedBlock = lastBlock;
  }

  std::byte* newHeapEnd = reinterpret_cast<std::byte*>(prevUsedBlock) + sizeof(memBlock) + prevUsedBlock->length;
  
  while (newHeapEnd < heapEnd - m_capacity) {
    sbrk(-m_capacity);
    heapEnd = reinterpret_cast<std::byte*>(sbrk(0));
    --pageNum;
  }

  if (heapEnd - newHeapEnd > sizeof(memBlock) + 1) {
    memBlock* newLastUnusedBlock = reinterpret_cast<memBlock*>(newHeapEnd);
    newLastUnusedBlock->marker = BLOCKMARKER;
    newLastUnusedBlock->inUse = false;
    newLastUnusedBlock->prev = prevUsedBlock;
    newLastUnusedBlock->next = nullptr;
    newLastUnusedBlock->length = heapEnd - newHeapEnd - sizeof(memBlock);
    prevUsedBlock->next = newLastUnusedBlock;
  }
}

Allocator::Allocator(size_t capacity) : m_capacity(capacity) {
  if (capacity <= sizeof(memBlock)) throw std::bad_alloc();
  
  // Ensure heap start is alligned to 16 bytes
  std::uintptr_t initial = reinterpret_cast<std::uintptr_t>(sbrk(0));
  std::uintptr_t alligned = (initial + 15) & ~(15);
  sbrk(alligned - initial);

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

std::byte* Allocator::allocate(size_t size) {
  bool expected {false};
  size_t allignedSize = (size + 15) & ~(15); // align to 16 bytes

  while (!lock.compare_exchange_weak(expected, true, std::memory_order_acquire)) expected = false;

  memBlock* block = allocateBestFit(allignedSize);
  
  lock.store(false, std::memory_order_release);

  return reinterpret_cast<std::byte*>(block) + sizeof(memBlock);
}

void Allocator::deallocate(std::byte* ptr) {
  if (!ptr) return;

  bool expected = false;
  while (!lock.compare_exchange_weak(expected, true, std::memory_order_acquire)) expected = false;
  
  memBlock* block = reinterpret_cast<memBlock*>(ptr - sizeof(memBlock));
  if (block->marker != BLOCKMARKER) throw std::bad_alloc();

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
  
  reduceSizeIfPossible();
  lock.store(false, std::memory_order_release); 
}
