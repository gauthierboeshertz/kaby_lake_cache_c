#pragma once

#include "cache_mng.h"
#include "error.h"
#include "addr_mng.h"
#include <stdint.h>
#include <stdlib.h>
#include "lru.h"

#define bits_in_a_byte 8


uint32_t phy_addr_t_to_uint32_t(const phy_addr_t * paddr)
{

	M_REQUIRE_NON_NULL(paddr);

	uint32_t phy_page_num = paddr->phy_page_num << PAGE_OFFSET; //shifts to make place for the offset
	return phy_page_num | paddr->page_offset; //Adds the offset

}


//=========================================================================
#define PRINT_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE) \
    do { \
            fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: %1" PRIx8 ", TAG: 0x%03" PRIx16 ", values: ( ", \
                        cache_valid(TYPE, WAYS, LINE_INDEX, WAY), \
                        cache_age(TYPE, WAYS, LINE_INDEX, WAY), \
                        cache_tag(TYPE, WAYS, LINE_INDEX, WAY)); \
            for(int i_ = 0; i_ < WORDS_PER_LINE; i_++) \
                fprintf(OUTFILE, "0x%08" PRIx32 " ", \
                        cache_line(TYPE, WAYS, LINE_INDEX, WAY)[i_]); \
            fputs(")\n", OUTFILE); \
    } while(0)

#define PRINT_INVALID_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE) \
    do { \
            fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: -, TAG: -----, values: ( ---------- ---------- ---------- ---------- )\n", \
                        cache_valid(TYPE, WAYS, LINE_INDEX, WAY)); \
    } while(0)

#define DUMP_CACHE_TYPE(OUTFILE, TYPE, WAYS, LINES, WORDS_PER_LINE)  \
    do { \
        for(uint16_t index = 0; index < LINES; index++) { \
            foreach_way(way, WAYS) { \
                fprintf(output, "%02" PRIx8 "/%04" PRIx16 ": ", way, index); \
                if(cache_valid(TYPE, WAYS, index, way)) \
                    PRINT_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE); \
                else \
                    PRINT_INVALID_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE);\
            } \
        } \
    } while(0)

//=========================================================================
// see cache_mng.h
int cache_dump(FILE* output, const void* cache, cache_t cache_type)
{
	M_REQUIRE_NON_NULL(output);
	M_REQUIRE_NON_NULL(cache);

	fputs("WAY/LINE: V: AGE: TAG: WORDS\n", output);
	switch (cache_type)
		{
		case L1_ICACHE:
			DUMP_CACHE_TYPE(output, l1_icache_entry_t, L1_ICACHE_WAYS,
			                L1_ICACHE_LINES, L1_ICACHE_WORDS_PER_LINE);
			break;
		case L1_DCACHE:
			DUMP_CACHE_TYPE(output, l1_dcache_entry_t, L1_DCACHE_WAYS,
			                L1_DCACHE_LINES, L1_DCACHE_WORDS_PER_LINE);
			break;
		case L2_CACHE:
			DUMP_CACHE_TYPE(output, l2_cache_entry_t, L2_CACHE_WAYS,
			                L2_CACHE_LINES, L2_CACHE_WORDS_PER_LINE);
			break;
		default:
			debug_print("%d: unknown cache type", cache_type);
			return ERR_BAD_PARAMETER;
		}
	putc('\n', output);

	return ERR_NONE;
}


int cache_entry_init(const void * mem_space,
                     const phy_addr_t * paddr,
                     void * cache_entry,
                     cache_t cache_type)
{

	M_REQUIRE_NON_NULL(mem_space);//check validity of arguments
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(cache_entry);
	M_REQUIRE(phy_addr_t_to_uint32_t(paddr) % (4 * sizeof(word_t)) == 0, ERR_BAD_PARAMETER, "Wrong physical address");
	uint32_t tag = 0;

	switch (cache_type)
		{
		case L1_ICACHE:
			cache_init(l1_icache_entry_t, L1_ICACHE_TAG_REMAINING_BITS);
			return ERR_NONE;
		case L1_DCACHE:
			cache_init(l1_dcache_entry_t, L1_DCACHE_TAG_REMAINING_BITS);
			return ERR_NONE;
		case L2_CACHE:
			cache_init(l2_cache_entry_t, L2_CACHE_TAG_REMAINING_BITS);
			return ERR_NONE;
		default:
			return ERR_BAD_PARAMETER;
		}

	return 0;
}


