/*==============================================================================
** wuzi.c -- Wuzi Qi Game. <white> ---- <black>
**
** MODIFY HISTORY:
**
** 2012-04-17 wdf Create.
==============================================================================*/
#include "../string.h" /* memset */
#include "wuzi.h"

/*======================================================================
  Constant
======================================================================*/
#define ROW_NUM                 WUZI_ROW_NUM    /* just for short */
#define COL_NUM                 WUZI_COL_NUM
#define CHESSMAN_NUM            (ROW_NUM * COL_NUM)
#define WIN_CASE_NUM            (ROW_NUM * (COL_NUM - 4) + \
                                 (ROW_NUM - 4) * COL_NUM + \
                                 (ROW_NUM - 4) * (COL_NUM - 4) + \
                                 (ROW_NUM - 4) * (COL_NUM - 4))
/*======================================================================
  Chessman
======================================================================*/
#define CHESSMAN_WHITE          0   /* 白棋子 */
#define CHESSMAN_BLACK          1   /* 黑棋子 */
#define CHESSMAN_NONE           2   /* 无棋子 */

/*======================================================================
  Occupy
======================================================================*/
#define WIN_NEVER               6   /* 死棋标志 */

/*======================================================================
  Helper
======================================================================*/
#define FOR_EACH_WIN_CASE(i)    for (i = 0; i < WIN_CASE_NUM; i++)
#define FOR_EACH_ROW(row)       for (row = 0; row < ROW_NUM; row++)
#define FOR_EACH_COL(col)       for (col = 0; col < COL_NUM; col++)

/*======================================================================
  记录当前棋盘上的棋子个数 [0 -- CHESSMAN_NUM]
======================================================================*/
int  _G_chessman_num;

/*======================================================================
  记录当前双方棋子摆放 [0, 1, 2]
  CHESSMAN_NONE  : 此位置无棋子
  CHESSMAN_WHITE : white 的棋子摆放在此位置
  CHESSMAN_BLACK : black 的棋子摆放在此位置
======================================================================*/
char _G_chessboard[ROW_NUM][COL_NUM];

/*======================================================================
  记录 white 和 black 在每种获胜方式中已经摆放的棋子数 或 死棋标志
  [0, 1, 2, 3, 4, 5, WIN_NEVER]
  0 -- 5    : white 或 black 已在该获胜方式中摆放了 0 -- 5 个棋子
  WIN_NEVER : white 或 black 已在该获胜方式中已经无法再取胜
======================================================================*/
unsigned char _G_occupy_num[2][WIN_CASE_NUM];

/*======================================================================
  每一种获胜方式在棋盘上所占据的位置 [0, 1]
  0 : 本种获胜方式不占据此位置
  1 : 本种获胜方式占据此位置
======================================================================*/
unsigned char _G_win_case_table[ROW_NUM][COL_NUM][WIN_CASE_NUM];

/*==============================================================================
 * - wuzi_start()
 *
 * - initialize global varibles
 */
void wuzi_start ()
{
    static int initialized =0;
	int i;
    int row, col;
    int win_case_index = 0;

    /*
     * clear
     */
    _G_chessman_num = 0;
    memset (_G_chessboard, CHESSMAN_NONE, sizeof(_G_chessboard)); /* 棋盘状态清 0 */
    memset (_G_occupy_num, 0, sizeof(_G_occupy_num)); /* 占领数目清 0 */

    if (initialized == 1) {
        return;
    }

    /*
     * init win case table only once
     */
    FOR_EACH_ROW(row) { /* 设定水平方向的获胜位置 */
        for (col = 0; col < COL_NUM - 4; col++) {
            for (i = 0; i < 5; i++) {
                _G_win_case_table[row][col + i][win_case_index] = 1;
            }
            win_case_index++; /* 设置下一种获胜方式 */
        }
    }
    /* 设定垂直方向的获胜位置 */
    FOR_EACH_COL(col) {
        for (row = 0; row < ROW_NUM - 4; row++) {
            for (i = 0; i < 5; i++) {
                _G_win_case_table[row + i][col][win_case_index] = 1;
            }
            win_case_index++; /* 设置下一种获胜方式 */
        }
    
    }
    /* 设定正对角线方向的获胜位置 */
    for (row = 0; row < ROW_NUM - 4; row++) {
        for (col = 0; col < COL_NUM - 4; col++) {
            for (i = 0; i < 5; i++) {
                _G_win_case_table[row + i][col + i][win_case_index] = 1;
            }
            win_case_index++; /* 设置下一种获胜方式 */
        }
    }
    /* 设定反对角线方向的获胜位置 */
    for (row = 0; row < ROW_NUM - 4; row++) {
        for (col = COL_NUM - 1; col >= 4; col--) {
            for (i = 0; i < 5; i++) {
                _G_win_case_table[row + i][col - i][win_case_index] = 1;
            }
            win_case_index++; /* 设置下一种获胜方式 */
        }
    }

    initialized = 1;
}

/*==============================================================================
 * - _put_a_chessman()
 *
 * - put a chessman on (row, col).
 *   <chessman> must be [CHESSMAN_WHITE] or [CHESSMAN_BLACK]
 */
