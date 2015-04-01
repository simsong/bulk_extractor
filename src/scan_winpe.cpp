/**
 * Alex Eubanks, June 2012 - endeavor at rainbowsandpwnies.com
 * The structures used for this code come from
 * https://github.com/endeav0r/Rainbows-And-Pwnies-Tools .
 * 
 * Originally (C) 2011 Alex Eubanks.
 * Released into the public domain on June 7, 2012.
 *
 * The names and values used in XML may seem large/redundant, but they
 * mirror those found in the PE specification given by Microsoft.
 *
 * For a description of the meanings of the field names below, please see
 * "Microsoft PE and COFF Specification," 
 * http://msdn.microsoft.com/en-us/library/windows/hardware/gg463119.aspx

 * Revision history:
 * 2015-april   bda - Removed requirement that pe_NumberOfRvaAndSizes, hardcoded
                      to 16, be defined, since some data was found where it was
                      0, resulting in failure to extract valid dll filenames.
 * 2015-april   bda - added PE carving.
 */
 
/**
 * XML_SPEC
 PE
 FileHeader
 @Machine =>   "IMAGE_FILE_MACHINE_AMD64"
 | "IMAGE_FILE_MACHINE_ARM"
 | "IMAGE_FILE_MACHINE_ARMV7"
 | "IMAGE_FILE_MACHINE_I386"
 | "IMAGE_FILE_MACHINE_IA64"
 @NumberOfSections     => %d
 @TimeDateStamp        => %d
 @TimeDateStampISO     => %s
 @PointToSymbolTable   => %d
 @NumberOfSymbols      => %d
 @SizeOfOptionalHeader => %d
 Characteristics
 IMAGE_FILE_RELOCS_STRIPPED  // these tags are present if
 ...                         // bits were set in
 IMAGE_FILE_BYTES_REVERSE_HI // Characteristics
 OptionalHeaderStandard // Same name and attributes for both PE
 *                      // and PE+ to simplify parser writing
 @Magic => "PE32" | "PE32+"
 @MajorLinkerVersion    => %d
 @MinorLinkerVersion    => %d
 @SizeOfCode            => %d
 @SizeOfInitializedData => %d
 @AddressOfEntryPoint   => 0x%x
 @BaseOfCode            => 0x%x
 OptionalHeaderWindows // Same name and attributes for both PE
 *                     // and PE+ to simplify parser writing
 @ImageBase        => 0x%x
 @SectionAlignment => %d
 @FileAlignment    => %d
 @MajorOperatingSystemVersion => %d
 @MinorOperatingSystemVersion => %d
 @MajorImageVersion     => %d
 @MinorImageVersion     => %d
 @MajorSubsystemVersion => %d
 @MinorSubsystemVersion => %d
 @Win32VersionValue     => %d
 @SizeOfImage           => %d
 @SizeOfHeaders         => %d
 @CheckSum              => 0x%x
 @Subsystem =>    IMAGE_SUBSYSTEM_UNKNOWN
 ..
 IMAGE_SUBSYSTEM_XBOX
 @SizeOfStackReserve  => %d
 @SizeOfStackCommit   => %d
 @SizeOfHeapReserve   => %d
 @SizeOfHeadCommit    => %d
 @LoaderFlags         => %d // reserved, should always be
 *                          // zero
 @NumberOfRvaAndSizes => %d
 DllCharacteristics
 IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE // tags present
 ..                                     // if bits set
 IMAGE_DLL_CHARACTERISTICS_TERMINAL_SERVER_AWARE
 sections
 SectionHeader
 @Name                 => %s
 @VirtualSize          => %d
 @VirtualAddress       => %d
 @SizeOfRawData        => %d
 @PointerToRawData     => %d
 @PointerToRelocations => %d
 @PointerToLinenumbers => %d
 @NumberOfRelocations  => %d
 @NumberOfLinenumbers  => %d
 Characteristics
 IMAGE_SCN_TYPE_REG  // tags present if bits set
 ..
 IMAGE_SCN_MEM_WRITE
 dlls
 dll => %s
 // <dlls><dll>msvcrt.dll</dll><dll>ntdll.dll</dll></dlls>
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"

#include <stdio.h>

#define PE_SIGNATURE         "PE\0\0"
#define PE_SIGNATURE_SIZE    4

enum E {
    PE_FILE_OFFSET = 0x3c,
};

#ifndef IMAGE_FILE_MACHINE_AM33
#define IMAGE_FILE_MACHINE_UNKNOWN   0x0
#define IMAGE_FILE_MACHINE_AM33      0x1d3
#define IMAGE_FILE_MACHINE_ALPHA     0x184
#define IMAGE_FILE_MACHINE_AMD64     0x8664
#define IMAGE_FILE_MACHINE_ARM       0x1c0
#define IMAGE_FILE_MACHINE_ARMV7     0x1c4
#define IMAGE_FILE_MACHINE_ALPHA64   0x284
#define IMAGE_FILE_MACHINE_I386      0x14c
#define IMAGE_FILE_MACHINE_IA64      0x200
#define IMAGE_FILE_MACHINE_M68K      0x268
#define IMAGE_FILE_MACHINE_MIPS16    0x266
#define IMAGE_FILE_MACHINE_MIPSFPU   0x366
#define IMAGE_FILE_MACHINE_MIPSFPU16 0x466
#define IMAGE_FILE_MACHINE_POWERPC   0x1f0
#define IMAGE_FILE_MACHINE_R3000     0x162
#define IMAGE_FILE_MACHINE_R4000     0x166
#define IMAGE_FILE_MACHINE_R10000    0x168
#define IMAGE_FILE_MACHINE_SH3       0x1a2
#define IMAGE_FILE_MACHINE_SH4       0x1a6
#define IMAGE_FILE_MACHINE_THUMB     0x1c2
#endif

#define IMAGE_FILE_RELOCS_STRIPPED         0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE        0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED      0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED     0x0008
#define IMAGE_FILE_AGRESSIVE_WS_TRIM       0x0010
#define IMAGE_FILE_LARGE_ADDRESS_AWARE     0x0020
#define IMAGE_FILE_16BIT_MACHINE           0x0040
#define IMAGE_FILE_BYTES_REVERSED_LO       0x0080
#define IMAGE_FILE_32BIT_MACHINE           0x0100
#define IMAGE_FILE_DEBUG_STRIPPED          0x0200
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP 0x0400
#define IMAGE_FILE_SYSTEM                  0x1000
#define IMAGE_FILE_DLL                     0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY          0x4000
#define IMAGE_FILE_BYTES_REVERSE_HI        0x8000

#define IMAGE_SUBSYSTEM_UNKNOWN                 0
#define IMAGE_SUBSYSTEM_NATIVE                  1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI             2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI             3
#define IMAGE_SUBSYSTEM_POSIX_CUI               7
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI          9
#define IMAGE_SUBSYSTEM_EFI_APPLICATION         10
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER 11
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER      12
#define IMAGE_SUBSYSTEM_EFI_ROM                 13
#define IMAGE_SUBSYSTEM_XBOX                    14

#define IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE          0x0040
#define IMAGE_DLL_CHARACTERISTICS_FORCE_INTEGRITY       0x0080
#define IMAGE_DLL_CHARACTERISTICS_NX_COMPAT             0x0100
#define IMAGE_DLL_CHARACTERISTICS_NO_ISOLATION          0x0200
#define IMAGE_DLL_CHARACTERISTICS_NO_SEH                0x0400
#define IMAGE_DLL_CHARACTERISTICS_NO_BIND               0x0800
#define IMAGE_DLL_CHARACTERISTICS_WDM_DRIVER            0x2000
#define IMAGE_DLL_CHARACTERISTICS_TERMINAL_SERVER_AWARE 0x8000

#define IMAGE_SCN_TYPE_REG               0x00000000
#define IMAGE_SCN_TYPE_DSECT             0x00000001
#define IMAGE_SCN_TYPE_NOLOAD            0x00000002
#define IMAGE_SCN_TYPE_GROUP             0x00000004
#define IMAGE_SCN_TYPE_NO_PAD            0x00000008
#define IMAGE_SCN_TYPE_COPY              0x00000010
#define IMAGE_SCN_CNT_CODE               0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_LNK_OTHER              0x00000100
#define IMAGE_SCN_LNK_INFO               0x00000200
#define IMAGE_SCN_TYPE_OVER              0x00000400
#define IMAGE_SCN_LNK_REMOVE             0x00000800
#define IMAGE_SCN_LNK_COMDAT             0x00001000
#define IMAGE_SCN_MEM_FARDATA            0x00008000
#define IMAGE_SCN_MEM_PURGEABLE          0x00020000
#define IMAGE_SCN_MEM_16BIT              0x00020000
#define IMAGE_SCN_MEM_LOCKED             0x00040000
#define IMAGE_SCN_MEM_PRELOAD            0x00080000
#define IMAGE_SCN_ALIGN_1BYTES           0x00100000
#define IMAGE_SCN_ALIGN_2BYTES           0x00200000
#define IMAGE_SCN_ALIGN_4BYTES           0x00300000
#define IMAGE_SCN_ALIGN_8BYTES           0x00400000
#define IMAGE_SCN_ALIGN_16BYTES          0x00500000
#define IMAGE_SCN_ALIGN_32BYTES          0x00600000
#define IMAGE_SCN_ALIGN_64BYTES          0x00700000
#define IMAGE_SCN_ALIGN_128BYTES         0x00800000
#define IMAGE_SCN_ALIGN_256BYTES         0x00900000
#define IMAGE_SCN_ALIGN_512BYTES         0x00A00000
#define IMAGE_SCN_ALIGN_1024BYTES        0x00B00000
#define IMAGE_SCN_ALIGN_2048BYTES        0x00C00000
#define IMAGE_SCN_ALIGN_4096BYTES        0x00D00000
#define IMAGE_SCN_ALIGN_8192BYTES        0x00E00000
#define IMAGE_SCN_LNK_NRELOC_OVFL        0x01000000
#define IMAGE_SCN_MEM_DISCARDABLE        0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED         0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED          0x08000000
#define IMAGE_SCN_MEM_SHARED             0x10000000
#define IMAGE_SCN_MEM_EXECUTE            0x20000000
#define IMAGE_SCN_MEM_READ               0x40000000
#define IMAGE_SCN_MEM_WRITE              0x80000000

#define IMAGE_FILE_TYPE_PE32     0x10b
#define IMAGE_FILE_TYPE_PE32PLUS 0x20b

#define PE_SECTION_RESERVED_FLAGS 0x00000417

/* For information on __attribute__((packed)), see:
 * http://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Type-Attributes.html
 */

