/*
 * 配置视频外设驱动程序所占用的内存, 单位: KByte
 */
#define CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC   (3 * 1024)                  /*  FIMC                        */
#define CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC    (5 * 1024)                  /*  MFC  多格式编解码器         */
#define CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG   (2 * 1024)                  /*  JPEG 编解码器               */
#define CONFIG_VIDEO_SAMSUNG_MEMSIZE_POST   (0 * 1024)                  /*  后端处理器                  */

#define CONFIG_CPU_S3C6410
