#pragma once

/**
 * @file cache.h
 * @brief definitions associated to a a two-level hierarchy of cache memories
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */

#include <stdint.h>
#include "addr.h"

#define L1_ICACHE_WORDS_PER_LINE 4
#define L1_ICACHE_LINE   16u // 16 bytes (4 words) per line
#define L1_ICACHE_WAYS   4u
#define L1_ICACHE_LINES  64u  // Do not modify this!
#define L1_ICACHE_TAG_REMAINING_BITS   10 // 2(select byte) + 2(select word) + 6(select line)
#define L1_ICACHE_TAG_BITS             22 // 32 - L1_ICACHE_TAG_REMAINING_BITS

#define L1_DCACHE_WORDS_PER_LINE L1_ICACHE_WORDS_PER_LINE
#define L1_DCACHE_LINE   L1_ICACHE_LINE
#define L1_DCACHE_WAYS   L1_ICACHE_WAYS
#define L1_DCACHE_LINES  L1_ICACHE_LINES
#define L1_DCACHE_TAG_REMAINING_BITS L1_ICACHE_TAG_REMAINING_BITS
#define L1_DCACHE_TAG_BITS           L1_ICACHE_TAG_BITS

#define L2_CACHE_WORDS_PER_LINE L1_ICACHE_WORDS_PER_LINE
#define L2_CACHE_LINE   L1_ICACHE_LINE
#define L2_CACHE_WAYS   8u
#define L2_CACHE_LINES  512u  // Do not modify this!
#define L2_CACHE_TAG_REMAINING_BITS   13 // 2(select byte) + 2(select word) + 9(select line)
#define L2_CACHE_TAG_BITS             19 // 32 - L1_ICACHE_TAG_REMAINING_BITS

#define VALID 1
#define INVALID 0


/**
 * L1 ICACHE, L1 DCACHE:
 *  - byte addressing
 *  - physically addressed
 *  - 4-way set-associative
 *  - 4 words/way, where word = 4 bytes (=> 128 bits/way)
 *  - 64 sets (= 64 blocks per way) (= 6 bits to index)
 *  - total capacity = 4kiB
 *  - write-through policy (no dirty bit)
 *  - write-allocate on write miss
 *
 * L2 CACHE:
 *  - byte addressing
 *  - physically addressed
 *  - 8-way set-associative
 *  - 4 words/way, where word = 4 bytes (=> 128 bits/way)
 *  - 512 sets (= 512 blocks per way) (= 9 bits to index)
 *  - total capacity = 64kiB
 *  - write-through policy (no dirty bit)
 *  - write-allocate on write miss
 *
 *  Exclusive policy (https://en.wikipedia.org/wiki/Cache_inclusion_policy)
 *      Consider the case when L2 is exclusive of L1. Suppose there is a
 *      processor read request for block X. If the block is found in L1 cache,
 *      then the data is read from L1 cache and returned to the processor. If
 *      the block is not found in the L1 cache, but present in the L2 cache,
 *      then the cache block is moved from the L2 cache to the L1 cache. If
 *      this causes a block to be evicted from L1, the evicted block is then
 *      placed into L2. This is the only way L2 gets populated. Here, L2
 *      behaves like a victim cache. If the block is not found in both L1 and
 *      L2, then it is fetched from main memory and placed just in L1 and not
 *      in L2.
 *
 */
typedef struct
{

	uint8_t v: 1;
	uint8_t age: 2;
	uint32_t tag: 22;
	word_t line[L1_ICACHE_WORDS_PER_LINE];

} l1_icache_entry_t;

typedef struct
{

	uint8_t v: 1;
	uint8_t age: 2;
	uint32_t tag: 22;
	word_t line[L1_ICACHE_WORDS_PER_LINE];

} l1_dcache_entry_t;

typedef struct
{

	uint8_t v: 1;
	uint8_t age: 3;
	uint32_t tag: 19;
	word_t line[L2_CACHE_WORDS_PER_LINE];

} l2_cache_entry_t;

typedef enum
{
	L1_ICACHE, L1_DCACHE, L2_CACHE
} cache_t;

