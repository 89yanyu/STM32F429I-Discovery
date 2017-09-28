/*
 * main.c
 *
 *  Created on: 2017��9��24��
 *      Author: 89YY
 */
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_ltdc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "fonts.h"

#include "stdio.h"

QueueHandle_t g_hNumberBuff;
SemaphoreHandle_t g_uiMutexRML;
uint32_t g_uiDiff;
uint32_t g_uiLoopCnt;
uint64_t g_ulSum;
uint32_t g_uiState;
char g_SumBuff[16] = {};
char *g_SumForm     = "              0";
char g_LoopBuff[16] = {};
char *g_LoopForm    = "  Loop:       0";
char g_FailedBuff[16] = {};
char *g_FailedForm  = "  Failed:     0";
char g_ErrorBuff[16] = {};
char *g_ErrorForm   = "  Error:      0";
char g_FPSBuff[16] = {};
char *g_FPSForm     = "         FPS: 0";

SemaphoreHandle_t g_uiMutexSL;
uint32_t g_uiFailed;

#define LED_NONE 0
#define LED_GREEN 1
#define LED_RED 2

/**
 * LEDSet: Set led
 */
void LEDSet(uint32_t uiLedState)
{
    GPIO_WriteBit(GPIOG, GPIO_Pin_13, (uiLedState & LED_GREEN) != 0 ? 1 : 0);
    GPIO_WriteBit(GPIOG, GPIO_Pin_14, (uiLedState & LED_RED) != 0 ? 1 : 0);
}

/**
 * Sender_Task: Product the number
 */
void SenderTask(void* arg)
{
    traceString stLogger = xTraceRegisterString("Sender Task");
    uint32_t uiNum = 1;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        if (pdPASS != xQueueSend(g_hNumberBuff, &uiNum, 0))
        {
            vTracePrintF(stLogger, "Send to queue failed.");
            if (pdPASS == xSemaphoreTake(g_uiMutexSL, 1))
            {
                g_uiFailed++;
                vTracePrintF(stLogger, "Fail: %d", g_uiFailed);
                xSemaphoreGive(g_uiMutexSL);
            }
        }
        uiNum += 1;
        if (uiNum > 10000)
        {
            uiNum = 1;
        }
        vTaskDelayUntil(&xLastWakeTime, 2 / portTICK_RATE_MS);
    }
}

/**
 * Receiver_Task: Produce the number
 */
void ReceiverTask(void* arg)
{
    traceString stLogger = xTraceRegisterString("Receiver Task");
    uint64_t ulTmpSum = 0;
    uint32_t uiNumber = 0;
    uint32_t uiLast = 0;
    uint32_t uiTmpDiff;
    uint32_t uiTmpLoopCnt;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        uiTmpDiff = 0;
        uiTmpLoopCnt = 0;
        ulTmpSum = 0;
        while (pdPASS == xQueueReceive(g_hNumberBuff, &uiNumber, 0))
        {
            ulTmpSum += uiNumber;
            if (uiNumber > uiLast)
            {
                uiTmpDiff += uiNumber - uiLast - 1;
            }
            else
            {
                uiTmpDiff += uiNumber + 10000 - uiLast - 1;
                uiTmpLoopCnt += 1;
            }
            uiLast = uiNumber;
        }
        if (pdPASS == xSemaphoreTake(g_uiMutexRML, 10 / portTICK_RATE_MS))
        {
            g_uiDiff += uiTmpDiff;
            g_uiLoopCnt += uiTmpLoopCnt;
            g_ulSum += ulTmpSum;
            xSemaphoreGive(g_uiMutexRML);
        }
        vTracePrintF(stLogger, "The sum of this round is %u\n", ulTmpSum);
        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_RATE_MS);
    }
}

/**
 * Monitor_Task: Check the task
 */
