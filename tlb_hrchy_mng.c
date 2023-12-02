#include "tlb_hrchy_mng.h"
#include "stdlib.h"
#include "error.h"
#include "addr.h"
#include "memory.h"
#include "addr_mng.h"
#include "page_walk.h"



#define init(TYPE,LINES) \
((TYPE *)tlb_entry)->tag =  (virtual_page_number >> LINES);\
((TYPE *)tlb_entry)->phy_page_num = phy_page_num;\
((TYPE *)tlb_entry)->v = 1;\
return ERR_NONE;

int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    void * tlb_entry,
                    tlb_t tlb_type)
{

    M_REQUIRE_NON_NULL(vaddr);//check validity of arguments
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(tlb_entry);

    uint64_t phy_page_num = paddr->phy_page_num;
    uint64_t virtual_page_number = virt_addr_t_to_virtual_page_number(vaddr);

    switch (tlb_type)
        {
        case L1_ITLB:
            init(l1_itlb_entry_t, L1_ITLB_LINES_BITS);
        case L1_DTLB:
            init(l1_dtlb_entry_t, L1_DTLB_LINES_BITS);
        case L2_TLB:
            init(l2_tlb_entry_t, L2_TLB_LINES_BITS);
        default:
            return ERR_BAD_PARAMETER;
        }

}

#define flush(TYPE,LINES)\
for (int i = 0; i < LINES; ++i){\
                    ((TYPE *)tlb)[i].v = 0;\
                    ((TYPE *)tlb)[i].tag = 0;\
                    ((TYPE *)tlb)[i].phy_page_num = 0;\
                }\
return ERR_NONE;\

int tlb_flush(void *tlb, tlb_t tlb_type)
{

    M_REQUIRE_NON_NULL(tlb);//check validity of arguments

    switch (tlb_type)
        {
        case L1_ITLB:
            flush(l1_itlb_entry_t, L1_ITLB_LINES);
        case L1_DTLB:
            flush(l1_dtlb_entry_t, L1_DTLB_LINES);
        case L2_TLB:
            flush(l2_tlb_entry_t, L2_TLB_LINES);
        default:
            return ERR_BAD_PARAMETER;
        }

}


#define insert(TYPE,LINES)\
if (line_index >= LINES)\
                {\
                    return ERR_BAD_PARAMETER;\
                }\
            ((TYPE *)tlb)[line_index] = *((TYPE *)tlb_entry);\
            return ERR_NONE;\


int tlb_insert( uint32_t line_index,
                const void * tlb_entry,
                void * tlb,
                tlb_t tlb_type)
{

    M_REQUIRE_NON_NULL(tlb);//check validity of arguments
    M_REQUIRE_NON_NULL(tlb_entry);


    switch (tlb_type)
        {
        case L1_ITLB:
            insert(l1_itlb_entry_t, L1_ITLB_LINES);
        case L1_DTLB:
            insert(l1_dtlb_entry_t, L1_DTLB_LINES);
        case L2_TLB:
            insert(l2_tlb_entry_t, L2_TLB_LINES);
        default:
            return ERR_BAD_PARAMETER;
        }


}

#define hit(TYPE,LINES,BITS)\
line_index = virt_page_num % LINES;\
tag = virt_page_num >> BITS;\
if (((TYPE *)tlb)[line_index].v && tag == ((TYPE *)tlb)[line_index].tag )\
    {\
        paddr->phy_page_num =  ((TYPE *)tlb)[line_index].phy_page_num ;\
        paddr->page_offset =  vaddr->page_offset;\
        return 1;\
    }\
return 0;\

int tlb_hit( const virt_addr_t * vaddr,
             phy_addr_t * paddr,
             const void  * tlb,
             tlb_t tlb_type)
{

    M_REQUIRE_NON_NULL(vaddr);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(tlb);

    uint64_t virt_page_num = virt_addr_t_to_virtual_page_number(vaddr);
    uint64_t line_index = 0;
    uint64_t tag = 0;

    switch (tlb_type)
        {
        case L1_ITLB:
            hit(l1_itlb_entry_t, L1_ITLB_LINES, L1_ITLB_LINES_BITS);
        case L1_DTLB:
            hit(l1_dtlb_entry_t, L1_DTLB_LINES, L1_DTLB_LINES_BITS);
        case L2_TLB:
            hit(l2_tlb_entry_t, L2_TLB_LINES, L2_TLB_LINES_BITS);
        default:
            return 0;
        }
}



#define search_invalid(L1,LINES)\
tl1_vaddr = (L1[tl1_index].tag << LINES) | tl1_index;\
if ( L1[tl1_index].v == 1  && tl1_vaddr == tl2_vaddr)\
    {\
        L1[tl1_index].v = 0;\
    }\
return ERR_NONE;\

