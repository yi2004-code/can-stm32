/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "can.h"
#include "gpio.h"
#include "usart.h"
#include <stdio.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include <math.h>
#include "pid_control.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ID             0x002      //给其一个ID

#define VREF           3.3f      // ADC 参考电压 (V)
#define ADC_MAX        4095.0f   // 12位 ADC 最大值
#define R_FIXED        10000.0f  // 固定分压电阻 (Ω)
#define R0             10000.0f  // NTC 在 25°C 时的阻值 (Ω)
#define B              3950.0f   // NTC B值 (K)
#define T0_K           298.15f   // 25°C 开尔文
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t ADC_Value = 0; 
volatile float Temperature=0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int fputc(int ch,FILE *f){
  HAL_UART_Transmit(&huart1,(uint8_t *)&ch,1,HAL_MAX_DELAY);
  return ch;
}

float NTC_ADC_To_Temperature(uint32_t adc_value) {
    if (adc_value == 0) adc_value = 1;
    if (adc_value >= ADC_MAX) adc_value = ADC_MAX - 1;
    float voltage = (float)adc_value * VREF / ADC_MAX;
    float R_ntc = R_FIXED * voltage / (VREF - voltage);
    // 限制电阻范围，避免 log 参数过小或过大
    if (R_ntc < 100.0f) R_ntc = 100.0f;   // 不会低于100Ω
    if (R_ntc > 200000.0f) R_ntc = 200000.0f;
    float T_kelvin = 1.0f / (1.0f/T0_K + (1.0f/B) * logf(R_ntc / R0));
    float T_celsius = T_kelvin - 273.15f;
    // 输出范围限制
    if (T_celsius < -20.0f) T_celsius = -20.0f;
    if (T_celsius > 80.0f) T_celsius = 80.0f;
    return T_celsius;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_CAN_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
  CAN_Start_Receive();
  HAL_ADC_Start_DMA(&hadc1,&ADC_Value,1);
//关闭传输完成和传输一半数据中断
  __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_TC | DMA_IT_HT);
  pid_control fan;
  fan.target=28.00f;
  PID_Init(&fan,25.00f,0.1f,0.0f,1000.00f);
  uint8_t test_data[2]={0} ;
 printf("启动成功\r\n");
 static uint32_t last_can_send_time=0;
  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  Temperature=NTC_ADC_To_Temperature(ADC_Value);
	  printf("%0.2f\r\n",Temperature);
	  //__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500);
    /* USER CODE END WHILE */
         //CAN_SendData(0x123, test_data, 8);
	  if(rx_flag==1){
	      for(int i=0;i<8;i++){
		     printf("%d\r\n",rx_data[i]);
			  rx_flag=0;
		  }
	  }
//        HAL_Delay(1000); 
    /* USER CODE BEGIN 3 */
	  if((HAL_GetTick()-last_can_send_time)>=500){
	   uint16_t temp=(Temperature*100);//将温度乘100，好将其放到can总线中
	   test_data[0]=(uint8_t)(temp>>8);
	   test_data[1]=(uint8_t)(temp&0x00FF);
	   CAN_SendData(ID,test_data,2);
	   last_can_send_time=HAL_GetTick();
	  }
	  //判断温度是否高于设定值，当高于设定值时，由pid来控制电机pwm
	  if(Temperature<fan.target){
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2,0);
		  fan.integral = 0.0f; 
	  }
	  else if(Temperature>=fan.target){
	      int compare=PID_Calculate(&fan,Temperature);
		  printf("pwm:%d\r\n",compare);
		  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2,compare);
	  }
	  
	  HAL_Delay(50);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
