/*==============================================================================
** rotator.c -- S3C6410X Rotator Driver.
**
** MODIFY HISTORY:
**
** 2012-03-17 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "rotator.h"

/*======================================================================
  Rotator Controller Registers
======================================================================*/
#define rCTRLCFG            *(volatile unsigned int *)(0x77200000)
#define rSRCADDRREG0        *(volatile unsigned int *)(0x77200004)
#define rSRCSIZEREG         *(volatile unsigned int *)(0x77200010)
#define rDSTADDRREG0        *(volatile unsigned int *)(0x77200018)
#define rSTATCFG            *(volatile unsigned int *)(0x7720002C)

/*======================================================================
  LCD Controller Registers
======================================================================*/
#define rVIDW00ADD2     (*(volatile unsigned int *)(0x77100000 + 0x100))
#define rVIDW00ADD0B0   (*(volatile unsigned int *)(0x77100000 + 0x0A0))

/*==============================================================================
 * - rotator()
 *
 * - rotate a picture
 */
int rotator (void *srcAddr, void *dstAddr,
             int src_width, int src_height,
             ROTATE_DEGREE degree)
{
    rSRCADDRREG0 = (unsigned int)srcAddr;
    rDSTADDRREG0 = (unsigned int)dstAddr;
    rSRCSIZEREG  = (src_height << 16) | src_width;
    rCTRLCFG     = (4 << 13)     | /* RGB 565 */
                   (degree << 6) | /* degree  */
                    1;             /* start   */

    /* wait for rotate over */
    while (rSTATCFG & 0x3) {
        ;
    }
    /* printk("rator done"); */
    return 0;
}

/*==============================================================================
 * - rotator_scr()
 *
 * - rotate part of lcd screen
 */
int rotator_scr (int src_x, int src_y, int src_width, int src_height,
                 int dst_x, int dst_y, ROTATE_DEGREE degree)
{
    int i;
    unsigned int screen_w = ((rVIDW00ADD2 >> 13) + (rVIDW00ADD2 & 0x1FFFF)) >> 1;
    unsigned short *fbAddr = (unsigned short *)rVIDW00ADD0B0;
    unsigned short *src_fb = fbAddr + src_y * screen_w + src_x;
    unsigned short *dst_fb = fbAddr + dst_y * screen_w + dst_x;
    unsigned short *src_slot = NULL;
    unsigned short *dst_slot = NULL;
    int dst_width, dst_height;

    /* check degree and get dst size */
    switch (degree) {
        case DEGREE_90:
        case DEGREE_270:
            dst_width = src_height;
            dst_height = src_width;
            break;
        case DEGREE_0:
        case DEGREE_180:
            dst_width = src_width;
            dst_height = src_height;
            break;
        default:
            return -1;
    }

    src_slot = malloc(src_width * src_height * 2);
    if (src_slot == NULL) {
        return -1;
    }

    dst_slot = malloc(dst_width * dst_height * 2);
    if (dst_slot == NULL) {
        free(src_slot);
        return -1;
    }

    /* copy src --> src_slot */
    for (i = 0; i < src_height; i++) {
        memcpy (src_slot + src_width * i, src_fb + screen_w * i, src_width * 2);
    }

    /* rototor src_slot --> dst_slot */
    rotator (src_slot, dst_slot, src_width, src_height, degree);

    /* copy dst_slot --> dst */
    for (i = 0; i < dst_height; i++) {
        memcpy (dst_fb + screen_w * i, dst_slot + dst_width * i, dst_width * 2);
    }

    free(src_slot);
    free(dst_slot);
    /* printk("rator done23232"); */
    return 0;
}

/*==============================================================================
** FILE END
==============================================================================*/