static int _put_a_chessman (int chessman, int row, int col)
{
    int i;

    /* 是否超出棋盘范围 */
	if (row >= ROW_NUM || col >= COL_NUM || row < 0 || col < 0) {
		return WUZI_NONLEGAL;
	}

    /* 是否已经被占领 */
    if (_G_chessboard[row][col] != CHESSMAN_NONE) {
        return WUZI_NONLEGAL;
    }

    /* 更新 white 或 black 每一种获胜情况下已经放置棋子个数 */
    FOR_EACH_WIN_CASE(i) {
        if (_G_win_case_table[row][col][i]) {

            /* chessman 在第 i 种获胜方式中已经放置棋子个数加 1 */
            if (_G_occupy_num[chessman][i] != WIN_NEVER) {
                _G_occupy_num[chessman][i]++;

                /* chessman 获胜 */
                if (_G_occupy_num[chessman][i] == 5) {
                    return chessman;
                }
            }

            /* 另一方在第 i 种获胜方式中不可能再获胜, 所以置为 WIN_NEVER */
            _G_occupy_num[1 - chessman][i] = WIN_NEVER;   
        }
    }
    
    _G_chessman_num++;
    _G_chessboard[row][col] = chessman; /* 放置 chessman 的棋子 */

    if (_G_chessman_num == CHESSMAN_NUM) {
        return WUZI_DOGFALL; /* 平局 */
    }

    return WUZI_GO_AHEAD;
}

/*==============================================================================
 * - _player_do()
 *
 * - player put his chessman at <row, col>
 */
static int _player_do (int row, int col)
{
    return wuzi_white_do (row, col);
}

/*==============================================================================
 * - _computer_do()
 *
 * - computer reply at <*p_row, *p_col>
 */
static int _computer_do (int *p_row, int *p_col)
{
    int i, j;
	int k;
    int score[7] = {1, 5, 50, 100, 400, 0, 0};
    int player_grades[ROW_NUM][COL_NUM];
    int comput_grades[ROW_NUM][COL_NUM];
    int max_player_grade = -1;
    int max_comput_grade = -1;
    int defend_row, defend_col;
    int attack_row, attack_col;
    
    /* 计算 player 和 computer 在空格子上的分数 */
    FOR_EACH_ROW(i) {
        FOR_EACH_COL(j) {

            player_grades[i][j] = 0;
            comput_grades[i][j] = 0;

            if (_G_chessboard[i][j] == CHESSMAN_NONE) { /* 位置i,j为空 */
                /* 计算 player 和 computer 在空格子上的分数 */
                FOR_EACH_WIN_CASE(k) {
                    if (_G_win_case_table[i][j][k]) { /* 位置i,j在第k种获胜方式中 */
                        player_grades[i][j] += score[_G_occupy_num[CHESSMAN_WHITE][k]];
                        comput_grades[i][j] += score[_G_occupy_num[CHESSMAN_BLACK][k]];
                    } 
                }

                /* 计算 player 和 computer 在空格子上的最大分数 */
                if (player_grades[i][j] > max_player_grade) {
                    max_player_grade = player_grades[i][j];   
                    defend_row = i;
                    defend_col = j;
                }
                if (comput_grades[i][j] > max_comput_grade) {
                    max_comput_grade = comput_grades[i][j];   
                    attack_row = i;
                    attack_col = j;
                }
            }
        }
    }

    /* 得到 computer 的应对位置 */
    if ((max_player_grade >= max_comput_grade) && (max_player_grade > 100)) { /* 防守 */
        *p_row = defend_row;
        *p_col = defend_col;
    } else { /* 攻击 */
        *p_row = attack_row;
        *p_col = attack_col;
    }

    return wuzi_black_do (*p_row, *p_col);
}

/*==============================================================================
 * - wuzi_white_do()
 *
 * - white put his chessman on (row, col)
 */
int wuzi_white_do (int row, int col)
{
    return _put_a_chessman (CHESSMAN_WHITE, row, col);
}

/*==============================================================================
 * - wuzi_black_do()
 *
 * - black put her chessman on (row, col)
 */
int wuzi_black_do (int row, int col)
{
    return _put_a_chessman (CHESSMAN_BLACK, row, col);
}

/*==============================================================================
 * - wuzi_round_do()
 *
 * - do a player & computer round
 */
int wuzi_round_do (int row, int col, int *p_row, int *p_col)
{
    int chess_status;

    /* 处理 player 棋步 */
    chess_status = _player_do (row, col);
    if (chess_status != WUZI_GO_AHEAD) {
        return chess_status;
    }

    /* computer 回应棋步 */
    return _computer_do (p_row, p_col);
}

/*==============================================================================
** FILE END
==============================================================================*/

#if 0 /* Test In VC++6.0*/
#include <stdio.h>
int main ()
{
    int row, col;
    int chess_status;
    char *msg[3] = {"玩家获胜", "计算机胜", "平局"};

_start:

    wuzi_start ();
	chess_status = WUZI_GO_AHEAD;
	
    while (chess_status == WUZI_GO_AHEAD ||
           chess_status == WUZI_NONLEGAL) {
        scanf ("%d %d", &row, &col);
        chess_status = wuzi_round_do (row, col, &row, &col);

        if (chess_status != WUZI_NONLEGAL) {
            printf ("computer reply : (%d, %d)\n", row, col);
        } else {
            printf ("(%d, %d) 已被占领!\n", row, col);
        }
    }
    printf ("%s\n", msg[chess_status]);

    goto _start;

    return 0;
}
#endif
