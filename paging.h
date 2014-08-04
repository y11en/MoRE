/**
	@file
	Windows driver to manipulate the page tables (header file)
		
	@date 1/30/2012
***************************************************************/

#ifndef _MORE_PAGING_H_
#define _MORE_PAGING_H_

#include "../stdint.h"
#include "ntddk.h"

#define PAGE_SIZE__LARGE 0x400000
#define PAGE_SIZE__SMALL 0x1000

#pragma pack(push, hook, 1)

/**
    Structure defining the page directory entry
    from Intel Manual 3A - 4.3
*/
struct PageDirectoryEntry_s
{
    uint32 p :1; // Present
    uint32 rw :1; // Read/Write
    uint32 us :1; // User/Superuser
    uint32 pwt :1; // Page write through
    uint32 pcd :1; // Page level cache disable
    uint32 a :1; // Accessed
    uint32 d :1; // Dirty
    uint32 ps :1; // Large page
    uint32 g :1; // Global
    uint32 reserved1 :3; // Must be 0
    uint32 pat :1;	// PAT must be 0
    uint32 reserved2 :9; // Must be 0
    uint32 address :10;	// Address of page.
};

typedef struct PageDirectoryEntry_s PageDirectoryEntry;

/**
    Structure defining the page directory entry
    from Intel Manual 3A - 4.3 (for small pages)
*/
struct PageDirectoryEntrySmallPage_s
{
    uint32 p :1; // Present
    uint32 rw :1; // Read/Write
    uint32 us :1; // User/Superuser
    uint32 pwt :1; // Page write through
    uint32 pcd :1; // Page level cache disable
    uint32 a :1; // Accessed
    uint32 ignored :1; // Ignored
    uint32 ps :1; // Page Size
    uint32 reserved1 :4; // Must be 0
    uint32 address :20;	// Address of page.
};

typedef struct PageDirectoryEntrySmallPage_s PageDirectoryEntrySmallPage;

/**
    Structure defining the page table entry
    from Intel Manual 3A
*/
struct PageTableEntry_s
{
    uint32 p :1; // Present
    uint32 rw :1; // Read/Write
    uint32 us :1; // User/Superuser
    uint32 pwt :1; // Page write through
    uint32 pcd :1; // Page level cache disable
    uint32 a :1; // Accessed
    uint32 d :1; // Dirty
    uint32 pat :1; // PAT must be 0
    uint32 g :1; // G must be 0
    uint32 reserved1 :3; // Must be 0
    uint32 address :20;	// Address of page.
};

typedef struct PageTableEntry_s PageTableEntry;

#pragma pack(pop, hook)

/**
    Structure storing the needed information to complete a piecemeal walk of the
    page-tables
*/
struct PageWalkContext_s
{
    PHYSICAL_ADDRESS targetAddress;
    PageDirectoryEntry *pde;
    uint32 pdeOff;
    PageTableEntry *pte;
    uint32 pteOff;
};

typedef struct PageWalkContext_s PageWalkContext;

struct PagingContext_s
{
    PageTableEntry *PageTable;
    uint32 VirtualPrefix;
    uint8 *PageArray;
    uint32 NumPages;
    uint8 *PageArrayBitmap;
    uint32 CR3Val;
};

typedef struct PagingContext_s PagingContext;

/**
    Maps a PTE/PDE out of memory
    
    @param ptr Virtual address of PDE/PTE
*/
void pagingMapOutEntry(void *ptr);

/**
    Maps a PTE/PDE out of memory, using the custom MM
    
    @param ptr Virtual address of PDE/PTE
    @param context Pointer to paging context
*/
void pagingMapOutEntryDirql(void *ptr, PagingContext * context);

/**
    Function to map the PTE for a given CR3:Virtual address into memory
    
    @param CR3 CR3 value
    @param virtualAddress Virtual address of the memory
    
    @return Pointer to mapped in PTE, or NULL is PS = 1
*/
PageTableEntry * pagingMapInPte(uint32 CR3, void *virtualAddress);

/**
    Function to map the PTE for a given CR3:Virtual address into memory (DIRQL)
    
    @param CR3 CR3 value
    @param virtualAddress Virtual address of the memory
    @param context Pointer to paging context
    @return Pointer to mapped in PTE, or NULL is PS = 1
*/
PageTableEntry * pagingMapInPteDirql(uint32 CR3, void *virtualAddress, PagingContext * context);

/**
    Function to map the PDE for a given CR3:Virtual address into memory
    
    @param CR3 CR3 value
    @param virtualAddress Virtual address of the memory
    
    @return Pointer to mapped in PDE
*/
PageDirectoryEntry * pagingMapInPde(uint32 CR3, void *virtualAddress);

/**
    Function to map the PDE for a given CR3:Virtual address into memory
    
    @param CR3 CR3 value
    @param virtualAddress Virtual address of the memory
    
    @return Pointer to mapped in PDE
*/
PageDirectoryEntry * pagingMapInPdeDirql(uint32 CR3, void *virtualAddress, PagingContext * context);

/**
    Function which allocates a page table walk to find all virtual
    addresses which reference a given physical memory address
    
    @param physicalAddress Target physical address
    @return A pointer to the page walk context for future calls to pagingGetNext
*/
PageWalkContext * pagingInitWalk(PHYSICAL_ADDRESS physicalAddress);

/**
    Function to return the next virtual address that points to the physical
    address of interest
    
    @param context Pointer to PageWalkContext generated by pagingInitWalk
    @return NULL if no more references are found, otherwise a virtual address
*/
uint32 pagingGetNext(PageWalkContext *context);

/**
    Function to deallocate a PageWalkContext
    
    @param context Pointer to PageWalkContext to free
*/
void pagingFreeWalk(PageWalkContext *context);

/** 
    Function to 'lock' a process' memory into physical memory and prevent paging
    
    @param startAddr Starting virtual address to lock
    @param len Number of bytes to lock
    @param proc PEPROCESS pointer to the process
    @param apcstate Pointer to an APC state memory location
    @return MDL to be used later to unlock the memory or NULL if lock failed
*/
PMDLX pagingLockProcessMemory(PVOID startAddr, uint32 len, PEPROCESS proc, PKAPC_STATE apcstate);

/** 
    Function to 'lock' a process' memory into physical memory and prevent paging
    
    @param proc PEPROCESS pointer to the process
    @param apcstate Pointer to an APC state memory location
    @param mdl Pointer to previously locked process MDL
*/
void pagingUnlockProcessMemory(PEPROCESS proc, PKAPC_STATE apcstate, PMDLX mdl);

/**
    Initializes the page-fault-free memory operations
    
    @note Must be called at IRQL = 0
    
    @param context Pointer to the paging context to initialize
    @param numPages Number of pages to reserve to hand out later
*/
void pagingInitMappingOperations(PagingContext *context, uint32 numPages);

/**
    Frees the pre-allocated buffer and ends all mapping operations
    
    @param context Pointer to the paging context to free
*/
void pagingEndMappingOperations(PagingContext *context);

/**
    Allocates a page of non-paged memory
    
    @param context Pointer to paging context
    @return Pointer to the allocated page, or NULL if no pages are left
*/
void * pagingAllocPage(PagingContext *context);

/** 
    Frees allocated page
    
    @param context Pointer to paging context
    @param ptr Pointer to page to be freed
*/
void pagingFreePage(PagingContext *context, void * ptr);

#endif // _MORE_PAGING_H_

