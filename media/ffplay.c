/*==============================================================================
** ffplay.c -- play a video and audio.
**
** MODIFY HISTORY:
**
** 2012-01-12 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include "middleware/6410_swscale/s3c6410_swscale.h"

#include "ffplay_lcd_cfg.h"

/*======================================================================
  Configs
======================================================================*/
#define FFPLAY_MSG_NUM                  4 /* <_G_ffplay_msgQ> max num */
#define DEFAULT_AV_SYNC_TYPE            AV_SYNC_VIDEO_MASTER
#define MSG_TASK_STACK_SIZE             (4 * KB)
#define PLAY_TASK_STACK_SIZE            (12 * KB)
#define VIDEO_TASK_STACK_SIZE           (20 * KB)
#define MSG_TASK_PRIORITY               120
#define PLAY_TASK_PRIORITY              122
#define VIDEO_TASK_PRIORITY             121

/*======================================================================
  Constants
======================================================================*/
#define AV_SYNC_THRESHOLD               0.01
#define AV_NOSYNC_THRESHOLD             10.0

/*======================================================================
  'tFFmsg' task can recevie message type
======================================================================*/
typedef enum ffplay_msg {
    SEEK_LITTLE_BACKWARD,
    SEEK_LITTLE_FORWARD,
    SEEK_LARGE_FORWARD,
    SEEK_LARGE_BACKWARD,
    SEEK_GOTO,
    FFPLAY_MSG_HOLD,
    FFPLAY_MSG_QUIT,
    FFPLAY_MSG_SET_XY,
    FFPLAY_MSG_SET_SZ
} FFPLAY_MSG;

/*======================================================================
  synchronize type
======================================================================*/
enum {
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_AUDIO_MASTER,
    AV_SYNC_EXTERNAL_MASTER,
};

/*======================================================================
  packet queue type
======================================================================*/
typedef struct PacketQueue {
    MSG_QUE  *msgQ;
} PacketQueue;

/*======================================================================
  video status type
======================================================================*/
typedef struct video_status {
    int    av_sync_type;

    AVFormatContext *pFormatCtx;
    int             videoStream;
    int             audioStream;
    int             seek_req;
    int             seek_flags;
    int64_t         seek_pos;

    double video_clock; //<pts of last decoded frame / predicted pts of next decoded frame 
    double audio_clock;

    AVStream  *video_st;
    AVStream  *audio_st;

    PacketQueue audioq;
    PacketQueue videoq;

    double  frame_timer;
    double  frame_last_pts;
    double  frame_last_delay;
    double  video_current_pts;
    int64_t video_current_pts_time;

    int      percent; /* SEEK_GOTO message use it */
    atomic_t start_xy;
    atomic_t video_size;

    int      hold;
    atomic_t quit;
    atomic_t alive_thread_num;
} VIDEO_STATUS;


/*======================================================================
  Global Variables
======================================================================*/
static MSG_QUE      _G_ffplay_msgQ;
static AVPacket     _G_flush_pkt;
static VIDEO_STATUS _G_vs;
static FUNC_PTR     _G_over_func = NULL;
static uint32       _G_over_func_arg1;
static uint32       _G_over_func_arg2;

/*======================================================================
  Functions Forward Declere
======================================================================*/
static void _T_msg_thread ();
static void _T_play_thread ();
static void _T_video_thread ();
static OS_STATUS _open_media_file (const char *file_name);
static int  _our_get_buffer(struct AVCodecContext *c, AVFrame *pic);
static void _our_release_buffer(struct AVCodecContext *c, AVFrame *pic);
static double _synchronize_video(AVFrame *src_frame, double pts);
static double _get_video_clock();
static double _get_external_clock();
static double _get_master_clock();
static int _get_video_delay (double pts);

/*==============================================================================
 * - packet_queue_init()
 *
 * - init a packet queue
 */
