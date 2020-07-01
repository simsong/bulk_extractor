/*
 * /usr/include/elf.h from archlinux.org June 2012
 * 
 * data structures modified from GNU C Library which is released under
 * LGPL 2.1 license
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"

/* tunable constants */
u_int sht_null_counter_max = 10;
u_int slt_max_name_size = 65535;

/* constants from the ELF specification */



#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI	7		/* OS ABI identification */
#define EI_ABIVERSION	8		/* ABI version */

#define ELFCLASS32   1
#define ELFCLASS64   2

#define ELFDATA2LSB	1		/* 2's complement, little endian */
#define ELFDATA2MSB	2		/* 2's complement, big endian */

#define ELFOSABI_NONE		0	/* UNIX System V ABI */
#define ELFOSABI_SYSV		0	/* Alias.  */
#define ELFOSABI_HPUX		1	/* HP-UX */
#define ELFOSABI_NETBSD		2	/* NetBSD.  */
#define ELFOSABI_GNU		3	/* Object uses GNU ELF extensions.  */
#define ELFOSABI_LINUX		ELFOSABI_GNU /* Compatibility alias.  */
#define ELFOSABI_SOLARIS	6	/* Sun Solaris.  */
#define ELFOSABI_AIX		7	/* IBM AIX.  */
#define ELFOSABI_IRIX		8	/* SGI Irix.  */
#define ELFOSABI_FREEBSD	9	/* FreeBSD.  */
#define ELFOSABI_TRU64		10	/* Compaq TRU64 UNIX.  */
#define ELFOSABI_MODESTO	11	/* Novell Modesto.  */
#define ELFOSABI_OPENBSD	12	/* OpenBSD.  */
#define ELFOSABI_ARM_AEABI	64	/* ARM EABI */
#define ELFOSABI_ARM		97	/* ARM */
#define ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */

/* Legal values for e_type (object file type).  */
#define ET_NONE		0		/* No file type */
#define ET_REL		1		/* Relocatable file */
#define ET_EXEC		2		/* Executable file */
#define ET_DYN		3		/* Shared object file */
#define ET_CORE		4		/* Core file */
#define	ET_NUM		5		/* Number of defined types */
#define ET_LOOS		0xfe00		/* OS-specific range start */
#define ET_HIOS		0xfeff		/* OS-specific range end */
#define ET_LOPROC	0xff00		/* Processor-specific range start */
#define ET_HIPROC	0xffff		/* Processor-specific range end */


/* Legal values for sh_type (section type).  */
#define SHT_NULL	  0		/* Section header table entry unused */
#define SHT_PROGBITS	  1		/* Program data */
#define SHT_SYMTAB	  2		/* Symbol table */
#define SHT_STRTAB	  3		/* String table */
#define SHT_RELA	  4		/* Relocation entries with addends */
#define SHT_HASH	  5		/* Symbol hash table */
#define SHT_DYNAMIC	  6		/* Dynamic linking information */
#define SHT_NOTE	  7		/* Notes */
#define SHT_NOBITS	  8		/* Program space with no data (bss) */
#define SHT_REL		  9		/* Relocation entries, no addends */
#define SHT_SHLIB	  10		/* Reserved */
#define SHT_DYNSYM	  11		/* Dynamic linker symbol table */
#define SHT_INIT_ARRAY	  14		/* Array of constructors */
#define SHT_FINI_ARRAY	  15		/* Array of destructors */
#define SHT_PREINIT_ARRAY 16		/* Array of pre-constructors */
#define SHT_GROUP	  17		/* Section group */
#define SHT_SYMTAB_SHNDX  18		/* Extended section indeces */
#define	SHT_NUM		  19		/* Number of defined types.  */
#define SHT_LOOS	  0x60000000	/* Start OS-specific.  */
#define SHT_GNU_ATTRIBUTES 0x6ffffff5	/* Object attributes.  */
#define SHT_GNU_HASH	  0x6ffffff6	/* GNU-style hash table.  */
#define SHT_GNU_LIBLIST	  0x6ffffff7	/* Prelink library list */
#define SHT_CHECKSUM	  0x6ffffff8	/* Checksum for DSO content.  */
#define SHT_LOSUNW	  0x6ffffffa	/* Sun-specific low bound.  */
#define SHT_SUNW_move	  0x6ffffffa
#define SHT_SUNW_COMDAT   0x6ffffffb
#define SHT_SUNW_syminfo  0x6ffffffc
#define SHT_GNU_verdef	  0x6ffffffd	/* Version definition section.  */
#define SHT_GNU_verneed	  0x6ffffffe	/* Version needs section.  */
#define SHT_GNU_versym	  0x6fffffff	/* Version symbol table.  */
#define SHT_HISUNW	  0x6fffffff	/* Sun-specific high bound.  */
#define SHT_HIOS	  0x6fffffff	/* End OS-specific type */
#define SHT_LOPROC	  0x70000000	/* Start of processor-specific */
#define SHT_HIPROC	  0x7fffffff	/* End of processor-specific */
#define SHT_LOUSER	  0x80000000	/* Start of application-specific */
#define SHT_HIUSER	  0x8fffffff	/* End of application-specific */

