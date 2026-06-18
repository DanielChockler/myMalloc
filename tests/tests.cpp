#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <cassert>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include "allocator.h"

void callTest(std::function<void()> testFunction, const std::string& msg) {
  pid_t pid = fork();
  if (pid == 0) {
    testFunction();
    exit(0);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) std::cout << msg << " crashed with signal " << WTERMSIG(status) << '\n';
    else std::cout << msg << " passed\n";
  }
}

void testBasicMalloc() {
  Allocator allocator(4096);
  std::byte* ptr = allocator.allocate(128);
  assert(ptr != nullptr);

  memBlock* blockHeader = reinterpret_cast<memBlock*>(ptr - sizeof(memBlock));
  assert(blockHeader->marker == BLOCKMARKER);
  assert(blockHeader->inUse == true);
  assert(blockHeader->length == 128);
}

void testBiggerThanAvailableMalloc() {
  Allocator allocator(1024);
  uint16_t* ptr = reinterpret_cast<uint16_t*>(allocator.allocate(5000));
  assert(ptr != nullptr);

  for (uint16_t i {0}; i <= 2499; ++i) *(ptr + i) = i;
  
  assert(*ptr == 0);
  assert(*(ptr + 2) == 2);
  assert(*(ptr + 2499) == 2499);
}

void testFree() {
  Allocator allocator(4096);
  std::byte* firstPtr = allocator.allocate(2048);
  memBlock* firstBlock = reinterpret_cast<memBlock*>(firstPtr - sizeof(memBlock));
  assert(firstBlock->next != nullptr);
  assert(firstBlock->length == 2048);

  memBlock* secondBlock = firstBlock->next;
  assert(secondBlock->marker == BLOCKMARKER);
  assert(secondBlock->inUse == false);
  assert(secondBlock->next == nullptr);
  assert(secondBlock->length == (4096 - sizeof(memBlock) - sizeof(memBlock) - firstBlock->length));

  allocator.deallocate(firstPtr);
  assert(firstBlock->marker == BLOCKMARKER);
  assert(firstBlock->next == nullptr);
  assert(firstBlock->length == (4096 - sizeof(memBlock)));
}

int main() {
  std::cout << "Running allocator unit tests:\n";
  callTest(testBasicMalloc,"Basic Malloc");
  callTest(testBiggerThanAvailableMalloc, "Request more memory Malloc");
  callTest(testFree, "Basic Free");

  std::cout << "DONE\n";
  return 0;
}
