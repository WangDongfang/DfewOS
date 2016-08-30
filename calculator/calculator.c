/*==============================================================================
** calculator.c -- calculate a string expression value.
**
** MODIFY HISTORY:
**
** 2011-11-20 wdf Create.
==============================================================================*/
#define OS_DFEWOS

#ifdef OS_DFEWOS
#include <dfewos.h>
#include "../string.h"
#define printf  serial_printf
int atoi (const char *s);
double atof (const char *s);
#else
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#endif

#include "calculator.h"

/*======================================================================
  configs
======================================================================*/
#define CAL_DATA_MAX_LEN    20

/*======================================================================
  Binary tree node
======================================================================*/
typedef struct _Btree_node {
    char   data[CAL_DATA_MAX_LEN];

    struct _Btree_node *pLNode;
    struct _Btree_node *pRNode;

} _BTREE_NODE;

/*======================================================================
  Function forward declare
======================================================================*/
static char *_skip_blank (char *str);
static _BTREE_NODE *_get_operand_node (char *expr, char **p_expr);
static _BTREE_NODE *_get_operator_node (char *expr, char **p_expr);
static _BTREE_NODE *_get_prim (char *expr, char **p_expr);
static _BTREE_NODE *_get_term_tree (char *expr, char **p_expr);
static _BTREE_NODE *_get_expr_tree (char *expr, char **p_expr);
static void _destory_tree (_BTREE_NODE* pRoot);
static CAL_TYPE _cal_btree_value (_BTREE_NODE* pRoot);
static void _error (const char *string);

/*==============================================================================
 * - calculate()
 *
 * - entry
 */
CAL_TYPE calculate (char *expression)
{
    _BTREE_NODE *pBTree = NULL;
    CAL_TYPE     value = 0;

    pBTree = _get_expr_tree (expression, &expression);
    if (pBTree == NULL) {
        printf ("Unlegal Expression! Stop at \"%s\"\n", expression);
        return 0;
    }

    value = _cal_btree_value (pBTree);

    _destory_tree (pBTree);

    return value;
}

/*==============================================================================
 * - _skip_blank()
 *
 * - skip a string front space char
 */
static char *_skip_blank (char *str)
{
    while (isspace (*str)) {
        str++;
    }

    return str;
}

/*==============================================================================
 * - _get_operand_node()
 *
 * - 从字符串表达式 <expr> 的最左端开始读入 操作数, 存放到一个二叉树节点
 *   并更新表达式
 */
