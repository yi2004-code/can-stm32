/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "can.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;
uint8_t rx_data[8];
uint8_t  rx_flag=0;
/* CAN1 init function */
void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 7;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_8TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;   
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void CAN_SendData(uint32_t ID,uint8_t *Data,uint8_t Lenth){
    CAN_TxHeaderTypeDef TxHeader;
	TxHeader.StdId=ID;
	TxHeader.ExtId=0;
	TxHeader.IDE=CAN_ID_STD;
	TxHeader.RTR=CAN_RTR_DATA;
    TxHeader.DLC=Lenth;
	uint32_t MessageMailBox;
	if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1)>0){
	HAL_CAN_AddTxMessage(&hcan1,&TxHeader,Data,&MessageMailBox);
	}
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){
    CAN_RxHeaderTypeDef RxHeader;
	uint8_t data[8];
	if(hcan->Instance==CAN1){
	    HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&RxHeader,data);
	    for (uint8_t i = 0; i < RxHeader.DLC; i++) {
			rx_data[i]=data[i];
	    }
		rx_flag=1;
	}
}


void CAN_Start_Receive(void) {
    // 1. 配置过滤器（掩码模式，掩码全0表示接收所有ID，方便调试）
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 0;                  // 使用过滤器组0
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; // 掩码模式
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32位宽度
    sFilterConfig.FilterIdHigh = 0x0000;           // ID高16位设为0
    sFilterConfig.FilterIdLow = 0x0000;            // ID低16位设为0
    sFilterConfig.FilterMaskIdHigh = 0x0000;       // 掩码高16位设为0（全匹配）
    sFilterConfig.FilterMaskIdLow = 0x0000;        // 掩码低16位设为0（全匹配）
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // 将收到的数据分配给 FIFO0
    sFilterConfig.FilterActivation = ENABLE;       // 激活该过滤器
    
    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
        Error_Handler();
    }

    // 2. 启动 CAN 外设
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        Error_Handler();
    }

    // 3. 开启 FIFO0 接收中断通知
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        Error_Handler();
    }
}
/* USER CODE END 1 */

