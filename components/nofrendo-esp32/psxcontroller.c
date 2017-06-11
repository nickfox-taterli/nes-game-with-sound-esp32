// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"


#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "psxcontroller.h"
#include "sdkconfig.h"

#define PSX_CLK CONFIG_HW_PSX_CLK
#define PSX_DAT CONFIG_HW_PSX_DAT
#define PSX_ATT CONFIG_HW_PSX_ATT
#define PSX_CMD CONFIG_HW_PSX_CMD

#define DELAY() asm("nop; nop; nop; nop;nop; nop; nop; nop;nop; nop; nop; nop;nop; nop; nop; nop;nop; nop; nop;")


#if CONFIG_HW_PSX_ENA

/* Sends and receives a byte from/to the PSX controller using SPI */
static int psxSendRecv(int send)
{
    int x;
    int ret = 0;
    volatile int delay;

#if 0
    while(1)
    {
        GPIO.out_w1ts = (1 << PSX_CMD);
        GPIO.out_w1ts = (1 << PSX_CLK);
        GPIO.out_w1tc = (1 << PSX_CMD);
        GPIO.out_w1tc = (1 << PSX_CLK);
    }
#endif

    GPIO.out_w1tc = (1 << PSX_ATT);
    for (delay = 0; delay < 100; delay++);
    for (x = 0; x < 8; x++)
    {
        if (send & 1)
        {
            GPIO.out_w1ts = (1 << PSX_CMD);
        }
        else
        {
            GPIO.out_w1tc = (1 << PSX_CMD);
        }
        DELAY();
        for (delay = 0; delay < 100; delay++);
        GPIO.out_w1tc = (1 << PSX_CLK);
        for (delay = 0; delay < 100; delay++);
        GPIO.out_w1ts = (1 << PSX_CLK);
        ret >>= 1;
        send >>= 1;
        if (GPIO.in & (1 << PSX_DAT)) ret |= 128;
    }
    return ret;
}

static void psxDone()
{
    DELAY();
    GPIO_REG_WRITE(GPIO_OUT_W1TS_REG, (1 << PSX_ATT));
}


int psxReadInput()
{
    int b1, b2;

    psxSendRecv(0x01); //wake up
    psxSendRecv(0x42); //get data
    psxSendRecv(0xff); //should return 0x5a
    b1 = psxSendRecv(0xff); //buttons byte 1
    b2 = psxSendRecv(0xff); //buttons byte 2
    psxDone();
    return (b2 << 8) | b1;

}


void psxcontrollerInit()
{
    volatile int delay;
    int t;
    gpio_config_t gpioconf[2] =
    {
        {
            .pin_bit_mask = (1 << PSX_CLK) | (1 << PSX_CMD) | (1 << PSX_ATT),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_PIN_INTR_DISABLE
        }, {
            .pin_bit_mask = (1 << PSX_DAT),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_PIN_INTR_DISABLE
        }
    };
    gpio_config(&gpioconf[0]);
    gpio_config(&gpioconf[1]);

    //Send a few dummy bytes to clean the pipes.
    psxSendRecv(0);
    psxDone();
    for (delay = 0; delay < 500; delay++) DELAY();
    psxSendRecv(0);
    psxDone();
    for (delay = 0; delay < 500; delay++) DELAY();
    //Try and detect the type of controller, so we can give the user some diagnostics.
    psxSendRecv(0x01);
    t = psxSendRecv(0x00);
    psxDone();
    if (t == 0 || t == 0xff)
    {
        printf("No PSX/PS2 controller detected (0x%X). You will not be able to control the game.\n", t);
    }
    else
    {
        printf("PSX controller type 0x%X\n", t);
    }
}


#else


/*

Can use free GPIO

GPIO26 - AUDIO

MISO - 25
MOSI - 23
CLK - 19
CS - 22
DC - 21
RST - 18
BCKL - 5

GPIO4
GPIO16
GPIO17
GPIO20 (NO IN MODULE)
GPIO27

BIT0 = SELECT
BIT3 = START
BIT4 = U
BIT5 = R
BIT6 = D
BIT7 = L
BIT12 = RST
BIT13 = A
BIT14 = B

*/

