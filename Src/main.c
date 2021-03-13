/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
extern uint8_t Res2_Buf[256];
extern uint8_t Res2_Sign;
extern uint8_t Res2_Count;
extern uint8_t Res2;

extern uint8_t Res1_Buf[256];
extern uint8_t Res1_Sign;
extern uint8_t Res1_Count;
extern uint8_t Res1;

uint16_t BAT_DATA;   
float BAT_Value;
float VDDA_Value;
uint16_t VREFINT_CAL;    //ADC的Vref+
uint16_t VREFINT_DATA;  // ADC的
uint8_t BAT_Q;
uint8_t BAT_Q_Last;
uint8_t NB_Signal_Value;

uint8_t NB_OBSERVERSP_CMD[50]="AT+MIPLOBSERVERSP=0,";
uint8_t NB_DISCOVERRSP_CMD[50]="AT+MIPLDISCOVERRSP=0,";
uint8_t NB_NOTIFY5700_CMD[60]="AT+MIPLNOTIFY=0,"; // 电池电量
uint8_t NB_NOTIFY5513_CMD[60]="AT+MIPLNOTIFY=0,"; // 纬度
uint8_t NB_NOTIFY5514_CMD[60]="AT+MIPLNOTIFY=0,"; // 经度
uint8_t NB_OB3320_count; // 响应订阅请求的ID值位数
uint8_t NB_OB3336_count;
uint8_t onenet_ok=0;
uint32_t temp_int;
float temp_float;
uint32_t weidu_int; // 获取纬度的dd
float weidu_float; // 获取纬度的mm
uint32_t jingdu_int; // 获取经度的dd
float jingdu_float; // 获取经度的mm
uint8_t shutdown=0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void Get_BAT_Value(void);
uint8_t Send_Cmd(uint8_t *cmd, uint8_t len, char *recdata);
void NB_Rec_Handle(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
	while((USART2->ISR&0X40)==0);  //ISR表示是否发送完成，Interrupt Service Routines
	USART2->TDR=(uint8_t)ch;
	return ch;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	uint16_t main_count=0;
	uint8_t i=0;
	float temp;
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
  MX_ADC_Init();
  MX_LPUART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_GPIO_WritePin(PWR_LED_GPIO_Port, PWR_LED_Pin, GPIO_PIN_RESET); // 点亮电源指示灯
	HAL_GPIO_WritePin(GPIOA, PWR_EN_Pin, GPIO_PIN_SET); // 保持电源
	while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1)==0);
	HAL_GPIO_WritePin(GPIOA, NB_PWR_Pin, GPIO_PIN_SET); // 让BC20模组开机
	HAL_Delay(600);     //根据电路图得拉低,再根据BC20模组特性保持600ms
	HAL_GPIO_WritePin(GPIOA, NB_PWR_Pin, GPIO_PIN_RESET);
