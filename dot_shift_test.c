//------------------------------------------------------------------------------
//
// Dot-Matrix LED Test Application.(ODROIDs SPIDEV)
//
// if defined USE_WIRING_PI_LIB
//      Compile : gcc -o <create excute file name> <source file name> -lwiringPi -lwiringPiDev -lpthread
// else
//      Compile : gcc -o <create excute file name> <source file name>
//
// Run : sudo ./<created excute file name>
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
 
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

// wiringPi Library use flag (if disable then normal spidev control)
//#define USE_WIRING_PI_LIB

// Display Item flag (if disable then Data & Time Display)
#define DISPLAY_IP_ADDR

#define START_STR               "Hello! Odroid!"

#define SPI_MAX_SPEED           1000000     // 1Mhz SPI Speed

//------------------------------------------------------------------------------
//
// Application use value define
//
//------------------------------------------------------------------------------
#include "dotfont.h"

#define SHIFT_DELAY_MS          70
#define DISPLAY_DELAY_MS        1000
#define MAX_SHIFT_CHAR_COUNT    256
#define DOT_MATRIX_8X8_COUNT    MAX_SHIFT_CHAR_COUNT

//
// Virtual Dotmatrix framebuffer define.
//
unsigned char   DotMatrixFb[DOT_MATRIX_8X8_COUNT][8] = { 0 };

//------------------------------------------------------------------------------
//
// wiringPi use header file
//
//------------------------------------------------------------------------------
#if defined(USE_WIRING_PI_LIB)
    #include <wiringPi.h>
    #include <wiringPiSPI.h>
#else
    #define SPIDEV_NODE_NAME    "/dev/spidev0.0"    // ODROID-C1/C1+
    //#define SPIDEV_NODE_NAME    "/dev/spidev1.0"    // ODROID-XU3/XU4
    int SpidevFd = -1;
#endif

//------------------------------------------------------------------------------
//
// SPI Dot Matrix Initialize data
//
//------------------------------------------------------------------------------
unsigned char dotled_init[5][4] = {
    {0x09,0x00,0x09,0x00},
    {0x0a,0x03,0x0a,0x03},
    {0x0b,0x07,0x0b,0x07},
    {0x0c,0x01,0x0c,0x01},
    {0x0f,0x00,0x0f,0x00},
};   

//------------------------------------------------------------------------------
//
// Get ethernet ip addr
//
//------------------------------------------------------------------------------
int get_ethernet_ip (unsigned char *buf)
{
    struct  ifaddrs *ifa;
    int     str_cnt;

    getifaddrs(&ifa);
    
    while(ifa)  {
        if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)    {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)ifa->ifa_addr;
            
            if(0==strncmp(ifa->ifa_name, "eth", 2)) {
//                str_cnt = sprintf(buf, "IP(%s):%s", ifa->ifa_name, inet_ntoa(pAddr->sin_addr));
                str_cnt = sprintf(buf, "IP:%s", inet_ntoa(pAddr->sin_addr));
            }
        }
        ifa = ifa->ifa_next;
    }
    freeifaddrs(ifa);
    
    return  str_cnt;
}

//------------------------------------------------------------------------------
//
// Get date & time
//
//------------------------------------------------------------------------------
int get_date_time   (unsigned char *buf)
{
    time_t      tm_time;
    struct tm   *st_time;
    int         str_cnt;

    time(&tm_time);     

    st_time = localtime(&tm_time);

    return  (strftime(buf, MAX_SHIFT_CHAR_COUNT , "%Y/%m/%d %a %H:%M:%S %p", st_time));
}

//------------------------------------------------------------------------------
//
// SPI Dot Matrix H/W Write
//
//------------------------------------------------------------------------------
#if !defined(USE_WIRING_PI_LIB)
void dotmatrix_write (unsigned char *buf, unsigned int len)
{
    struct spi_ioc_transfer tr;

    memset(&tr, 0, sizeof(tr));
    tr.tx_buf           = (unsigned long)buf;
    tr.rx_buf           = (unsigned long)buf;
    tr.len              = len;
    tr.speed_hz         = SPI_MAX_SPEED;
    tr.bits_per_word    = 8;
    tr.delay_usecs      = 0;
    tr.cs_change        = 0;

    if (ioctl(SpidevFd, SPI_IOC_MESSAGE(1), &tr) < 1)
        fprintf (stderr, "%s: can't send SPI message", __func__);
}    
#endif
//------------------------------------------------------------------------------
//
// SPI Dot Matrix update
//
//------------------------------------------------------------------------------
void dotmatrix_update   (void)
{
    unsigned char spi_data[4] = { 0, }, i;
    
    for(i = 0; i < 8; i++)  {
        spi_data[0] = i +1; spi_data[1] = DotMatrixFb[1][i];
        spi_data[2] = i +1; spi_data[3] = DotMatrixFb[0][i];
#if defined(USE_WIRING_PI_LIB)
        wiringPiSPIDataRW(0, &spi_data[0], sizeof(spi_data));
#else
        dotmatrix_write(&spi_data[0], sizeof(spi_data));
#endif
    }
}