#define TATER_KP_TM_STB 4
#define TATER_KP_TM_CLK 16
#define TATER_KP_TM_DIO 17
#define TATER_KP_GAME_RST 27

#define TM163_SCAN_DIV 10

volatile int delay;

gpio_config_t tater_kp_gpioconf[4] =
{
    {
        .pin_bit_mask = (1 << TATER_KP_TM_STB) | (1 << TATER_KP_TM_CLK),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_PIN_INTR_DISABLE
    }, {
        .pin_bit_mask = (1 << TATER_KP_GAME_RST),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_PIN_INTR_DISABLE
    }, {
        .pin_bit_mask = (1 << TATER_KP_TM_DIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_PIN_INTR_DISABLE
    }, {
        .pin_bit_mask = (1 << TATER_KP_TM_DIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_PIN_INTR_DISABLE
    }
};

void TM1638_Write(unsigned char	DATA)
{
    unsigned char i;
    gpio_config(&tater_kp_gpioconf[2]);
    for(i = 0; i < 8; i++)
    {
        //for (delay=0; delay<10; delay++);
        GPIO.out_w1tc = (1 << TATER_KP_TM_CLK);
        for (delay = 0; delay < TM163_SCAN_DIV; delay++);
        if(DATA & 0X01)
            GPIO.out_w1ts = (1 << TATER_KP_TM_DIO);
        else
            GPIO.out_w1tc = (1 << TATER_KP_TM_DIO);
        DATA >>= 1;
        for (delay = 0; delay < TM163_SCAN_DIV; delay++);
        GPIO.out_w1ts = (1 << TATER_KP_TM_CLK);
        for (delay = 0; delay < TM163_SCAN_DIV; delay++);
    }
}
unsigned char TM1638_Read(void)
{
    unsigned char i;
    unsigned char temp = 0;;
    gpio_config(&tater_kp_gpioconf[3]);
    for(i = 0; i < 8; i++)
    {
        temp >>= 1;
        //for (delay=0; delay<10; delay++);
        GPIO.out_w1tc = (1 << TATER_KP_TM_CLK);
        for (delay = 0; delay < TM163_SCAN_DIV; delay++);
        if(GPIO.in & (1 << TATER_KP_TM_DIO))
            temp |= 0x80;
        for (delay = 0; delay < TM163_SCAN_DIV; delay++);
        GPIO.out_w1ts = (1 << TATER_KP_TM_CLK);
        for (delay = 0; delay < TM163_SCAN_DIV; delay++);
    }
    return temp;
}

int psxReadInput()
{
    int ret = 0x0000;
    unsigned char c[4];


    //gpio_config(&tater_kp_gpioconf[1]); /* DIO OUTPUT */
    GPIO.out_w1tc = (1 << TATER_KP_TM_STB);
    //for (delay=0; delay<100; delay++);
    TM1638_Write(0x42);

    //gpio_config(&tater_kp_gpioconf[2]);	/* DIO INPUT */

    c[0] = TM1638_Read();
    c[1] = TM1638_Read();
    c[2] = TM1638_Read();
    c[3] = TM1638_Read();

    //for (delay=0; delay<100; delay++);
    GPIO.out_w1ts = (1 << TATER_KP_TM_STB);


    if (c[0] & 0x01) ret |= (1 << 14); /* K1 */
    if (c[0] & 0x10) ret |= (1 << 13); /* K2 */
    if (c[1] & 0x01) ret |= (1 << 3); /* K3 */
    if (c[1] & 0x10) ret |= (1 << 0); /* K4 */
    if (c[2] & 0x01) ret |= (1 << 6); /* K5 */
    if (c[2] & 0x10) ret |= (1 << 5); /* K6 */
    if (c[3] & 0x01) ret |= (1 << 7); /* K7 */
    if (c[3] & 0x10) ret |= (1 << 4); /* K8 */

    if (GPIO.in & (1 << TATER_KP_GAME_RST)) ret |= (1 << 12);


    return 	~ret;
}


void psxcontrollerInit()
{

    unsigned char i;


    gpio_config(&tater_kp_gpioconf[0]);
    gpio_config(&tater_kp_gpioconf[1]);

    gpio_config(&tater_kp_gpioconf[2]);

}

#endif