# myMalloc - Heap allocator built in C++

A heap allocator I implemented in C++. Built to understand what malloc does under the hood.

---

## Features

- **Best-fit allocation** - decided to implement allocation by finding the smallest block that fits the payload. This makes the allocator memory-efficient rather than time efficient.
- **Block splitting** - following allocation, any leftover space that is at able to store the 32 byte header as well as at least 1 byte of payload is split into a new free block.
- **Bidirectional coalescing** - following deallocation, adjacent free blocks in both directions are merged, reducing fragmentation.
- **Dynamic heap growth** - during allocation if no block that is large enough can be found, `sbrk` is used to double the size of the heap on demand.
- **Dynamic heap shrinkage** - following deallocation, `sbrk` is used to release any trailing pages back to the OS if they are free.
- **16-byte alignment** - all pointers returned from `allocate` are aligned to 16 bytes, and the heap start is also padded so that the `heapStart` pointer is aligned to 16 bytes at construction.
- **Corruption detection** - the header of every block has a magic marker `0xBEEF` to indicate that the memory access is valid. `std::bad_alloc()` is thrown otherwise.
- **Thread-safety** - a spinlock is used to protect both allocate and deallocate calls.

---

## Memory layout of the block header

Each block is preceded by a 32-byte header, `memBlock`. `sizeof(memBlock)` == 32

```
+--------+--------+--------+--------+-------+---padding---+
|  prev  |  next  | length | marker | inUse |             |
|  8 B   |  8 B   |  8 B   |  4 B   |  1 B  |    3 B      |  =  32 B (alignas(16))
+--------+--------+--------+--------+-------+---padding---+
|                    user data (at least 1 byte)          |
+----------------------------------------------------------+
```

Fields are ordered largest to smallest to minimise the amount of padding between each field. `alignas(16)` ensures that every header and, so every pointer is 16-byte aligned.

## Design decisions

**Best-fit**
Best-fit reduces fragmentation at the cost of a full scan of the free-list per allocation which is an O(n) operation. A production allocator would use segregated free-lists that are separated by size class to get O(1) amortised allocation.

**`sbrk` over `mmap`**
`sbrk` gives a single contiguous heap, which helps keep the implementation simple, which is good for a learning exercise such as this. The tradeoff is that `sbrk` is not thread-safe at the OS level if anything else in the process calls `malloc`. A production allocator would likely use `mmap(MAP_ANONYMOUS)`.

## Building

```bash
git clone "https://github.com/DanielChockler/myMalloc.git"
mkdir build && cd build
cmake --build .
```

## Testing

```bash
cd build
./tests/run_tests
```

Runs each test in a forked child process, so a crash or signal in one test is caught and reported without crashing the entire test suite.

## Limitations and improvements

- The free list scan is O(n), which is acceptable for a single-threaded allocator with a relatively small number of live block. A segregated free list keyed by size class or a sorted tree of free blocks would be more time efficient
- `sbrk` is deprecated in POSIX 2018. `mmap` would be the modern, albeit more complex, replacement.