//------------------------------------------------------------------------------
//
// Dot Matrix Frambuffer Shift
//
//------------------------------------------------------------------------------
void dotmatrix_buffer_shift(int delay_ms)
{
    int     i, j;

    for(j = 0; j < DOT_MATRIX_8X8_COUNT; j++)   {
        for(i = 0; i < 8; i++)  {
            DotMatrixFb[j][i] <<= 1;
            if((j != DOT_MATRIX_8X8_COUNT -1) && (DotMatrixFb[j + 1][i] & 0x80))
                DotMatrixFb[j][i] |= 0x01;
        }
    }
    dotmatrix_update();
    
    usleep(delay_ms * 1000);
}

//------------------------------------------------------------------------------
//
// Dot Matrix Framebuffer Update
//
//------------------------------------------------------------------------------
int dotmatrix_buffer_update (unsigned char *str, int delay_ms)
{
    int     i, buf_pos = 0, bits_count = 0;
    
    while(*str != 0x00) {
        for(i = 0; i < 8; i++)  {
            DotMatrixFb[buf_pos][i] = VINCENT_FONT[*str][i];
            bits_count++;
        }
        buf_pos++;  str++;
    }

    dotmatrix_update();     

    usleep(delay_ms * 1000);

    return  (bits_count);    // String total bits
}

//------------------------------------------------------------------------------
//
// system init
//
//------------------------------------------------------------------------------
int system_init(void)
{
    int i, spi_speed = SPI_MAX_SPEED;

#if defined(USE_WIRING_PI_LIB)
    // spi channel open, speed = 1Mhz       
    if(wiringPiSPISetup(0, spi_speed) < 0)    
    {
        fprintf (stderr, "%s: wiringPiSPISetup failed\n", __func__);    return -1;
    }
 
    for(i = 0; i < 5; i++)  {
        wiringPiSPIDataRW(0, &dotled_init[i][0], 4);
    }

#else
    if ((SpidevFd = open(SPIDEV_NODE_NAME, O_RDWR)) < 0)  {
        fprintf (stderr, "%s : Hardware Init(%s) failed\n", __func__, SPIDEV_NODE_NAME);
        return  -1;
    }
    if(ioctl(SpidevFd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0)    {
        fprintf (stderr, "%s : can't set max speed Hz", __func__);
        return  -1;
    }

    for(i = 0; i < 5; i++)  {
        dotmatrix_write(&dotled_init[i][0], 4);
    }
#endif    
 
    return  0;
}
 
//------------------------------------------------------------------------------
//
// Start Program
//
//------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    int             speed, shift_len;
    unsigned char   disp_str[MAX_SHIFT_CHAR_COUNT];
     
#if defined(USE_WIRING_PI_LIB)
    wiringPiSetup ();
#endif    
 
    if (system_init() < 0)
    {
        fprintf (stderr, "%s: System Init failed\n", __func__);     return -1;
    }

#if defined(USE_WIRING_PI_LIB)
    if (ioctl(wiringPiSPIGetFd(0), SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1)   {
        fprintf (stderr, "%s: Get Max Speed Error!\n", __func__);   return -1;
    }
#else
    if (ioctl(SpidevFd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1)   {
        fprintf (stderr, "%s: Get Max Speed Error!\n", __func__);   return -1;
    }
#endif

    printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

    while(1)
    {
        // display buffer clear
        memset(disp_str, 0x00, sizeof(disp_str));

#if defined(START_STR)        
        shift_len  = sprintf(&disp_str[0], "%s ", START_STR);
#else
        shift_len = 0;
#endif
        
#if defined(DISPLAY_IP_ADDR)        
        get_ethernet_ip(&disp_str[shift_len]);
#else        
        get_date_time(&disp_str[shift_len]);
#endif

        shift_len = dotmatrix_buffer_update(disp_str, DISPLAY_DELAY_MS);

        while(shift_len--)  dotmatrix_buffer_shift(SHIFT_DELAY_MS);
    }
    return 0 ;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
