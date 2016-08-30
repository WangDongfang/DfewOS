/*==============================================================================
** yaffs_init.c -- init yaffs component.
**
** MODIFY HISTORY:
**
** 2011-10-08 wdf Create.
==============================================================================*/
/* yaffs/direct/ */
#include "yaffscfg.h"
#include "yaffsfs.h"

/* yaffs/ */
#include "yaffs_guts.h"
#include "yaffs_trace.h"
#include "yaffs_linux.h"
#include "yaffs_packedtags2.h"
#include "yaffs_mtdif.h"
#include "yaffs_mtdif2.h"

/* mtd/ */
#include "operate_nand.h"

#define printf                  serial_printf

/*======================================================================
  extern function declare
======================================================================*/
extern void yaffs_cmd_init();

/*======================================================================
  local function prototype
======================================================================*/
static void _mount_partition(int iIndex);
static int  _start_up(void);

/*======================================================================
  Partitions in NAND FLASH K9F2G08U0A
======================================================================*/
#define NAND_PARTITION_N      2
static struct  {
    char pcName[8];
    int  iStartBlk;
    int  iEndBlk;
    int  iReservedBlk;
} _G_nand_partition_info[NAND_PARTITION_N] = {{"/boot", 1, 256,  10},
                                              {"/n1", 257, 2047, 24}};
/*==============================================================================
 * - yaffs_init()
 *
 * - the yaffs module init routine. include mount partitions init shell cmd
 */
int yaffs_init ()
{
	int i;

    /* init nand hardware and mtd interface */
    _start_up();

    /* mount yaffs partition */
    for (i = 0; i < NAND_PARTITION_N; i++) {
        _mount_partition(i);
    }

    /* init shell cmd */
    yaffs_cmd_init();

    return 0;
}

/*==============================================================================
 * - _mount_partition()
 *
 * - mount a partition
 */
static void _mount_partition(int iIndex)
{
    struct yaffs_linux_context  *pylcTemp = NULL; /* OS context */
    struct yaffs_dev            *pflashDev= NULL; /* yaffs device pointor */
    struct mtd_info             *pmtd     = &nand_info[0];
    int nBlocks;

    /* check partition */
    nBlocks = pmtd->size / pmtd->erasesize;
    if (_G_nand_partition_info[iIndex].iStartBlk < 0
        || _G_nand_partition_info[iIndex].iEndBlk >= nBlocks) {
        printf("## ERROR!\n\tYAFFS_DEV_PARTITION_TABLE error, value out of range.\n"
               "\tNAND infomation:\n"
               "\ttatol blocks:%d\n"
               "\tblock size=%d\n",
               nBlocks, pmtd->erasesize);
        return ;
    }

    /* allocate memory */
    pflashDev = calloc(1, sizeof(struct yaffs_dev));  /* allocate yaffs device */
    pylcTemp  = calloc(1, sizeof(*pylcTemp));  /* allocate os context */
    if (pflashDev == NULL || pylcTemp  == NULL) {
        printf("### memory alloc ERROR 1\n");
        if (pflashDev) {
            free(pflashDev);
        }
        if (pylcTemp) {
            free(pylcTemp);
        }
        return ;
    }

    /* ex-linked */
    pylcTemp->dev          = pflashDev;
    pflashDev->os_context  = pylcTemp;
    pylcTemp->spare_buffer = calloc(1, pmtd->oobsize);
    if ( NULL == pylcTemp->spare_buffer ) {
        free(pflashDev);
        free(pylcTemp);
        printf("### memory alloc ERROR 2\n");
        return ;
    }
    
    /* Set up device */
    pflashDev->param.name                  = _G_nand_partition_info[iIndex].pcName;
    pflashDev->param.start_block           = _G_nand_partition_info[iIndex].iStartBlk;
    pflashDev->param.end_block             = _G_nand_partition_info[iIndex].iEndBlk;
    pflashDev->param.n_reserved_blocks     = _G_nand_partition_info[iIndex].iReservedBlk;
    pflashDev->param.total_bytes_per_chunk = pmtd->writesize;
    pflashDev->param.chunks_per_block      = pmtd->erasesize / pmtd->writesize;
    pflashDev->param.spare_bytes_per_chunk = pmtd->oobsize;
    pflashDev->param.use_nand_ecc          = 1;
    pflashDev->param.no_tags_ecc           = 0;
    pflashDev->param.n_caches              = 10;
    pflashDev->driver_context              = (void *)pmtd; /* mtd device for later use */
    pflashDev->param.write_chunk_tags_fn   = nandmtd2_write_chunk_tags;
    pflashDev->param.read_chunk_tags_fn    = nandmtd2_read_chunk_tags;
    pflashDev->param.bad_block_fn          = nandmtd2_mark_block_bad;
    pflashDev->param.query_block_fn        = nandmtd2_query_block;
    pflashDev->param.erase_fn              = nandmtd_erase_block;
    pflashDev->param.initialise_flash_fn   = nandmtd_initialise;
    pflashDev->param.is_yaffs2             = 1;

    /* install device */
    yaffs_add_device (pflashDev);
    yaffs_mount (pflashDev->param.name);
}

/*==============================================================================
 * - _start_up()
 *
 * - init nand hardware and os relatively obj
 */
static int _start_up (void)
{
	int nBlock = 0;
    /* init mtd and nand hardware */
    nand_init();

    nBlock = (nand_info[0].size) / (nand_info[0].erasesize);
    /* print nand flash info */
    printf("NAND INFO:\n"
           "\t* name:            : %s\n", nand_info[0].name );
    printf("\t* bytes per chunk  : %d\n", nand_info[0].writesize );
    printf("\t* chunks per block : %d\n", nand_info[0].erasesize );
    printf("\t* total blocks     : %d\n", nBlock);

    /* init os semaphore */
    yaffsfs_OSInitialisation ();

    return 0;
}

/*==============================================================================
** FILE END
==============================================================================*/

