/*==============================================================================
** dlist.h -- the doubly linked tool header.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __DLIST_H__
#define __DLIST_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  doubly linked node struct, it is usually other struct first member
======================================================================*/
typedef struct dlist_node {
    struct dlist_node *next;
    struct dlist_node *previous;
} DL_NODE;

/*======================================================================
  doubly linked list struct
======================================================================*/
typedef struct dlist_list {
    DL_NODE *head;
    DL_NODE *tail;
} DL_LIST;

/*======================================================================
  the dlist_each <routine> type
======================================================================*/
typedef OS_STATUS (*EACH_FUNC_PTR)(DL_NODE *pNode, int arg);

/*==============================================================================
 * - DL_FIRST()
 *
 * - Finds the first node in a doubly linked list
 */
#define DL_FIRST(pList)        (((DL_LIST *)(pList))->head)

/*==============================================================================
 * - DL_LAST()
 *
 * - Finds the last node in a doubly linked list
 */
#define DL_LAST(pList)         (((DL_LIST *)(pList))->tail)

/*==============================================================================
 * - DL_NEXT()
 *
 * - Locates the node immediately after the node pointed to by the <pNode>
 */
#define DL_NEXT(pNode)         (((DL_NODE *)(pNode))->next)

/*==============================================================================
 * - DL_PREVIOUS()
 *
 * - Locates the node immediately before the node pointed to by the <pNode>
 */
#define DL_PREVIOUS(pNode)     (((DL_NODE *)(pNode))->previous)

/*==============================================================================
 * - DL_EMPTY()
 *
 * - boolean function to check for empty list
 */
#define DL_EMPTY(pList)        (((DL_LIST *)pList)->head == NULL)    

/*======================================================================
  doubly linked list tool APIs
======================================================================*/
OS_STATUS dlist_init   (DL_LIST *pList);
void      dlist_insert (DL_LIST *pList, DL_NODE *pPrev, DL_NODE *pNode);
void      dlist_add    (DL_LIST *pList, DL_NODE *pNode);
void      dlist_remove (DL_LIST *pList, DL_NODE *pNode);
DL_NODE  *dlist_get    (DL_LIST *pList);
int       dlist_count  (DL_LIST *pList);
int       dlist_check  (DL_LIST *pList, DL_NODE *pNode);
DL_NODE  *dlist_each   (DL_LIST *pList, EACH_FUNC_PTR routine, int routineArg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DLIST_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

