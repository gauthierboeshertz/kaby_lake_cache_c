
#include <stdio.h>
#include "error.h"
#include "addr.h"
#include <inttypes.h>


int init_virt_addr(virt_addr_t * vaddr,
                   uint16_t pgd_entry,
                   uint16_t pud_entry, uint16_t pmd_entry,
                   uint16_t pte_entry, uint16_t page_offset)
{

	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE(pgd_entry >> PGD_ENTRY == 0, ERR_BAD_PARAMETER, "pgd has to be on at most 9"); //Check for correct sizes
	M_REQUIRE(pud_entry >> PUD_ENTRY == 0, ERR_BAD_PARAMETER, "pud has to be on at most 9");
	M_REQUIRE(pte_entry >> PTE_ENTRY == 0, ERR_BAD_PARAMETER, "pte has to be on at most 9");
	M_REQUIRE(pmd_entry >> PMD_ENTRY == 0, ERR_BAD_PARAMETER, "pmd has to be on at most 9");
	M_REQUIRE(page_offset >> PAGE_OFFSET == 0, ERR_BAD_PARAMETER, "page_offset has to be on at most 12");


	vaddr->pgd_entry = pgd_entry; //Assign each value to its correspondi place
	vaddr->pud_entry = pud_entry;
	vaddr->pte_entry = pte_entry;
	vaddr->pmd_entry = pmd_entry;
	vaddr->page_offset = page_offset;
	vaddr->reserved = 0;

	return ERR_NONE; //Return ERR_NONE if everything goes righ

}


int init_virt_addr64(virt_addr_t * vaddr, uint64_t vaddr64)
{

	M_REQUIRE_NON_NULL(vaddr);
	vaddr->page_offset = vaddr64 & mask_offset; //extracts the offset using a mask
	vaddr->pte_entry = (vaddr64 >> PAGE_OFFSET & mask_9bit);
	vaddr->pmd_entry  = ( vaddr64 >> (PAGE_OFFSET + shift) & mask_9bit); //shifs the necessary bits to extract each value
	vaddr->pud_entry  = ( vaddr64 >> (PAGE_OFFSET + 2 * shift) & mask_9bit);
	vaddr->pgd_entry  = ( vaddr64 >> (PAGE_OFFSET + 3 * shift) & mask_9bit);
	vaddr->reserved = 0; //sets reserved to 0

	return ERR_NONE; //Return ERR_NONE if everything goes righ
}


int init_phy_addr(phy_addr_t* paddr, uint32_t page_begin, uint32_t page_offset)
{

	M_REQUIRE(page_offset >> PAGE_OFFSET  == 0, ERR_BAD_PARAMETER, "page_offset must be a 16 bit number");
	M_REQUIRE(page_begin % PAGE_SIZE == 0, ERR_BAD_PARAMETER, "page_begin not a multiple of page_size");
	M_REQUIRE_NON_NULL(paddr);

	paddr->phy_page_num = page_begin >> PAGE_OFFSET; //removes the offset
	paddr->page_offset =  page_offset; //sets the offset

	return ERR_NONE; //Return ERR_NONE if everything goes righ

}


uint64_t virt_addr_t_to_virtual_page_number(const virt_addr_t * vaddr)
{

	M_REQUIRE_NON_NULL(vaddr);

	uint64_t pte = (uint64_t)vaddr->pte_entry;           //Shifts each element to its corresponding place without taking into account the offset
	uint64_t pmd = (uint64_t)vaddr->pmd_entry << PMD_ENTRY;
	uint64_t pud = (uint64_t)vaddr->pud_entry << (PUD_ENTRY * 2);
	uint64_t pgd = (uint64_t)vaddr->pgd_entry << (PGD_ENTRY * 3);

	return pte | pmd | pud | pgd;  //Merges all the values to creat a 64 bit value

}

uint64_t virt_addr_t_to_uint64_t(const virt_addr_t * vaddr)
{

	M_REQUIRE_NON_NULL(vaddr);

	uint64_t temporal = virt_addr_t_to_virtual_page_number(vaddr) << PAGE_OFFSET; //shifts to make place for the offset
	return temporal | vaddr->page_offset; //Adds the offset

}

int print_virtual_address(FILE* where, const virt_addr_t* vaddr)
{

	M_REQUIRE_NON_NULL(where);
	M_REQUIRE_NON_NULL(vaddr);

	int numcar = fprintf(where, "PGD=0x%"PRIX16 "; PUD=0x%"PRIX16"; PMD=0x%"PRIX16 "; PTE=0x%" PRIX16 "; offset=0x%" PRIX16 , vaddr->pgd_entry, vaddr->pud_entry, vaddr->pmd_entry, vaddr->pte_entry, vaddr->page_offset);
	return numcar;
}


int print_physical_address(FILE* where, const phy_addr_t* paddr)
{

	M_REQUIRE_NON_NULL(where);
	M_REQUIRE_NON_NULL(paddr);

	int numcar = fprintf(where, "page num=0x%" PRIX32 "; offset=0x%" PRIX16 , paddr->phy_page_num , paddr->page_offset);

	return numcar;


}