int cache_flush(void *cache, cache_t cache_type)
{
	M_REQUIRE_NON_NULL(cache);//check validity of arguments
	int i;

	switch (cache_type)
		{
		case L1_ICACHE:
			flush(l1_icache_entry_t, L1_ICACHE_WAYS, L1_ICACHE_LINES, L1_ICACHE_WORDS_PER_LINE);
			return ERR_NONE;
		case L1_DCACHE:
			flush(l1_dcache_entry_t, L1_DCACHE_WAYS, L1_DCACHE_LINES, L1_DCACHE_WORDS_PER_LINE);
			return ERR_NONE;
		case L2_CACHE:
			flush(l2_cache_entry_t, L2_CACHE_WAYS, L2_CACHE_LINES, L2_CACHE_WORDS_PER_LINE);
			return ERR_NONE;
		default:
			return ERR_BAD_PARAMETER;
		}


}



int cache_insert(uint16_t cache_line_index,
                 uint8_t cache_way,
                 const void * cache_line_in,
                 void * cache,
                 cache_t cache_type)
{

	M_REQUIRE_NON_NULL(cache_line_in);//check validity of arguments
	M_REQUIRE_NON_NULL(cache);

	switch (cache_type)
		{
		case L1_ICACHE:
			insert(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_WAYS, L1_ICACHE_WORDS_PER_LINE);
			return ERR_NONE;

		case L1_DCACHE:
			insert(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_WAYS, L1_DCACHE_WORDS_PER_LINE);
			return ERR_NONE;

		case L2_CACHE:
			insert(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_WAYS, L2_CACHE_WORDS_PER_LINE);
			return ERR_NONE;

		default:
			return ERR_BAD_PARAMETER;
		}
}



int cache_hit (const void * mem_space,
               void * cache,
               phy_addr_t * paddr,
               const uint32_t ** p_line,
               uint8_t *hit_way,
               uint16_t *hit_index,
               cache_t cache_type)
{

	M_REQUIRE_NON_NULL(mem_space);//check validity of arguments
	M_REQUIRE_NON_NULL(cache);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(p_line);
	M_REQUIRE_NON_NULL(hit_way);
	M_REQUIRE_NON_NULL(hit_index);


	uint32_t line_index = 0;
	uint32_t tag = 0;
	switch (cache_type)
		{
		case L1_ICACHE:
			hit(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_TAG_REMAINING_BITS, L1_ICACHE_WAYS);
		case L1_DCACHE:
			hit(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_TAG_REMAINING_BITS, L1_DCACHE_WAYS);
		case L2_CACHE:
			hit(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_TAG_REMAINING_BITS, L2_CACHE_WAYS);
		default:
			return 0;
		}
}

int index_from_l2_to_l1(void * cache, void * entry,  cache_t cache_type, uint8_t way, uint16_t index )
{

	switch (cache_type)
		{
		case L1_ICACHE :
			index_from_l2_to_l1_(l1_icache_entry_t, L1_ICACHE_WORDS_PER_LINE);
			break;

		case L1_DCACHE:
			index_from_l2_to_l1_(l1_dcache_entry_t, L1_DCACHE_WORDS_PER_LINE);
			break;
		default :
			return ERR_BAD_PARAMETER;
		}
	return ERR_NONE;

}


uint8_t apply_lru(void* cache , cache_t type , uint32_t line_index)
{

	uint8_t max_age = 0;
	uint8_t way = 0;
	if (type == L1_ICACHE)
		{
			lru(l1_icache_entry_t, L1_ICACHE_WAYS);
		}
	if (type == L1_DCACHE)
		{
			lru(l1_dcache_entry_t, L1_DCACHE_WAYS);
		}
	if (type == L2_CACHE)
		{
			lru(l2_cache_entry_t, L2_CACHE_WAYS);
		}

	return way;
}


uint8_t  find_place( void * cache, cache_t type, uint32_t line_index )
{

	switch (type)
		{
		case L1_ICACHE :
			find_place_(l1_icache_entry_t, L1_ICACHE_WAYS);
			break;

		case L1_DCACHE :
			find_place_(l1_dcache_entry_t, L1_DCACHE_WAYS);
			break;

		case L2_CACHE :
			find_place_(l2_cache_entry_t, L2_CACHE_WAYS);
			break;
		default :
			return HIT_WAY_MISS ;

		}


	return HIT_WAY_MISS ;


}


