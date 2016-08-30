/*==============================================================================
** jpeg_util.c -- decompress picture util.
**
** MODIFY HISTORY:
**
** 2011-11-07 wdf Create.
==============================================================================*/
/*
 * example.c
 *
 * This file illustrates how to use the IJG code as a subroutine library
 * to read or write JPEG image files.  You should look at this code in
 * conjunction with the documentation file libjpeg.txt.
 *
 * This code will not do anything useful as-is, but it may be helpful as a
 * skeleton for constructing routines that call the JPEG library.  
 *
 * We present these routines in the same coding style used in the JPEG code
 * (ANSI function definitions, etc); but you are of course free to code your
 * routines in a different style if you prefer.
 */

/* #include <stdio.h> */
#include <dfewos.h>
#include <yaffs_guts.h>
#include "../gui.h"
#include "../driver/lcd.h"
#define printf serial_printf

#include <media/jpeg.h> /* jpeg hardware decode */
void _fill_margin (const GUI_COOR *p_left_up, const GUI_SIZE *p_expect_size, const GUI_SIZE *p_actual_size);

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#include "src/jpeglib.h"

/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
 * It's a bit more refined than the above, in that we show:
 *   (a) how to modify the JPEG library's standard error-reporting behavior;
 *   (b) how to allocate workspace using the library's memory manager.
 *
 * Just to make this example a little different from the first one, we'll
 * assume that we do not intend to put the whole image into an in-memory
 * buffer, but to send it line-by-line someplace else.  We need a one-
 * scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
 * memory manager allocate it for us.  This approach is actually quite useful
 * because we don't need to remember to deallocate the buffer separately: it
 * will go away automatically when the JPEG object is cleaned up.
 */


/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

#if 0
  jmp_buf setjmp_buffer;    /* for return to caller */
#endif
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

GLOBAL(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  //my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
/*   longjmp(myerr->setjmp_buffer, 1); */
}


/*
 * Sample routine for JPEG decompression.  We assume that the source file name
 * is passed in.  We want to return 1 on success, 0 on error.
 */


#if 0
GLOBAL(int)
read_JPEG_file (char * filename)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  /* More stuff */
  int infile;        /* source file */
  JSAMPARRAY buffer;        /* Output row buffer */
  int row_stride;        /* physical row width in output buffer */

  extern void *lcd_base;
  unsigned short *pu16 = NULL; /* point to draw frame buffer */
  static int draw_fb_index = 0;

  int i;
  char *p;
  unsigned short r;
  unsigned short g;
  unsigned short b;

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  infile = yaffs_open(filename, 0, 0);
  if ( infile < 0 ) {
      printf ("can't open %s\n", filename);
      return 0;
  }

  draw_fb_index = 1 - draw_fb_index;  /* change draw fb between 0 and 1 */

  lcd_draw_fb (draw_fb_index);
  pu16 = lcd_base;
  

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.txt for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;
  /* Make a one-row-high sample array that will go away when done with image */
  buffer = (*cinfo.mem->alloc_sarray)
      ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  /* printf("decompresing: output_width=%d output_height=%d\n", cinfo.output_width, cinfo.output_height ); */
  while ((cinfo.output_scanline < cinfo.output_height) && cinfo.output_scanline < U_LCD_YSIZE) {
      int res;
      
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
      res = jpeg_read_scanlines(&cinfo, buffer, 1);
      //printf("%d\n", res);
      /* fflush( stdout ); */
      /* Assume put_scanline_someplace wants a pointer and sample count. */
      /* put_scanline_someplace(buffer[0], row_stride); */
      {
           p = (char*)buffer[0]; 

           for (i = 0; i < cinfo.output_width && i< U_LCD_XSIZE; i++) {
               r = *p++;
               g = *p++;
               b = *p++;
               *pu16++ = ((r >> 3) << 11) |
                         ((g >> 2) << 5)  |
                          (b >> 3);
           }

           for ( ; i < U_LCD_XSIZE; i++) {
               *pu16++ = *(pu16 - cinfo.output_width);
           }
      }
  }
  pu16 = lcd_base;
  for (i = cinfo.output_height; i < U_LCD_YSIZE; i++) {
      memcpy (pu16 + (i * U_LCD_XSIZE), pu16 + (i - cinfo.output_height) * U_LCD_XSIZE, U_LCD_XSIZE * 2);
  }

  /* move image */
