#pragma once

#include <stdint.h>

class MemoryDMI {
    uint8_t *mem;
    uint64_t start;
    uint64_t size;
    uint64_t end;

    MemoryDMI(uint8_t *mem, uint64_t start, uint64_t size)
            : mem(mem), start(start), size(size), end(start+size) {
    }

public:
    static MemoryDMI create_start_end_mapping(uint8_t *mem, uint64_t start, uint64_t end) {
        assert (end > start);
        return create_start_size_mapping(mem, start, end-start);
    }

    static MemoryDMI create_start_size_mapping(uint8_t *mem, uint64_t start, uint64_t size) {
        assert (start + size > start);
        return MemoryDMI(mem, start, size);
    }

    uint8_t *get_mem_ptr() {
        return mem;
    }

    template <typename T>
    T *get_mem_ptr_to_global_addr(uint64_t addr) {
        assert (contains(addr));
        assert ((addr + sizeof(T)) <= end);
        assert ((addr % sizeof(T)) == 0 && "unaligned access");
        return reinterpret_cast<T*>(mem + (addr - start));
    }

    uint64_t get_start() {
        return start;
    }

    uint64_t get_end() {
        return start+size;
    }

    uint64_t get_size() {
        return size;
    }

    bool contains(uint64_t addr) {
        return addr >= start && addr < end;
    }
};