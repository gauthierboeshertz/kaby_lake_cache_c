/**
 * @memory.c
 * @brief memory management functions (dump, init from file, etc.)
 *
 * @date 2018-19
 */

#if defined _WIN32  || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "memory.h"
#include "page_walk.h"
#include "addr_mng.h"
#include "util.h" // for SIZE_T_FMT
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset()
#include <inttypes.h> // for SCNx macros
#include <assert.h>

// ======================================================================
/**
 * @brief Tool function to print an address.
 *
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param reference the reference address; i.e. the top of the main memory
 * @param addr the address to be displayed
 * @param sep a separator to print after the address (and its colon, printed anyway)
 *
 */
static void address_print(addr_fmt_t show_addr, const void* reference,
                          const void* addr, const char* sep)
{
    switch (show_addr)
        {
        case POINTER:
            (void)printf("%p", addr);
            break;
        case OFFSET:
            (void)printf("%zX", (const char*)addr - (const char*)reference);
            break;
        case OFFSET_U:
            (void)printf(SIZE_T_FMT, (const char*)addr - (const char*)reference);
            break;
        default:
            // do nothing
            return;
        }
    (void)printf(":%s", sep);
}

// ======================================================================
/**
 * @brief Tool function to print the content of a memory area
 *
 * @param reference the reference address; i.e. the top of the main memory
 * @param from first address to print
 * @param to first address NOT to print; if less that `from`, nothing is printed;
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param line_size how many memory byted to print per stdout line
 * @param sep a separator to print after the address and between bytes
 *
 */
static void mem_dump_with_options(const void* reference, const void* from, const void* to,
                                  addr_fmt_t show_addr, size_t line_size, const char* sep)
{
    assert(line_size != 0);
    size_t nb_to_print = line_size;
    for (const uint8_t* addr = from; addr < (const uint8_t*) to; ++addr)
        {
            if (nb_to_print == line_size)
                {
                    address_print(show_addr, reference, addr, sep);
                }
            (void)printf("%02"PRIX8"%s", *addr, sep);
            if (--nb_to_print == 0)
                {
                    nb_to_print = line_size;
                    putchar('\n');
                }
        }
    if (nb_to_print != line_size) putchar('\n');
}

// ======================================================================
// See memory.h for description
int vmem_page_dump_with_options(const void *mem_space, const virt_addr_t* from,
                                addr_fmt_t show_addr, size_t line_size, const char* sep)
{
#ifdef DEBUG
    debug_print("mem_space=%p\n", mem_space);
    (void)fprintf(stderr, __FILE__ ":%d:%s(): virt. addr.=", __LINE__, __func__);
    print_virtual_address(stderr, from);
    (void)fputc('\n', stderr);
#endif
    phy_addr_t paddr;
    zero_init_var(paddr);

    M_EXIT_IF_ERR(page_walk(mem_space, from, &paddr),
                  "calling page_walk() from vmem_page_dump_with_options()");
#ifdef DEBUG
    (void)fprintf(stderr, __FILE__ ":%d:%s(): phys. addr.=", __LINE__, __func__);
    print_physical_address(stderr, &paddr);
    (void)fputc('\n', stderr);
#endif

    const uint32_t paddr_offset = ((uint32_t) paddr.phy_page_num << PAGE_OFFSET);
    const char * const page_start = (const char *)mem_space + paddr_offset;
    const char * const start = page_start + paddr.page_offset;
    const char * const end_line = start + (line_size - paddr.page_offset % line_size);
    const char * const end   = page_start + PAGE_SIZE;
    debug_print("start=%p (offset=%zX)\n", (const void*) start, start - (const char *)mem_space);
    debug_print("end  =%p (offset=%zX)\n", (const void*) end, end   - (const char *)mem_space) ;
    mem_dump_with_options(mem_space, page_start, start, show_addr, line_size, sep);
    const size_t indent = paddr.page_offset % line_size;
    if (indent == 0) putchar('\n');
    address_print(show_addr, mem_space, start, sep);
    for (size_t i = 1; i <= indent; ++i) printf("  %s", sep);

    mem_dump_with_options(mem_space, start, end_line, NONE, line_size, sep);
    mem_dump_with_options(mem_space, end_line, end, show_addr, line_size, sep);
    return ERR_NONE;
}


int mem_init_from_dumpfile(const char* filename, void** memory, size_t* mem_capacity_in_bytes)
{


    M_REQUIRE_NON_NULL(filename);//checks that all the arguments are non null
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE_NON_NULL(mem_capacity_in_bytes);

    //opens the file as a binary file ("rb")
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
        {
            fclose(file);
            return ERR_BAD_PARAMETER;
        }

    // va tout au bout du fichier
    fseek(file, 0L, SEEK_END);
    // indique la position, et donc la taille (en octets)
    *mem_capacity_in_bytes = (size_t) ftell(file);
    // revient au début du fichier (pour le lire par la suite)
    rewind(file);

    //allocates the necessary space
    *memory = malloc(*mem_capacity_in_bytes);
    M_REQUIRE_NON_NULL(memory);

    //reads the file's data and stores it in the memory
    M_REQUIRE(fread(*memory, 1, *mem_capacity_in_bytes, file) > 0, ERR_BAD_PARAMETER, " ",);


    //closes the file
    fclose(file);

    return ERR_NONE;
}