int tlb_search_invalid(l1_itlb_entry_t* l1_itlb,
                       l1_dtlb_entry_t* l1_dtlb,
                       uint64_t virt_page_number,
                       mem_access_t access,
                       l2_tlb_entry_t* l2_tlb)
{

    M_REQUIRE_NON_NULL(l1_dtlb);//checks validity of the arguments
    M_REQUIRE_NON_NULL(l1_itlb);
    M_REQUIRE_NON_NULL(l2_tlb);



    uint8_t tl2_index = virt_page_number & 0b111111; //apply mask
    uint32_t tl2_vaddr = (l2_tlb[tl2_index].tag << L2_TLB_LINES_BITS) | tl2_index;
    uint8_t tl1_index  =  virt_page_number % L1_ITLB_LINES;
    uint32_t tl1_vaddr;

    switch (access)
        {
        case INSTRUCTION:
            search_invalid(l1_dtlb, L1_DTLB_LINES_BITS);
        case DATA:
            search_invalid(l1_itlb, L1_ITLB_LINES_BITS);
        default:
            return ERR_BAD_PARAMETER;
        }


}


int tlb_search( const void * mem_space,
                const virt_addr_t * vaddr,
                phy_addr_t * paddr,
                mem_access_t access,
                l1_itlb_entry_t * l1_itlb,
                l1_dtlb_entry_t * l1_dtlb,
                l2_tlb_entry_t * l2_tlb,
                int* hit_or_miss)
{

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(vaddr);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(l1_itlb);
    M_REQUIRE_NON_NULL(l1_dtlb);
    M_REQUIRE_NON_NULL(l2_tlb);
    M_REQUIRE_NON_NULL(hit_or_miss);


    if (access == INSTRUCTION)
        {
            *hit_or_miss = tlb_hit(vaddr, paddr, l1_itlb, L1_ITLB); //assigns value to hit or miss
        }
    else if (access == DATA)
        {
            *hit_or_miss = tlb_hit(vaddr, paddr, l1_dtlb, L1_DTLB);//assigns value to hit or miss
        }
    if (*hit_or_miss)
        {
            return ERR_NONE; //returns if it is a hit
        }

    *hit_or_miss = tlb_hit(vaddr, paddr, l2_tlb, L2_TLB);
    uint64_t virt_page_num = virt_addr_t_to_virtual_page_number(vaddr); //extract virtual page number

    if (*hit_or_miss)
        {
            uint64_t line_index = 0;

            if (access == INSTRUCTION)
                {

                    line_index = virt_page_num % L1_ITLB_LINES; //computes line index
                    void* new_tlb =  malloc(sizeof(l1_itlb_entry_t));//Allocates enough space
                    M_REQUIRE_NON_NULL(new_tlb);
                    tlb_entry_init(vaddr, paddr, new_tlb, L1_ITLB);//initializes the new tlb
                    tlb_insert(line_index, new_tlb, l1_itlb, L1_ITLB);//inserts it at the corresponding place
                    return ERR_NONE;

                }

            else if (access == DATA)
                {

                    line_index = virt_page_num % L1_DTLB_LINES;//computes line index
                    void* new_tlb =  malloc(sizeof(l1_itlb_entry_t));//Allocates enough space
                    M_REQUIRE_NON_NULL(new_tlb);
                    tlb_entry_init(vaddr, paddr, new_tlb, L1_ITLB);//initializes the new tlb
                    tlb_insert(line_index, new_tlb, l1_itlb, L1_ITLB);//inserts it at the corresponding place

                    return ERR_NONE;
                }
        }

    else
        {

            *hit_or_miss = 0;//sets hit or miss to 0
            M_REQUIRE(page_walk(mem_space, vaddr, paddr) == ERR_NONE, ERR_BAD_PARAMETER, " ",);
            M_REQUIRE(tlb_search_invalid(l1_itlb, l1_dtlb, virt_page_num,  access, l2_tlb) == ERR_NONE, ERR_BAD_PARAMETER, "BAd parameter"); //call the invalidation function


            void* new_tlb =  malloc(sizeof(l2_tlb_entry_t));//Allocates enough space
            M_REQUIRE_NON_NULL(new_tlb);
            tlb_entry_init(vaddr, paddr, new_tlb, L2_TLB);//initializes the new l2_tlb
            uint64_t line_index = virt_page_num % L2_TLB_LINES;
            tlb_insert(line_index, new_tlb, l2_tlb, L2_TLB);//inserts it at the corresponding place

            if (access == INSTRUCTION)
                {
                    void* new_itlb = malloc(sizeof(l1_itlb_entry_t));//Allocates enough space
                    M_REQUIRE_NON_NULL(new_itlb);
                    tlb_entry_init(vaddr, paddr, new_itlb, L1_ITLB);//initializes the new itlb
                    uint8_t line_index = virt_page_num % L1_ITLB_LINES;
                    tlb_insert(line_index, new_itlb, l1_itlb, L1_ITLB);
                }
            else if (access == DATA)
                {
                    void* new_dtlb = malloc(sizeof(l1_dtlb_entry_t));//Allocates enough space
                    M_REQUIRE_NON_NULL(new_dtlb);
                    tlb_entry_init(vaddr, paddr, new_dtlb, L1_DTLB);//initializes the new dtlb
                    uint8_t line_index = virt_page_num % L1_DTLB_LINES;
                    tlb_insert(line_index, new_dtlb, l1_dtlb, L1_DTLB);//inserts it at the corresponding place
                }

        }
    return ERR_NONE;

}