void packet_queue_init(PacketQueue *q, int max_pkt)
{
    memset(q, 0, sizeof(PacketQueue));

    q->msgQ = msgQ_init (NULL, max_pkt, sizeof (AVPacket));
}

/*==============================================================================
 * - packet_queue_put()
 *
 * - put a packet to queue. wait 3 seconds, if can't we discard it
 */
int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    if(pkt != &_G_flush_pkt && av_dup_packet(pkt) < 0) {
        return -1;
    }
    msgQ_send(q->msgQ, (char *)pkt, sizeof(AVPacket), 3 * SYS_CLK_RATE);

    return 0;
}

/*==============================================================================
 * - packet_queue_get()
 *
 * - get a packet from queue, wait 1 second, if can't we quit play
 */
int packet_queue_get(PacketQueue *q, AVPacket *pkt)
{
    int ret;

    ret = msgQ_receive(q->msgQ, (char *)pkt, sizeof(AVPacket), 1 * SYS_CLK_RATE);

    return ret;
}

/*==============================================================================
 * - packet_queue_flush()
 *
 * - flush a queue. discard all packets
 */
void packet_queue_flush(PacketQueue *q)
{
    AVPacket pkt;

    while (msgQ_receive(q->msgQ, (char *)&pkt, sizeof(AVPacket), NO_WAIT) > 0) {
        av_free_packet(&pkt);
    }
}

/*==============================================================================
 * - packet_queue_deinit()
 *
 * - deinit a packet queue
 */
void packet_queue_deinit (PacketQueue *q)
{
    packet_queue_flush (q);

    msgQ_delete (q->msgQ);
}

/*==============================================================================
 * - _open_media_file()
 *
 * - try to open a media file
 */