const uint32_t SHF_WRITE            = (1 << 0);
const uint32_t SHF_ALLOC            = (1 << 1);
const uint32_t SHF_EXECINSTR        = (1 << 2);
const uint32_t SHF_MERGE            = (1 << 4);
const uint32_t SHF_STRINGS          = (1 << 5);
const uint32_t SHF_INFO_LINK        = (1 << 6);
const uint32_t SHF_LINK_ORDER       = (1 << 7);
const uint32_t SHF_OS_NONCONFORMING = (1 << 8);
const uint32_t SHF_GROUP            = (1 << 9);
const uint32_t SHF_TLS              = (1 << 10);
const uint32_t SHF_MASKOS           = 0x0ff00000;
const uint32_t SHF_MASKPROC         = 0xf0000000;
const uint32_t SHF_ORDERED          = (1 << 30);
const uint32_t SHF_EXCLUDE          = (1 << 31);

#define EM_NONE		 0		/* No machine */
#define EM_M32		 1		/* AT&T WE 32100 */
#define EM_SPARC	 2		/* SUN SPARC */
#define EM_386		 3		/* Intel 80386 */
#define EM_68K		 4		/* Motorola m68k family */
#define EM_88K		 5		/* Motorola m88k family */
#define EM_860		 7		/* Intel 80860 */
#define EM_MIPS		 8		/* MIPS R3000 big-endian */
#define EM_S370		 9		/* IBM System/370 */
#define EM_MIPS_RS3_LE	10		/* MIPS R3000 little-endian */
#define EM_PARISC	15		/* HPPA */
#define EM_VPP500	17		/* Fujitsu VPP500 */
#define EM_SPARC32PLUS	18		/* Sun's "v8plus" */
#define EM_960		19		/* Intel 80960 */
#define EM_PPC		20		/* PowerPC */
#define EM_PPC64	21		/* PowerPC 64-bit */
#define EM_S390		22		/* IBM S390 */
#define EM_V800		36		/* NEC V800 series */
#define EM_FR20		37		/* Fujitsu FR20 */
#define EM_RH32		38		/* TRW RH-32 */
#define EM_RCE		39		/* Motorola RCE */
#define EM_ARM		40		/* ARM */
#define EM_FAKE_ALPHA	41		/* Digital Alpha */
#define EM_SH		42		/* Hitachi SH */
#define EM_SPARCV9	43		/* SPARC v9 64-bit */
#define EM_TRICORE	44		/* Siemens Tricore */
#define EM_ARC		45		/* Argonaut RISC Core */
#define EM_H8_300	46		/* Hitachi H8/300 */
#define EM_H8_300H	47		/* Hitachi H8/300H */
#define EM_H8S		48		/* Hitachi H8S */
#define EM_H8_500	49		/* Hitachi H8/500 */
#define EM_IA_64	50		/* Intel Merced */
#define EM_MIPS_X	51		/* Stanford MIPS-X */
#define EM_COLDFIRE	52		/* Motorola Coldfire */
#define EM_68HC12	53		/* Motorola M68HC12 */
#define EM_MMA		54		/* Fujitsu MMA Multimedia Accelerator*/
#define EM_PCP		55		/* Siemens PCP */
#define EM_NCPU		56		/* Sony nCPU embeeded RISC */
#define EM_NDR1		57		/* Denso NDR1 microprocessor */
#define EM_STARCORE	58		/* Motorola Start*Core processor */
#define EM_ME16		59		/* Toyota ME16 processor */
#define EM_ST100	60		/* STMicroelectronic ST100 processor */
#define EM_TINYJ	61		/* Advanced Logic Corp. Tinyj emb.fam*/
#define EM_X86_64	62		/* AMD x86-64 architecture */
#define EM_PDSP		63		/* Sony DSP Processor */
#define EM_FX66		66		/* Siemens FX66 microcontroller */
#define EM_ST9PLUS	67		/* STMicroelectronics ST9+ 8/16 mc */
#define EM_ST7		68		/* STmicroelectronics ST7 8 bit mc */
#define EM_68HC16	69		/* Motorola MC68HC16 microcontroller */
#define EM_68HC11	70		/* Motorola MC68HC11 microcontroller */
#define EM_68HC08	71		/* Motorola MC68HC08 microcontroller */
#define EM_68HC05	72		/* Motorola MC68HC05 microcontroller */
#define EM_SVX		73		/* Silicon Graphics SVx */
#define EM_ST19		74		/* STMicroelectronics ST19 8 bit mc */
#define EM_VAX		75		/* Digital VAX */
#define EM_CRIS		76		/* Axis Communications 32-bit embedded processor */
#define EM_JAVELIN	77		/* Infineon Technologies 32-bit embedded processor */
#define EM_FIREPATH	78		/* Element 14 64-bit DSP Processor */
#define EM_ZSP		79		/* LSI Logic 16-bit DSP Processor */
#define EM_MMIX		80		/* Donald Knuth's educational 64-bit processor */
#define EM_HUANY	81		/* Harvard University machine-independent object files */
#define EM_PRISM	82		/* SiTera Prism */
#define EM_AVR		83		/* Atmel AVR 8-bit microcontroller */
#define EM_FR30		84		/* Fujitsu FR30 */
#define EM_D10V		85		/* Mitsubishi D10V */
#define EM_D30V		86		/* Mitsubishi D30V */
#define EM_V850		87		/* NEC v850 */
#define EM_M32R		88		/* Mitsubishi M32R */
#define EM_MN10300	89		/* Matsushita MN10300 */
#define EM_MN10200	90		/* Matsushita MN10200 */
#define EM_PJ		91		/* picoJava */
#define EM_OPENRISC	92		/* OpenRISC 32-bit embedded processor */
#define EM_ARC_A5	93		/* ARC Cores Tangent-A5 */
#define EM_XTENSA	94		/* Tensilica Xtensa Architecture */