static _BTREE_NODE *_get_operand_node (char *expr, char **p_expr)
{
    int data_pos = 0;
    _BTREE_NODE *pNode = NULL;

    /* skip the space chars */
    expr = _skip_blank (expr);
    if (expr[0] == '\0') {
        return NULL;
    }

    /* copy digit char to BNode's <data> */
    pNode = (_BTREE_NODE *)calloc (1, sizeof (_BTREE_NODE));
    if (pNode != NULL) {
        /* sign */
        if (*expr == '+' || *expr == '-') {
            pNode->data[data_pos++] = *expr++;
        }

#ifdef CAL_TYPE_INT
        while (isdigit (*expr)) {
#else
        while (isdigit (*expr) || (*expr == '.')) {
#endif
            pNode->data[data_pos++] = *expr++;
        }
        pNode->data[data_pos] = '\0';
    }

    /* check operand length */
    if ((data_pos == 0) ||
        (pNode->data[data_pos - 1] == '+') ||
        (pNode->data[data_pos - 1] == '-')) {
        free (pNode);
        return NULL;
    }

    /* update expression */
    *p_expr = expr;

    return pNode;
}

/*==============================================================================
 * - _get_operator_node()
 *
 * - 从字符串表达式 <expr> 的最左端开始读入 操作符, 存放到一个二叉树节点
 *   并更新表达式
 */
static _BTREE_NODE *_get_operator_node (char *expr, char **p_expr)
{
    _BTREE_NODE *pNode = NULL;

    /* skip the space chars */
    expr = _skip_blank (expr);
    if (expr[0] == '\0') {
        return NULL;
    }

    /* copy digit char to BNode's <data> */
    pNode = (_BTREE_NODE *)calloc (1, sizeof (_BTREE_NODE));
    if (pNode != NULL) {
        pNode->data[0] = *expr++;
        pNode->data[1] = '\0';
    }

    /* update expression */
    *p_expr = expr;

    return pNode;
}

/*==============================================================================
 * - _get_prim()
 *
 * - get a prim. a prim just like: 23, (23-4/6)
 */
static _BTREE_NODE *_get_prim (char *expr, char **p_expr)
{
    _BTREE_NODE *pNode = NULL;

    if (expr[0] == '(') {
        int  left_bracket_num = 1;
        char *sub_expr_end = expr;

        while (left_bracket_num > 0) {
            switch (*++sub_expr_end) {
                case '(':
                    left_bracket_num++;
                    break;
                case ')':
                    left_bracket_num--;
                    break;
                case '\0':
                    _error ("unmatched brack_num");
                    return NULL;
                default:
                    break;
            }
        }
        *sub_expr_end = '\0';
        pNode = _get_expr_tree (expr+1, &expr);
        *sub_expr_end = ')';

        *p_expr = ++expr;
    } else {
        pNode = _get_operand_node (expr, &expr);
        *p_expr = expr;
    }

    return pNode;
}

/*==============================================================================
 * - _get_term_tree()
 *
 * - get a term tree. a term just like: 23*45/34, 6*7
 */
static _BTREE_NODE *_get_term_tree (char *expr, char **p_expr)
{
    _BTREE_NODE *pLeft     = _get_prim (expr, &expr);
    _BTREE_NODE *pOperator = NULL;
    _BTREE_NODE *pRight    = NULL;

    if (pLeft == NULL) {
        return NULL;
    }

    /* skip the space chars */
    expr = _skip_blank (expr);
    while (expr[0] == '*' || expr[0] == '/') {
        pOperator = _get_operator_node (expr, &expr);
        pRight    = _get_prim (expr, &expr);

        if (pRight == NULL) {
            free (pOperator);
            pOperator = NULL;
            _destory_tree (pLeft);
            pLeft = NULL;
            break;
        }
        pOperator->pLNode = pLeft;
        pOperator->pRNode = pRight;
        
        pLeft = pOperator;
        _skip_blank(expr);
    }

    *p_expr = expr;

    return pLeft;
}

/*==============================================================================
 * - _get_expr_tree()
 *
 * - get the whole expression binary tree
 */
static _BTREE_NODE *_get_expr_tree (char *expr, char **p_expr)
{
    _BTREE_NODE *pLeft     = _get_term_tree (expr, &expr);
    _BTREE_NODE *pOperator = NULL;
    _BTREE_NODE *pRight    = NULL;

    if (pLeft == NULL) {
        return NULL;
    }

    pOperator = _get_operator_node (expr, &expr);
    while (pOperator != NULL) {

        pRight = _get_term_tree (expr, &expr);

        if (pRight == NULL) {
            free (pOperator);
            pOperator = NULL;
            _destory_tree (pLeft);
            pLeft = NULL;
            break;
        }

        pOperator->pLNode = pLeft;
        pOperator->pRNode = pRight;
        
        pLeft = pOperator;
        pOperator = _get_operator_node (expr, &expr);
    }

    *p_expr = expr;

    return pLeft;
}

/*==============================================================================
 * - _destory_tree()
 *
 * - free a binary tree memory
 */
static void _destory_tree (_BTREE_NODE* pRoot)
{
    if (pRoot != NULL) {
        _destory_tree (pRoot->pLNode);
        _destory_tree (pRoot->pRNode);
        free (pRoot);
    }
}

/*==============================================================================
 * - _cal_btree_value()
 *
 * - throught the tree, calculate the expr value
 */
static CAL_TYPE _cal_btree_value (_BTREE_NODE* pRoot)
{
    if (pRoot == NULL){
        return 0;
    }

    if (strlen (pRoot->data) > 1 || isdigit (pRoot->data[0])) {
#ifdef CAL_TYPE_INT
        return atoi (pRoot->data);
#else
        return atof (pRoot->data);
#endif
    }

    switch (pRoot->data[0]) {
        case '+':
            return _cal_btree_value (pRoot->pLNode) + _cal_btree_value (pRoot->pRNode);
        case '-':
            return _cal_btree_value (pRoot->pLNode) - _cal_btree_value (pRoot->pRNode);
        case '*':
            return _cal_btree_value (pRoot->pLNode) * _cal_btree_value (pRoot->pRNode);
        case '/':
            {
                CAL_TYPE r_val =  _cal_btree_value (pRoot->pRNode);
                if (r_val != 0) {
                    return _cal_btree_value (pRoot->pLNode) / r_val;
                } else {
                    _error ("divide by 0");
                    return 0;
                }
            }
        default:
            _error ("unknown operator!");
            return 0;
    }
}

/*==============================================================================
 * - _error()
 *
 * - dump warning message
 */
static void _error (const char *string)
{
    printf ("%s\n", string);
}

#ifdef OS_DFEWOS
int C_cal (int argc, char **argv)
{
    if (argc == 1) {
        printf ("useage: cal 2+3*(7+8)/2\n");
        return 0;
    }
#ifdef CAL_TYPE_INT
    printf ("%s = %d", argv[1], calculate (argv[1]));
#else
    char buffer [200];
    extern int sprintf(char *buf, const char *, ...);
    sprintf (buffer, "%s = %f", argv[1], calculate (argv[1]));
    printf ("%s", buffer);
#endif
    return 0;
}
#else
/*==============================================================================
 * - main()
 *
 * - test routine
 */
int main ()
{
    char      expression[100];
    CAL_TYPE  value = 0;

    for (;;) {
        expression[0] = '\0';
        gets(expression);

        if (expression[0] == '\0') {
            continue;
        }

        if (strcmp (expression, "exit") == 0) {
            break;
        }

        value = calculate (expression);
#ifdef CAL_TYPE_INT
        printf ("%s = %d\n\n", expression, value);
#else
        printf ("%s = %f\n\n", expression, value);
#endif

    }

    return 0;
}
#endif

/*==============================================================================
** FILE END
==============================================================================*/