// tunable parameter
static uint32_t winpe_carve_mode = feature_recorder::CARVE_ENCODED;

typedef struct _Pe_Fileheader {
    uint16_t Machine              __attribute__((packed));
    uint16_t NumberOfSections     __attribute__((packed));
    uint32_t TimeDateStamp        __attribute__((packed));
    uint32_t PointerToSymbolTable __attribute__((packed));
    uint32_t NumberOfSymbols      __attribute__((packed));
    uint16_t SizeOfOptionalHeader __attribute__((packed));
    uint16_t Characteristics      __attribute__((packed));
} __attribute__((packed)) Pe_FileHeader;

typedef struct _Pe_OptionalHeaderStandard {
    uint16_t Magic;                   // 2
    uint8_t  MajorLinkerVersion;      // 3
    uint8_t  MinorLinkerVersion;      // 4
    uint32_t SizeOfCode;              // 8
    uint32_t SizeOfInitializedData;   // 12
    uint32_t SizeOfUninitializedData; // 16
    uint32_t AddressOfEntryPoint;     // 20
    uint32_t BaseOfCode;              // 24
    uint32_t BaseOfData;              // 28
} __attribute__((packed)) Pe_OptionalHeaderStandard;

typedef struct _Pe_OptionalHeaderStandardPlus {
    uint16_t Magic;                   // 2
    uint8_t  MajorLinkerVersion;      // 3
    uint8_t  MinorLinkerVersion;      // 4
    uint32_t SizeOfCode;              // 8
    uint32_t SizeOfInitializedData;   // 12
    uint32_t SizeOfUninitializedData; // 16
    uint32_t AddressOfEntryPoint;     // 20
    uint32_t BaseOfCode;              // 24
} __attribute__((packed)) Pe_OptionalHeaderStandardPlus;

typedef struct _Pe_OptionalHeaderWindows {
    uint32_t ImageBase;                   // 32
    uint32_t SectionAlignment;            // 36
    uint32_t FileAlignment;               // 40
    uint16_t MajorOperatingSystemVersion; // 42
    uint16_t MinorOperatingSystemVersion; // 44
    uint16_t MajorImageVersion;           // 46
    uint16_t MinorImageVersion;           // 48
    uint16_t MajorSubsystemVersion;       // 50
    uint16_t MinorSubsystemVersion;       // 52
    uint32_t Win32VersionValue;           // 56
    uint32_t SizeOfImage;                 // 60
    uint32_t SizeOfHeaders;               // 64
    uint32_t CheckSum;                    // 68
    uint16_t Subsystem;                   // 70
    uint16_t DllCharacteristics;          // 72
    uint32_t SizeOfStackReserve;          // 76
    uint32_t SizeOfStackCommit;           // 80
    uint32_t SizeOfHeapReserve;           // 84
    uint32_t SizeOfHeapCommit;            // 88
    uint32_t LoaderFlags;                 // 92
    uint32_t NumberOfRvaAndSizes;         // 96
} __attribute__((packed)) Pe_OptionalHeaderWindows;