/* If it is necessary to assign new unofficial EM_* values, please
   pick large random numbers (0x8523, 0xa7f2, etc.) to minimize the
   chances of collision with official or non-GNU unofficial values.  */

#define EM_ALPHA	0x9026


typedef struct
{
  uint8_t	e_ident[16];	/* Magic number and other info */
  uint16_t	e_type;			/* Object file type */
  uint16_t	e_machine;		/* Architecture */
  uint32_t	e_version;		/* Object file version */
  uint32_t	e_entry;		/* Entry point virtual address */
  uint32_t	e_phoff;		/* Program header table file offset */
  uint32_t	e_shoff;		/* Section header table file offset */
  uint32_t	e_flags;		/* Processor-specific flags */
  uint16_t	e_ehsize;		/* ELF header size in bytes */
  uint16_t	e_phentsize;	/* Program header table entry size */
  uint16_t	e_phnum;		/* Program header table entry count */
  uint16_t	e_shentsize;	/* Section header table entry size */
  uint16_t	e_shnum;		/* Section header table entry count */
  uint16_t	e_shstrndx;		/* Section header string table index */
} __attribute__((packed)) Elf32_Ehdr;

typedef struct
{
  unsigned char	e_ident[16];	/* Magic number and other info */
  uint16_t	e_type;			/* Object file type */
  uint16_t	e_machine;		/* Architecture */
  uint32_t	e_version;		/* Object file version */
  uint64_t	e_entry;		/* Entry point virtual address */
  uint64_t	e_phoff;		/* Program header table file offset */
  uint64_t	e_shoff;		/* Section header table file offset */
  uint32_t	e_flags;		/* Processor-specific flags */
  uint16_t	e_ehsize;		/* ELF header size in bytes */
  uint16_t	e_phentsize;		/* Program header table entry size */
  uint16_t	e_phnum;		/* Program header table entry count */
  uint16_t	e_shentsize;		/* Section header table entry size */
  uint16_t	e_shnum;		/* Section header table entry count */
  uint16_t	e_shstrndx;		/* Section header string table index */
} __attribute__((packed)) Elf64_Ehdr;

typedef struct
{
  uint32_t	sh_name;		/* Section name (string tbl index) */
  uint32_t	sh_type;		/* Section type */
  uint32_t	sh_flags;		/* Section flags */
  uint32_t	sh_addr;		/* Section virtual addr at execution */
  uint32_t	sh_offset;		/* Section file offset */
  uint32_t	sh_size;		/* Section size in bytes */
  uint32_t	sh_link;		/* Link to another section */
  uint32_t	sh_info;		/* Additional section information */
  uint32_t	sh_addralign;		/* Section alignment */
  uint32_t	sh_entsize;		/* Entry size if section holds table */
} __attribute__((packed)) Elf32_Shdr;

typedef struct
{
  uint32_t	sh_name;		/* Section name (string tbl index) */
  uint32_t	sh_type;		/* Section type */
  uint64_t	sh_flags;		/* Section flags */
  uint64_t	sh_addr;		/* Section virtual addr at execution */
  uint64_t	sh_offset;		/* Section file offset */
  uint64_t	sh_size;		/* Section size in bytes */
  uint32_t	sh_link;		/* Link to another section */
  uint32_t	sh_info;		/* Additional section information */
  uint64_t	sh_addralign;		/* Section alignment */
  uint64_t	sh_entsize;		/* Entry size if section holds table */
} __attribute__((packed)) Elf64_Shdr;

