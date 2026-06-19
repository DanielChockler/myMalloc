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