typedef struct _Pe_OptionalHeaderWindowsPlus {
    uint64_t ImageBase;                   // 32
    uint32_t SectionAlignment;            // 36
    uint32_t FileAlignment;               // 40
    uint16_t MajorOperatingSystemVersion; // 42
    uint16_t MinorOperatingSystemVersion; // 44
    uint16_t MajorImageVersion;           // 46
    uint16_t MinorImageVersion;           // 48
    uint16_t MajorSubsystemVersion;       // 50
    uint16_t MinorSubsystemVersion;       // 52
    uint32_t Win32VersionValue;           // 56
    uint32_t SizeOfImage;                 // 60
    uint32_t SizeOfHeaders;               // 64
    uint32_t CheckSum;                    // 68
    uint16_t Subsystem;                   // 70
    uint16_t DllCharacteristics;          // 72
    uint64_t SizeOfStackReserve;          // 80
    uint64_t SizeOfStackCommit;           // 88
    uint64_t SizeOfHeapReserve;           // 96
    uint64_t SizeOfHeapCommit;            // 104
    uint32_t LoaderFlags;                 // 108
    uint32_t NumberOfRvaAndSizes;         // 112
} __attribute__((packed)) Pe_OptionalHeaderWindowsPlus;

typedef struct _Pe_SectionHeader {
    char Name[8];                  // 8
    uint32_t VirtualSize;          // 12
    uint32_t VirtualAddress;       // 16
    uint32_t SizeOfRawData;        // 20
    uint32_t PointerToRawData;     // 24
    uint32_t PointerToRelocations; // 28
    uint32_t PointerToLinenumbers; // 32
    uint16_t NumberOfRelocations;  // 34
    uint16_t NumberOfLinenumbers;  // 36
    uint32_t Characteristics;      // 40
} __attribute__((packed)) Pe_SectionHeader;

typedef struct _Pe_ImportDirectoryTable {
    uint32_t ImportLookupTableRVA;
    uint32_t TimeDateStamp;
    uint32_t ForwarderChain;
    uint32_t NameRVA;
    uint32_t ImportAddressTableRVA;
} Pe_ImportDirectoryTable;

std::ostream & operator << (std::ostream &os,const Pe_ImportDirectoryTable &idt){
    os << " ImportLookupTableRVA=" << idt.ImportLookupTableRVA 
       << " TimeDateStamp=" << idt.TimeDateStamp
       << " ForwarderChain=" << idt.ForwarderChain 
       << " NameRVA=" << idt.NameRVA
       << " ImportAddressTableRVA=" << idt.ImportAddressTableRVA;
    return os;
};

typedef struct _Pe_DataDirectory {
    uint32_t VirtualAddress;
    uint32_t Size;
} Pe_DataDirectory;