int cache_read(const void * mem_space, phy_addr_t * paddr, mem_access_t access, void * l1_cache, void * l2_cache, uint32_t * word, cache_replace_t replace)
{


	M_REQUIRE_NON_NULL(mem_space);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(l1_cache);
	M_REQUIRE_NON_NULL(l2_cache);
	M_REQUIRE_NON_NULL(word);

	uint32_t addr = phy_addr_t_to_uint32_t(paddr);
	uint8_t word_select = (addr & 0b1111) >> 2;
	const uint32_t * p_line = calloc(L1_ICACHE_WORDS_PER_LINE, sizeof(word_t));
	M_REQUIRE_NON_NULL(p_line);

	uint8_t hit_way = 0 ;
	uint16_t hit_index = 0;
	M_REQUIRE(access == INSTRUCTION || access == DATA, ERR_BAD_PARAMETER, "Wrong access");
	cache_t cache_type = access == INSTRUCTION ? L1_ICACHE : L1_DCACHE;
	void * cache = l1_cache;


	M_REQUIRE(cache_hit(mem_space, l1_cache, paddr, &p_line, &hit_way, &hit_index, cache_type) == ERR_NONE, ERR_BAD_PARAMETER, " ");

	if (hit_way != HIT_WAY_MISS)
		{

			if (cache_type == L1_ICACHE)
				{
					*word = cache_line(l1_icache_entry_t, L1_ICACHE_WAYS, hit_index, hit_way)[word_select];
				}
			if (cache_type == L1_DCACHE)
				{
					*word = cache_line(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_index, hit_way)[word_select];
				}
		}

	else
		{

			uint16_t indexl2 = 0;
			M_REQUIRE(cache_hit(mem_space, l2_cache, paddr, &p_line, &hit_way, &indexl2, L2_CACHE) == 0, ERR_BAD_PARAMETER, " ");

			uint32_t newaddr  = phy_addr_t_to_uint32_t(paddr);
			int line_1 = (newaddr & 0b1111110000) >> 4;
			int new_tag = newaddr >> L1_ICACHE_TAG_REMAINING_BITS;


			if (hit_way != HIT_WAY_MISS)
				{

					if (cache_type == L1_ICACHE)
						{
							hit_L2(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE, L1_ICACHE_WAYS);
						}
					if (cache_type == L1_DCACHE)
						{
							hit_L2(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE, L1_DCACHE_WAYS);

						}
				}

			else
				{
					line_1 &= 0b111111;

					if (cache_type == L1_ICACHE)
						{
							not_hit_L1_or_L2(l1_icache_entry_t, L1_ICACHE_WAYS, L1_ICACHE);

						}

					if (cache_type == L1_DCACHE)
						{

							not_hit_L1_or_L2(l1_dcache_entry_t, L1_DCACHE_WAYS, L1_DCACHE);

						}
				}
		}


	return ERR_NONE;

}
int cache_write(void * mem_space,
                phy_addr_t * paddr,
                void * l1_cache,
                void * l2_cache,
                const uint32_t * word,
                cache_replace_t replace)
{


	M_REQUIRE_NON_NULL(mem_space);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(l1_cache);
	M_REQUIRE_NON_NULL(l2_cache);
	M_REQUIRE_NON_NULL(word);

	uint32_t addr = phy_addr_t_to_uint32_t(paddr);
	uint8_t word_index = (addr & 0b1111) >> 2;

	const uint32_t * p_line = calloc(L1_DCACHE_WORDS_PER_LINE, sizeof(word_t));
	M_REQUIRE_NON_NULL(p_line);
	const uint32_t * p_line2 =  calloc(L2_CACHE_WORDS_PER_LINE, sizeof(word_t));
	M_REQUIRE_NON_NULL(p_line2);
	void * cache = l1_cache;


	uint8_t hit_way = 0;
	uint16_t hit_index = 0;
	cache_hit(mem_space, l1_cache, paddr, &p_line, &hit_way, &hit_index, L1_DCACHE);


// cas ou on a un hit dans la l1 cache
	if (hit_way != HIT_WAY_MISS)
		{
			//inserer le nouveau mot dans la cache et dans la mémoire centrale

			uint32_t * new_line = calloc(L1_DCACHE_WORDS_PER_LINE, sizeof(word_t));
			M_REQUIRE_NON_NULL(new_line);
			memcpy(new_line, p_line, 4 * sizeof(word_t));
			new_line[word_index] = *word;

			memcpy(cache_line(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_index, hit_way), new_line, 4 * sizeof(word_t));

			LRU_age_update(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_way, hit_index);
			memcpy( mem_space + phy_addr_t_to_uint32_t(paddr), new_line, 4 * sizeof(word_t));

			return ERR_NONE;
		}

	uint8_t hit_way2 = 0;
	uint16_t hit_index2 = 0;
	cache_hit(mem_space, l2_cache, paddr, &p_line2, &hit_way2, &hit_index2, L1_DCACHE);


// cas ou il y a un hit dans le deuxieme cache, écrire dans la mémoire centrale et dans le l2cache, puis insérer dans la l1cache
	if (hit_way2 != HIT_WAY_MISS)
		{

			uint32_t * new_line = calloc(L2_CACHE_WORDS_PER_LINE, sizeof(word_t));
			M_REQUIRE_NON_NULL(new_line);
			memcpy(new_line, p_line2, 4 * sizeof(word_t));
			new_line[word_index] = *word;
			cache = l2_cache;
			memcpy(cache_line(l2_cache_entry_t, L2_CACHE_WAYS, hit_index2, hit_way2), new_line, 4 * sizeof(word_t));


			cache = l2_cache;
			LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS, hit_way2, hit_index2);

			l1_dcache_entry_t *  newentry = malloc(sizeof(l1_dcache_entry_t));
			M_REQUIRE_NON_NULL(newentry);
			index_from_l2_to_l1(l2_cache, newentry, L1_DCACHE , hit_way2, hit_index2);

			uint16_t indexL1 = hit_index2  & 0b111111;
			uint8_t wayL1 = find_place(l1_cache, L1_DCACHE, hit_index2);
			cache = l1_cache;

			memcpy(mem_space + phy_addr_t_to_uint32_t(paddr), new_line , 4 * sizeof(word_t));
//pendant l'insertion dans le l1 rechercher s"il y a un cold start.

			if ( wayL1 != HIT_WAY_MISS)
				{
					cache_insert(indexL1, wayL1, newentry, l1_cache, L1_DCACHE);
					LRU_age_increase(l1_dcache_entry_t, L1_DCACHE_WAYS, wayL1, indexL1);
					return ERR_NONE;

				}
			else
				{
// s'il n'y a pas de cold start alors sortir la plus ancienne entrée du l1  à cet index et l'inserer dans le l2
					uint8_t waydel = apply_lru(l1_cache, L1_DCACHE, indexL1);

					l2_cache_entry_t * old_entry = malloc(sizeof(l2_cache_entry_t));
					old_entry->v = VALID;
					old_entry->tag = (addr >> L2_CACHE_TAG_REMAINING_BITS);
					old_entry->age = 0;

					memcpy(old_entry->line, cache_line(l1_dcache_entry_t, L1_DCACHE_WAYS, indexL1, wayL1), 4 * sizeof(word_t));

					cache_insert(indexL1, waydel, newentry, l1_cache, L1_DCACHE);
					LRU_age_update(l1_dcache_entry_t, L1_DCACHE_WAYS, waydel, indexL1);

					uint8_t wayaddl2 = find_place(l2_cache, L2_CACHE, hit_index2);
					cache = l2_cache;

// cherche un cold start dans le l2
					if (wayaddl2 != HIT_WAY_MISS)
						{
							cache_insert(hit_index2, wayaddl2, old_entry, l2_cache, L2_CACHE);
							LRU_age_increase(l2_cache_entry_t, L2_CACHE_WAYS, wayaddl2, hit_index2);

						}
					else
						{
							//s'il n'y en a pas sortir l'entrée la plus ancienne
							uint8_t waydel2 = apply_lru(l2_cache, L2_CACHE, hit_index2);
							cache_insert(hit_index2, waydel2, old_entry, l2_cache, L2_CACHE);
							LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS, waydel2, hit_index2);
						}



				}

			return ERR_NONE;
		}
	else
		{
// cas ou il n'y a eu aucun hit. crée une nouvelle entrée pour la mettre dans la l1cache et écrire dans la mémoire


			uint32_t  * mem_line = calloc(L1_DCACHE_WORDS_PER_LINE, sizeof(word_t));
			memcpy( mem_line, mem_space + phy_addr_t_to_uint32_t(paddr), 4 * sizeof(word_t));
			mem_line[word_index] = *word;
			memcpy( mem_space + phy_addr_t_to_uint32_t(paddr), mem_line, 4 * sizeof(word_t));
			l1_dcache_entry_t * newd = malloc(sizeof(l1_dcache_entry_t));
			cache_entry_init(mem_space, paddr, newd, L1_DCACHE);
			uint16_t newindex = (addr >> 4) & 0b111111;
			uint8_t wayld = find_place(l1_cache, L1_DCACHE, newindex);
			cache = l1_cache;
// regarde s'il y a une cold start dans le l1 cache
			if (wayld != HIT_WAY_MISS)
				{
					cache_insert(newindex, wayld, newd, l1_cache, L1_DCACHE);
					LRU_age_increase(l1_dcache_entry_t, L1_DCACHE_WAYS, wayld, newindex);
					return ERR_NONE;
				}

			else
				{
// s'il n'y a pas de cold start trouver la plus ancienne entrée du l1 et la remplacer par la nouvelle
					uint8_t waydel2 = apply_lru(l1_cache, L1_DCACHE, newindex);



					l2_cache_entry_t * old_entry = malloc(sizeof(l2_cache_entry_t));

					memcpy(old_entry->line, cache_entry(l1_dcache_entry_t, L1_DCACHE_WAYS, newindex, waydel2)->line, 4 * sizeof(word_t));

					old_entry->age = 0;
					old_entry->v = INVALID;
					old_entry->tag = (cache_tag(l1_dcache_entry_t, L1_DCACHE_WAYS, newindex, waydel2) >> 3);


					cache_insert(newindex, waydel2, newd, l1_cache, L1_DCACHE);

					LRU_age_update(l1_dcache_entry_t, L1_DCACHE_WAYS, waydel2, newindex);

					uint16_t l2index = (phy_addr_t_to_uint32_t(paddr) >> 4) & 0b111111111;
					uint8_t l2way = find_place(l2_cache, L2_CACHE, l2index);
// inserer l'entrée sortie dans le l2

					cache = l2_cache;
					if (l2way != HIT_WAY_MISS)
						{
							cache_insert(l2index, l2way, old_entry, l2_cache, L2_CACHE);
							LRU_age_increase(l2_cache_entry_t, L2_CACHE_WAYS, l2way, l2index);
						}
					else
						{

							uint8_t way2 = apply_lru(l2_cache, L2_CACHE, l2index);

							cache_insert(l2index, way2, old_entry, l2_cache, L2_CACHE);

							LRU_age_update(l2_cache_entry_t, L2_CACHE_WAYS, way2, l2index);
						}
				}

			return ERR_NONE;

		}
	return ERR_NONE;
}