#if 0
  {
      static int dfasdf = 0;
      if (dfasdf == 0) {
          dfasdf = 1;
      } else {
          uint16 *show_fb = lcd_get_show_fb ();
          int col;
          int line;
          for (col = 0; col < U_LCD_XSIZE; col++) { /* erase the col */
              for (line = 0; line < U_LCD_YSIZE; line++) { /* move the line */
                  memcpy ((show_fb + line * U_LCD_XSIZE), (show_fb + line * U_LCD_XSIZE + 1), (U_LCD_XSIZE - 1) * 2);
                  *(show_fb + ((line + 1) * U_LCD_XSIZE) - 1) = *(pu16 + line * U_LCD_XSIZE + col);
              }
          }
      }
  }
#endif
  /* after draw the frame buffer, show it */
  lcd_show_fb (draw_fb_index);

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
#if 0                           /* uboot */
  fclose(infile);
#else
  yaffs_close( infile );
#endif

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return 1;
}
#endif

/*==============================================================================
 * - get_pic_pixel()
 *
 * - decompress a picture and store it's pixel color to a array.
 *   The caller should free the array
 */
uint16 *get_pic_pixel (const char *file_name, int *p_width, int *p_height)
{
    int res;
    unsigned char *p;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPARRAY buffer;        /* Output row buffer */
    int row_stride;        /* physical row width in output buffer */
    unsigned short r;
    unsigned short g;
    unsigned short b;

    uint16 *pic = NULL;
    uint16 *pixel = NULL;
    int fd;
    int i;

    /* try to decode with hardware */
    if ((pic = jpeg_hw_pixel (file_name, p_width, p_height)) != NULL) { 
        return pic;
    }

    /* open picture file */
    fd = yaffs_open(file_name, 0, 0);
    if ( fd < 0 ) {
        printf ("can't open %s\n", file_name);
        return NULL;
    }

    /* init jpeg lib */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, (void *)fd);
    (void) jpeg_read_header(&cinfo, TRUE);
    (void) jpeg_start_decompress(&cinfo);

    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* alloc memory for user */
    if (p_width != NULL) {
        *p_width = cinfo.output_width;
    }
    if (p_height != NULL) {
        *p_height = cinfo.output_height;
    }
    pic = malloc (cinfo.output_height * cinfo.output_width * 2);
    if (pic == NULL) {
        goto finish;
    }
    pixel = pic;

    /* decompress */
    while ((cinfo.output_scanline < cinfo.output_height)) {

        res = jpeg_read_scanlines(&cinfo, buffer, 1);
        if (res == 1) {
            p = (unsigned char*)buffer[0]; 

            for (i = 0; i < cinfo.output_width ; i++) {
                r = *p++;
                g = *p++;
                b = *p++;
                *pixel++ = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            }
        }
    }

finish:
    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    yaffs_close( fd );

    return pic;
}

/*==============================================================================
 * - jpeg_dump_pic()
 *
 * - 解压一个图片到 FrameBuffer 中
 *   如果 <p_expect_size> 为0，则将其修改为图片的大小
 *   如果 <p_expect_size> 大于图片大小，则将图片显示在中间
 *   如果 <p_expect_size> 小于图片大小，则返回错误
 */
