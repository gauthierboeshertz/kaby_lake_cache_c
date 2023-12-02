#include "page_walk.h"
#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "addr_mng.h"
#include <inttypes.h>
#include "memory.h"




/**
 * @brief      Returns corresponding address index
 *
 * @param[in]  start       The start of the used memory
 * @param[in]  page_start  The page start is the index of the start of the page we want to use
 * @param[in]  index       The index of the actual page of the information we want
 *
 * @return     { computed address }
 */
static inline pte_t read_page_entry(const pte_t* start, pte_t page_start, uint16_t index)
{

	return start[ page_start / (sizeof(pte_t)) + index ];

}


int page_walk(const void* mem_space, const virt_addr_t* vaddr, phy_addr_t* paddr)
{

	M_REQUIRE_NON_NULL(mem_space); //Tests if all passed arguments are non null
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE_NON_NULL(paddr);

	uint32_t PUD_start = read_page_entry(mem_space, 0, vaddr->pgd_entry); 		//Follows the address path as ilustrated in the notes
	uint32_t PMD_start = read_page_entry(mem_space, PUD_start, vaddr->pud_entry);
	uint32_t PT_start = read_page_entry(mem_space, PMD_start, vaddr->pmd_entry);
	uint32_t phys_page_number = read_page_entry(mem_space, PT_start, vaddr->pte_entry);

	return init_phy_addr(paddr, phys_page_number, vaddr->page_offset);

}