void MonitorTask(void* arg)
{
    traceString stLogger = xTraceRegisterString("Monitor Task");
    uint32_t uiTmpDiff;
    uint32_t uiTmpLoopCnt;
    uint32_t uiTmpState;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        uiTmpState = 0;
        if (pdPASS == xSemaphoreTake(g_uiMutexRML, 10))
        {
            uiTmpDiff = g_uiDiff;
            uiTmpLoopCnt = g_uiLoopCnt;
            g_uiState = (uiTmpDiff != 0);
            xSemaphoreGive(g_uiMutexRML);
        }

        if (uiTmpDiff == 0)
        {
            vTracePrintF(stLogger, "OK, %d loops are done.", uiTmpLoopCnt);
        }
        else
        {
            STM_EVAL_LEDOn(LED4);
            STM_EVAL_LEDOff(LED3);
            vTracePrintF(stLogger, "Wrong, difference is %d.", uiTmpDiff);
        }
        vTaskDelayUntil(&xLastWakeTime, 10000 / portTICK_RATE_MS);
    }
}

void buildString(char *buff, char *format, uint64_t number)
{
    for (int64_t i = 14; i >= 0; i--)
    {
        if (number > 0)
        {
            buff[i] = number % 10 + '0';
            number /= 10;
        }
        else
        {
            buff[i] = format[i];
        }
    }
}

/**
 * LCDShow_Task: Show info in LCD
 */
void LCDShowTask(void* arg)
{
    traceString stLogger = xTraceRegisterString("LCD Task");
    uint32_t uiTmpDiff, uiLastDiff = -1;
    uint32_t uiTmpLoopCnt, uiLastLoopCnt = -1;
    uint64_t ulTmpSum, ulLastSum = -1;
    uint32_t uiTmpFailed, uiLastFailed = -1;
    uint32_t uiTmpState, uiLastState = -1;
    TickType_t stLast = xTaskGetTickCount(), stNow;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        if (pdPASS == xSemaphoreTake(g_uiMutexRML, 15))
        {
            uiTmpDiff = g_uiDiff;
            uiTmpLoopCnt = g_uiLoopCnt;
            ulTmpSum = g_ulSum;
            uiTmpState = g_uiState;
            xSemaphoreGive(g_uiMutexRML);
        }
        if (pdPASS == xSemaphoreTake(g_uiMutexSL, 15))
        {
            uiTmpFailed = g_uiFailed;
            xSemaphoreGive(g_uiMutexSL);
        }

        if (uiTmpFailed != uiLastFailed)
        {
            buildString(g_FailedBuff, g_FailedForm, uiTmpFailed);
            LCD_DisplayStringLine(LCD_LINE_2, g_FailedBuff);
            uiLastFailed = uiTmpFailed;
        }

        if (ulTmpSum != ulLastSum)
        {
            buildString(g_SumBuff, g_SumForm, ulTmpSum);
            LCD_DisplayStringLine(LCD_LINE_5, g_SumBuff);
            ulLastSum = ulTmpSum;
        }
        if (uiTmpDiff != uiLastDiff)
        {
            buildString(g_ErrorBuff, g_ErrorForm, uiTmpDiff);
            LCD_DisplayStringLine(LCD_LINE_6, g_ErrorBuff);
            uiLastDiff = uiTmpDiff;
        }
        if (uiTmpLoopCnt != uiLastLoopCnt)
        {
            buildString(g_LoopBuff, g_LoopForm, uiTmpLoopCnt);
            LCD_DisplayStringLine(LCD_LINE_8, g_LoopBuff);
            uiLastLoopCnt = uiTmpLoopCnt;
        }

        if (uiTmpState != uiLastState)
        {
            if (uiTmpState)
            {
                LCD_DisplayStringLine(LCD_LINE_9, "  State:   FAIL");
            }
            else
            {
                LCD_DisplayStringLine(LCD_LINE_9, "  State:     OK");
            }
            uiLastState = uiTmpState;
        }

        stNow = xTaskGetTickCount();
        buildString(g_FPSBuff, g_FPSForm, 1000 / (stNow - stLast));
        LCD_DisplayStringLine(LCD_LINE_0, g_FPSBuff);
        stLast = stNow;
        vTaskDelayUntil(&xLastWakeTime, 16.6 / portTICK_RATE_MS);
    }
}

