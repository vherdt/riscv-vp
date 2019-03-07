#pragma once

#include <boost/iostreams/device/mapped_file.hpp>

#include <cstdint>
#include <vector>


// see: "ELF-64 Object File Format" document for ELF64 type definitions

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;


#define ELF_NIDENT 16

typedef struct {
    unsigned char e_ident[16]; /* ELF identification */
    Elf64_Half e_type; /* Object file type */
    Elf64_Half e_machine; /* Machine type */
    Elf64_Word e_version; /* Object file version */
    Elf64_Addr e_entry; /* Entry point address */
    Elf64_Off e_phoff; /* Program header offset */
    Elf64_Off e_shoff; /* Section header offset */
    Elf64_Word e_flags; /* Processor-specific flags */
    Elf64_Half e_ehsize; /* ELF header size */
    Elf64_Half e_phentsize; /* Size of program header entry */
    Elf64_Half e_phnum; /* Number of program header entries */
    Elf64_Half e_shentsize; /* Size of section header entry */
    Elf64_Half e_shnum; /* Number of section header entries */
    Elf64_Half e_shstrndx; /* Section name string table index */
} Elf64_Ehdr;

typedef struct
{
    Elf64_Word sh_name; /* Section name */
    Elf64_Word sh_type; /* Section type */
    Elf64_Xword sh_flags; /* Section attributes */
    Elf64_Addr sh_addr; /* Virtual address in memory */
    Elf64_Off sh_offset; /* Offset in file */
    Elf64_Xword sh_size; /* Size of section */
    Elf64_Word sh_link; /* Link to other section */
    Elf64_Word sh_info; /* Miscellaneous information */
    Elf64_Xword sh_addralign; /* Address alignment boundary */
    Elf64_Xword sh_entsize; /* Size of entries, if section has table */
} Elf64_Shdr;

typedef struct
{
    Elf64_Word st_name; /* Symbol name */
    unsigned char st_info; /* Type and Binding attributes */
    unsigned char st_other; /* Reserved */
    Elf64_Half st_shndx; /* Section table index */
    Elf64_Addr st_value; /* Symbol value */
    Elf64_Xword st_size; /* Size of object (e.g., common) */
} Elf64_Sym;

typedef struct
{
    Elf64_Word p_type; /* Type of segment */
    Elf64_Word p_flags; /* Segment attributes */
    Elf64_Off p_offset; /* Offset in file */
    Elf64_Addr p_vaddr; /* Virtual address in memory */
    Elf64_Addr p_paddr; /* Reserved */
    Elf64_Xword p_filesz; /* Size of segment in file */
    Elf64_Xword p_memsz; /* Size of segment in memory */
    Elf64_Xword p_align; /* Alignment of segment */
} Elf64_Phdr;


enum Elf32_PhdrType {
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    PT_SHLIB = 5,
    PT_PHDR = 6
};


/* NOTE: identic to ELF32 Loader except for using 64 bit data types -> put both together. */
struct ELFLoader {
    const char *filename;
    boost::iostreams::mapped_file_source elf;
    const Elf64_Ehdr *hdr;

    ELFLoader(const char *filename) : filename(filename), elf(filename) {
        assert(elf.is_open() && "file not open");

        hdr = reinterpret_cast<const Elf64_Ehdr *>(elf.data());
    }

    std::vector<const Elf64_Phdr *> get_load_sections() {
        std::vector<const Elf64_Phdr *> sections;

        for (int i = 0; i < hdr->e_phnum; ++i) {
            const Elf64_Phdr *p =
                    reinterpret_cast<const Elf64_Phdr *>(elf.data() + hdr->e_phoff + hdr->e_phentsize * i);

            if (p->p_type != PT_LOAD)
                continue;

            sections.push_back(p);
        }

        return sections;
    }

