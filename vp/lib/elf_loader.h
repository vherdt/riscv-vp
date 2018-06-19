#ifndef RISCV_ISA_ELF_LOADER_H
#define RISCV_ISA_ELF_LOADER_H


#include <boost/iostreams/device/mapped_file.hpp>


// see: http://wiki.osdev.org/ELF_Tutorial
// for ELF definitions

typedef uint16_t Elf32_Half;	// Unsigned half int
typedef uint32_t Elf32_Off;	    // Unsigned offset
typedef uint32_t Elf32_Addr;	// Unsigned address
typedef uint32_t Elf32_Word;	// Unsigned int
typedef int32_t  Elf32_Sword;	// Signed int

#define ELF_NIDENT 16

typedef struct {
    uint8_t		e_ident[ELF_NIDENT];
    Elf32_Half	e_type;
    Elf32_Half	e_machine;
    Elf32_Word	e_version;
    Elf32_Addr	e_entry;
    Elf32_Off	e_phoff;
    Elf32_Off	e_shoff;
    Elf32_Word	e_flags;
    Elf32_Half	e_ehsize;
    Elf32_Half	e_phentsize;
    Elf32_Half	e_phnum;
    Elf32_Half	e_shentsize;
    Elf32_Half	e_shnum;
    Elf32_Half	e_shstrndx;
} Elf32_Ehdr;


typedef struct {
    Elf32_Word	p_type;
    Elf32_Off	p_offset;
    Elf32_Addr	p_vaddr;
    Elf32_Addr	p_paddr;
    Elf32_Word	p_filesz;
    Elf32_Word	p_memsz;
    Elf32_Word	p_flags;
    Elf32_Word	p_align;
} Elf32_Phdr;


enum Elf32_PhdrType {
    PT_NULL     = 0,
    PT_LOAD     = 1,
    PT_DYNAMIC  = 2,
    PT_INTERP   = 3,
    PT_NOTE     = 4,
    PT_SHLIB    = 5,
    PT_PHDR     = 6
};


struct ELFLoader {
    const char *filename;
    boost::iostreams::mapped_file_source elf;
    const Elf32_Ehdr *hdr;

    ELFLoader(const char *filename)
        : filename(filename), elf(filename) {
        assert (elf.is_open() && "file not open");

        hdr = reinterpret_cast<const Elf32_Ehdr *>(elf.data());
    }


    void load_executable_image(uint8_t *dst, uint32_t size, uint32_t offset) {
        for (int i=0; i<hdr->e_phnum; ++i) {
            const Elf32_Phdr *p = reinterpret_cast<const Elf32_Phdr *>(
                    elf.data() + hdr->e_phoff + hdr->e_phentsize * i);

            if (p->p_type != PT_LOAD)
                continue;

            assert ((p->p_vaddr+p->p_memsz >= offset) && (p->p_vaddr+p->p_memsz < offset+size));

            //NOTE: if memsz is larger than filesz, the additional bytes are zero initialized (auto. done for memory)
            memcpy(dst+p->p_vaddr-offset, elf.data()+p->p_offset, p->p_filesz);
        }
    }

    uint32_t get_memory_end() {
        const Elf32_Phdr *last = reinterpret_cast<const Elf32_Phdr *>(
                elf.data() + hdr->e_phoff + hdr->e_phentsize * (hdr->e_phnum - 1));

        return last->p_vaddr + last->p_memsz;
    }

    uint32_t get_heap_addr() {
        // return first 8 byte aligned address after the memory image
        auto s = get_memory_end();
        return s + s % 8;
    }

    uint32_t get_entrypoint() {
        return hdr->e_entry;
    }
};


#endif //RISCV_ISA_ELF_LOADER_H