void LCDInit()
{
    /*Initialize the LCD */
    LCD_Init();
    LCD_LayerInit();
    /* Enablethe LTDC */
    LTDC_Cmd(ENABLE);
    /* Set LCDBackground Layer  */
    LCD_SetLayer(LCD_BACKGROUND_LAYER);
    /* Clearthe Background Layer */
    LCD_Clear(LCD_COLOR_WHITE);
    /*Configure the transparency for background */
    LCD_SetTransparency(255);
    /* Set LCDForeground Layer  */
    LCD_SetLayer(LCD_FOREGROUND_LAYER);
    /*Configure the transparency for foreground */
    LCD_SetTransparency(255);
    LCD_Clear(LCD_COLOR_WHITE);
    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_SetFont(&Font16x24);
    LCD_DisplayStringLine(LCD_LINE_0, "         FPS:  ");
    LCD_DisplayStringLine(LCD_LINE_1, "Sender:        ");
    LCD_DisplayStringLine(LCD_LINE_2, "  Failed:      ");
    LCD_DisplayStringLine(LCD_LINE_3, "Receiver:      ");
    LCD_DisplayStringLine(LCD_LINE_4, "  Sum:         ");
    LCD_DisplayStringLine(LCD_LINE_5, "               ");
    LCD_DisplayStringLine(LCD_LINE_6, "  Error:       ");
    LCD_DisplayStringLine(LCD_LINE_7, "Monitor:       ");
    LCD_DisplayStringLine(LCD_LINE_8, "  Loop:        ");
    LCD_DisplayStringLine(LCD_LINE_9, "  State:       ");
    LCD_DisplayOn();
}

int main(void)
{
    //��ʼ��LED
    STM_EVAL_LEDInit(LED3);
    STM_EVAL_LEDInit(LED4);

    STM_EVAL_LEDOn(LED3);
    STM_EVAL_LEDOn(LED4);

    //����Trace
    STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
    if (STM_EVAL_PBGetState(BUTTON_USER))
    {
        vTraceEnable(TRC_START_AWAIT_HOST);
    }

    //LCDInit();

    //���õ�ǰ������Logger
    traceString stLogger = xTraceRegisterString("Main");

    //����ȫ�ֶ���
    g_hNumberBuff = xQueueCreate(512, sizeof(int));
    if (NULL == g_hNumberBuff)
    {
        //��������ʧ��
        STM_EVAL_LEDOff(LED3);
        vTracePrint(stLogger, "Number buffer create failed");
        for(;;);
    }

    //����RML֮���ȫ��������
    g_uiMutexRML = xSemaphoreCreateMutex();
    if (NULL == g_uiMutexRML)
    {
        //����������ʧ��
        STM_EVAL_LEDOff(LED3);
        vTracePrint(stLogger, "Mutex of Receiver, Monitor & LCD create failed");
        for(;;);
    }

    //����SL֮���ȫ��������
    g_uiMutexSL = xSemaphoreCreateMutex();
    if (NULL == g_uiMutexSL)
    {
        //����������ʧ��
        STM_EVAL_LEDOff(LED3);
        vTracePrint(stLogger, "Mutex of Sender & LCD create failed");
        for(;;);
    }

    //��ʼ��ȫ�ֱ���
    g_uiDiff = 0;
    g_uiLoopCnt = 0;
    g_ulSum = 0;
    g_uiState = 0;

    //��������
    xTaskCreate(SenderTask,
                "Sender Task",
                configMINIMAL_STACK_SIZE,
                (void*) NULL,
                tskIDLE_PRIORITY + 5UL,
                NULL);
    xTaskCreate(ReceiverTask,
                "Receiver Task",
                configMINIMAL_STACK_SIZE,
                (void*) NULL,
                tskIDLE_PRIORITY + 4UL,
                NULL);
    xTaskCreate(MonitorTask,
                "Monitor Task",
                configMINIMAL_STACK_SIZE,
                (void*) NULL,
                tskIDLE_PRIORITY + 3UL,
                NULL);
    /*
    xTaskCreate(LCDShowTask,
                "LCDShow Task",
                configMINIMAL_STACK_SIZE,
                (void*) NULL,
                tskIDLE_PRIORITY + 2UL,
                NULL);
                */
    STM_EVAL_LEDOff(LED4);

    //�����������
    vTaskStartScheduler();

    for( ;; );

}

void vApplicationTickHook( void )
{
}

void vApplicationMallocFailedHook( void )
{
    taskDISABLE_INTERRUPTS();
    for( ;; );
}

void vApplicationIdleHook( void )
{
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    taskDISABLE_INTERRUPTS();
    for( ;; );
}
