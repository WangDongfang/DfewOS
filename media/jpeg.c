/*==============================================================================
** jpeg.c -- Use hardware decode a jpeg picture and show it on lcd.
**
** MODIFY HISTORY:
**
** 2012-05-10 wdf Create.
==============================================================================*/

#include <dfewos.h>

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#include <yaffs_guts.h>

#include "driver/src/jpeg/jpg_mem.h"
#include "driver/src/jpeg/jpg_opr.h"
#include "driver/src/jpeg/s3c_jpeg.h"

#include "driver/src/post/s3c_pp.h"

/*======================================================================
  JPEG driver function declare
======================================================================*/
int s3c_jpeg_init(void);
int s3c_jpeg_open(struct inode *inode, struct file *file);
int s3c_jpeg_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
int s3c_jpeg_release(struct inode *inode, struct file *file);

/*======================================================================
  POST driver function declare
======================================================================*/
int s3c_pp_init(void);
int s3c_pp_open(struct inode *inode, struct file *file);
int s3c_pp_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
int s3c_pp_release(struct inode *inode, struct file *file);

/*======================================================================
  TTY function declare
======================================================================*/
int my_yaffs_open(const char *file_name, int oflag, int mode);
int my_yaffs_stat(const char *file_name, struct yaffs_stat *buf);

/*======================================================================
  Function declare forward
======================================================================*/
static char *_jpeg_decode_pixel (const char *file_name, int *p_width, int *p_height);
static OS_STATUS _pp_show_jpeg (const char *yuv_pixel, int src_w, int src_h,
                                int dst_x, int dst_y, int dst_w, int dst_h);

/*======================================================================
  Global variables
======================================================================*/
static int    _G_get_pixel = 0;
static void * _G_pixel = NULL;

/*==============================================================================
 * - jpeg_show_file()
 *
 * - show a picture
 */
int jpeg_show_file (const char *file_name, int x, int y, int w, int h)
{
    char *yuv_pixel;
    int   width;
    int   height;

    /* deocde */
    yuv_pixel = _jpeg_decode_pixel (file_name, &width, &height);
    if (yuv_pixel == NULL) {
        return -1;
    }

    /* convert & show */
    _pp_show_jpeg (yuv_pixel, width, height, x, y, w, h);

    return 0;
}

/*==============================================================================
 * - _jpeg_decode_pixel()
 *
 * - uset JPEG device decode the picture file to YUV pixel
 */
static char *_jpeg_decode_pixel (const char *file_name, int *p_width, int *p_height)
{
    struct file jpeg_dev = {0};
    int         file_fd;
    int         file_size;
    char  *pcMMapAddr, *pcStreamBuffer, *pcFrameBuffer;
    JPG_RETURN_STATUS ret;
    JPG_DEC_PROC_PARAM DecodeParam = {0};

    file_fd = my_yaffs_open (file_name, O_RDONLY, 0666); /* open picture file */
    if (file_fd < 0) {
        serial_printf ("jpeg hw decode: Open \"%s\" picture file failed!\n", file_name);
        return NULL;
    }

    s3c_jpeg_init ();
    if (s3c_jpeg_open (NULL, &jpeg_dev) < 0) {
        serial_printf ("jpeg hw decode: Open jpeg device failed!\n");
        yaffs_close (file_fd); /* close picture file */
        return NULL;
    }

    s3c_jpeg_ioctl (NULL, &jpeg_dev, IOCTL_JPG_MMAP, (unsigned long)&pcMMapAddr);

    /* get jpeg device stream buffer */
    pcStreamBuffer = (char *)s3c_jpeg_ioctl (NULL, &jpeg_dev, IOCTL_JPG_GET_STRBUF, (unsigned long)pcMMapAddr);

    file_size = yaffs_read (file_fd, pcStreamBuffer, 1 * MB); /* read picture file to stream buffer */
    yaffs_close (file_fd); /* close picture file */
    if (file_size <= 0) {
        serial_printf ("jpeg hw decode: Read picture file failed!\n");
        s3c_jpeg_release (NULL, &jpeg_dev);
        return NULL;
    }

    /* decode */
    DecodeParam.decType = JPG_MAIN;
    DecodeParam.fileSize = file_size;
    ret = s3c_jpeg_ioctl (NULL, &jpeg_dev, IOCTL_JPG_DECODE, (unsigned long)&DecodeParam);
    if (ret != JPG_SUCCESS) {
        serial_printf ("jpeg hw decode: decode \"%s\" failed!\n", file_name);
        s3c_jpeg_release (NULL, &jpeg_dev);
        return NULL;
    }
    *p_width = DecodeParam.width;
    *p_height = DecodeParam.height;

    /* get jpeg device frame buffer */
    pcFrameBuffer = (char *)s3c_jpeg_ioctl (NULL, &jpeg_dev, IOCTL_JPG_GET_FRMBUF, (unsigned long)pcMMapAddr);

    s3c_jpeg_release (NULL, &jpeg_dev);

    return pcFrameBuffer;
}

/*==============================================================================
 * - _pp_show_jpeg()
 *
 * - use POST device conver YUV to RGB pixel and show it on lcd
 */