typedef struct
{
  int32_t d_tag;			/* Dynamic entry type */
  union
    {
      uint32_t d_val;			/* Integer value */
      uint32_t d_ptr;			/* Address value */
    } d_un;
} __attribute__((packed)) Elf32_Dyn;

typedef struct
{
  int64_t d_tag;			/* Dynamic entry type */
  union
    {
      uint64_t d_val;		/* Integer value */
      uint64_t d_ptr;			/* Address value */
    } d_un;
} __attribute__((packed)) Elf64_Dyn;

#define FLAGNAME(STR) {STR,#STR}
struct flagnames_t {
    uint32_t flag;
    const char * name;
};

struct flagnames_t elf_sh_flags[] = {
    FLAGNAME(SHF_WRITE),
    FLAGNAME(SHF_ALLOC),
    FLAGNAME(SHF_EXECINSTR),
    FLAGNAME(SHF_MERGE),
    FLAGNAME(SHF_STRINGS),
    FLAGNAME(SHF_INFO_LINK),
    FLAGNAME(SHF_LINK_ORDER),
    FLAGNAME(SHF_OS_NONCONFORMING),
    FLAGNAME(SHF_GROUP),
    FLAGNAME(SHF_TLS),
    FLAGNAME(SHF_MASKOS),
    FLAGNAME(SHF_MASKPROC),
    FLAGNAME(SHF_ORDERED),
    FLAGNAME(SHF_EXCLUDE),
    {0,0}
};

struct flagnames_t elf_e_osabi[] = {
    FLAGNAME(ELFOSABI_NONE),
    FLAGNAME(ELFOSABI_SYSV),
    FLAGNAME(ELFOSABI_HPUX),
    FLAGNAME(ELFOSABI_NETBSD),
    FLAGNAME(ELFOSABI_GNU),
    FLAGNAME(ELFOSABI_LINUX),
    FLAGNAME(ELFOSABI_SOLARIS),
    FLAGNAME(ELFOSABI_AIX),
    FLAGNAME(ELFOSABI_IRIX),
    FLAGNAME(ELFOSABI_FREEBSD),
    FLAGNAME(ELFOSABI_TRU64),
    FLAGNAME(ELFOSABI_MODESTO),
    FLAGNAME(ELFOSABI_OPENBSD),
    FLAGNAME(ELFOSABI_ARM_AEABI),
    FLAGNAME(ELFOSABI_ARM),
    FLAGNAME(ELFOSABI_STANDALONE),
    {0,0}
};

struct flagnames_t elf_e_type[] = {
    FLAGNAME(ET_NONE),
    FLAGNAME(ET_REL),
    FLAGNAME(ET_EXEC),
    FLAGNAME(ET_DYN),
    FLAGNAME(ET_CORE),
    FLAGNAME(ET_NUM),
    FLAGNAME(ET_LOOS),
    FLAGNAME(ET_HIOS),
    FLAGNAME(ET_LOPROC),
    FLAGNAME(ET_HIPROC),
    {0,0}
};

struct flagnames_t elf_e_machine[] = {
    FLAGNAME(EM_NONE),
    FLAGNAME(EM_M32),
    FLAGNAME(EM_SPARC),
    FLAGNAME(EM_386),
    FLAGNAME(EM_68K),
    FLAGNAME(EM_88K),
    FLAGNAME(EM_860),
    FLAGNAME(EM_MIPS),
    FLAGNAME(EM_S370),
    FLAGNAME(EM_MIPS_RS3_LE),
    FLAGNAME(EM_PARISC),
    FLAGNAME(EM_VPP500),
    FLAGNAME(EM_SPARC32PLUS),
    FLAGNAME(EM_960),
    FLAGNAME(EM_PPC),
    FLAGNAME(EM_PPC64),
    FLAGNAME(EM_S390),
    FLAGNAME(EM_V800),
    FLAGNAME(EM_FR20),
    FLAGNAME(EM_RH32),
    FLAGNAME(EM_RCE),
    FLAGNAME(EM_ARM),
    FLAGNAME(EM_FAKE_ALPHA),
    FLAGNAME(EM_SH),
    FLAGNAME(EM_SPARCV9),
    FLAGNAME(EM_TRICORE),
    FLAGNAME(EM_ARC),
    FLAGNAME(EM_H8_300),
    FLAGNAME(EM_H8_300H),
    FLAGNAME(EM_H8S),
    FLAGNAME(EM_H8_500),
    FLAGNAME(EM_IA_64),
    FLAGNAME(EM_MIPS_X),
    FLAGNAME(EM_COLDFIRE),
    FLAGNAME(EM_68HC12),
    FLAGNAME(EM_MMA),
    FLAGNAME(EM_PCP),
    FLAGNAME(EM_NCPU),
    FLAGNAME(EM_NDR1),
    FLAGNAME(EM_STARCORE),
    FLAGNAME(EM_ME16),
    FLAGNAME(EM_ST100),
    FLAGNAME(EM_TINYJ),
    FLAGNAME(EM_X86_64),
    FLAGNAME(EM_PDSP),
    FLAGNAME(EM_FX66),
    FLAGNAME(EM_ST9PLUS),
    FLAGNAME(EM_ST7),
    FLAGNAME(EM_68HC16),
    FLAGNAME(EM_68HC11),
    FLAGNAME(EM_68HC08),
    FLAGNAME(EM_68HC05),
    FLAGNAME(EM_SVX),
    FLAGNAME(EM_ST19),
    FLAGNAME(EM_VAX),
    FLAGNAME(EM_CRIS),
    FLAGNAME(EM_JAVELIN),
    FLAGNAME(EM_FIREPATH),
    FLAGNAME(EM_ZSP),
    FLAGNAME(EM_MMIX),
    FLAGNAME(EM_HUANY),
    FLAGNAME(EM_PRISM),
    FLAGNAME(EM_AVR),
    FLAGNAME(EM_FR30),
    FLAGNAME(EM_D10V),
    FLAGNAME(EM_D30V),
    FLAGNAME(EM_V850),
    FLAGNAME(EM_M32R),
    FLAGNAME(EM_MN10300),
    FLAGNAME(EM_MN10200),
    FLAGNAME(EM_PJ),
    FLAGNAME(EM_OPENRISC),
    FLAGNAME(EM_ARC_A5),
    FLAGNAME(EM_XTENSA),
    {0,0}
};

