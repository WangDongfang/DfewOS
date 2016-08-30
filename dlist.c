/*==============================================================================
** dlist.c -- the doubly linked list tool.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#include "types.h"
#include "dlist.h"

/*==============================================================================
 * - dlist_init()
 *
 * - init the DL_LIST struct
 */
OS_STATUS dlist_init (DL_LIST *pList)
{
    pList->head  = NULL;
    pList->tail  = NULL;

    return (OS_STATUS_OK);    
}

/*==============================================================================
 * - dlist_insert()
 *
 * - insert <pNode> after <pPrev>. the caller must be sure of <pPrev> in list
 */
void dlist_insert (DL_LIST *pList,        /* pointer to list descriptor */
                   DL_NODE *pPrev,        /* pointer to node after which to insert */
                   DL_NODE *pNode)        /* pointer to node to be inserted */
{
    DL_NODE *pNext;

    if (pPrev == NULL) {                /* new node is to be first in list */
        pNext = pList->head;
        pList->head = pNode;
    } else {                            /* make prev node point fwd to new */
        pNext = pPrev->next;
        pPrev->next = pNode;
    }

    if (pNext == NULL) {
        pList->tail = pNode;        /* new node is to be last in list */
    } else {
        pNext->previous = pNode;    /* make next node point back to new */
    }

    /* set pointers in new node */

    pNode->next        = pNext;
    pNode->previous    = pPrev;
}

/*==============================================================================
 * - dlist_add()
 *
 * - add node to end of list
 */
void dlist_add (DL_LIST *pList,     /* pointer to list descriptor */
                DL_NODE *pNode)     /* pointer to node to be added */
{
    dlist_insert (pList, pList->tail, pNode);
}

/*==============================================================================
 * - dlist_remove()
 *
 * - remove specified node in list.
 *   the user must be sure the node in list, otherwise destory the list totally
 */
void dlist_remove (DL_LIST *pList,             /* pointer to list descriptor */
                   DL_NODE *pNode)             /* pointer to node to be deleted */
{

    if (pNode->previous == NULL)
        pList->head = pNode->next;
    else
        pNode->previous->next = pNode->next;

    if (pNode->next == NULL)
        pList->tail = pNode->previous;
    else
        pNode->next->previous = pNode->previous;


}

/*==============================================================================
 * - dlist_get()
 *
 * - get (delete and return) first node from list
 *
 * - RETURNS: Pointer to the node gotten, or NULL if the list is empty.
 */
DL_NODE *dlist_get (DL_LIST *pList)         /* pointer to list from which to get node */
{
    DL_NODE *pNode = pList->head;

    if (pNode != NULL) {                    /* is list empty? */
        pList->head = pNode->next;          /* make next node be 1st */

        if (pNode->next == NULL) {          /* is there any next node? */
            pList->tail = NULL;             /*   no - list is empty */
        } else {
            pNode->next->previous = NULL;   /*   yes - make it 1st node */
        }
    }

    return (pNode);
}

/*==============================================================================
 * - dlist_count()
 *
 * - report number of nodes in list
 */
int dlist_count (DL_LIST *pList)      /* pointer to list descriptor */
{
    DL_NODE *pNode = DL_FIRST (pList);
    int count = 0;

    while (pNode != NULL) {
        count++;
        pNode = DL_NEXT (pNode);
    }

    return (count);
}

/*==============================================================================
 * - dlist_check()
 *
 * - check whether the node is in the list. if in return 1, otherwise return 0
 */
int dlist_check (DL_LIST *pList, DL_NODE *pNode)
{
    DL_NODE *pListNode = DL_FIRST (pList);
    while (pListNode != NULL && pListNode != pNode) {
    	pListNode = DL_NEXT (pListNode);
    }

    return (pListNode ? 1 : 0);
}

/*==============================================================================
 * - dlist_each()
 *
 * - call a routine for each node in a linked list
 *
 * This routine calls a user-supplied routine once for each node in the
 * linked list.  The routine should be declared as follows:
 *
 * .CS
 *  OS_STATUS routine (DL_NODE *pNode, int arg);
 * .CE
 *
 * The user-supplied routine should return OS_STATUS_OK if dlist_each() is to
 * continue calling it with the remaining nodes, or OS_STATUS_ERROR if it is done and
 * dlist_each() can exit.
 *
 * - RETURNS: NULL if traversed whole linked list, or pointer to DL_NODE that
 *            dlist_each ended with.
 */
DL_NODE *dlist_each (DL_LIST     *pList,         /* linked list of nodes to call routine for */
                    EACH_FUNC_PTR routine,       /* the routine to call for each list node */
                     int          routineArg)    /* arbitrary user-supplied argument */
{
    DL_NODE *pNode = DL_FIRST (pList);

    while ( (pNode != NULL) && ((* routine)(pNode, routineArg) == OS_STATUS_OK) )
        pNode = DL_NEXT (pNode);

    return (pNode);            /* return node we ended with */
}


/*==============================================================================
** FILE END
==============================================================================*/

