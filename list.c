#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h> // for PRIx macros

#include "list.h"
#include "error.h"

int is_empty_list(const list_t* this)
{
    M_REQUIRE_NON_NULL(this); //checks if the list is null

    if ((NULL == this->back && NULL != this->front ) ||  (NULL != this->back && NULL == this->front))
        {
            M_EXIT_ERR_NOMSG(ERR_BAD_PARAMETER);  //return error if list has a bak and not a front ot vice versa
        }

    return (NULL == this->back && NULL == this->front);

}


void init_list(list_t* this)
{

    if (this == NULL)
        {
            return ; //checks if the list is null
        }

    this->front = NULL; //sets the front and the back to null
    this->back = NULL;

}

void clear_list(list_t* this)
{


    if (this == NULL)
        {
            return ; //checks if the list is null
        }

    if (is_empty_list(this))
        {
            return;  //does nothing if list is empty
        }
    for_all_nodes(node, this) //loops through all nodes
    {
        node->value = 0;
        free(node->previous);
        node->previous = NULL; //sets the previous node to null
    }


    free(this->back);

    this->front = NULL;  //clears front and back of list
    this->back = NULL;


}



node_t* push_back(list_t* this, const list_content_t* value)
{

    if (this == NULL)
        {
            return NULL;;
        }

    node_t * newback = malloc(sizeof(node_t)); //Allocates the needed space
    if (newback == NULL)
        {
            return NULL;
        }

    newback->value = *value;

    newback->previous = this->back;  //sets the new node's previous to the current back
    newback->next = NULL;       //sets the new node's next to null as it will becom the new back and a back doesnt have a next


    if (!is_empty_list(this))
        {
            this->back->next = newback;     //we set the next of the current back as it will no longer be the back if the list is not empty
        }
    else
        {
            this->front = newback;          //if the list is empty we also set the front
        }

    this->back = newback; //sets the new back

    return newback;
}

node_t* push_front(list_t* this, const list_content_t* value)
{

    if (this == NULL)
        {
            return NULL;
        }


    node_t * newfront = malloc(sizeof(node_t)) ; //Allocates the needed space
    if (newfront == NULL)
        {
            return NULL;
        }
    newfront->value = *value; //sets the new value
    newfront->previous = NULL; //sets the previous to null because the front does not have a previous
    newfront->next = this->front;

    if (is_empty_list(this))
        {
            this->back = newfront; //is the list is empty the back is also the front
        }
    else
        {
            this->front->previous = newfront;
        }

    this->front = newfront; //sets the new front

    return newfront;
}

void pop_back(list_t* this)
{

    if (this == NULL)
        {
            return ; //checks if the list is null
        }


    if (is_empty_list(this))
        {
            return;
        }
    if (this->front == this->back)
        {

            free(this->front);
            this->front = NULL;
        }

    node_t * newback = this->back->previous;
    this->back->value = 0;

    free(this->back->previous);
    this->back->previous = NULL;

    this->back = newback;



}

void pop_front(list_t * this)
{

    if (this == NULL)
        {
            return ; //checks if the list is null
        }

    if (is_empty_list(this))
        {
            return;//if the list is empty just return
        }
    if (this->front == this->back)
        {
            free(this->back);
            free(this->back);

        }

    this->front = this->front->next ; //sets the front to the one after the current front
    this->front->previous = NULL; //sets the previous front to null

}

void move_back(list_t* this, node_t* node)
{

    if (this == NULL || node == NULL)
        {
            return ; //checks if the list is null
        }

    if (is_empty_list(this))
        {
            return;//if the list is empty just return
        }

    else  if (this->front == this->back || this->back == node ) //if there is only one node or the node passed as argument is already the back
        {
            return;
        }

    else if (this->front == node) //if the node passed as argument is the front
        {
            push_back(this, &node->value);
            pop_front(this);
        }

    else
        {

            if (node->next != NULL)
                {
                    node->previous->next = node->next;
                    node->next->previous = node->previous;
                    this->back->next = node;
                    node->previous = this->back;
                    node->next = NULL;
                    this->back = node;
                }
        }

}

int print_list(FILE* stream, const list_t* this)
{

    M_REQUIRE_NON_NULL(stream);
    M_REQUIRE_NON_NULL(this);

    if ( fprintf(stream, "(") == 0)
        {
            return 0;
        }
    unsigned int counter = 1;

    for_all_nodes(node, this)
    {
        print_node(stream, node->value);
        if (this->back != node) //if we have not reached the end of the list
            {
                if (fprintf(stream, ", " ) == 0)
                    {
                        return 0;
                    }
            }
        ++counter; //increase th counter
    }
    fprintf(stream, ")" );
    return counter;
}
int print_reverse_list(FILE* stream, const list_t* this)
{

    M_REQUIRE_NON_NULL(stream);
    M_REQUIRE_NON_NULL(this);

    if ( fprintf(stream, "(") == 0)
        {
            return 0;
        }
    unsigned int counter = 1;
    for_all_nodes_reverse(node, this)
    {
        print_node(stream, node->value);
        if (this->front != node) //if we have not reached the end of the list
            {
                if (fprintf(stream, ", " ) == 0)
                    {
                        return 0;
                    }
            }
        ++counter; //increase th counter
    }
    if (fprintf(stream, ")" ) == 0)
        {
            return 0;
        }
    return counter;



}