struct flagnames_t elf_sh_type[] = {
    FLAGNAME(SHT_NULL),
    FLAGNAME(SHT_PROGBITS),
    FLAGNAME(SHT_SYMTAB),
    FLAGNAME(SHT_STRTAB),
    FLAGNAME(SHT_RELA),
    FLAGNAME(SHT_HASH),
    FLAGNAME(SHT_DYNAMIC),
    FLAGNAME(SHT_NOTE),
    FLAGNAME(SHT_NOBITS),
    FLAGNAME(SHT_REL),
    FLAGNAME(SHT_SHLIB),
    FLAGNAME(SHT_DYNSYM),
    FLAGNAME(SHT_INIT_ARRAY),
    FLAGNAME(SHT_FINI_ARRAY),
    FLAGNAME(SHT_PREINIT_ARRAY),
    FLAGNAME(SHT_GROUP),
    FLAGNAME(SHT_SYMTAB_SHNDX),
    FLAGNAME(SHT_NUM),
    FLAGNAME(SHT_LOOS),
    FLAGNAME(SHT_GNU_ATTRIBUTES),
    FLAGNAME(SHT_GNU_HASH),
    FLAGNAME(SHT_GNU_LIBLIST),
    FLAGNAME(SHT_CHECKSUM),
    FLAGNAME(SHT_LOSUNW),
    FLAGNAME(SHT_SUNW_COMDAT),
    FLAGNAME(SHT_SUNW_syminfo),
    FLAGNAME(SHT_GNU_verdef),
    FLAGNAME(SHT_GNU_verneed),
    FLAGNAME(SHT_GNU_versym),
    FLAGNAME(SHT_LOPROC),
    FLAGNAME(SHT_HIPROC),
    FLAGNAME(SHT_LOUSER),
    FLAGNAME(SHT_HIUSER),
    {0,0}
};

static void decode_flags (std::stringstream &xml,
                          const std::string &sectionName,
                          const struct flagnames_t flagnames[],
                          const uint32_t flags)
{
    xml << "<" << sectionName << ">";
    for (size_t i = 0; flagnames[i].name; i++) {
        if (flags & flagnames[i].flag) xml << "<" << flagnames[i].name << " />";
    }
    xml << "</" << sectionName << ">";
}

static std::string match_switch_case (const struct flagnames_t flagnames[],
                                      const uint32_t needle)
{
    for (int i = 0; flagnames[i].name; i++) {
        if (needle == flagnames[i].flag) return flagnames[i].name;
    }
    return std::string("");
}

std::string getAsciiString (const sbuf_t & sbuf, size_t offset)
{
    std::string str;
    
    while (sbuf[offset] != '\0') {
        if (sbuf[offset] & 0x80) return "";
        str += sbuf[offset++];
    }
    
    return str;
}


