/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dac.h"
#include "dma.h"
#include "lwip.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tcpclient.h"
#include "tcpserver.h"

#include "tcp_echoserver.h"
#include "tftpserver.h"
#include "iap.h"

#include "bsp_dac.h"
#include "bsp_pwm.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DELAY_TIME 10     //10ms
#define WAIT_TIME  3000  //3s=5000ms
#define WAIT_FREQUENCY WAIT_TIME/DELAY_TIME
#define PER_SECOND_FREQUENVY (1000/DELAY_TIME)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern struct netif gnetif;
extern u8_t upload_flag;
u32_t JUMP_FLASH_APP1_ADDR=0x08020000;  	//第一个应用程序起始地址(存放在FLASH)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	u32 t=0;
	u32 wait_time=WAIT_TIME/DELAY_TIME/PER_SECOND_FREQUENVY; //等待时间
	u8 tcp_server_sendbuf[200]; //发送缓冲区
	u8* tcp_recv_data; //接收缓存
	u8 tcp_server_flag;	 //tcp接收标志
	u8 tcp_connect_flag=0; //连接标志
	u8 wait_flag=1; //等待标志
	u8 is_oneshot = 0;
  /* USER CODE END 1 */
  
  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
	
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	HAL_Delay(100);
	SCB->CACR|=1<<2;   //强制D-Cache透写,如不开启,实际使用中可能遇到各种问题
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_LWIP_Init();
  MX_DAC1_Init();
  MX_TIM8_Init();
  /* USER CODE BEGIN 2 */
	printf("This is a TFTP IAP test\r\n");
	tftp_server_init();
	tcp_echoserver_init();
	Track_Motor_Init();
	pwm_init();
	//TCP_Client_Init();
	//TCP_Echo_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		/* 获取STM32H743服务器接收到的数据和标志位 */
		get_tcp_recv_data_flag(&tcp_recv_data,&tcp_server_flag);
		
		/* 检测是否有客户端连接到STM32H743服务器，如果连接，则设置tcp_connect_flag为1 */
		if(tcp_server_flag&1<<5) tcp_connect_flag=1;
		else tcp_connect_flag=0;
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if(tcp_connect_flag==1)
		{
			if(wait_flag==1)	t++;
			if(t%PER_SECOND_FREQUENVY==0)
			{
				wait_time--;
				if(wait_flag == 1)
				{
					sprintf((char*)tcp_server_sendbuf,"Wait Time: %ds\r\n",wait_time);
					tcp_server_usersent(tcp_server_sendbuf,32);
					ethernetif_input(&gnetif);
					sys_check_timeouts();
				}
				
				if(upload_flag==1)
				{
					if(is_oneshot == 0)
					{
						is_oneshot = 1;
						wait_flag=0;
						sprintf((char*)tcp_server_sendbuf,"Start Write File...\r\n");
						tcp_server_usersent(tcp_server_sendbuf,32);
						ethernetif_input(&gnetif);
						sys_check_timeouts();
					}
					t=0;
					wait_time=WAIT_TIME/DELAY_TIME/PER_SECOND_FREQUENVY;
				}
				else if(upload_flag==2)
				{
					sprintf((char*)tcp_server_sendbuf,"Write File Finish!\r\n");
					tcp_server_usersent(tcp_server_sendbuf,32);
					ethernetif_input(&gnetif);
					sys_check_timeouts();
					t=0;
					wait_time=WAIT_TIME/DELAY_TIME/PER_SECOND_FREQUENVY;
					wait_flag=1;
					upload_flag=0;
				}
			}
			
			if(t==WAIT_FREQUENCY)
			{
				if(((*(vu32*)(JUMP_FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)
				{
					HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
					HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_2);
					SysTick->CTRL = 0x00;  //禁止SysTick
					SysTick->LOAD = 0;
					SysTick->VAL  = 0;
					__disable_irq(); //禁止全局中断
					iap_load_app(JUMP_FLASH_APP1_ADDR);
				}
				else
				{
					sprintf((char*)tcp_server_sendbuf,"Illegal FLASH APP\r\n");
					tcp_server_usersent(tcp_server_sendbuf,32);
					ethernetif_input(&gnetif);
					sys_check_timeouts();
				}	
			}
		}
		
		HAL_Delay(DELAY_TIME);
		
		MX_LWIP_Process();
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable 
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();
  /** Initializes and configures the Region and the memory to be protected 
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x30040000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected 
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30044000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