static OS_STATUS _open_media_file (const char *file_name)
{
    int i;

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if (avformat_open_input(&_G_vs.pFormatCtx, file_name, NULL, NULL) < 0) {
        serial_printf("avformat_open_input() fail! file name = %s\n", file_name);
        return -1; // Couldn't open file
    }

    // Retrieve stream information
    if(av_find_stream_info(_G_vs.pFormatCtx) < 0) {
        serial_printf("av_find_stream_info() fail!\n");
        return -1; // Couldn't find stream information
    }

    // Dump information about file onto standard error
    av_dump_format(_G_vs.pFormatCtx, 0, file_name, 0);
    
    // Find the first video and audio stream
    _G_vs.videoStream = -1;
    _G_vs.audioStream = -1;
    for(i = 0; i < _G_vs.pFormatCtx->nb_streams; i++) {
        if(_G_vs.pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO &&
                _G_vs.videoStream < 0) {
            _G_vs.videoStream=i;
        }
        if(_G_vs.pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO &&
                _G_vs.audioStream < 0) {
            _G_vs.audioStream=i;
        }
    }

    // Just check video stream
    if(_G_vs.videoStream == -1) {
        return OS_STATUS_ERROR; // Didn't find a video stream
    }

    // Init video packet queue
    packet_queue_init (&_G_vs.videoq, 30);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _T_play_thread()
 *
 * - a task that reads packets from media file and dispatch them to queue
 */
static void _T_play_thread ()
{
    AVPacket        packet;
    int             wait_times = 0;

    //serial_printf("\033[H\033[0J"); /* clear screen */

    av_init_packet(&_G_flush_pkt);
    _G_flush_pkt.data = (uint8_t *)"FLUSH";

    // Main loop
    while (atomic_get(&_G_vs.quit) == 0) {

        /* hold */
        if (_G_vs.hold) {
            delayQ_delay (10);
            continue;
        }

        // seek stuff goes here
        if (_G_vs.seek_req) {
            int stream_index= -1;
            int64_t seek_target = _G_vs.seek_pos;

            if (_G_vs.videoStream >= 0) {
                stream_index = _G_vs.videoStream;
            } else if(_G_vs.audioStream >= 0) {
                stream_index = _G_vs.audioStream;
            }

            if(stream_index>=0){
                seek_target= av_rescale_q(seek_target, AV_TIME_BASE_Q, _G_vs.pFormatCtx->streams[stream_index]->time_base);
            }
            if(av_seek_frame(_G_vs.pFormatCtx, stream_index, seek_target, _G_vs.seek_flags) < 0) {
                serial_printf("%s: error while seeking\n", _G_vs.pFormatCtx->filename);
            } else {
                if(_G_vs.audioStream >= 0) {
                }
                if(_G_vs.videoStream >= 0) {
                    packet_queue_flush(&_G_vs.videoq);
                    packet_queue_put(&_G_vs.videoq, &_G_flush_pkt);
                }
            }
            _G_vs.seek_req = 0;
        }

        if (av_read_frame(_G_vs.pFormatCtx, &packet) >= 0) {
            // Is this a packet from the video stream?
            if(packet.stream_index == _G_vs.videoStream) {
                packet_queue_put(&_G_vs.videoq, &packet); 
            } else if (packet.stream_index == _G_vs.audioStream) {
                av_free_packet(&packet);
            } else {
                av_free_packet(&packet);
            }
        } else {
            if(_G_vs.pFormatCtx->pb && _G_vs.pFormatCtx->pb->error) {
                goto _play_over;
            } else {
                if (wait_times >= 10) {
                    goto _play_over;
                } else {
                    wait_times++;
                    delayQ_delay(SYS_CLK_RATE / 10); /* no error; wait for user input */
                    continue;
                }
            }
        }

    } // (atomic_get(&_G_vs.quit) == 0)

_play_over:
    /* Mark this task is over */
    atomic_dec (&_G_vs.alive_thread_num);

    serial_printf ("tPlay is over.\n");
}


uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;
/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
static int _our_get_buffer(struct AVCodecContext *c, AVFrame *pic) {
    int ret = avcodec_default_get_buffer(c, pic);
    uint64_t *pts = av_malloc(sizeof(uint64_t));
    *pts = global_video_pkt_pts;
    pic->opaque = pts;
    return ret;
}
static void _our_release_buffer(struct AVCodecContext *c, AVFrame *pic) {
    if(pic) av_freep(&pic->opaque);
    avcodec_default_release_buffer(c, pic);
}

/*==============================================================================
 * - _T_video_thread()
 *
 * - decode video packets and show them
 */
static void _T_video_thread ()
{
    static uint8_t *fb_data[4] = {(uint8_t *)FB_ADDR, 0, 0, 0};
    static int      fb_stride[4] = {FB_STRIDE, 0, 0, 0};

    AVPacket packet;
    AVFrame *pFrame = NULL; 
    int      frameFinished;
    int      tick_start;
    double   pts;
    int      delay;

    int i, pixels;
    int bar_width = 2;

    AVCodecContext    *pCodecCtx = NULL; /* video codec context */
    AVCodec           *pCodec = NULL;
    struct SwsContext *img_convert_ctx = NULL;

    _G_vs.video_st = _G_vs.pFormatCtx->streams[_G_vs.videoStream];
    // Get a pointer to the codec context for the video stream
    pCodecCtx = _G_vs.pFormatCtx->streams[_G_vs.videoStream]->codec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL) {
        serial_printf("Unsupported codec!\n");
        goto _video_over0; // Codec not found
    }

    // Open video codec context
    if(avcodec_open(pCodecCtx, pCodec) < 0) {
        serial_printf("Can't open decodec.\n");
        goto _video_over0; // Could not open codec
    }

    _G_vs.video_clock = 0;
    _G_vs.frame_timer = (double)av_gettime() / 1000000.0;
    _G_vs.frame_last_pts = 0;
    _G_vs.frame_last_delay = 40e-3; /* 1/24 seconds */
    _G_vs.video_current_pts = 0;
    _G_vs.video_current_pts_time = av_gettime();

    // instead defualt get/release buffer function
    pCodecCtx->get_buffer = _our_get_buffer;
    pCodecCtx->release_buffer = _our_release_buffer;

    // create a scale context
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
                                     LCD_WIDTH, LCD_HEIGHT, PIX_FMT_RGB565, 
                                     SWS_BICUBIC, NULL, NULL, NULL);
    if (img_convert_ctx == NULL) {
        serial_printf("Can't initialize the conversion context!\n");
        goto _video_over1;
    }

    // Allocate video frame
    pFrame = avcodec_alloc_frame();
    if (pFrame == NULL) {
        serial_printf("Can't allocate video frame!\n");
        goto _video_over2;
    }

    tick_start = tick_get();
    while (atomic_get(&_G_vs.quit) == 0) {

        /* hold */
        if (_G_vs.hold) {
            delayQ_delay (10);
            continue;
        }

        /* get packet failed */
        if(packet_queue_get(&_G_vs.videoq, &packet) < 0) {
            goto _video_over;
        }

        /* get a flush packet */
        if(packet.data == _G_flush_pkt.data) {
            avcodec_flush_buffers(_G_vs.video_st->codec);
            continue;
        }

        // Save global pts to be stored in pFrame in first call
        global_video_pkt_pts = packet.pts;
        
        // Decode video frame
        avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

        if (packet.dts == AV_NOPTS_VALUE && pFrame->opaque &&
                  *(uint64_t *)pFrame->opaque != AV_NOPTS_VALUE) {
            pts = *(uint64_t *)pFrame->opaque;
        } else if (packet.dts != AV_NOPTS_VALUE) {
            pts = packet.dts;
        } else {
            pts = 0;
        }
        pts *= av_q2d(_G_vs.video_st->time_base);

        // Did we get a video frame?
        if (frameFinished) {

            // if we receive a set size message
            if (atomic_get(&_G_vs.video_size) != 0) {
                int dst_width = atomic_get (&_G_vs.video_size) & (0xFFFF);
                int dst_height = (atomic_get (&_G_vs.video_size) >> 16) & (0xFFFF);
                // Destroy scale context
                sws_freeContext(img_convert_ctx);
                // create a new scale context
                img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
                                                 dst_width, dst_height, PIX_FMT_RGB565, 
                                                 SWS_BICUBIC, NULL, NULL, NULL);
                atomic_set (&_G_vs.video_size, 0);
                if (img_convert_ctx == NULL) {
                    serial_printf("Can't initialize the conversion context!\n");
                    // Close the codec
                    avcodec_close(pCodecCtx);
                    goto _video_over1;
                }
            }

            // if we receive a set postion message
            if (atomic_get(&_G_vs.start_xy) != -1) {
                int x = atomic_get (&_G_vs.start_xy) & (0xFFFF);
                int y = (atomic_get (&_G_vs.start_xy) >> 16) & (0xFFFF);
                sws_6410_set_des_xy (img_convert_ctx, x, y);
                atomic_set (&_G_vs.start_xy, -1);
            }


            // show picture!
            sws_scale(img_convert_ctx,
                      (const uint8 * const *)pFrame->data,
                      pFrame->linesize,
                      0, pCodecCtx->height - 1, // clip 0 rows
                      fb_data, fb_stride);

            // synchronize
            pts = _synchronize_video(pFrame, pts);

            //serial_printf ("\033[20D""%.2f second", pts);// at line start print pts
            pixels = (int)(LCD_WIDTH * pts / (_G_vs.pFormatCtx->duration / AV_TIME_BASE) + 0.5); 
            for (i = 1; i <= bar_width; i++) {
                memset ((void *)(FB_ADDR + FB_STRIDE * (LCD_HEIGHT - i)), 0x0f, pixels * 2);
            }

            // delay
            delay = _get_video_delay(pts);
            if (delay > 0) {
                delayQ_delay (delay);
            }

        }
        
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
        