OS_STATUS jpeg_dump_pic (const char *pic_file_name,
                         const GUI_COOR *p_left_up,
                         GUI_SIZE       *p_expect_size)
{
	int res;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPARRAY buffer;        /* Output row buffer */
    int row_stride;        /* physical row width in output buffer */
    unsigned char *p;
    unsigned short r;
    unsigned short g;
    unsigned short b;
    GUI_SIZE actual_size;

    int fd;
    int row, col;
    int save_col;
    int i;

    OS_STATUS status = OS_STATUS_OK;

    /* try to decode with hardware */
    if (jpeg_hw_dump (pic_file_name,
                      &p_left_up->x,
                      &p_left_up->y,
                      &p_expect_size->w, 
                      &p_expect_size->h,
                      &actual_size.w,
                      &actual_size.h) == OS_STATUS_OK) {

        _fill_margin (p_left_up, p_expect_size, &actual_size);
        return OS_STATUS_OK;
    }

    /* open picture file */
    fd = yaffs_open(pic_file_name, 0, 0);
    if ( fd < 0 ) {
        printf ("can't open %s\n", pic_file_name);
        return OS_STATUS_ERROR;
    }

    /* init jpeg lib */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, (void *)fd);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* calculate where to place the color */
    row = p_left_up->y;
    col = p_left_up->x;

    if (p_expect_size->w == 0) {
        p_expect_size->w = cinfo.output_width;
        p_expect_size->h = cinfo.output_height;
    } else if ((p_expect_size->w >= cinfo.output_width) &&
               (p_expect_size->h >= cinfo.output_height)) {

        /* calculate start row % col to set pixel */
        col += (p_expect_size->w - cinfo.output_width) / 2;
        row += (p_expect_size->h - cinfo.output_height) / 2;

        /* fill the margin */
        actual_size.w = cinfo.output_width;
        actual_size.h = cinfo.output_height;
        _fill_margin (p_left_up, p_expect_size, &actual_size);

    } else { /* 暂不支持 */
        printf ("%s is biger than it's cbi size!\n", pic_file_name);
        status = OS_STATUS_ERROR;
        goto finish;
    }
    save_col = col;

    /* decompress */
    while ((cinfo.output_scanline < cinfo.output_height)) {

        res = jpeg_read_scanlines(&cinfo, buffer, 1);
        if (res == 1) {
            p = (unsigned char*)buffer[0]; 
            col = save_col;

            for (i = 0; i < cinfo.output_width ; i++) {
                r = *p++; g = *p++; b = *p++;
                lcd_set_pixel (row, col, ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
                col++;
            }
        }
        row++;
    }

finish:
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    yaffs_close( fd );

    return status;
}

/*==============================================================================
 * - _fill_margin()
 *
 * - helper routine, fill picture frame margin
 */
void _fill_margin (const GUI_COOR *p_left_up, const GUI_SIZE *p_expect_size, const GUI_SIZE *p_actual_size)
{
    GUI_SIZE blank_size;
    blank_size.w = (p_expect_size->w - p_actual_size->w) / 2; /* length of pixels */
    blank_size.h = (p_expect_size->h - p_actual_size->h) / 2;

    /* the more area fill with GUI_BG_COLOR */
    {
        GUI_COOR st;
        GUI_SIZE sz;

        /* top block */
        st.x = p_left_up->x;
        st.y = p_left_up->y;
        sz.w = p_expect_size->w;
        sz.h = blank_size.h;
        gra_block (&st, &sz, NULL);

        /* bottom */
        st.y = p_left_up->y + blank_size.h + p_actual_size->h;
        sz.h += p_actual_size->h % 2;
        gra_block (&st, &sz, NULL);

        /* left */
        st.y = p_left_up->y + blank_size.h;
        sz.w = blank_size.w;
        sz.h = p_actual_size->h;
        gra_block (&st, &sz, NULL);

        /* right */
        st.x = p_left_up->x + blank_size.w +p_actual_size->w;
        sz.w += p_actual_size->w % 2;
        gra_block (&st, &sz, NULL);

    }
}

/*
 * SOME FINE POINTS:
 *
 * In the above code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.txt for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.txt for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.txt.
 */

/*==============================================================================
** FILE END
==============================================================================*/