// --------------------------------------------------
#define cache_cast(TYPE) ((TYPE *)cache)

// --------------------------------------------------
#define cache_entry(TYPE, WAYS, LINE_INDEX, WAY) \
        (cache_cast(TYPE) + (LINE_INDEX) * (WAYS) + (WAY))

// --------------------------------------------------
#define cache_valid(TYPE, WAYS, LINE_INDEX, WAY) \
        cache_entry(TYPE, WAYS, LINE_INDEX, WAY)->v

// --------------------------------------------------
#define cache_age(TYPE, WAYS, LINE_INDEX, WAY) \
        cache_entry(TYPE, WAYS, LINE_INDEX, WAY)->age

// --------------------------------------------------
#define cache_tag(TYPE, WAYS, LINE_INDEX, WAY) \
        cache_entry(TYPE, WAYS, LINE_INDEX, WAY)->tag

// --------------------------------------------------
#define cache_line(TYPE, WAYS, LINE_INDEX, WAY) \
        cache_entry(TYPE, WAYS, LINE_INDEX, WAY)->line


#define hit_L2(TYPE,LINES,TYPE2,WAYS)\
TYPE *  newentry = malloc(sizeof(TYPE));\
M_REQUIRE_NON_NULL(newentry);\
newentry->tag = new_tag;\
M_REQUIRE(index_from_l2_to_l1(l2_cache, newentry, cache_type , hit_way, indexl2) == 0, ERR_BAD_PARAMETER, " ");\
cache_valid(l2_cache_entry_t, L2_CACHE_LINES, indexl2, hit_way) = INVALID;\
uint8_t nway = find_place(l1_cache, cache_type, line_1);/*   if there is a cold start nway != HIT_WAY_MISS so insert then age increase else apply the lru */\
if ( nway != HIT_WAY_MISS )\
{\
M_REQUIRE(cache_insert(line_1, nway, newentry, l1_cache, cache_type) == ERR_NONE, ERR_BAD_PARAMETER, " ");\
LRU_age_increase(TYPE, WAYS, nway, line_1);\
}\
else\
{/* if there is no place in L1 we need to evict an entry and put it in the L2 cache*/\
uint8_t waydel = apply_lru(l1_cache, TYPE2, line_1);\
TYPE  old_entry = *cache_entry(TYPE, LINES, line_1, waydel);\
cache_insert(line_1, waydel, newentry, l1_cache, TYPE2 );\
LRU_age_update(TYPE, WAYS, waydel, line_1);\
uint8_t l2way = find_place(l2_cache, L2_CACHE, indexl2);\
*word = cache_line(TYPE, WAYS, line_1, waydel)[word_select];\
cache = l2_cache; /* when inserting in the l2 if there is a cold start directly insert  into the l2 else search with the LRU*/\
if (l2way != HIT_WAY_MISS)\
{\
cache_insert(indexl2, l2way, &old_entry, l2_cache, L2_CACHE );\
LRU_age_increase(l2_cache_entry_t, L2_CACHE_WAYS, l2way, indexl2 );\
}\
else\
{\
uint8_t waydel = apply_lru(l2_cache, L2_CACHE, indexl2);\
M_REQUIRE(cache_insert(indexl2, waydel, &old_entry, l2_cache, L2_CACHE ) == ERR_NONE, ERR_BAD_PARAMETER, " ");\
LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS, waydel, indexl2);\
}\
}\


#define not_hit_L1_or_L2(TYPE,WAYS,TYPE2)\
TYPE* newentry = malloc(sizeof(TYPE));\
M_REQUIRE_NON_NULL(newentry);\
cache_entry_init(mem_space, paddr, newentry, TYPE2); \
uint8_t nway = find_place(l1_cache, TYPE2, line_1);/* search in the l1 cache if there is an invalid entry, if there is insert at this place*/\
if (nway == HIT_WAY_MISS)/* if there is no valid place then search with the LRU policy*/\
	{\
		nway = apply_lru(l1_cache, cache_type, line_1); \
	}\