_video_over:
    // Close the codec
    avcodec_close(pCodecCtx);
_video_over2:
    // Destroy scale context
    sws_freeContext(img_convert_ctx);
_video_over1:
    // Free video frame
    av_free(pFrame);
_video_over0:
    // Mark this task is over
    atomic_dec (&_G_vs.alive_thread_num);

    serial_printf ("tVideo is over. used %d Seconds.\n",
                    (tick_get() - tick_start) / SYS_CLK_RATE);
}

/*==============================================================================
 * - _synchronize_video()
 *
 * - get the <src_frame> pts, and update <video_clock>
 */
static double _synchronize_video(AVFrame *src_frame, double pts)
{
    double frame_delay;

    if(pts != 0) {
        /* if we have pts, set video clock to it */
        _G_vs.video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = _G_vs.video_clock;
    }
    /* update the video clock */
    frame_delay = av_q2d(_G_vs.video_st->codec->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    _G_vs.video_clock += frame_delay;

    return pts;
}

static double _get_video_clock()
{
    double delta;

    delta = (av_gettime() - _G_vs.video_current_pts_time) / 1000000.0;
    return _G_vs.video_current_pts + delta;
}
static double _get_external_clock()
{
    return av_gettime() / 1000000.0;
}
static double _get_master_clock()
{
    if (_G_vs.av_sync_type == AV_SYNC_VIDEO_MASTER) {
        return _get_video_clock();
    } else {
        return _get_external_clock();
    }
}
/*==============================================================================
 * - _get_video_delay()
 *
 * - get the delay ticks to process next video packet
 */
static int _get_video_delay (double pts)
{
    double delay;
    double actual_delay;
    int    delay_tick;
    double sync_threshold;
    double ref_clock; 
    double diff;

    _G_vs.video_current_pts = pts;
    _G_vs.video_current_pts_time = av_gettime();

    delay = pts - _G_vs.frame_last_pts; /* the pts from last time */
    if(delay <= 0 || delay >= 1.0) {
        /* if incorrect delay, use previous one */
        delay = _G_vs.frame_last_delay;
    }
    /* save for next time */
    _G_vs.frame_last_delay = delay;
    _G_vs.frame_last_pts = pts;

    /* update delay to sync to audio if not master source */
    if(_G_vs.av_sync_type != AV_SYNC_VIDEO_MASTER) {
        ref_clock = _get_master_clock();
        diff = pts - ref_clock;

        /* Skip or repeat the frame. Take delay into account
           FFPlay still doesn't "know if this is the best guess." */
        sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
        if(fabs(diff) < AV_NOSYNC_THRESHOLD) {
            if(diff <= -sync_threshold) {
                delay = 0;
            } else if(diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }
    }

    _G_vs.frame_timer += delay;
    /* computer the REAL delay */
    actual_delay = _G_vs.frame_timer - (av_gettime() / 1000000.0);
    if(actual_delay < 0.010) {
        /* Really it should skip the picture instead */
        actual_delay = 0.010;
    }

    delay_tick = (int)(actual_delay * SYS_CLK_RATE);

    return delay_tick;
}

/*==============================================================================
 * - _stream_seek()
 *
 * - notice <_T_play_thread> user want a seek
 */
static void _stream_seek (int64_t pos, int rel)
{
    if(!_G_vs.seek_req) {
        _G_vs.seek_pos = pos;
        _G_vs.seek_flags = rel < 0 ? AVSEEK_FLAG_BACKWARD : 0;
        _G_vs.seek_req = 1;
    }
}

/*==============================================================================
 * - _T_msg_thread()
 *
 * - a task that wait for user message
 */
static void _T_msg_thread ()
{
    FFPLAY_MSG msg;
    double incr, pos;

    /* create message queue */
    if (msgQ_init(&_G_ffplay_msgQ, FFPLAY_MSG_NUM, sizeof(FFPLAY_MSG)) == NULL) {
        return ;
    }

    FOREVER {

        msgQ_receive (&_G_ffplay_msgQ, &msg, sizeof(msg), WAIT_FOREVER);
        switch (msg) {
            case SEEK_LITTLE_BACKWARD:
                incr = -10.0;
                goto pre_seek;
            case SEEK_LITTLE_FORWARD:
                incr = 10.0;
                goto pre_seek;
            case SEEK_LARGE_FORWARD:
                incr = 60.0;
                goto pre_seek;
            case SEEK_LARGE_BACKWARD:
                incr = -60.0;
                goto pre_seek;
            case SEEK_GOTO:
                pos = _G_vs.percent * (_G_vs.pFormatCtx->duration / AV_TIME_BASE);
                incr = pos - _get_master_clock();
                goto do_seek;
pre_seek:
                pos = _get_master_clock();
                pos += incr;
                if (pos < 0) {
                    pos = 0.1;
                }
                if (pos > (_G_vs.pFormatCtx->duration / AV_TIME_BASE)) {
                    pos = (_G_vs.pFormatCtx->duration / AV_TIME_BASE) - 1;
                }
do_seek:
                serial_printf ("pos = %d.%d, \n", (int)pos, (int)(pos * 10) % 10);
                _stream_seek((int64_t)(pos * AV_TIME_BASE), incr);
                break;

            case FFPLAY_MSG_HOLD:
                _G_vs.hold = 1 - _G_vs.hold;
                break;
            case FFPLAY_MSG_QUIT:
                goto _msg_over;
            default:
                serial_printf("receive a unknown message. %d\n", msg);
                break;
        }
    }

_msg_over:
    /* notice other tasks to quit */
	atomic_set (&_G_vs.quit, 1);
    /* delete message queue */
    msgQ_delete (&_G_ffplay_msgQ);

    // Wait other tasks are over
    while (atomic_get(&_G_vs.alive_thread_num) > 1) {
        delayQ_delay (SYS_CLK_RATE / 2);
    }

    // Delete packet queue
    packet_queue_deinit (&_G_vs.videoq);
    // Close the video file
    av_close_input_file(_G_vs.pFormatCtx);
    _G_vs.pFormatCtx = NULL;

    if (_G_over_func != NULL) {
        _G_over_func (_G_over_func_arg1, _G_over_func_arg2);
    }

    /* mark this task is over */
    atomic_dec (&_G_vs.alive_thread_num);

    serial_printf ("tFFmsg is over.\n");
}

/*==============================================================================
 ** FILE END
==============================================================================*/


/*==============================================================================
 * - media_play()
 *
 * - play a media file at default position and with default size on lcd
 *   default position is middle of lcd, default size is video size
 */
OS_STATUS media_play (const char *file_name)
{
    /* check the player is idel */
    if (atomic_get(&_G_vs.alive_thread_num) != 0) {
        return OS_STATUS_ERROR;
    }

    /* clear video status */
    _G_vs.hold = 0;
    atomic_clear (&_G_vs.quit);
    _G_vs.av_sync_type = DEFAULT_AV_SYNC_TYPE;

    /* try to open media file */
    if (_open_media_file (file_name) != OS_STATUS_OK) {
        return OS_STATUS_ERROR;
    }

    /* create a task to receive user message */
    if (task_create("tFFmsg", MSG_TASK_STACK_SIZE, MSG_TASK_PRIORITY,
                        _T_msg_thread, 0, 0) == NULL) {
    	return OS_STATUS_ERROR;
    }
    atomic_inc (&_G_vs.alive_thread_num);

    /* create a task to read packer form media file */
    if (task_create("tPlay", PLAY_TASK_STACK_SIZE, PLAY_TASK_PRIORITY,
                        _T_play_thread, 0, 0) == NULL) {
    	return OS_STATUS_ERROR;
    }
    atomic_inc (&_G_vs.alive_thread_num);

    /* create a task to decode video frame */
    if (task_create("tVideo", VIDEO_TASK_STACK_SIZE, VIDEO_TASK_PRIORITY,
                        _T_video_thread, 0, 0) == NULL) {
    	return OS_STATUS_ERROR;
    }
    atomic_inc (&_G_vs.alive_thread_num);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - media_little_forward()
 *
 * - forward 10 seconds if we can
 */
OS_STATUS media_little_forward ()
{
    int seek_event = SEEK_LITTLE_FORWARD;
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;
    return msgQ_send (&_G_ffplay_msgQ, &seek_event, sizeof(int), NO_WAIT);
}

/*==============================================================================
 * - media_large_forward()
 *
 * - forward 60 seconds if we can
 */
OS_STATUS media_large_forward ()
{
    int seek_event = SEEK_LARGE_FORWARD;
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;
    return msgQ_send (&_G_ffplay_msgQ, &seek_event, sizeof(int), NO_WAIT);
}

/*==============================================================================
 * - media_little_backward()
 *
 * - backward 10 seconds if we can
 */
OS_STATUS media_little_backward ()
{
    int seek_event = SEEK_LITTLE_BACKWARD;
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;
    return msgQ_send (&_G_ffplay_msgQ, &seek_event, sizeof(int), NO_WAIT);
}

/*==============================================================================
 * - media_large_backward()
 *
 * - backward 60 seconds if we can
 */
OS_STATUS media_large_backward ()
{
    int seek_event = SEEK_LARGE_BACKWARD;
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;
    return msgQ_send (&_G_ffplay_msgQ, &seek_event, sizeof(int), NO_WAIT);
}

/*==============================================================================
 * - media_pause()
 *
 * - pause the play
 */
OS_STATUS media_pause ()
{
    int msg = FFPLAY_MSG_HOLD;
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;
    if (_G_vs.hold == 0) {
        return msgQ_send (&_G_ffplay_msgQ, &msg, sizeof(int), NO_WAIT);
    }
    return OS_STATUS_OK;
}

/*==============================================================================
 * - media_continue()
 *
 * - continue to play
 */
OS_STATUS media_continue ()
{
    int msg = FFPLAY_MSG_HOLD;
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;
    if (_G_vs.hold == 1) {
        return msgQ_send (&_G_ffplay_msgQ, &msg, sizeof(int), NO_WAIT);
    }
    return OS_STATUS_OK;
}

/*==============================================================================
 * - media_stop()
 *
 * - close the media play
 */
OS_STATUS media_stop ()
{
    int msg = FFPLAY_MSG_QUIT;
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 1)
        return OS_STATUS_ERROR;
    return msgQ_send (&_G_ffplay_msgQ, &msg, sizeof(int), NO_WAIT);
}

/*==============================================================================
 * - media_goto()
 *
 * - jump to <percent>%
 */
OS_STATUS media_goto (int percent)
{
    int msg = SEEK_GOTO;

    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;
    if ((percent < 0) || (percent > 99))
        return OS_STATUS_ERROR;
    _G_vs.percent = percent;
    return msgQ_send (&_G_ffplay_msgQ, &msg, sizeof(int), NO_WAIT);
}


/*==============================================================================
 * - media_size_set()
 *
 * - set the video size
 */
OS_STATUS media_size_set (int width, int height)
{
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;

    atomic_set (&_G_vs.video_size, (height << 16) & width);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - media_position_set()
 *
 * - set the video position on lcd
 */
OS_STATUS media_position_set (int x, int y)
{
    if (atomic_get (&_G_vs.quit) == 1 || atomic_get (&_G_vs.alive_thread_num) < 3)
        return OS_STATUS_ERROR;

    atomic_set (&_G_vs.start_xy, (y << 16) & x);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - media_set_over_callback()
 *
 * - set the function ptr which is called when video is play over
 */
OS_STATUS media_set_over_callback (FUNC_PTR over_func, uint32 arg1, uint32 arg2)
{
    _G_over_func = over_func;
    _G_over_func_arg1 = arg1;
    _G_over_func_arg2 = arg2;

    return OS_STATUS_OK;
}

/*==============================================================================
** FILE END 2
==============================================================================*/