//--------------------------  BC20的RST复位
	HAL_GPIO_WritePin(GPIOA, NB_RST_Pin, GPIO_PIN_SET); // 让BC20模组开机
	HAL_Delay(60);
	HAL_GPIO_WritePin(GPIOA, NB_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(10000);
	
	
	// 验证BC20是否正常
	while(Send_Cmd((uint8_t *)"AT\r\n", 4, "OK")!=0)
	{
		HAL_Delay(1000);
	}
	printf("BC20模组正常\r\n");
	// 关闭回显
	while(Send_Cmd((uint8_t *)"ATE0\r\n", 6, "OK")!=0)
	{
		HAL_Delay(1000);
	}
	printf("已经关闭回显\r\n");
	// 检测信号
	NB_Signal_Value=0;
	while((NB_Signal_Value==0)||(NB_Signal_Value==99))
	{
		if(Send_Cmd((uint8_t *)"AT+CSQ\r\n", 8, "OK")==0)  //AT+CSQ 检查网络信号强度和SIM卡情况
		{
			if(Res1_Buf[2]=='+') 
			{
				if(Res1_Buf[9]==',')// 信号值是个位数,用ASCII码表示
				{
					NB_Signal_Value=Res1_Buf[8]-0x30;	
				}
				else if(Res1_Buf[10]==',')// 信号值是十位数
				{
					NB_Signal_Value=(Res1_Buf[8]-0x30)*10+(Res1_Buf[9]-0x30);
				}
			}
		}
		HAL_Delay(1000);
	}
	printf("NB_Signal_Value=%d\r\n", NB_Signal_Value);
	// 等待EPS网络注册成功
	while(Send_Cmd((uint8_t *)"AT+CEREG?\r\n", 11, "+CEREG: 0,1")!=0)   
		HAL_Delay(1000);
	}
	printf("EPS网络注册成功\r\n");
	// 等待PS附着
	while(Send_Cmd((uint8_t *)"AT+CGATT?\r\n", 11, "+CGATT: 1")!=0)
	{
		HAL_Delay(1000);
	}
	printf("PS已附着\r\n");
	// 打开GNSS电源
	while(Send_Cmd((uint8_t *)"AT+QGNSSC=1\r\n", 13, "OK")!=0)
	{
		HAL_Delay(1000);
	}
	printf("GNSS打开命令已发送\r\n");
	HAL_Delay(2000);   //电源需要两秒钟打开
	// 检测GNSS电源是否打开
	while(Send_Cmd((uint8_t *)"AT+QGNSSC?\r\n", 12, "+QGNSSC: 1")!=0)
	{
		HAL_Delay(1000);
	}
	printf("GNSS电源已打开\r\n");
	// 创建通信套件实例
	Send_Cmd((uint8_t *)"AT+MIPLCREATE\r\n", 15, "OK");
	HAL_Delay(100);
	// 添加3320对象
	Send_Cmd((uint8_t *)"AT+MIPLADDOBJ=0,3320,1,\"1\",1,0\r\n", 32, "OK");
	// 添加3336对象
	Send_Cmd((uint8_t *)"AT+MIPLADDOBJ=0,3336,1,\"1\",2,0\r\n", 32, "OK");
	HAL_Delay(100);
	// 向OneNET发送注册请求
	Send_Cmd((uint8_t *)"AT+MIPLOPEN=0,86400\r\n", 21, "OK");   //设备id 0,在86400s内有效
  /* USER CODE END 2 */
 
 

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if(Res2_Sign==1) // 如果电脑发过来数据
		{
			//接收到最后一个字节，因为接收到字节REs为0，只有停止接受才可以加数字，再延时10ms
			do{
				Res2++;
				HAL_Delay(1);
			}while(Res2<10);
			//////////////////////////////
			Res2_Sign=0;	
			HAL_UART_Transmit(&hlpuart1, Res2_Buf, Res2_Count, 1000);//把接收到的数据发送到NB模组
			Res2_Count=0;
		}
		if(Res1_Sign==1) // 如果BC20模组发来数据
		{
			//接收到最后一个字节 再延时10ms
			do{
				Res1++;
				HAL_Delay(1);
			}while(Res1<10);
			//////////////////////////////
			Res1_Sign=0;	
			HAL_UART_Transmit(&huart2, Res1_Buf, Res1_Count, 1000); // 把接收到的数据发送到串口调试助手
			Res1_Count=0;
			NB_Rec_Handle();
		}
		if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1)==0) // 如果需要关机
		{
			HAL_GPIO_WritePin(PWR_LED_GPIO_Port, PWR_LED_Pin, GPIO_PIN_SET); // 熄灭电源指示灯
			while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1)==0);
			HAL_GPIO_WritePin(GPIOA, PWR_EN_Pin, GPIO_PIN_RESET); // 关闭电源
		}
		main_count++;
		HAL_Delay(0);
		if(main_count>1000) 
		{
			main_count=0;
			Get_BAT_Value();
			temp=0;
			for(i=0;i<10;i++)
			{
				Get_BAT_Value();
				temp+=BAT_Value;
			}
			BAT_Value=temp/10;
			if(BAT_Value>4.1)
				BAT_Q=100;
			else if(BAT_Value<3.0)
			{
				BAT_Q=0;
				shutdown++;
				if(shutdown>2)
				{
					HAL_GPIO_WritePin(GPIOA, PWR_EN_Pin, GPIO_PIN_RESET); // 关闭电源
				}
			}
			else
			{
				BAT_Q=100-(4.1-BAT_Value)*100/1.1;
			}
			if(onenet_ok==2) // 如果已经成功注册到ONENET
			{
				NB_NOTIFY5700_CMD[33+NB_OB3320_count]=BAT_Q/100+0x30;
				NB_NOTIFY5700_CMD[34+NB_OB3320_count]=BAT_Q%100/10+0x30;
				NB_NOTIFY5700_CMD[35+NB_OB3320_count]=BAT_Q%10+0x30;
				if(BAT_Q!=BAT_Q_Last) // 如果当前电量和上次的不一样
				{
					Send_Cmd(NB_NOTIFY5700_CMD, 42+NB_OB3320_count, "OK");
					HAL_UART_Transmit(&huart2, NB_NOTIFY5700_CMD, 42+NB_OB3320_count, 1000);
					BAT_Q_Last=BAT_Q;
				}
				HAL_UART_Transmit(&hlpuart1, (uint8_t *)"AT+QGNSSRD=\"NMEA/RMC\"\r\n", 23, 1000);
			}
//			printf("BAT_Q=%d\r\n",BAT_Q);
//			printf("BAT_Value=%.2f\r\n",BAT_Value);
		}
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_LPUART1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */
	uint8_t i=0;
  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = ENABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = ENABLE;
  hadc.Init.LowPowerFrequencyMode = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = ENABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel to be converted. 
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel to be converted. 
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */
	ADC1->CR |= ADC_CR_ADCAL;   //此寄存器表示校准，即ADC启动时需要重新校准才能电压精确
	i=0;
	while(((ADC1->ISR&ADC_ISR_EOCAL)!=ADC_ISR_EOCAL)&&(i<10))  //等待校准转换完成
	{
		i++;
		HAL_Delay(1);
	}
	if(i==10)
	{
		printf("ADC初始化校准失败！\r\n");
	}
	ADC1->ISR |= ADC_ISR_EOCAL;              //校准后需要软件位变1
	VREFINT_CAL = *(uint16_t*)0x1FF80078;
	ADC1->CR |= ADC_CR_ADSTART;    //表示ADC启动转换
  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 9600;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */
	__HAL_UART_ENABLE_IT(&hlpuart1, UART_IT_RXNE);// 打开串口1接收中断
  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
	__HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);// 打开串口2接收中断
  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PWR_LED_GPIO_Port, PWR_LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, NB_PSM_Pin|BAT_ADC_EN_Pin|NB_RST_Pin|NB_PWR_Pin 
                          |PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PWR_LED_Pin */
  GPIO_InitStruct.Pin = PWR_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PWR_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : NB_PSM_Pin BAT_ADC_EN_Pin NB_RST_Pin NB_PWR_Pin 
                           PWR_EN_Pin */
  GPIO_InitStruct.Pin = NB_PSM_Pin|BAT_ADC_EN_Pin|NB_RST_Pin|NB_PWR_Pin 
                          |PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SHUT_DOWN_Pin */
  GPIO_InitStruct.Pin = SHUT_DOWN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SHUT_DOWN_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void Get_BAT_Value(void)
{
	uint8_t i;
	
	i=0;
	while(((ADC1->ISR&ADC_ISR_EOC)!=ADC_ISR_EOC)&&(i<10))   //寄存器表示转换完成，ichanl4
	{
		i++;
		HAL_Delay(1);
	}
	if(i==10)
	{
		printf("通道4转换失败！\r\n");
	}
	BAT_DATA = ADC1->DR;
	i=0;
	while(((ADC1->ISR&ADC_ISR_EOC)!=ADC_ISR_EOC)&&(i<10))
	{
		i++;
		HAL_Delay(1);
	}
	if(i==10)
	{
		printf("通道17转换失败！\r\n");
	}
	VREFINT_DATA = ADC1->DR;   //由于连续转换，通道17后得到
	VDDA_Value = 3.0*VREFINT_CAL/VREFINT_DATA;
	BAT_Value=VDDA_Value*BAT_DATA/2048;
}