    void load_executable_image(uint8_t *dst, uint64_t size, uint64_t offset, bool use_vaddr = true) {
        for (auto section : get_load_sections()) {
            if (use_vaddr) {
                assert((section->p_vaddr >= offset) && (section->p_vaddr + section->p_memsz < offset + size));

                // NOTE: if memsz is larger than filesz, the additional bytes
                // are zero initialized (auto. done for memory)
                memcpy(dst + section->p_vaddr - offset, elf.data() + section->p_offset, section->p_filesz);
            } else {
                if (section->p_filesz == 0) {
                    // skipping empty sections, we are 0 initialized
                    continue;
                }
                if (section->p_paddr < offset) {
                    std::cerr << "Section physical address 0x" << std::hex << section->p_paddr
                              << " not in local offset (0x" << std::hex << offset << ")!" << std::endl;
                    // raise(std::runtime_error("elf cant be loaded"));
                }
                if (section->p_paddr + section->p_memsz >= offset + size) {
                    std::cerr << "Section would overlap memory (0x" << std::hex << section->p_paddr << " + 0x"
                              << std::hex << section->p_memsz << ") >= 0x" << std::hex << offset + size << std::endl;
                    // raise(std::runtime_error("elf cant be loaded"));
                }
                assert((section->p_paddr >= offset) && (section->p_paddr + section->p_memsz < offset + size));

                // NOTE: if memsz is larger than filesz, the additional bytes
                // are zero initialized (auto. done for memory)
                memcpy(dst + section->p_paddr - offset, elf.data() + section->p_offset, section->p_filesz);
            }
        }
    }

    uint64_t get_memory_end() {
        const Elf64_Phdr *last =
                reinterpret_cast<const Elf64_Phdr *>(elf.data() + hdr->e_phoff + hdr->e_phentsize * (hdr->e_phnum - 1));

        return last->p_vaddr + last->p_memsz;
    }

    uint64_t get_heap_addr() {
        // return first 8 byte aligned address after the memory image
        auto s = get_memory_end();
        return s + s % 8;
    }

    uint64_t get_entrypoint() {
        return hdr->e_entry;
    }

    const char *get_section_string_table() {
        assert(hdr->e_shoff != 0 && "string table section not available");

        const Elf64_Shdr *s =
                reinterpret_cast<const Elf64_Shdr *>(elf.data() + hdr->e_shoff + hdr->e_shentsize * hdr->e_shstrndx);
        const char *start = elf.data() + s->sh_offset;
        return start;
    }

    const char *get_symbol_string_table() {
        auto s = get_section(".strtab");
        return elf.data() + s->sh_offset;
    }

    const Elf64_Sym *get_symbol(const char *symbol_name) {
        const Elf64_Shdr *s = get_section(".symtab");
        const char *strings = get_symbol_string_table();

        assert(s->sh_size % sizeof(Elf64_Sym) == 0);

        auto num_entries = s->sh_size / sizeof(Elf64_Sym);
        for (unsigned i = 0; i < num_entries; ++i) {
            const Elf64_Sym *p = reinterpret_cast<const Elf64_Sym *>(elf.data() + s->sh_offset + i * sizeof(Elf64_Sym));

            //std::cout << "check symbol: " << strings + p->st_name << std::endl;

            if (!strcmp(strings + p->st_name, symbol_name)) {
                return p;
            }
        }

        throw std::runtime_error("unable to find symbol in the symbol table " + std::string(symbol_name));
    }

    uint64_t get_begin_signature_address() {
        auto p = get_symbol("begin_signature");
        return p->st_value;
    }

    uint64_t get_end_signature_address() {
        auto p = get_symbol("end_signature");
        return p->st_value;
    }

    const Elf64_Shdr *get_section(const char *section_name) {
        if (hdr->e_shoff == 0) {
            throw std::runtime_error("unable to find section address, section table not available: " + std::string(section_name));
        }

        const char *strings = get_section_string_table();

        for (unsigned i = 0; i < hdr->e_shnum; ++i) {
            const Elf64_Shdr *s =
                    reinterpret_cast<const Elf64_Shdr *>(elf.data() + hdr->e_shoff + hdr->e_shentsize * i);

            //std::cout << "check section: " << strings + s->sh_name << std::endl;

            if (!strcmp(strings + s->sh_name, section_name)) {
                return s;
            }
        }

        throw std::runtime_error("unable to find section address, section seems not available: " + std::string(section_name));
    }
};