/** Machinery to turn bitfields into XML */
#define FLAGNAME(STR) {STR,#STR}
struct  flagnames_t {
    uint32_t flag;
    const char *name;
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

// this is going to return a string because it returns empty string if
// the value was not found
static std::string match_switch_case (const struct flagnames_t flagnames[],
                                 const uint32_t needle)
{
    int i;
    for (i = 0; flagnames[i].flag; i++) {
        if (needle == flagnames[i].flag) return flagnames[i].name;
    }
    return "";
}

// Breaks out PE Section Characteristics into xml. This number of these
// make section header parsing code ugly, so we break it out

struct flagnames_t pe_section_characteristic_names[] = {
    FLAGNAME(IMAGE_SCN_TYPE_REG),
    FLAGNAME(IMAGE_SCN_TYPE_DSECT),
    FLAGNAME(IMAGE_SCN_TYPE_NOLOAD),
    FLAGNAME(IMAGE_SCN_TYPE_GROUP),
    FLAGNAME(IMAGE_SCN_TYPE_NO_PAD),
    FLAGNAME(IMAGE_SCN_TYPE_COPY),
    FLAGNAME(IMAGE_SCN_CNT_CODE),
    FLAGNAME(IMAGE_SCN_CNT_INITIALIZED_DATA),
    FLAGNAME(IMAGE_SCN_CNT_UNINITIALIZED_DATA),
    FLAGNAME(IMAGE_SCN_LNK_OTHER),
    FLAGNAME(IMAGE_SCN_LNK_INFO),
    FLAGNAME(IMAGE_SCN_TYPE_OVER),
    FLAGNAME(IMAGE_SCN_LNK_REMOVE),
    FLAGNAME(IMAGE_SCN_LNK_COMDAT),
    FLAGNAME(IMAGE_SCN_MEM_FARDATA),
    FLAGNAME(IMAGE_SCN_MEM_PURGEABLE),
    FLAGNAME(IMAGE_SCN_MEM_16BIT),
    FLAGNAME(IMAGE_SCN_MEM_LOCKED),
    FLAGNAME(IMAGE_SCN_MEM_PRELOAD),
    FLAGNAME(IMAGE_SCN_ALIGN_1BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_2BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_4BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_8BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_16BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_32BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_64BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_128BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_256BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_512BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_1024BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_2048BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_4096BYTES),
    FLAGNAME(IMAGE_SCN_ALIGN_8192BYTES),
    FLAGNAME(IMAGE_SCN_LNK_NRELOC_OVFL),
    FLAGNAME(IMAGE_SCN_MEM_DISCARDABLE),
    FLAGNAME(IMAGE_SCN_MEM_NOT_CACHED),
    FLAGNAME(IMAGE_SCN_MEM_NOT_PAGED),
    FLAGNAME(IMAGE_SCN_MEM_SHARED),
    FLAGNAME(IMAGE_SCN_MEM_EXECUTE),
    FLAGNAME(IMAGE_SCN_MEM_READ),
    FLAGNAME(IMAGE_SCN_MEM_WRITE),
    {0,0}
};

struct flagnames_t pe_fileheader_characteristic_names[] = {
    FLAGNAME(IMAGE_FILE_RELOCS_STRIPPED),
    FLAGNAME(IMAGE_FILE_EXECUTABLE_IMAGE),
    FLAGNAME(IMAGE_FILE_LINE_NUMS_STRIPPED),
    FLAGNAME(IMAGE_FILE_LOCAL_SYMS_STRIPPED),
    FLAGNAME(IMAGE_FILE_AGRESSIVE_WS_TRIM), // misspelling in spec!
    FLAGNAME(IMAGE_FILE_LARGE_ADDRESS_AWARE),
    FLAGNAME(IMAGE_FILE_16BIT_MACHINE),
    FLAGNAME(IMAGE_FILE_BYTES_REVERSED_LO),
    FLAGNAME(IMAGE_FILE_32BIT_MACHINE),
    FLAGNAME(IMAGE_FILE_DEBUG_STRIPPED),
    FLAGNAME(IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP),
    FLAGNAME(IMAGE_FILE_SYSTEM),
    FLAGNAME(IMAGE_FILE_DLL),
    FLAGNAME(IMAGE_FILE_UP_SYSTEM_ONLY),
    FLAGNAME(IMAGE_FILE_BYTES_REVERSE_HI),
    {0,0}
};

struct flagnames_t pe_optionalwindowsheader_dllcharacteristic[] = {
    FLAGNAME(IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE),
    FLAGNAME(IMAGE_DLL_CHARACTERISTICS_FORCE_INTEGRITY),
    FLAGNAME(IMAGE_DLL_CHARACTERISTICS_NX_COMPAT),
    FLAGNAME(IMAGE_DLL_CHARACTERISTICS_NO_ISOLATION),
    FLAGNAME(IMAGE_DLL_CHARACTERISTICS_NO_SEH),
    FLAGNAME(IMAGE_DLL_CHARACTERISTICS_NO_BIND),
    FLAGNAME(IMAGE_DLL_CHARACTERISTICS_WDM_DRIVER),
    FLAGNAME(IMAGE_DLL_CHARACTERISTICS_TERMINAL_SERVER_AWARE),
    {0,0}
};

// There are more options available in the defines above, but we only
// check the values we expect to find in windows executables
struct flagnames_t pe_fileheader_machine[] = {
    FLAGNAME(IMAGE_FILE_MACHINE_AMD64),
    FLAGNAME(IMAGE_FILE_MACHINE_ARM),
    FLAGNAME(IMAGE_FILE_MACHINE_ARMV7),
    FLAGNAME(IMAGE_FILE_MACHINE_I386),
    FLAGNAME(IMAGE_FILE_MACHINE_IA64),
    {0,0}
};

struct flagnames_t pe_optionalwindowsheader_subsystem[] = {
    FLAGNAME(IMAGE_SUBSYSTEM_UNKNOWN),
    FLAGNAME(IMAGE_SUBSYSTEM_NATIVE),
    FLAGNAME(IMAGE_SUBSYSTEM_WINDOWS_GUI),
    FLAGNAME(IMAGE_SUBSYSTEM_WINDOWS_CUI),
    FLAGNAME(IMAGE_SUBSYSTEM_POSIX_CUI),
    FLAGNAME(IMAGE_SUBSYSTEM_WINDOWS_CE_GUI),
    FLAGNAME(IMAGE_SUBSYSTEM_EFI_APPLICATION),
    FLAGNAME(IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER),
    FLAGNAME(IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER),
    FLAGNAME(IMAGE_SUBSYSTEM_EFI_ROM),
    FLAGNAME(IMAGE_SUBSYSTEM_XBOX),
    {0,0}
};

// Takes a pointer to data which matches PE_SIGNATURE at PE_FILE_OFFSET
// Returns an empty string if this is not a PE, or a bunch of XML
// describing the PE if it is

//#define sbuf_struct(sbuf,struct_name,pos) (pos+sizeof(struct_name) <= sbuf.bufsize ? (const struct_name *)(sbuf.buf+pos) : 0)
//template<class Type>
//const Type * sbuf_struct(const sbuf_t &sbuf, uint32_t pos)
//{
//    if (pos + sizeof(Type) <= sbuf.bufsize)
//  return (const Type *) (sbuf.buf+pos);
//    return NULL;
//}

static bool valid_dll_name(const std::string &dllname)
{
    if(!validASCIIName(dllname)) return false;
    if(dllname.size()<5) return false; /* DLL names have at least a character, a period and an extension */
    if(dllname.at(dllname.size()-4)!='.') return false; // check for the '.'
    return true;			// looks valid
}

static bool valid_section_name(const std::string &sectionName)
{
    if(!validASCIIName(sectionName)) return false;
    if(sectionName.size()<1) return false;
    return true;
}

static std::string scan_winpe_verify (const sbuf_t &sbuf)
{
    //const uint8_t * data = sbuf.buf;
    size_t size          = sbuf.bufsize;

    size_t ohs_offset                          = 0;    // OptionalHeaderStandard
    size_t ohw_offset                          = 0;    // OptionalHeaderWindows
    //uint32_t     header_offset;
    int          section_i;
    std::stringstream xml;
    //int dlli;
    
    // set Pe_FileHeader to correct address
    uint32_t header_offset = sbuf.get32u(PE_FILE_OFFSET);
    
    // we adjust header_offset to account for PE_SIGNATURE_SIZE, and use
    // header_offset throughout the code to insure we don't have
    // out-of-bounds errors
    header_offset += PE_SIGNATURE_SIZE;
   
    /******************************************
     * BEGIN HEADER                            *
     ******************************************/
    if (header_offset + sizeof(Pe_FileHeader) > size)
        return "";
    
    xml << "<PE><FileHeader";
    
    uint16_t pe_Machine              = sbuf.get16u(header_offset);
    uint16_t pe_NumberOfSections     = sbuf.get16u(header_offset + 2);
    uint32_t pe_TimeDateStamp        = sbuf.get32u(header_offset + 4);
    std::string pe_TimeDateStampISO  = unixTimeToISODate(pe_TimeDateStamp);
    uint32_t pe_PointerToSymbolTable = sbuf.get32u(header_offset + 8);
    uint32_t pe_NumberOfSymbols      = sbuf.get32u(header_offset + 12);
    uint16_t pe_SizeOfOptionalHeader = sbuf.get16u(header_offset + 16);
    uint16_t pe_Characteristics      = sbuf.get16u(header_offset + 18);

    // 2^11.75 confidence, 2^20.25 to go
    std::string Machine = match_switch_case(pe_fileheader_machine, pe_Machine);
    if (Machine == "") return "";
        
    // A PE with (0|>256) sections? Doubtful
    if ((pe_NumberOfSections == 0) || (pe_NumberOfSections > 256)) return "";
    
    if (    ((pe_NumberOfSymbols == 0) && (pe_PointerToSymbolTable != 0))
	    || (pe_NumberOfSymbols > 1000000)) return "";

    if (pe_SizeOfOptionalHeader & 0x1) return "";

    xml << " Machine=\""              << Machine                  << "\"";
    xml << " NumberOfSections=\""     << pe_NumberOfSections     << "\"";
    xml << " TimeDateStamp=\""        << pe_TimeDateStamp        << "\"";
    xml << " TimeDateStampISO=\""     << pe_TimeDateStampISO     << "\"";
    xml << " PointerToSymbolTable=\"" << pe_PointerToSymbolTable << "\"";
    xml << " NumberOfSymbols=\""      << pe_NumberOfSymbols      << "\"";
    xml << " SizeOfOptionalHeader=\"" << pe_SizeOfOptionalHeader << "\"";
    xml << ">";             // close FileHeader

    decode_flags(xml,
                 "Characteristics",
                 pe_fileheader_characteristic_names,
                 pe_Characteristics);

    xml << "</FileHeader>";

    
    /******************************************
     * BEGIN OPTIONAL HEADER STANDARD          *
     ******************************************/

    // we assume an optional header exists
    uint16_t pe_Magic;
    
    xml << "<OptionalHeaderStandard";
    
    // if we're going to segfault on this dereference, we don't have
    // enough information to confirm this PE. return false
    // Note: sizeof(*ohs) == sizeof(*ohsp) + 4
    if (header_offset + sizeof(Pe_FileHeader)
        + sizeof(Pe_OptionalHeaderStandard) > size)
        return "";
    ohs_offset = header_offset + sizeof(Pe_FileHeader);
    
    pe_Magic = sbuf.get16u(ohs_offset);
    uint8_t  pe_MajorLinkerVersion;
    uint8_t  pe_MinorLinkerVersion;
    uint32_t pe_SizeOfCode;
    uint32_t pe_SizeOfInitializedData;
    uint32_t pe_SizeOfUninitializedData;
    uint32_t pe_AddressOfEntryPoint;
    uint32_t pe_BaseOfCode;
    switch (pe_Magic) {
    case IMAGE_FILE_TYPE_PE32 :
        pe_MajorLinkerVersion      = sbuf.get8u(ohs_offset + 2);
        pe_MinorLinkerVersion      = sbuf.get8u(ohs_offset + 3);
        pe_SizeOfCode              = sbuf.get32u(ohs_offset + 4);
        pe_SizeOfInitializedData   = sbuf.get32u(ohs_offset + 8);
        pe_SizeOfUninitializedData = sbuf.get32u(ohs_offset + 12);
        pe_AddressOfEntryPoint     = sbuf.get32u(ohs_offset + 16);
        pe_BaseOfCode              = sbuf.get32u(ohs_offset + 20);
        // this field isn't used to make OptionalHeaderStandard and
        // OptionalHeaderStandardPlus consistent (this field isn't in Plus header)
        //uint32_t pe_BaseOfData              = sbuf.get32u(ohs_offset + 24);
        
        // check for values resembling sanity
        if (pe_BaseOfCode          > 0x10000000) return "";
        if (pe_AddressOfEntryPoint > 0x10000000) return "";
        
        xml << " Magic=\"PE32\"";
        xml << " MajorLinkerVersion=\""      << ((int) pe_MajorLinkerVersion) << "\"";
        xml << " MinorLinkerVersion=\""      << ((int) pe_MinorLinkerVersion) << "\"";
        xml << " SizeOfCode=\""              << pe_SizeOfCode                 << "\"";
        xml << " SizeOfInitializedData=\""   << pe_SizeOfInitializedData      << "\"";
        xml << " SizeOfUninitializedData=\"" << pe_SizeOfUninitializedData    << "\"";
        xml << " AddressOfEntryPoint=\"0x"   << std::hex << pe_AddressOfEntryPoint << "\"";
        xml << " BaseOfCode=\"0x"            << std::hex << pe_BaseOfCode          << "\"";
        
        break;

    case IMAGE_FILE_TYPE_PE32PLUS :
        pe_MajorLinkerVersion      = sbuf.get8u(ohs_offset + 2);
        pe_MinorLinkerVersion      = sbuf.get8u(ohs_offset + 3);
        pe_SizeOfCode              = sbuf.get32u(ohs_offset + 4);
        pe_SizeOfInitializedData   = sbuf.get32u(ohs_offset + 8);
        pe_SizeOfUninitializedData = sbuf.get32u(ohs_offset + 12);
        pe_AddressOfEntryPoint     = sbuf.get32u(ohs_offset + 16);
        pe_BaseOfCode              = sbuf.get32u(ohs_offset + 20);
        
        if (pe_BaseOfCode          > 0x10000000) return "";
        if (pe_AddressOfEntryPoint > 0x10000000) return "";
        
        xml << " Magic=\"PE32+\"";
        xml << " MajorLinkerVersion=\""      << ((int) pe_MajorLinkerVersion) << "\"";
        xml << " MinorLinkerVersion=\""      << ((int) pe_MinorLinkerVersion) << "\"";
        xml << " SizeOfCode=\""              << pe_SizeOfCode                 << "\"";
        xml << " SizeOfInitializedData=\""   << pe_SizeOfInitializedData      << "\"";
        xml << " SizeOfUninitializedData=\"" << pe_SizeOfUninitializedData    << "\"";
        xml << " AddressOfEntryPoint=\"0x"   << std::hex << pe_AddressOfEntryPoint << "\"";
        xml << " BaseOfCode=\"0x"            << std::hex << pe_BaseOfCode          << "\"";
        
        break;
        
    default :
        return "";
    }

    xml << " />";
    

    /******************************************
     * BEGIN OPTIONAL HEADER WINDOWS           *
     ******************************************/
    // If the SizeOfOptionalHeader is large enough to support the
    // optional windows header,then we will pull out windows header
    // information.
    // At this point, our confidence level should be very high that this
    // is a PE, and we are only pulling information for XML
    uint32_t pe_SectionAlignment = 0;
    uint32_t pe_FileAlignment = 0;
    uint16_t pe_MajorOperatingSystemVersion = 0;
    uint16_t pe_MinorOperatingSystemVersion = 0;
    uint16_t pe_MajorImageVersion = 0;
    uint16_t pe_MinorImageVersion = 0;
    uint16_t pe_MajorSubsystemVersion = 0;
    uint16_t pe_MinorSubsystemVersion = 0;
    uint32_t pe_Win32VersionValue = 0;
    uint32_t pe_SizeOfImage = 0;
    uint32_t pe_SizeOfHeaders = 0;
    uint32_t pe_CheckSum = 0;
    uint16_t pe_Subsystem = 0;
    uint16_t pe_DllCharacteristics = 0;
    uint32_t pe_LoaderFlags = 0;
    uint32_t pe_NumberOfRvaAndSizes = 0;
    
    uint64_t pe_ImageBase = 0;
    uint64_t pe_SizeOfStackReserve = 0;
    uint64_t pe_SizeOfStackCommit = 0;
    uint64_t pe_SizeOfHeapReserve = 0;
    uint64_t pe_SizeOfHeapCommit = 0;
    
    std::string Subsystem = "";
    bool ohw_xml = true;
    
    if (    (pe_Magic == IMAGE_FILE_TYPE_PE32)
	    && (pe_SizeOfOptionalHeader > sizeof(Pe_OptionalHeaderStandard) +
		sizeof(Pe_OptionalHeaderWindows))
	    // do we have enough buffer space for this?
	    && (header_offset 
		+ sizeof(Pe_FileHeader)
		+ sizeof(Pe_OptionalHeaderStandard)
		+ sizeof(Pe_OptionalHeaderWindows) <= size)) {
        
        ohw_offset = ohs_offset + sizeof(Pe_OptionalHeaderStandard);
        
        pe_ImageBase                   = sbuf.get32u(ohw_offset);
        pe_SectionAlignment            = sbuf.get32u(ohw_offset + 4);
        pe_FileAlignment               = sbuf.get32u(ohw_offset + 8);
        pe_MajorOperatingSystemVersion = sbuf.get16u(ohw_offset + 12);
        pe_MinorOperatingSystemVersion = sbuf.get16u(ohw_offset + 14);
        pe_MajorImageVersion           = sbuf.get16u(ohw_offset + 16);
        pe_MinorImageVersion           = sbuf.get16u(ohw_offset + 18);
        pe_MajorSubsystemVersion       = sbuf.get16u(ohw_offset + 20);
        pe_MinorSubsystemVersion       = sbuf.get16u(ohw_offset + 22);
        pe_Win32VersionValue           = sbuf.get32u(ohw_offset + 24);
        pe_SizeOfImage                 = sbuf.get32u(ohw_offset + 28);
        pe_SizeOfHeaders               = sbuf.get32u(ohw_offset + 32);
        pe_CheckSum                    = sbuf.get32u(ohw_offset + 36);
        pe_Subsystem                   = sbuf.get16u(ohw_offset + 40);
        pe_DllCharacteristics          = sbuf.get16u(ohw_offset + 42);
        pe_SizeOfStackReserve          = sbuf.get32u(ohw_offset + 44);
        pe_SizeOfStackCommit           = sbuf.get32u(ohw_offset + 48);
        pe_SizeOfHeapReserve           = sbuf.get32u(ohw_offset + 52);
        pe_SizeOfHeapCommit            = sbuf.get32u(ohw_offset + 56);
        pe_LoaderFlags                 = sbuf.get32u(ohw_offset + 60);
        pe_NumberOfRvaAndSizes         = sbuf.get32u(ohw_offset + 64);
        
        Subsystem = match_switch_case(pe_optionalwindowsheader_subsystem, pe_Subsystem);
    }   
    else if (    (pe_Magic == IMAGE_FILE_TYPE_PE32PLUS)
		 && (pe_SizeOfOptionalHeader > sizeof(Pe_OptionalHeaderStandardPlus) +
		     sizeof(Pe_OptionalHeaderWindowsPlus))
		 // do we have enough buffer space for this?
		 && (header_offset
		     + sizeof(Pe_FileHeader)
		     + sizeof(Pe_OptionalHeaderStandardPlus)
		     + sizeof(Pe_OptionalHeaderWindowsPlus) <= size)) {
        
        ohw_offset = ohs_offset + sizeof(Pe_OptionalHeaderStandardPlus);

        pe_ImageBase                   = sbuf.get64u(ohw_offset);
        pe_SectionAlignment            = sbuf.get32u(ohw_offset + 8);
        pe_FileAlignment               = sbuf.get32u(ohw_offset + 12);
        pe_MajorOperatingSystemVersion = sbuf.get16u(ohw_offset + 16);
        pe_MinorOperatingSystemVersion = sbuf.get16u(ohw_offset + 18);
        pe_MajorImageVersion           = sbuf.get16u(ohw_offset + 20);
        pe_MinorImageVersion           = sbuf.get16u(ohw_offset + 22);
        pe_MajorSubsystemVersion       = sbuf.get16u(ohw_offset + 24);
        pe_MinorSubsystemVersion       = sbuf.get16u(ohw_offset + 26);
        pe_Win32VersionValue           = sbuf.get32u(ohw_offset + 30);
        pe_SizeOfImage                 = sbuf.get32u(ohw_offset + 32);
        pe_SizeOfHeaders               = sbuf.get32u(ohw_offset + 36);
        pe_CheckSum                    = sbuf.get32u(ohw_offset + 40);
        pe_Subsystem                   = sbuf.get16u(ohw_offset + 44);
        pe_DllCharacteristics          = sbuf.get16u(ohw_offset + 46);
        pe_SizeOfStackReserve          = sbuf.get64u(ohw_offset + 48);
        pe_SizeOfStackCommit           = sbuf.get32u(ohw_offset + 52);
        pe_SizeOfHeapReserve           = sbuf.get32u(ohw_offset + 56);
        pe_SizeOfHeapCommit            = sbuf.get32u(ohw_offset + 60);
        pe_LoaderFlags                 = sbuf.get32u(ohw_offset + 64);
        pe_NumberOfRvaAndSizes         = sbuf.get32u(ohw_offset + 68);
        
        Subsystem = match_switch_case(pe_optionalwindowsheader_subsystem, pe_Subsystem);
    }
    else ohw_xml = false;
    
    if (ohw_xml) {
        xml << "<OptionalHeaderWindows";
        xml << " ImageBase=\"0x"                 << std::hex << pe_ImageBase       << "\"";
        xml << " SectionAlignment=\""            << pe_SectionAlignment            << "\"";
        xml << " FileAlignment=\""               << pe_FileAlignment               << "\"";
        xml << " MajorOperatingSystemVersion=\"" << pe_MajorOperatingSystemVersion << "\"";
        xml << " MinorOperatingSystemVersion=\"" << pe_MinorOperatingSystemVersion << "\"";
        xml << " MajorImageVersion=\""           << pe_MajorImageVersion           << "\"";
        xml << " MinorImageVersion=\""           << pe_MinorImageVersion           << "\"";
        xml << " MajorSubsystemVersion=\""       << pe_MajorSubsystemVersion       << "\"";
        xml << " MinorSubsystemVersion=\""       << pe_MinorSubsystemVersion       << "\"";
        xml << " Win32VersionValue=\""           << pe_Win32VersionValue           << "\"";
        xml << " SizeOfImage=\""                 << pe_SizeOfImage                 << "\"";
        xml << " SizeOfHeaders=\""               << pe_SizeOfHeaders               << "\"";
        xml << " CheckSum=\"0x"                  << std::hex << pe_CheckSum        << "\"";
        xml << " SubSystem=\""                   << Subsystem                      << "\"";
        xml << " SizeOfStackReserve=\""          << pe_SizeOfStackReserve          << "\"";
        xml << " SizeOfStackCommit=\""           << pe_SizeOfStackCommit           << "\"";
        xml << " SizeOfHeapReserve=\""           << pe_SizeOfHeapReserve           << "\"";
        xml << " SizeOfHeapCommit=\""            << pe_SizeOfHeapCommit            << "\"";
        xml << " LoaderFlags=\""                 << pe_LoaderFlags                 << "\"";
        xml << " NumberOfRvaAndSizes=\""         << pe_NumberOfRvaAndSizes         << "\">";
        
        /* Note: This field name is DllCharacteristics in the Microsoft
         * spec, not Characteristics.  We have chosen to preserve
         * Microsoft's inconsistency in the XML.
         */
        decode_flags(xml,
                     "DllCharacteristics",
                     pe_optionalwindowsheader_dllcharacteristic,
                     pe_DllCharacteristics);
        xml << "</OptionalHeaderWindows>";
    }
    
    
    // Before we loop through sections, we're going to locate the
    // virtual address of the idata directory. If we load a valid
    // section that holds the idata, we'll save the address of the idata
    // so we can load DLL names later

    // find the .idata DataDirectory
    const Pe_DataDirectory  * idd  = NULL; // idata Data Directory
    if ((pe_Magic == IMAGE_FILE_TYPE_PE32) && (ohw_offset)) {
	idd = sbuf.get_struct_ptr<Pe_DataDirectory>(header_offset
						    + sizeof(Pe_FileHeader)
						    + sizeof(Pe_OptionalHeaderStandard)
						    + sizeof(Pe_OptionalHeaderWindows)
						    + sizeof(Pe_DataDirectory));
    }
    if ((pe_Magic == IMAGE_FILE_TYPE_PE32PLUS) && (ohw_offset)) {
	idd = sbuf.get_struct_ptr<Pe_DataDirectory>(header_offset
						    + sizeof(Pe_FileHeader)
						    + sizeof(Pe_OptionalHeaderStandardPlus)
						    + sizeof(Pe_OptionalHeaderWindowsPlus)
						    + sizeof(Pe_DataDirectory));
    }
    
    // Section Header for Imports (shi):
    // if this not NULL once we're done processing sections,
    // it points to the Section Header of the section which
    // holds the import table data
    const Pe_SectionHeader * shi  = NULL; 
    xml << "<Sections>";
    for (section_i = 0; section_i < pe_NumberOfSections; section_i++) {
        
	const Pe_SectionHeader * sh =
	    sbuf.get_struct_ptr<Pe_SectionHeader>(header_offset
						  + sizeof(Pe_FileHeader)
						  + pe_SizeOfOptionalHeader
						  + sizeof(Pe_SectionHeader) * section_i);
	if(sh==0) break; // no more!
        
        // the following flags are reserved for future use and should
        // not be set. If we get something invalid, we've probably gone
        // into garbage, so just give up on the sections.
        if (sh->Characteristics & PE_SECTION_RESERVED_FLAGS) break;
        
        if (sh->VirtualSize & 0x80000000) break; 
        
        // this allows for sections up to 256mb in size, more than
        // generous
        if (sh->SizeOfRawData & 0xe0000000) break;

        
        // section names do not have to be null-terminated,
        // so termiante it.
        char section_name[9];       
        strncpy(section_name, sh->Name, sizeof(section_name));
        section_name[8] = '\0';
        
	if(!valid_section_name(section_name)){
	    shi = 0;			// don't get the shi
	    goto end_of_sections;
	}

        xml << "<SectionHeader";
        xml << " Name=\""                 << section_name << "\"";
        xml << " VirtualSize=\""          << sh->VirtualSize << "\"";
        xml << " VirtualAddress=\""       << sh->VirtualAddress << "\"";
        xml << " SizeOfRawData=\""        << sh->SizeOfRawData << "\"";
        xml << " PointerToRawData=\""     << sh->PointerToRawData << "\"";
        xml << " PointerToRelocations=\"" << sh->PointerToRelocations << "\"";
        xml << " PointerToLinenumbers=\"" << sh->PointerToLinenumbers << "\"";
        xml << " >";
        
        decode_flags(xml,"Characteristics",pe_section_characteristic_names,sh->Characteristics);
        
        xml << "</SectionHeader>";
        
        // check if import data resides in this section
        if (idd) {
            if (    (idd->VirtualAddress >= sh->VirtualAddress)
		    && (idd->VirtualAddress < sh->VirtualAddress
			+ sh->SizeOfRawData)) {
                shi = sh;
            }
        }
    }
 end_of_sections:;
    xml << "</Sections>";

    // get DLL names
    if (shi) {
        // find offset into PE where the Import Data Table resides
	xml << "<dlls>";
        for (int idt_i = 0;  ; idt_i++) {
	    const Pe_ImportDirectoryTable * idt  = sbuf.get_struct_ptr<Pe_ImportDirectoryTable>(shi->PointerToRawData + (  idd->VirtualAddress - shi->VirtualAddress) + sizeof(Pe_ImportDirectoryTable)*idt_i);
	    if(idt==0) break;
            
            // The Import Data Table is terminated by a null entry
            if (idt->ImportLookupTableRVA == 0 && idt->TimeDateStamp==0 && idt->ForwarderChain==0 && idt->NameRVA==0 && idt->ImportAddressTableRVA==0){
		break;
	    }
                
            // We are given the RVA of a string which contains the
            // name of the DLL. We're going to loop through sections,
            // attempt to find a section which is loaded into this
            // RVA, and then extract the name from it
            for (int si = 0; si < pe_NumberOfSections; si++) {
                // is this section header out-of-bounds
                if (header_offset + sizeof(Pe_FileHeader)
                    + pe_SizeOfOptionalHeader
                    + sizeof(Pe_SectionHeader) * (si + 1) > size)
                    break;
                    
                const Pe_SectionHeader *sht =
		    sbuf.get_struct_ptr<Pe_SectionHeader>(header_offset
							  + sizeof(Pe_FileHeader)
							  + pe_SizeOfOptionalHeader
							  + sizeof(Pe_SectionHeader) * si);
                    
                // find target section
                if (    (sht->VirtualAddress < idt->NameRVA)
			&& (sht->VirtualAddress + sht->SizeOfRawData > idt->NameRVA)) {

                    // can we actually get the string out of this section
                    // max allowable string size is 32 chars
		    if (sht->PointerToRawData
			+ (idt->NameRVA - sht->VirtualAddress)
			+ 32 >= size)
			break;
            
		    // place dll name in a buffer string and add the xml
		    // Note --- currently this copies then verifies; it would be better
		    // to verify while copying...

		    // find length of dllname, <= 32 characters
		    // p. 90 of the pecoff specification (v8) assures that
		    // names contain ASCII strings and are null-terminated.
		    int dllname_length = 32;
		    for (int dlli = 0; dlli < 32; dlli++) {
			if (sbuf[sht->PointerToRawData + (idt->NameRVA - sht->VirtualAddress) + dlli] == 0) {
			    dllname_length = dlli;
			    break;
			}
		    }
		    std::string dllname = sbuf.substr(sht->PointerToRawData + (idt->NameRVA - sht->VirtualAddress), dllname_length);

		    // according to spec, these names are ASCII. perform
		    // a sanity check, as this data is deep in the file
		    // and may not be part of a contiguous PE
		    if(!valid_dll_name(dllname)) goto end_of_dlls;
		    xml << "<dll>" << dllname << "</dll>";
                }
            }
        }
    end_of_dlls:;
	xml << "</dlls>";
    }
    xml << "</PE>";
    return xml.str();
}

// the data that ends the furthest out is the carve size
static size_t get_carve_size (const sbuf_t& sbuf)
{
    // heaser offset
    const uint32_t header_offset = sbuf.get32u(PE_FILE_OFFSET) + PE_SIGNATURE_SIZE;

    // OptionalHeaderStandard offset
    const size_t ohs_offset = header_offset + sizeof(Pe_FileHeader);

    // image file type
    const uint16_t pe_Magic = sbuf.get16u(ohs_offset);

    // check end of signature as potential carve size
    // point to the certificate table containing the digital signature,
    // IMAGE_DIRECTORY_ENTRY_SECURITY, index 4
    const Pe_DataDirectory* ctd;
    if (pe_Magic == IMAGE_FILE_TYPE_PE32) {
	ctd = sbuf.get_struct_ptr<Pe_DataDirectory>(header_offset
						    + sizeof(Pe_FileHeader)
						    + sizeof(Pe_OptionalHeaderStandard)
						    + sizeof(Pe_OptionalHeaderWindows)
						    + sizeof(Pe_DataDirectory)*4);
    } else if (pe_Magic == IMAGE_FILE_TYPE_PE32PLUS) {
	ctd = sbuf.get_struct_ptr<Pe_DataDirectory>(header_offset
						    + sizeof(Pe_FileHeader)
						    + sizeof(Pe_OptionalHeaderStandardPlus)
						    + sizeof(Pe_OptionalHeaderWindowsPlus)
						    + sizeof(Pe_DataDirectory)*4);
    } else {
        // corrupt magic number so do not carve
        return 0;
    }

    size_t carve_size = 0;
    if (ctd != NULL && ctd->VirtualAddress < 1024*1024*1024 && ctd->Size < 1024*1024) {
        // consider end of signature, which can be 0, as potential carve size
        carve_size = ctd->VirtualAddress + ctd->Size;
    }

    // consider each section size as potential carve size
    const uint16_t pe_NumberOfSections = sbuf.get16u(header_offset + 2);
    const uint16_t pe_SizeOfOptionalHeader = sbuf.get16u(header_offset + 16);
    for (int section_i = 0; section_i < pe_NumberOfSections; section_i++) {

	const Pe_SectionHeader * sh =
	    sbuf.get_struct_ptr<Pe_SectionHeader>(header_offset
						  + sizeof(Pe_FileHeader)
						  + pe_SizeOfOptionalHeader
						  + sizeof(Pe_SectionHeader) * section_i);
	if(sh==0) break; // end of sbuf

        if (sh->PointerToRawData + sh->SizeOfRawData > carve_size) {
            carve_size = sh->PointerToRawData + sh->SizeOfRawData;
        }
    }

    // the carve size must not be totally unreasonable
    if (carve_size > 1024*1024*1024) { // 1GiB
        return 0;
    }

    return carve_size;
}

extern "C"
void scan_winpe (const class scanner_params &sp,
		 const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    std::string xml;
    
    if (sp.phase == scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name            = "winpe";
        sp.info->description     = "Scan for Windows PE headers";
        sp.info->scanner_version = "1.0.0";
        sp.info->feature_names.insert("winpe");
        sp.info->feature_names.insert("winpe_carved");
        sp.info->get_config("winpe_carve_mode", &winpe_carve_mode,
                            "0=carve none; 1=carve encoded; 2=carve all");

        return;
    }

    if(sp.phase==scanner_params::PHASE_INIT){
        sp.fs.get_name("winpe_carved")->set_carve_mode(
                  static_cast<feature_recorder::carve_mode_t>(winpe_carve_mode));
    }

    if(sp.phase == scanner_params::PHASE_SCAN){    // phase 1
	feature_recorder *f = sp.fs.get_name("winpe");
    
	/* 
	 * Portable Executables start with a MS-DOS stub. The actual PE
	 * begins at an offset of 0x3c (PE_FILE_OFFSET) bytes. The signature
	 * is "PE\0\0"
	 */
	// we are going to make sure we have at least enough room for a
	// complete MS-Dos Stub, Signature and PE header, and an optional
	// header and/or at least one section

    
	/* Return if the buffer is impossibly small to contain a PE header */
	if ((sp.sbuf.bufsize < PE_SIGNATURE_SIZE + sizeof(Pe_FileHeader) + 40) ||
	    (sp.sbuf.bufsize < PE_FILE_OFFSET+4)){
	    return;
	}
    
	/* Loop through the sbuf, go to the offset, and see if there is a PE signature */
	for (size_t pos = 0; pos < sp.sbuf.pagesize; pos++) {
	    size_t bytes_left = sp.sbuf.bufsize - pos;
	    if(bytes_left < PE_FILE_OFFSET+4) break; // ran out of space

	    // offset to the header is at 0x3c
	    const uint32_t pe_header_offset = sp.sbuf.get32u(pos+PE_FILE_OFFSET);
	    if (pe_header_offset + 4 > bytes_left) continue; // points to region outside the sbuf

	    // check for magic number. If found, analyze
	    if (    (sp.sbuf[pos+pe_header_offset    ] == PE_SIGNATURE[0])
		    && (sp.sbuf[pos+pe_header_offset + 1] == PE_SIGNATURE[1])
		    && (sp.sbuf[pos+pe_header_offset + 2] == PE_SIGNATURE[2])
		    && (sp.sbuf[pos+pe_header_offset + 3] == PE_SIGNATURE[3])) {

		const sbuf_t data(sp.sbuf + pos);

		xml = scan_winpe_verify(data);
		if (xml != "") {
		    // If we have 4096 bytes, generate md5 hash
                    sbuf_t first4k(data,0,4096);
		    std::string hash = f->fs.hasher.func(first4k.buf,first4k.bufsize);
		    f->write(data.pos0,hash,xml);
                    size_t carve_size = get_carve_size(data);
                    feature_recorder *f_carved = sp.fs.get_name("winpe_carved");
                    f_carved->carve(data, 0, carve_size, ".winpe");
		}
	    }
	}
    }
}
