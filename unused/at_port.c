/*
 * File	: at_port.c
 * This file is part of Espressif's AT+ command set program.
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "at.h"
#include "user_interface.h"
#include "osapi.h"
#include "driver/uart.h"

/** @defgroup AT_PORT_Defines
  * @{
  */
#define at_cmdLenMax 128
#define at_dataLenMax 2048
/**
  * @}
  */

/** @defgroup AT_PORT_Extern_Variables
  * @{
  */

//extern UartDevice UartDev;
//extern bool IPMODE;

/**
  * @}
  */

/** @defgroup AT_PORT_Extern_Functions
  * @{
  */
extern void at_ipDataSending(uint8_t *pAtRcvData);
extern void at_ipDataSendNow(void);
/**
  * @}
  */

os_event_t    at_recvTaskQueue[at_recvTaskQueueLen];
//os_event_t    at_busyTaskQueue[at_busyTaskQueueLen];
os_event_t    at_procTaskQueue[at_procTaskQueueLen];

BOOL specialAtState = TRUE;
at_stateType  at_state;
uint8_t *pDataLine;
BOOL echoFlag = TRUE;

static uint8_t at_cmdLine[at_cmdLenMax];
uint8_t at_dataLine[at_dataLenMax];/////
//uint8_t *at_dataLine;

/** @defgroup AT_PORT_Functions
  * @{
  */

static void at_procTask(os_event_t *events);

/**
  * @brief  Task of process command or txdata.
  * @param  events: no used
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_procTask(os_event_t *events)
{
  if(at_state == at_statProcess)
  {
    at_cmdProcess(at_cmdLine);
    if(specialAtState)
    {
      at_state = at_statIdle;
    }
  }
  else if(at_state == at_statIpSended)
  {
    at_ipDataSending(at_dataLine);//UartDev.rcv_buff.pRcvMsgBuff);
    if(specialAtState)
    {
      at_state = at_statIdle;
    }
  }
  else if(at_state == at_statIpTraning)
  {
    at_ipDataSendNow();//UartDev.rcv_buff.pRcvMsgBuff);
  }
}


/**
  * @brief  Initializes build two tasks.
  * @param  None
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_init(void)
{

  system_os_task(at_procTask, at_procTaskPrio, at_procTaskQueue, at_procTaskQueueLen);
}

/**
  * @}
  */