int cache_read_byte(const void * mem_space,
                    phy_addr_t * p_paddr,
                    mem_access_t access,
                    void * l1_cache,
                    void * l2_cache,
                    uint8_t * p_byte,
                    cache_replace_t replace)
{

	M_REQUIRE_NON_NULL(mem_space);
	M_REQUIRE_NON_NULL(p_paddr);
	M_REQUIRE_NON_NULL(l1_cache);
	M_REQUIRE_NON_NULL(l2_cache);
	M_REQUIRE_NON_NULL(p_byte);
//trouve l'index du byte à enlever
	uint8_t byte_select = phy_addr_t_to_uint32_t(p_paddr) & 0b11;
	word_t * word = malloc(sizeof(word_t));
	M_REQUIRE_NON_NULL(word);
	// met la valeur dans le mot

	int err = cache_read(mem_space, p_paddr, access, l1_cache, l2_cache, word, replace);
	if (err != ERR_NONE)
		{
			return ERR_BAD_PARAMETER;
		}

	// met la valeur du byte dans le byte.
	*p_byte = (*word >> (byte_select * bits_in_a_byte)) & 0b11111111;

	return ERR_NONE;
}

int cache_write_byte(void * mem_space,
                     phy_addr_t * paddr,
                     void * l1_cache,
                     void * l2_cache,
                     uint8_t* p_byte,
                     cache_replace_t replace)
{

	M_REQUIRE_NON_NULL(mem_space);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(l1_cache);
	M_REQUIRE_NON_NULL(l2_cache);
	M_REQUIRE_NON_NULL(p_byte);

// trouve l'index du byte a changer

	uint8_t byte_select = phy_addr_t_to_uint32_t(paddr) & 0b11;
	word_t * word = malloc(sizeof(word_t));
	M_REQUIRE_NON_NULL(word);


	M_REQUIRE(cache_read(mem_space, paddr, DATA, l1_cache, l2_cache, word, replace) == ERR_NONE, ERR_BAD_PARAMETER, " ");

// met le byte dans le word à inserer
	*word |=  (*p_byte << (bits_in_a_byte * byte_select));
//insere le word dans la cache.
	M_REQUIRE(cache_write(mem_space, paddr, l1_cache, l2_cache, word, replace) == ERR_NONE, ERR_BAD_PARAMETER, " ");

	return ERR_NONE;



}