int mem_init_from_description(const char* master_filename, void** memory,
                              size_t* mem_capacity_in_bytes)
{

    M_REQUIRE_NON_NULL(master_filename);
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE_NON_NULL(mem_capacity_in_bytes);

    FILE *file = fopen(master_filename, "r");
    if (file == NULL)
        {
            fclose(file);
            return ERR_BAD_PARAMETER;
        }

    char * line = NULL;
    size_t len = 0;

    size_t memory_size;
    char * file_name;
    int n_translation_pages;

    int address;
    char* file_name2 = calloc(128, sizeof(char));

    int counter = 0;

    while ((getline(&line, &len, file)) != -1)   //loops through the lines untils it runs out of lines
        {


            if (counter == 0)                               //enters here at the first line
                {

                    M_REQUIRE(sscanf(line, "%zu", &memory_size) > 0 , ERR_BAD_PARAMETER, " ",) ; //reads the first line
                    *memory = calloc(memory_size, 1);
                    M_REQUIRE_NON_NULL(memory);
                } //allocates enough space

            else if (counter == 1)                          //enters here at the second line
                {
                    file_name = calloc(128, sizeof(char));  //allocates enough space to the varibale that holds the name
                    M_REQUIRE(sscanf(line, "%s", file_name) > 0 , ERR_BAD_PARAMETER, " ",);      //reads the second line
                    M_REQUIRE(page_file_read(file_name, memory, 0) == 0, ERR_BAD_PARAMETER, " ",);
                }

            else if (counter == 2)                          //enters here at the third line
                {

                    M_REQUIRE(sscanf(line, "%d", &n_translation_pages) > 0, ERR_BAD_PARAMETER, " ",);
                } //reads the second line
            else if (counter < n_translation_pages + 3)     //enters here for all the translation pages
                {

                    M_REQUIRE(sscanf(line, "%d %s", &address, file_name2) > 0, ERR_BAD_PARAMETER, " ", );

                    M_REQUIRE(page_file_read(file_name2, memory, address) == 0 , ERR_BAD_PARAMETER, " ",);
                }
            else                                            //enters here for the last lines
                {

                    uint64_t tovadd = 0;

                    M_REQUIRE(sscanf(line, "%"SCNx64" %s", &tovadd, file_name2) > 0 , ERR_BAD_PARAMETER, " ",);

                    virt_addr_t v_address; //creates a virtual address
                    M_REQUIRE(init_virt_addr64(&v_address, tovadd) == 0, ERR_BAD_PARAMETER, " ",); //initializes virtual address

                    phy_addr_t paddr;      //creates a physical address
                    M_REQUIRE(page_walk(*memory, &v_address, &paddr) == 0 , ERR_BAD_PARAMETER, " ", );

                    M_REQUIRE(page_file_read(file_name2, memory, (paddr.phy_page_num << PAGE_OFFSET) | paddr.page_offset) == 0 , ERR_BAD_PARAMETER, " " , );

                }

            counter++;  //increases the counter
        }

    free(file_name); //free the variables
    free(file_name2);

    fclose(file); //closes the file

    return ERR_NONE;

}


/**
 * @brief      { Will put the information contained in a binary file in a specific address }
 *
 * @param[in]  filename  The file where the binary information is contained
 * @param      memory    The address where the memory starts
 * @param[in]  address   The address in the memory where we want to store de information of the binary file
 *
 * @return     { returns a error if something went wrong }
 */
int page_file_read(const char* filename, void** memory, pte_t address)
{



    M_REQUIRE_NON_NULL(filename);//requires that the arguments are null
    M_REQUIRE_NON_NULL(memory);

    const size_t size_of_file = 4096;

    //opens the file
    FILE *file = fopen(filename, "rb");
    M_REQUIRE_NON_NULL(file);
    // va tout au bout du fichier
    M_REQUIRE(fseek(file, 0L, SEEK_END) > 0, ERR_BAD_PARAMETER, " ",);

    // indique la position, et donc la taille (en octets)
    size_t mem_capacity_in_bytes = (size_t) ftell(file);
    // revient au début du fichier (pour le lire par la suite)
    rewind(file);

    //checks if the size is larger than the max size stated in the documentations
    M_REQUIRE(mem_capacity_in_bytes == size_of_file, ERR_IO, "The file size does not correspond to a page file");

    //reads the file and stores it in *memory + address
    fread(((*memory) + address), 1, mem_capacity_in_bytes, file);

    //closes the file
    fclose(file);

    return ERR_NONE;

}