std::string scan_elf32_dynamic_needed (const sbuf_t & data, const Elf32_Shdr * shdr)
{
    const int32_t DT_STRTAB = 5;
    const int32_t DT_NEEDED = 1;
    
    const Elf32_Ehdr * ehdr = data.get_struct_ptr<Elf32_Ehdr>(0);
    
    // we must first find the string table for the dynamics
    uint32_t dyn_strtab_addr = 0;
    
    for (uint32_t i = 0; i < shdr->sh_size / shdr->sh_entsize; i++) {
        size_t dyn_offset = shdr->sh_offset + (shdr->sh_entsize * i);
        const Elf32_Dyn * dyn = data.get_struct_ptr<Elf32_Dyn>(dyn_offset);
        if (dyn == 0) break;
        
        if (dyn->d_tag == DT_STRTAB) {
            dyn_strtab_addr = dyn->d_un.d_ptr;
            break;
        }   
    }
    
    if (dyn_strtab_addr == 0) return "";
    
    const Elf32_Shdr * dyn_strtab = 0;
    // we get an address, so we now must find the section which falls under this address
    for (int i = 0; i < ehdr->e_shnum; i++) {
        size_t shdr_tmp_offset = ehdr->e_shoff + (ehdr->e_shentsize * i);
        const Elf32_Shdr *shdr_tmp = data.get_struct_ptr<Elf32_Shdr>(shdr_tmp_offset);
        if (shdr_tmp == 0) break;
        
        if ((shdr_tmp->sh_addr <= dyn_strtab_addr) && (shdr_tmp->sh_addr + shdr_tmp->sh_size > dyn_strtab_addr)) {
            dyn_strtab = shdr_tmp;
            break;
        }
    }
    
    if (dyn_strtab == 0) return "";
    
    // we can now loop through the dynamic entries, pull out the entires corresponding to
    // required shared objects, and add their string table entries
    std::string shared_objects;
    for (uint32_t i = 0; i < shdr->sh_size / shdr->sh_entsize; i++) {
        size_t dyn_offset = shdr->sh_offset + (shdr->sh_entsize * i);
        const Elf32_Dyn * dyn = data.get_struct_ptr<Elf32_Dyn>(dyn_offset);
        if (dyn == 0) break;
        
        if (dyn->d_tag == DT_NEEDED) {
            shared_objects += "<so>"
                              + getAsciiString(data, dyn_strtab->sh_offset + dyn->d_un.d_val)
                              + "</so>";
        }
            
    }

    return shared_objects;
}


std::string scan_elf64_dynamic_needed (const sbuf_t & data, const Elf64_Shdr * shdr)
{
    const int32_t DT_STRTAB = 5;
    const int32_t DT_NEEDED = 1;
    
    
    // we must first find the string table for the dynamics
    uint32_t dyn_strtab_addr = 0;
    
    for (uint32_t i = 0; i < shdr->sh_size / shdr->sh_entsize; i++) {
        size_t dyn_offset = shdr->sh_offset + (shdr->sh_entsize * i);
        const Elf64_Dyn * dyn = data.get_struct_ptr<Elf64_Dyn>(dyn_offset);
        if (dyn == 0) break;
        
        if (dyn->d_tag == DT_STRTAB) {
            dyn_strtab_addr = dyn->d_un.d_ptr;
            break;
        }   
    }
    
    if (dyn_strtab_addr == 0) return "";
    
    const Elf64_Ehdr * ehdr = data.get_struct_ptr<Elf64_Ehdr>(0);
    const Elf64_Shdr * dyn_strtab = 0;
    // we get an address, so we now must find the section which falls under this address
    for (int i = 0; i < ehdr->e_shnum; i++) {
        size_t shdr_tmp_offset = ehdr->e_shoff + (ehdr->e_shentsize * i);
        const Elf64_Shdr *shdr_tmp = data.get_struct_ptr<Elf64_Shdr>(shdr_tmp_offset);
        if (shdr_tmp == 0) break;
        
        if ((shdr_tmp->sh_addr <= dyn_strtab_addr) && (shdr_tmp->sh_addr + shdr_tmp->sh_size > dyn_strtab_addr)) {
            dyn_strtab = shdr_tmp;
            break;
        }
    }
    
    if (dyn_strtab == 0) return "";
    
    // we can now loop through the dynamic entries, pull out the entires corresponding to
    // required shared objects, and add their string table entries
    std::string shared_objects;
    for (uint32_t i = 0; i < shdr->sh_size / shdr->sh_entsize; i++) {
        size_t dyn_offset = shdr->sh_offset + (shdr->sh_entsize * i);
        const Elf64_Dyn * dyn = data.get_struct_ptr<Elf64_Dyn>(dyn_offset);
        if (dyn == 0) break;
        
        if (dyn->d_tag == DT_NEEDED) {
            shared_objects += "<so>"
                              + getAsciiString(data, dyn_strtab->sh_offset + dyn->d_un.d_val)
                              + "</so>";
        }
            
    }

    return shared_objects;
}


