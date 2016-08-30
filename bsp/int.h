/*==============================================================================
** int.h -- interrupt controller driver interface for DfewOS.
**
** MODIFY HISTORY:
**
** 2012-03-03 wdf Create.
==============================================================================*/

#ifndef __INT_H__
#define __INT_H__

#ifdef __cpulsplus
extern "C" {
#endif

#define INT_NUMBER_ADC             63                                  /* EOC interrupt            VIC1*/
#define INT_NUMBER_PENDNUP         62                                  /* ADC Pen down/up interupt VIC1*/
#define INT_NUMBER_SEC             61                                  /* Security interrupt       VIC1*/
#define INT_NUMBER_RTC_ALARM       60                                  /* RTC alarm interrupt      VIC1*/
#define INT_NUMBER_IrDA            59                                  /* IrDA interrupt           VIC1*/
#define INT_NUMBER_OTG             58                                  /* USB OTG interrupt        VIC1*/
#define INT_NUMBER_HSMMC1          57                                  /* HSMMC1 interrupt         VIC1*/
#define INT_NUMBER_HSMMC0          56                                  /* HSMMC0 interrupt         VIC1*/
#define INT_NUMBER_HOSTIF          55                                  /* Host Interface interrupt VIC1*/
#define INT_NUMBER_MSM             54                                  /* MSM modem I/F interrupt  VIC1*/
#define INT_NUMBER_EINT4           53                                  /* External INT Group 1 ~ 9 VIC1*/
#define INT_NUMBER_HSIrx           52                                  /* HSI Rx interrupt         VIC1*/
#define INT_NUMBER_HSItx           51                                  /* HSI Tx interrupt         VIC1*/
#define INT_NUMBER_I2C0            50                                  /* I2C 0 interrupt          VIC1*/
#define INT_NUMBER_SPI1_HSMMC2     49                         /* SPI1 interrupt | HSMMC2 interrupt VIC1*/
#define INT_NUMBER_SPI0            48                                  /* SPI0 interrupt           VIC1*/
#define INT_NUMBER_UHOST           47                                  /* USB Host interrupt       VIC1*/
#define INT_NUMBER_CFC             46                                  /* CFCON interrupt          VIC1*/
#define INT_NUMBER_NFC             45                                  /* NFCON interrupt          VIC1*/
#define INT_NUMBER_ONENAND1        44                                  /* OneNAND interrupt bank1  VIC1*/
#define INT_NUMBER_ONENAND0        43                                  /* OneNAND interrupt bank0  VIC1*/
#define INT_NUMBER_DMA1            42                                  /* DMA1 interrupt           VIC1*/
#define INT_NUMBER_DMA0            41                                  /* DMA0 interrupt           VIC1*/
#define INT_NUMBER_UART3           40                                  /* UART3 interrupt          VIC1*/
#define INT_NUMBER_UART2           39                                  /* UART2 interrupt          VIC1*/
#define INT_NUMBER_UART1           38                                  /* UART1 interrupt          VIC1*/
#define INT_NUMBER_UART0           37                                  /* UART0 interrupt          VIC1*/
#define INT_NUMBER_AC97            36                                  /* AC97 interrupt           VIC1*/
#define INT_NUMBER_PCM1            35                                  /* PCM1 interrupt           VIC1*/
#define INT_NUMBER_PCM0            34                                  /* PCM0 interrupt           VIC1*/
#define INT_NUMBER_EINT3           33                                  /* External INT 20 ~ 27     VIC1*/
#define INT_NUMBER_EINT2           32                                  /* External INT 12 ~ 19     VIC1*/
#define INT_NUMBER_LCD_2           31                                  /* LCD System I/F done      VIC0*/
#define INT_NUMBER_LCD_1           30                                  /* LCD VSYNC interrupt      VIC0*/
#define INT_NUMBER_LCD_0           29                                  /* LCD FIFO underrun        VIC0*/
#define INT_NUMBER_TIMER4          28                                  /* Timer 4 interrupt        VIC0*/
#define INT_NUMBER_TIMER3          27                                  /* Timer 3 interrupt        VIC0*/
#define INT_NUMBER_WDT             26                                  /* Watchdog timer           VIC0*/
#define INT_NUMBER_TIMER2          25                                  /* Timer 2 interrupt        VIC0*/
#define INT_NUMBER_TIMER1          24                                  /* Timer 1 interrupt        VIC0*/
#define INT_NUMBER_TIMER0          23                                  /* Timer 0 interrupt        VIC0*/
#define INT_NUMBER_KEYPAD          22                                  /* Keypad interrupt         VIC0*/
#define INT_NUMBER_ARM_DMAS        21                                  /* ARM DMAS interrupt       VIC0*/
#define INT_NUMBER_ARM_DMA         20                                  /* ARM DMA interrupt        VIC0*/
#define INT_NUMBER_ARM_DMAERR      19                                  /* ARM DMA Error interrupt  VIC0*/
#define INT_NUMBER_SDMA1           18                                  /* Secure DMA1 interrupt    VIC0*/
#define INT_NUMBER_SDMA0           17                                  /* Secure DMA0 interrupt    VIC0*/
#define INT_NUMBER_MFC             16                                  /* MFC interrupt            VIC0*/
#define INT_NUMBER_JPEG            15                                  /* JPEG interrupt           VIC0*/
#define INT_NUMBER_BATF            14                                  /* Battery fault interrupt  VIC0*/
#define INT_NUMBER_SCALER          13                                  /* TV Scaler interrupt      VIC0*/
#define INT_NUMBER_TVENC           12                                  /* TV Encoder interrupt     VIC0*/
#define INT_NUMBER_2D              11                                  /* 2D interrupt             VIC0*/
#define INT_NUMBER_ROTATOR         10                                  /* Rotator interrupt        VIC0*/
#define INT_NUMBER_POST0           9                                   /* Post processor interrupt VIC0*/
#define INT_NUMBER_3D              8                                   /* 3D Graphic Controller    VIC0*/
#define INT_NUMBER_RESERVED        7                                   /* Reserved 7 Reserved      VIC0*/
#define INT_NUMBER_I2S0_1_V40      6      /* I2S 0 interrupt | I2S 1 interrupt | I2S V40 interrupt VIC0*/
#define INT_NUMBER_I2C1            5                                   /* I2C 1 interrupt          VIC0*/
#define INT_NUMBER_CAMIF_P         4                                   /* Camera interface         VIC0*/
#define INT_NUMBER_CAMIF_C         3                                   /* Camera interface         VIC0*/
#define INT_NUMBER_RTC_TIC         2                                   /* RTC TIC interrupt        VIC0*/
#define INT_NUMBER_EINT1           1                                   /* External interrupt 4~11  VIC0*/
#define INT_NUMBER_EINT0           0                                   /* External interrupt 0~3   VIC0*/

typedef void (*FUNC_ISR) (void *arg);

int int_init ();
int int_connect (unsigned int int_num, FUNC_ISR isr, void *arg);
int int_disconnect (unsigned int int_num);
int int_enable (unsigned int int_num);
int int_disable (unsigned int int_num);

#ifdef __cpulsplus
}
#endif

#endif /* __INT_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

