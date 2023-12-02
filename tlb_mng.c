#include "tlb_mng.h"
#include "tlb.h"
#include "addr_mng.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

#include "page_walk.h"


int tlb_entry_init( const virt_addr_t * vaddr, const phy_addr_t * paddr, tlb_entry_t * tlb_entry)
{

	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(tlb_entry);

	tlb_entry->tag = virt_addr_t_to_virtual_page_number(vaddr); //Converts the virtual address to a virtual page number

	tlb_entry->phy_page_num = paddr->phy_page_num;
	tlb_entry->v = 1; //sets the validity bit to 1
	return ERR_NONE;
}

int tlb_insert( uint32_t line_index, const tlb_entry_t * tlb_entry, tlb_entry_t * tlb)
{

	M_REQUIRE(line_index < TLB_LINES, ERR_BAD_PARAMETER, "index bigger than size");
	M_REQUIRE_NON_NULL(tlb_entry);
	M_REQUIRE_NON_NULL(tlb);

	tlb[line_index].phy_page_num = tlb_entry->phy_page_num;//inserts everything at the given index
	tlb[line_index].tag = tlb_entry->tag;
	tlb[line_index].v = tlb_entry->v;

	return ERR_NONE;
}

int tlb_flush(tlb_entry_t * tlb)
{

	M_REQUIRE_NON_NULL(tlb);

	for (int i = 0 ; i < TLB_LINES; ++i) //loops through the array and sets everything to 0
		{
			tlb[i].phy_page_num = 0;
			tlb[i].tag = 0;
			tlb[i].v = 0;
		}
	return ERR_NONE;

}

int tlb_hit(const virt_addr_t * vaddr,
            phy_addr_t * paddr,
            const tlb_entry_t * tlb,
            replacement_policy_t * replacement_policy)
{

	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(tlb);
	M_REQUIRE_NON_NULL(replacement_policy);

	uint64_t page_number = virt_addr_t_to_virtual_page_number(vaddr); //extract virtual page number

	for_all_nodes_reverse( node, replacement_policy->ll) //loops trough all the nodes
	{

		if ( tlb[ node->value].tag == page_number && tlb[node->value].v) //checks if the validity node==1 and that we have the corresponding tag
			{

				paddr->phy_page_num = tlb[node->value].phy_page_num;
				paddr->page_offset =  vaddr->page_offset;
				replacement_policy->move_back(replacement_policy->ll,  node);
				return 1; //return 1 if everything goes well and we hit
			}
	}
	return 0; //return 0 if we didnt hit

}


int tlb_search( const void * mem_space, const virt_addr_t * vaddr, phy_addr_t * paddr, tlb_entry_t * tlb,
                replacement_policy_t * replacement_policy, int* hit_or_miss)
{

	M_REQUIRE_NON_NULL(mem_space);
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(tlb);
	M_REQUIRE_NON_NULL(replacement_policy);
	M_REQUIRE_NON_NULL(hit_or_miss);

	*hit_or_miss = tlb_hit(vaddr, paddr, tlb, replacement_policy); //check for a hit

	if (0 == *hit_or_miss)
		{
			M_REQUIRE(page_walk(mem_space, vaddr, paddr) == ERR_NONE, ERR_MEM, "");

			tlb_entry_t * tlb_entry = malloc(sizeof(tlb_entry_t));//allocates enough space
			M_REQUIRE_NON_NULL(tlb_entry);

			M_REQUIRE(tlb_entry_init( vaddr, paddr, tlb_entry) == ERR_NONE, ERR_MEM, "");
			M_REQUIRE(tlb_insert(replacement_policy->ll->front->value, tlb_entry, tlb) == ERR_NONE, ERR_MEM, "");

			replacement_policy->move_back(replacement_policy->ll, replacement_policy->ll->front);

		}
	return ERR_NONE;
}