// cmd :表示要发送的命令
// len :表示命令的字节长度
// recdata :接收的响应是否有这个字符串
// ret : 0正确 1没有接收到响应 2接收到了响应但是不是正确的响应
uint8_t Send_Cmd(uint8_t *cmd, uint8_t len, char *recdata)
{
	uint8_t ret;
	uint16_t count=0;
	
	memset(Res1_Buf, 0, strlen((const char *)Res1_Buf)); //复制字符0整个数组
	HAL_UART_Transmit(&hlpuart1, cmd, len, 1000);//把命令发送到NB模组
	while((Res1_Sign==0)&&(count<300))
	{
		count++;
		HAL_Delay(1);
	}
	if(count==300)// 超时
	{
		ret=1; // 未接收到BC20的响应
	}
	else // 接收到了响应
	{
		// 接收到最后一个字节 再延时10ms
		do{
				Res1++;
				HAL_Delay(1);
		}while(Res1<10);
		//////////////////////////////
		HAL_UART_Transmit(&huart2, Res1_Buf, Res1_Count, 1000);//把响应数据发送到电脑
		Res1_Count=0;
		Res1_Sign=0;
		ret=2;
		if(strstr((const char*)Res1_Buf, recdata))// 如果接收到的数据里面有这个字符串
		{
			ret=0; // 正确接收到响应
		}
	}
	
	return ret;
}
// 处理NB的返回数据
void NB_Rec_Handle(void)
{
	uint8_t i,j;
	
	if(strstr((const char*)Res1_Buf, "+MIPLOBSERVE:")) // 接收到订阅请求
	{
		if(Res1_Buf[17]==',')
		{
			i=0;   //i 用来计算消息id长度
			while(Res1_Buf[18+i]!=',')
			{
				i++;
				if(i>10)
					break;
			}
			if(i>10)    
			{
				printf("+MIPLOBSERVE数据接收错误\r\n");
			}
			else
			{
				if(strstr((const char*)Res1_Buf, "3320"))
				{
					for(j=0;j<i;j++)
					{
						NB_OBSERVERSP_CMD[20+j]=Res1_Buf[18+j];
						NB_NOTIFY5700_CMD[16+j]=Res1_Buf[18+j];
					}
					NB_NOTIFY5700_CMD[16+j]=',';
					NB_NOTIFY5700_CMD[17+j]='3';
					NB_NOTIFY5700_CMD[18+j]='3';
					NB_NOTIFY5700_CMD[19+j]='2';
					NB_NOTIFY5700_CMD[20+j]='0';
					NB_NOTIFY5700_CMD[21+j]=',';
					NB_NOTIFY5700_CMD[22+j]='0';
					NB_NOTIFY5700_CMD[23+j]=',';
					NB_NOTIFY5700_CMD[24+j]='5';
					NB_NOTIFY5700_CMD[25+j]='7';
					NB_NOTIFY5700_CMD[26+j]='0';
					NB_NOTIFY5700_CMD[27+j]='0';
					NB_NOTIFY5700_CMD[28+j]=',';
					NB_NOTIFY5700_CMD[29+j]='4';
					NB_NOTIFY5700_CMD[30+j]=',';
					NB_NOTIFY5700_CMD[31+j]='4';
					NB_NOTIFY5700_CMD[32+j]=',';
					NB_NOTIFY5700_CMD[36+j]=',';
					NB_NOTIFY5700_CMD[37+j]='0';
					NB_NOTIFY5700_CMD[38+j]=',';
					NB_NOTIFY5700_CMD[39+j]='0';
					NB_NOTIFY5700_CMD[40+j]='\r';
					NB_NOTIFY5700_CMD[41+j]='\n';
					NB_OB3320_count = j;
				}
				else if(strstr((const char*)Res1_Buf, "3336"))
				{
					for(j=0;j<i;j++)
					{
						NB_OBSERVERSP_CMD[20+j]=Res1_Buf[18+j];
						NB_NOTIFY5513_CMD[16+j]=Res1_Buf[18+j];
						NB_NOTIFY5514_CMD[16+j]=Res1_Buf[18+j];
					}
				  // 5513
					NB_NOTIFY5513_CMD[16+j]=',';
					NB_NOTIFY5513_CMD[17+j]='3';
					NB_NOTIFY5513_CMD[18+j]='3';
					NB_NOTIFY5513_CMD[19+j]='3';
					NB_NOTIFY5513_CMD[20+j]='6';
					NB_NOTIFY5513_CMD[21+j]=',';
					NB_NOTIFY5513_CMD[22+j]='0';
					NB_NOTIFY5513_CMD[23+j]=',';
					NB_NOTIFY5513_CMD[24+j]='5';
					NB_NOTIFY5513_CMD[25+j]='5';
					NB_NOTIFY5513_CMD[26+j]='1';
					NB_NOTIFY5513_CMD[27+j]='3';
					NB_NOTIFY5513_CMD[28+j]=',';
					NB_NOTIFY5513_CMD[29+j]='1';
					NB_NOTIFY5513_CMD[30+j]=',';
					NB_NOTIFY5513_CMD[31+j]='9';
					NB_NOTIFY5513_CMD[32+j]=',';
					NB_NOTIFY5513_CMD[33+j]='\"';
					NB_NOTIFY5513_CMD[43+j]='\"';
					NB_NOTIFY5513_CMD[44+j]=',';
					NB_NOTIFY5513_CMD[45+j]='1';
					NB_NOTIFY5513_CMD[46+j]=',';
					NB_NOTIFY5513_CMD[47+j]='0';
					NB_NOTIFY5513_CMD[48+j]='\r';
					NB_NOTIFY5513_CMD[49+j]='\n';
					// 5514
					NB_NOTIFY5514_CMD[16+j]=',';
					NB_NOTIFY5514_CMD[17+j]='3';
					NB_NOTIFY5514_CMD[18+j]='3';
					NB_NOTIFY5514_CMD[19+j]='3';
					NB_NOTIFY5514_CMD[20+j]='6';
					NB_NOTIFY5514_CMD[21+j]=',';
					NB_NOTIFY5514_CMD[22+j]='0';
					NB_NOTIFY5514_CMD[23+j]=',';
					NB_NOTIFY5514_CMD[24+j]='5';
					NB_NOTIFY5514_CMD[25+j]='5';
					NB_NOTIFY5514_CMD[26+j]='1';
					NB_NOTIFY5514_CMD[27+j]='4';
					NB_NOTIFY5514_CMD[28+j]=',';
					NB_NOTIFY5514_CMD[29+j]='1';
					NB_NOTIFY5514_CMD[30+j]=',';
					NB_NOTIFY5514_CMD[31+j]='1';
					NB_NOTIFY5514_CMD[32+j]='0';
					NB_NOTIFY5514_CMD[33+j]=',';
					NB_NOTIFY5514_CMD[34+j]='\"';
					NB_NOTIFY5514_CMD[45+j]='\"';
					NB_NOTIFY5514_CMD[46+j]=',';
					NB_NOTIFY5514_CMD[47+j]='0';
					NB_NOTIFY5514_CMD[48+j]=',';
					NB_NOTIFY5514_CMD[49+j]='0';
					NB_NOTIFY5514_CMD[50+j]='\r';
					NB_NOTIFY5514_CMD[51+j]='\n';
					NB_OB3336_count = j;
				}
				
				NB_OBSERVERSP_CMD[20+j]=',';
				NB_OBSERVERSP_CMD[21+j]='1';
				NB_OBSERVERSP_CMD[22+j]='\r';
				NB_OBSERVERSP_CMD[23+j]='\n';
				Send_Cmd(NB_OBSERVERSP_CMD, 24+j, "OK");
			}
		}
	}
	else if(strstr((const char*)Res1_Buf, "+MIPLDISCOVER:")) //接收到发现资源请求
	{
		if(Res1_Buf[18]==',')
		{
			i=0;
			while(Res1_Buf[19+i]!=',')
			{
				i++;
				if(i>10)
					break;
			}
			if(i>10)
			{
				printf("+MIPLDISCOVER数据接收错误\r\n");
			}
			else
			{
				for(j=0;j<i;j++)
				{
					NB_DISCOVERRSP_CMD[21+j]=Res1_Buf[19+j];
				}
				if(strstr((const char*)Res1_Buf, "3336"))
				{
					NB_DISCOVERRSP_CMD[21+j]=',';
					NB_DISCOVERRSP_CMD[22+j]='1';
					NB_DISCOVERRSP_CMD[23+j]=',';
					NB_DISCOVERRSP_CMD[24+j]='9';
					NB_DISCOVERRSP_CMD[25+j]=',';
					NB_DISCOVERRSP_CMD[26+j]='\"';
					NB_DISCOVERRSP_CMD[27+j]='5';
					NB_DISCOVERRSP_CMD[28+j]='5';
					NB_DISCOVERRSP_CMD[29+j]='1';
					NB_DISCOVERRSP_CMD[30+j]='3';
					NB_DISCOVERRSP_CMD[31+j]=';';
					NB_DISCOVERRSP_CMD[32+j]='5';
					NB_DISCOVERRSP_CMD[33+j]='5';
					NB_DISCOVERRSP_CMD[34+j]='1';
					NB_DISCOVERRSP_CMD[35+j]='4';
					NB_DISCOVERRSP_CMD[36+j]='\"';
					NB_DISCOVERRSP_CMD[37+j]='\r';
					NB_DISCOVERRSP_CMD[38+j]='\n';
					Send_Cmd(NB_DISCOVERRSP_CMD, 39+j, "OK");
					onenet_ok++;
				}
				else if(strstr((const char*)Res1_Buf, "3320"))
				{
					NB_DISCOVERRSP_CMD[21+j]=',';
					NB_DISCOVERRSP_CMD[22+j]='1';
					NB_DISCOVERRSP_CMD[23+j]=',';
					NB_DISCOVERRSP_CMD[24+j]='4';
					NB_DISCOVERRSP_CMD[25+j]=',';
					NB_DISCOVERRSP_CMD[26+j]='\"';
					NB_DISCOVERRSP_CMD[27+j]='5';
					NB_DISCOVERRSP_CMD[28+j]='7';
					NB_DISCOVERRSP_CMD[29+j]='0';
					NB_DISCOVERRSP_CMD[30+j]='0';
					NB_DISCOVERRSP_CMD[31+j]='\"';
					NB_DISCOVERRSP_CMD[32+j]='\r';
					NB_DISCOVERRSP_CMD[33+j]='\n';
					Send_Cmd(NB_DISCOVERRSP_CMD, 34+j, "OK");
					onenet_ok++;
				}
			}
		}
	}
	if(strstr((const char*)Res1_Buf, "+QGNSSRD:")) // 接收到GNSS信息
	{
		if(Res1_Buf[29]=='A') // 如果获取到了经纬度信息
		{
			// +QGNSSRD: $GNRMC,032956.42,A,3748.7157,N,11238.7823,E,0.260,,100520,,,A,V*10
			// 获取纬度
			temp_int=(Res1_Buf[33]-0x30)*100000+(Res1_Buf[34]-0x30)*10000+(Res1_Buf[36]-0x30)*1000+(Res1_Buf[37]-0x30)*100+(Res1_Buf[38]-0x30)*10+(Res1_Buf[39]-0x30); // 487157
			temp_float = (float)temp_int/600000; // 487157/600000=0.811928
			weidu_int = (Res1_Buf[31]-0x30)*10+(Res1_Buf[32]-0x30); // 37 
			weidu_float = temp_float+(float)weidu_int; // 37.811928
			weidu_float = weidu_float * 1000000; // 37811928.0
			weidu_int = weidu_float; // 37811928
			NB_NOTIFY5513_CMD[34+NB_OB3336_count]=weidu_int/10000000+0x30; // 3
			NB_NOTIFY5513_CMD[35+NB_OB3336_count]=weidu_int%10000000/1000000+0x30; // 7
			NB_NOTIFY5513_CMD[36+NB_OB3336_count]='.'; // .
			NB_NOTIFY5513_CMD[37+NB_OB3336_count]=weidu_int%1000000/100000+0x30; // 8
			NB_NOTIFY5513_CMD[38+NB_OB3336_count]=weidu_int%100000/10000+0x30; // 1
			NB_NOTIFY5513_CMD[39+NB_OB3336_count]=weidu_int%10000/1000+0x30; // 1
			NB_NOTIFY5513_CMD[40+NB_OB3336_count]=weidu_int%1000/100+0x30; // 9
			NB_NOTIFY5513_CMD[41+NB_OB3336_count]=weidu_int%100/10+0x30; // 2
			NB_NOTIFY5513_CMD[42+NB_OB3336_count]=weidu_int%10+0x30; // 8
			printf("纬度=%d\r\n", weidu_int);
			// 获取经度
			temp_int=(Res1_Buf[46]-0x30)*100000+(Res1_Buf[47]-0x30)*10000+(Res1_Buf[49]-0x30)*1000+(Res1_Buf[50]-0x30)*100+(Res1_Buf[51]-0x30)*10+(Res1_Buf[52]-0x30); // 387823
			temp_float = (float)temp_int/600000; // 387823/600000=0.646372
			jingdu_int = (Res1_Buf[43]-0x30)*100+(Res1_Buf[44]-0x30)*10+(Res1_Buf[45]-0x30); // 112
			jingdu_float = temp_float+(float)jingdu_int; // 112.646372
			jingdu_float = jingdu_float * 1000000; // 112646372.0
			jingdu_int = jingdu_float; // 112646372
			NB_NOTIFY5514_CMD[35+NB_OB3336_count]=jingdu_int/100000000+0x30; // 1
			NB_NOTIFY5514_CMD[36+NB_OB3336_count]=jingdu_int%100000000/10000000+0x30; // 1
			NB_NOTIFY5514_CMD[37+NB_OB3336_count]=jingdu_int%10000000/1000000+0x30; // 2
			NB_NOTIFY5514_CMD[38+NB_OB3336_count]='.'; // .
			NB_NOTIFY5514_CMD[39+NB_OB3336_count]=jingdu_int%1000000/100000+0x30; // 6
			NB_NOTIFY5514_CMD[40+NB_OB3336_count]=jingdu_int%100000/10000+0x30; // 4
			NB_NOTIFY5514_CMD[41+NB_OB3336_count]=jingdu_int%10000/1000+0x30; // 6
			NB_NOTIFY5514_CMD[42+NB_OB3336_count]=jingdu_int%1000/100+0x30; // 3
			NB_NOTIFY5514_CMD[43+NB_OB3336_count]=jingdu_int%100/10+0x30; // 7
			NB_NOTIFY5514_CMD[44+NB_OB3336_count]=jingdu_int%10+0x30; // 2
			printf("经度=%d\r\n", jingdu_int);
			
			// 发送经纬度信息到云平台
			Send_Cmd(NB_NOTIFY5513_CMD, 50+NB_OB3336_count, "OK");
			HAL_UART_Transmit(&huart2, NB_NOTIFY5513_CMD, 50+NB_OB3336_count, 1000);
			Send_Cmd(NB_NOTIFY5514_CMD, 52+NB_OB3336_count, "OK");
			HAL_UART_Transmit(&huart2, NB_NOTIFY5514_CMD, 52+NB_OB3336_count, 1000);
		}
	}
}
/* USER CODE END 4 */

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
