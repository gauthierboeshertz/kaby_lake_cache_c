#include <inttypes.h>
#include "commands.h"
#include "addr_mng.h"

#include "mem_access.h" // for mem_access_t
#include "addr.h" // for virt_addr_t
#include <stdio.h> // for size_t, FILE
#include <stdint.h> // for uint32_t
#include "error.h"
#include <ctype.h> //  for isspace
#include <stdlib.h> // realloc

#define firstsize 10



int program_init(program_t * program)
{

	M_REQUIRE_NON_NULL(program);

	program->listing = calloc(firstsize, sizeof(command_t));
	M_REQUIRE_NON_NULL(program->listing);
	program->nb_lines = 0;
	program->allocated = sizeof(program->listing);

	return ERR_NONE;
}

int program_print(FILE* output, const program_t* program)
{
	M_REQUIRE_NON_NULL(output);
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(output);
	M_REQUIRE_NON_NULL(program->listing );


	for_all_lines(line, program)
	{

		char order = (line->order == READ) ? 'R' : 'W';
		char instruction = (line->type == INSTRUCTION) ? 'I' : 'D';
		char size = 0;

		if (instruction == 'D')
			{
				size = (line->data_size == sizeof(word_t)) ? 'W' : 'B';
			}

		if (order == 'R')
			{

				if (instruction == 'I' )
					{

						fprintf(output, "R " "I " "@0x%016" PRIX64 "\n" , virt_addr_t_to_uint64_t(&line->vaddr));
					}
				else
					{
						fprintf(output, "R " "D%c " "@0x%016" PRIX64 "\n" , size, virt_addr_t_to_uint64_t(&line->vaddr) );
					}
			}
		if (order == 'W')
			{
				if (size == 'W')
					{
						fprintf(output, "W " "%c" "%c " "0x%08" PRIX16" @0x%016" PRIX64 "\n", instruction , size, line->write_data, virt_addr_t_to_uint64_t(&line->vaddr));
					}
				else
					{
						fprintf(output, "W " "%c" "%c " "0x%" PRIX8 " @0x%016" PRIX64 "\n", instruction , size, line->write_data, virt_addr_t_to_uint64_t(&line->vaddr));
					}
			}

	}
	return ERR_NONE;
}

int program_shrink(program_t* program)
{

	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(program->listing);

	if (program->nb_lines > firstsize)
		{
			program->allocated = program->nb_lines * sizeof(command_t);
			program->listing = realloc(program->listing, program->allocated );
			M_REQUIRE_NON_NULL(program->listing);
		}

	else
		{
			program->allocated = firstsize * sizeof(command_t);
			program->listing = realloc(program->listing, program->allocated );
			M_REQUIRE_NON_NULL(program->listing);
		}
	return ERR_NONE;
}

int program_free(program_t* program)
{
	if (program != NULL)
		{
			free(program->listing);
			program->allocated = 0;
			program->nb_lines = 0;
		}
	return ERR_NONE;
}


int program_add_command(program_t* program, const command_t* command)
{

	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(program->listing);

	M_REQUIRE_NON_NULL(command);
	M_REQUIRE(command->order == READ || command->order == WRITE , ERR_BAD_PARAMETER, " ",);
	M_REQUIRE(command->type == DATA || command->type == INSTRUCTION , ERR_BAD_PARAMETER, " ",);

	M_REQUIRE((command->vaddr.page_offset % sizeof(word_t))  == 0 , ERR_BAD_PARAMETER, " ",);
	if (command->order == READ)
		{
			M_REQUIRE(command->write_data == 0, ERR_BAD_PARAMETER, " ", );
		}

	if (command->order == WRITE && command->data_size == 1)
		{

			M_REQUIRE(command->write_data <= 0xFF, ERR_BAD_PARAMETER, " ",);
		}


	if (command->type == DATA)
		{
			M_REQUIRE(command->data_size == sizeof(word_t) || command->data_size == 1, ERR_BAD_PARAMETER, "data_size not 1 byteor equal to the size of a word", );
		}


	if (command->order == WRITE)
		{
			M_REQUIRE(command->type != INSTRUCTION , ERR_BAD_PARAMETER, "can't write an instruction",  );
		}
	if (command->type == DATA)
		{
			M_REQUIRE(command->data_size != 0 , ERR_BAD_PARAMETER, "data size should be bigger than 0",  );
		}


// adds a line when there are less lines than the allocated number
	if (program->nb_lines < program->allocated)
		{
			program->listing[program->nb_lines] = *command;
			++program->nb_lines;
		}

// adds a line when there is the same number of lines as the allocated numbers , must resize the allocated number
	else
		{

			program->allocated *= 2;
			program->listing = realloc(program->listing, program->allocated * sizeof(command_t));
			M_REQUIRE_NON_NULL(program->listing);
			program->listing[program->nb_lines] = *command;
			++program->nb_lines;
		}
	return ERR_NONE;
}