// function begins with 32 bits of confidence
// So look for excuses to throw out the ELF
std::string scan_elf_verify (const sbuf_t & data)
{
    std::stringstream xml;        
    std::stringstream so_xml;		// collect shared object names
    u_int sht_null_count=0;
    
    
    xml << "<ELF";
    
    if      (data[EI_CLASS] == ELFCLASS32) xml << " class=\"ELFCLASS32\"";
    else if (data[EI_CLASS] == ELFCLASS64) xml << " class=\"ELFCLASS64\"";
    else return "";
    
    if      (data[EI_DATA] == ELFDATA2LSB) xml << " data=\"ELFDATA2LSB\"";
    else if (data[EI_DATA] == ELFDATA2MSB) xml << " data=\"ELFDATA2MSB\"";
    else return "";
    
    xml << " osabi=\"" << match_switch_case(elf_e_osabi, data[EI_OSABI]) << "\"";
    xml << " abiversion=\"" << (int) data[EI_ABIVERSION] << "\" >";
        
    if (data[EI_CLASS] == ELFCLASS32) {
        const Elf32_Ehdr * ehdr = data.get_struct_ptr<Elf32_Ehdr>(0);
        
        if (ehdr == 0) return "";
        if (ehdr->e_phentsize & 1) return "";
        if (ehdr->e_shentsize & 1) return "";
        if (ehdr->e_shstrndx > ehdr->e_shnum) return "";
        
        std::string type = match_switch_case(elf_e_type, ehdr->e_type);
        if (type == "") return "";
        
        std::string machine = match_switch_case(elf_e_machine, ehdr->e_machine);
        if (machine == "") return "";
        
	// Sanity checks
	if(ehdr->e_ehsize < sizeof(*ehdr)) return ""; // header is smaller than the header?


        xml << "<ehdr";
        
        xml << " type=\""      << type              << "\"";
        xml << " machine=\""   << machine           << "\"";
        xml << " version=\""   << ehdr->e_version   << "\"";
        xml << " entry=\""     << ehdr->e_entry     << "\"";
        xml << " phoff=\""     << ehdr->e_phoff     << "\"";
        xml << " shoff=\""     << ehdr->e_shoff     << "\"";
        xml << " flags=\""     << ehdr->e_flags     << "\"";
        xml << " ehsize=\""    << ehdr->e_ehsize    << "\"";
        xml << " phentsize=\"" << ehdr->e_phentsize << "\"";
        xml << " phnum=\""     << ehdr->e_phnum     << "\"";
        xml << " shentsize=\"" << ehdr->e_shentsize << "\"";
        xml << " shnum=\""     << ehdr->e_shnum     << "\"";
        xml << " shstrndx=\""  << ehdr->e_shstrndx  << "\"";
        xml << " />";
        
        xml << "<sections>";
        
        size_t shstr_offset = ehdr->e_shoff + (ehdr->e_shentsize * ehdr->e_shstrndx);
        const Elf32_Shdr * shstr = data.get_struct_ptr<Elf32_Shdr>(shstr_offset);
        
        for (int si = 0; si < ehdr->e_shnum; si++) {
            size_t shdr_offset = ehdr->e_shoff + (ehdr->e_shentsize * si);
            const Elf32_Shdr * shdr = data.get_struct_ptr<Elf32_Shdr>(shdr_offset);
            if (shdr == 0) break;
            
	    /* Sanity check */
	    if (shdr->sh_type == SHT_NULL && ++sht_null_count > sht_null_counter_max) return "";

            xml << "<section";
            
            if (shdr->sh_type == SHT_DYNAMIC) {
                // well-formed elf binaries will have entsize set to SOMETHING
                // (probably 8)

		if (shdr->sh_size == 0) return "";
                if (shdr->sh_entsize == 0) return "";
                so_xml << scan_elf32_dynamic_needed(data, shdr);
            }
            
            if (shstr != 0){
		std::string name = getAsciiString(data, shstr->sh_offset + shdr->sh_name);
		if(name.size()>slt_max_name_size) return ""; // sanity check
		xml << " name=\"" << name << "\"";
	    }
            xml << " type=\""      << match_switch_case(elf_sh_type, shdr->sh_type) << "\"";
            xml << " addr=\"0x"    << std::hex << shdr->sh_addr << "\"";
            xml << " offset=\""    << shdr->sh_offset           << "\"";
            xml << " size=\""      << shdr->sh_size             << "\"";
            xml << " link=\""      << shdr->sh_link             << "\"";
            xml << " info=\""      << shdr->sh_info             << "\"";
            xml << " addralign=\"" << shdr->sh_addralign        << "\"";
            xml << " shentsize=\"" << shdr->sh_entsize          << "\"";
            xml << ">";
            
            decode_flags(xml, "flags", elf_sh_flags, shdr->sh_flags);
            
            xml << "</section>";
        }
        xml << "</sections>";
        
    }
    else if (data[EI_CLASS] == ELFCLASS64) {
        const Elf64_Ehdr * ehdr = data.get_struct_ptr<Elf64_Ehdr>(0);
        
        if (ehdr == 0) return "";
        if (ehdr->e_phentsize & 1) return "";
        if (ehdr->e_shentsize & 1) return "";
        if (ehdr->e_shstrndx > ehdr->e_shnum) return "";
        
        std::string type = match_switch_case(elf_e_type, ehdr->e_type);
        if (type == "") return "";
        
        std::string machine = match_switch_case(elf_e_machine, ehdr->e_machine);
        if (machine == "") return "";
        
	// Sanity checks
	if(ehdr->e_ehsize < sizeof(*ehdr)) return ""; // header is smaller than the header?


        xml << "<ehdr";
        
        xml << " type=\""      << type              << "\"";
        xml << " machine=\""   << machine           << "\"";
        xml << " version=\""   << ehdr->e_version   << "\"";
        xml << " entry=\""     << ehdr->e_entry     << "\"";
        xml << " phoff=\""     << ehdr->e_phoff     << "\"";
        xml << " shoff=\""     << ehdr->e_shoff     << "\"";
        xml << " flags=\""     << ehdr->e_flags     << "\"";
        xml << " ehsize=\""    << ehdr->e_ehsize    << "\"";
        xml << " phentsize=\"" << ehdr->e_phentsize << "\"";
        xml << " phnum=\""     << ehdr->e_phnum     << "\"";
        xml << " shentsize=\"" << ehdr->e_shentsize << "\"";
        xml << " shnum=\""     << ehdr->e_shnum     << "\"";
        xml << " shstrndx=\""  << ehdr->e_shstrndx  << "\"";
        xml << " />";
        
        xml << "<sections>";
        
        size_t shstr_offset = ehdr->e_shoff + (ehdr->e_shentsize * ehdr->e_shstrndx);
        const Elf64_Shdr * shstr = data.get_struct_ptr<Elf64_Shdr>(shstr_offset);
        
        for (int si = 0; si < ehdr->e_shnum; si++) {
            size_t shdr_offset = ehdr->e_shoff + (ehdr->e_shentsize * si);
            const Elf64_Shdr * shdr = data.get_struct_ptr<Elf64_Shdr>(shdr_offset);
            if (shdr == 0) break;
            
	    /* Sanity check */
	    if (shdr->sh_type == SHT_NULL && ++sht_null_count > sht_null_counter_max) return "";

            xml << "<section";
            

            if (shdr->sh_type == SHT_DYNAMIC) {
                // well-formed elf binaries will have entsize set to SOMETHING
                // (probably 8)
		if (shdr->sh_size == 0) return "";
                if (shdr->sh_entsize == 0) return "";
                so_xml << scan_elf64_dynamic_needed(data, shdr);
            }
            
            if (shstr != 0){
		std::string name = getAsciiString(data, shstr->sh_offset + shdr->sh_name);
		if(name.size()>slt_max_name_size) return ""; // sanity check
		xml << " name=\"" << name << "\"";
	    }
            xml << " type=\""      << match_switch_case(elf_sh_type, shdr->sh_type) << "\"";
            xml << " addr=\"0x"    << std::hex << shdr->sh_addr << "\"";
            xml << " offset=\""    << shdr->sh_offset           << "\"";
            xml << " size=\""      << shdr->sh_size             << "\"";
            xml << " link=\""      << shdr->sh_link             << "\"";
            xml << " info=\""      << shdr->sh_info             << "\"";
            xml << " addralign=\"" << shdr->sh_addralign        << "\"";
            xml << " shentsize=\"" << shdr->sh_entsize          << "\"";
            xml << ">";
            
            decode_flags(xml, "flags", elf_sh_flags, shdr->sh_flags);
            
            xml << "</section>";
        }
        xml << "</sections>";
    }
    
    xml << "<shared_objects>" << so_xml.str() << "</shared_objects>";
    xml << "</ELF>";
    return xml.str();
}

extern "C"
void scan_elf (const class scanner_params          &sp,
               const       recursion_control_block &rcb)
{
    assert(sp.sp_version == scanner_params::CURRENT_SP_VERSION);
    std::string xml;
    
    if (sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name   = "elf";
	sp.info->author = "Alex Eubanks";
        sp.info->feature_names.insert("elf");
        return;
    }
    if (sp.phase==scanner_params::PHASE_SCAN){

	feature_recorder *f = sp.fs.get_name("elf");
    
	for (size_t pos = 0; pos < sp.sbuf.bufsize; pos++) {
	    // Look for the magic number
	    // If we find it, make an sbuf and analyze...
	    if ( (sp.sbuf[pos+0] == 0x7f)
		 && (sp.sbuf[pos+1] == 'E')
		 && (sp.sbuf[pos+2] == 'L')
		 && (sp.sbuf[pos+3] == 'F')) {

		const sbuf_t data(sp.sbuf + pos);
		xml = scan_elf_verify(data);
		if (xml != "") {
                    sbuf_t hdata(data,0,4096);
                    std::string hexhash = f->fs.hasher.func(hdata.buf, hdata.bufsize);
		    f->write(data.pos0, hexhash,xml);
		}
	    }
	}    
    }
}