static OS_STATUS _pp_show_jpeg (const char *yuv_pixel, int src_w, int src_h,
                                int dst_x, int dst_y, int dst_w, int dst_h)
{
    struct file      pp_dev = {0};
    s3c_pp_params_t  PostParam;

    s3c_pp_init ();
    s3c_pp_open (NULL, &pp_dev);

    PostParam.src_full_width  = src_w;
    PostParam.src_full_height = src_h;
    PostParam.src_start_x     = 0;
    PostParam.src_start_y     = 0;
    PostParam.src_width       = src_w;
    PostParam.src_height      = src_h;
    PostParam.src_color_space = YCBYCR;

    PostParam.dst_full_width  = _G_get_pixel ? src_w : (800 * 3);
    PostParam.dst_full_height = _G_get_pixel ? src_h : 600;
    PostParam.dst_start_x     = dst_x;
    PostParam.dst_start_y     = dst_y;
    PostParam.dst_width       = dst_w ? dst_w : src_w;
    PostParam.dst_height      = dst_h ? dst_h : src_h;
    PostParam.dst_color_space = RGB16;

    PostParam.out_path        = DMA_ONESHOT;
    PostParam.scan_mode       = PROGRESSIVE_MODE;

    PostParam.src_buf_addr_phy[0] = (UINT32)yuv_pixel;
    PostParam.src_buf_addr_phy[1] = (UINT32)(yuv_pixel + src_w * src_h);
    PostParam.src_buf_addr_phy[2] = (UINT32)(yuv_pixel + ((src_w * src_h* 3) >> 1));
    PostParam.src_buf_addr_phy[3] = 0;

    PostParam.dst_buf_addr_phy[0] = _G_get_pixel ? (unsigned int)_G_pixel : 0x57c00000;
    PostParam.dst_buf_addr_phy[1] = 0;
    PostParam.dst_buf_addr_phy[2] = 0;
    PostParam.dst_buf_addr_phy[3] = 0;

    s3c_pp_ioctl (NULL, &pp_dev, S3C_PP_SET_PARAMS, (unsigned long)&PostParam);
    s3c_pp_ioctl (NULL, &pp_dev, S3C_PP_SET_SRC_BUF_ADDR_PHY, (unsigned long)&PostParam);
    s3c_pp_ioctl (NULL, &pp_dev, S3C_PP_SET_DST_BUF_ADDR_PHY, (unsigned long)&PostParam);
    s3c_pp_ioctl (NULL, &pp_dev, S3C_PP_START, 0);

    s3c_pp_release (NULL, &pp_dev);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - jpeg_hw_dump()
 *
 * - call by <jpeg_dump_pic> in "jpeg_util.c", which show the picture on lcd directly
 */
OS_STATUS jpeg_hw_dump (const char *file_name, const int *p_x, const int *p_y,
                        int *p_expect_width, int *p_expect_height,
                        int *p_width, int *p_height)
{
    int width;
    int height;
    int start_x = *p_x;
    int start_y = *p_y;
    char *yuv_pixel;

    /* test file size */
    struct yaffs_stat st_buf;
    my_yaffs_stat(file_name, &st_buf);
    if (st_buf.st_size < 20 * KB) {
        return OS_STATUS_ERROR;
    }

    /* deocde */
    yuv_pixel = _jpeg_decode_pixel (file_name, &width, &height);
    if (yuv_pixel == NULL) {
        return OS_STATUS_ERROR;
    }

    /* calculate start coordinate */
    if (*p_expect_width == 0) {
        *p_expect_width = width;
        *p_expect_height = height;
    } else if ((*p_expect_width >= width) && (*p_expect_height >= height)) {

        start_x += (*p_expect_width - width) / 2;
        start_y += (*p_expect_height - height) / 2;

        /* the more area fill with GUI_BG_COLOR */
    } else { /* ÔÝ²»Ö§³Ö */
        serial_printf ("%s is biger than it's cbi size!\n", file_name);
        return OS_STATUS_ERROR;
    }

    /* save picture size */
    *p_width = width;
    *p_height = height;

    /* check start x */
    start_x &= ~0x1;

    /* convert to RGB16 and show */
    _pp_show_jpeg (yuv_pixel, width, height, start_x, start_y, width, height);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - jpeg_hw_pixel()
 *
 * - call by <get_pic_pixel> in "jpeg_util.c", which only get the picture pixel
 */
uint16 *jpeg_hw_pixel (const char *file_name, int *p_width, int *p_height)
{
    char *yuv_pixel;
    int width;
    int height;

    /* test file size */
    struct yaffs_stat st_buf;
    my_yaffs_stat(file_name, &st_buf);
    if (st_buf.st_size < 20 * KB) {
        return NULL;
    }

    /* deocde */
    yuv_pixel = _jpeg_decode_pixel (file_name, &width, &height);
    if (yuv_pixel == NULL) {
        return NULL;
    }

    /* alloc pixel data for user, this memory need user free */
    _G_pixel = malloc (width * height * 2);
    if (_G_pixel == NULL) {
        return NULL;
    }

    /* convert to RGB16 and show */
    _G_get_pixel = 1;
    _pp_show_jpeg (yuv_pixel, width, height, 0, 0, width, height);
    _G_get_pixel = 0;

    /* up width & height */
    if (p_width != NULL) { *p_width = width; }
    if (p_height != NULL) { *p_height = height; }

    return _G_pixel;
}

/*==============================================================================
** FILE END
==============================================================================*/