cache_insert(line_1, nway, newentry, l1_cache, TYPE2 );/* insert the entry at the right way in the l1 */\
LRU_age_update(TYPE, WAYS, nway, line_1); \
*word = cache_line(TYPE, WAYS, line_1, nway)[word_select];/* assign the word*/\


#define find_place_(TYPE,WAYS)/* find  an invalid entry*/\
foreach_way(i, WAYS)\
{\
if (cache_valid(TYPE, WAYS, line_index, i) == 0)\
{\
return i;\
}\
}\


#define index_from_l2_to_l1_(TYPE,WORDS)/* assign the line of the entry in l2 to the line of the entry in the l1 cache and make L1 entry valid */\
for (int i = 0; i < WORDS; ++i )\
{\
((TYPE *)entry)->line[i] = cache_line(l2_cache_entry_t, L2_CACHE_LINES, index, way)[i];\
}\
((TYPE *)entry)->v = VALID;\


#define lru(TYPE,WAYS)\
foreach_way(i, WAYS)\
			{\
				if (cache_age(TYPE, WAYS, line_index, i) >= max_age)\
					{\
						max_age = cache_age(TYPE, WAYS, line_index, i);\
						way = i;\
					}\
			}\




#define hit(TYPE,LINES,BITS,WAYS)/* macro for the hit*/\
line_index = (phy_addr_t_to_uint32_t(paddr) / 16) % LINES;\
tag = phy_addr_t_to_uint32_t(paddr) >> BITS ;\
foreach_way(i, WAYS){/* if there is an invalid entry this is a miss*/\
if (cache_valid(TYPE, WAYS, line_index, i) == INVALID){\
*hit_way = HIT_WAY_MISS; \
*hit_index = HIT_INDEX_MISS; \
return ERR_NONE; }/* if there is an entry whose tag matches that of the physical address and is valid then we hit, assign the way of the hit to hit_way*/\
if (cache_valid(TYPE, WAYS, line_index, i)==VALID && tag == cache_tag(TYPE, WAYS, line_index, i)){\
*hit_way = i; \
*hit_index = line_index; \
*p_line = cache_line(TYPE, WAYS,  line_index, i ); \
LRU_age_update(TYPE, WAYS, *hit_way, *hit_index); \
return ERR_NONE;}}\
*hit_way = HIT_WAY_MISS;\
*hit_index = HIT_INDEX_MISS;\
return 0;


#define insert(TYPE,LINES,WAYS,WORDS)/* insert a line in a given cache assign the values of the entry in the cache to be those of the entry*/\
if (cache_line_index >= LINES || cache_way >= WAYS)\
return ERR_NONE;\
(memcpy(cache_line(TYPE, WAYS, cache_line_index, cache_way), (*(TYPE*)cache_line_in).line, WORDS * sizeof(word_t)));\
cache_valid(TYPE, WAYS, cache_line_index, cache_way) = (*(TYPE*)cache_line_in).v;\
cache_age(TYPE, WAYS, cache_line_index, cache_way) = (*(TYPE*)cache_line_in).age;\
cache_tag(TYPE, WAYS, cache_line_index, cache_way) = (*(TYPE*)cache_line_in).tag;\



#define flush(TYPE,WAYS,LINES,WORDS)/*   put all the values to 0*/\
for (i = 0; i < WAYS * LINES; ++i)\
((TYPE*)cache)[i].v = INVALID;\
((TYPE*)cache)[i].tag = 0;\
((TYPE*)cache)[i].age = 0;\
(memset(((TYPE *)cache)->line, 0, WORDS * sizeof(word_t)))\


#define cache_init(TYPE,BITS) /* initialise the entry put the validity bit to 1, age to 0, assign the tag with the tag in the physical address and for the line assign with the values found in the memory*/\
tag = phy_addr_t_to_uint32_t(paddr) >> BITS;\
((TYPE*)cache_entry)->v = VALID;\
((TYPE *)cache_entry)->age = 0;\
((TYPE *)cache_entry)->tag = tag;\
(memcpy(((TYPE *)cache_entry)->line, mem_space + phy_addr_t_to_uint32_t(paddr), 4 * sizeof(word_t)));\