// read untile the next non space
char next_nonspace_read(FILE* entree)
{
	char c = ' ';
	do
		{
			c = (fgetc(entree));

		}
	while (isspace(c) && c != EOF);
	return c;
}



// read when the order is a read
int   read_read(FILE* entree, command_t* com )
{
	char order = next_nonspace_read(entree);
	M_REQUIRE(order != EOF, ERR_BAD_PARAMETER, " ", );
	virt_addr_t vaddr;


// read when the order is an instruction
	if (order == 'I')
		{
			if (next_nonspace_read(entree) == '@')
				{
					uint64_t tovaddr = 0;

					M_REQUIRE(fscanf(entree, "%"SCNx64, &tovaddr) > 0, ERR_BAD_PARAMETER, " ", );

					init_virt_addr64(&vaddr, tovaddr);

					com->order = READ;
					com->type = INSTRUCTION;
					com->vaddr = vaddr;
				}
			else
				{

					return ERR_BAD_PARAMETER;
				}
		}
	else
		{
// reads when the order is DATA
			char size  = next_nonspace_read(entree);
			M_REQUIRE(size == 'W' || size == 'B', ERR_BAD_PARAMETER, " ", );

			if (next_nonspace_read(entree) == '@')
				{
					uint64_t tovaddr = 0;
					M_REQUIRE(fscanf(entree, "%"SCNx64, &tovaddr) > 0, ERR_BAD_PARAMETER, " ", );
					init_virt_addr64(&vaddr, tovaddr);

				}
			else
				{
					return ERR_BAD_PARAMETER;
				}
			com->order = READ;
			com->type = DATA;
			com->vaddr = vaddr;
			com->data_size = (size == 'W') ? sizeof(word_t) : 1 ;
		}
	return ERR_NONE;
}


// read to use when the order of the command is a write
int  read_write(FILE* entree, command_t * com)
{

	char order = next_nonspace_read(entree);
	M_REQUIRE(order == WRITE || order == READ, ERR_BAD_PARAMETER, " ", );


	virt_addr_t vaddr;
	char size  = next_nonspace_read(entree);

	M_REQUIRE(size == 'W' || size == 'B', ERR_BAD_PARAMETER, " ", );


	word_t value = 0;
	M_REQUIRE(fscanf(entree, "%"SCNx32, &value) > 0, ERR_BAD_PARAMETER, " ", );

	if (next_nonspace_read(entree) == '@')
		{
			uint64_t tovaddr = 0;
			M_REQUIRE(fscanf(entree, "%64"SCNx64, &tovaddr) > 0, ERR_BAD_PARAMETER, " ", );
			init_virt_addr64(&vaddr, tovaddr);

		}

	else
		{
			return ERR_BAD_PARAMETER;
		}

	com->order = WRITE;
	com->type = DATA;
	com->write_data = value;
	com->data_size = (size == 'W') ? sizeof(word_t) : 1;
	com->vaddr = vaddr;
	return ERR_NONE;
}


int program_read(const char* filename, program_t* program)
{
	M_REQUIRE_NON_NULL(filename);
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(program->listing);


	FILE* entree = fopen(filename, "r");
	M_REQUIRE_NON_NULL_CUSTOM_ERR(entree, ERR_IO);
	M_REQUIRE(program_init(program) == ERR_NONE, ERR_BAD_PARAMETER, " ", );
	do
		{

			command_t   command;
//divides the reading of the line in two parts,reading and writing
			char first = next_nonspace_read(entree);
			M_REQUIRE_NON_NULL(&first);
			if (first == 'R')
				{

					read_read(entree, &command);
					M_REQUIRE( program_add_command( program, &command) == ERR_NONE, ERR_BAD_PARAMETER, "");
				}

			if (first == 'W')
				{

					read_write(entree, &command);
					M_REQUIRE( program_add_command( program, &command) == ERR_NONE, ERR_BAD_PARAMETER, "");
				}
		}
	while (!feof(entree) & !ferror(entree));
	fclose(entree);
	return ERR_NONE;
}
