#include "keys.h"


#ifdef _USE_HW_KEYS
#include "button.h"
#include "cli.h"


#define MATRIX_ROWS   4
#define MATRIX_COLS   12


#if CLI_USE(HW_KEYS)
static void cliCmd(cli_args_t *args);
#endif
static bool keysInitTimer(void);
static bool keysInitDma(void);
static bool keysInitGpio(void);


const static uint16_t row_wr_buf[MATRIX_ROWS]  = {0x01, 0x02, 0x04, 0x08};
const static uint16_t col_bit_tbl[MATRIX_COLS] = {1 << 7,
                                                  1 << 6,
                                                  1 << 5,
                                                  1 << 3,
                                                  1 << 15,
                                                  1 << 14,
                                                  1 << 13,
                                                  1 << 12,
                                                  1 << 10,
                                                  1 << 2,
                                                  1 << 1,
                                                  1 << 0};

static uint16_t col_rd_buf[MATRIX_ROWS] = {0,};


static TIM_HandleTypeDef htim1;
static DMA_HandleTypeDef hdma_tim1_ch1;
static DMA_HandleTypeDef hdma_tim1_up;




bool keysInit(void)
{

  keysInitGpio();
  keysInitDma();
  keysInitTimer();

  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1);

#if CLI_USE(HW_KEYS)
  cliAdd("keys", cliCmd);
#endif

  return true;
}

bool keysInitGpio(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // ROWS
  //
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Pin   = GPIO_PIN_0 |
                        GPIO_PIN_1 |
                        GPIO_PIN_2 |
                        GPIO_PIN_3;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); 


  // COLS
  //
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Pin   = GPIO_PIN_0 |
                        GPIO_PIN_1 |
                        GPIO_PIN_2 |
                        GPIO_PIN_3 |
                        GPIO_PIN_5 |
                        GPIO_PIN_6 |
                        GPIO_PIN_7 |
                        GPIO_PIN_10 |
                        GPIO_PIN_11 |
                        GPIO_PIN_12 |
                        GPIO_PIN_13 |
                        GPIO_PIN_14 |
                        GPIO_PIN_15;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  
  return true;
}

bool keysInitTimer(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};


  __HAL_RCC_TIM1_CLK_ENABLE();

  htim1.Instance               = TIM1;
  htim1.Init.Prescaler         = 95;
  htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim1.Init.Period            = 99;
  htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    return false;
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    return false;
  }
  if (HAL_TIM_OC_Init(&htim1) != HAL_OK)
  {
    return false;
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    return false;
  }
  sConfigOC.OCMode       = TIM_OCMODE_TIMING;
  sConfigOC.Pulse        = 1;
  sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    return false;
  }
  __HAL_TIM_ENABLE_OCxPRELOAD(&htim1, TIM_CHANNEL_1);


  __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_CC1);
  __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_UPDATE);

  return true;
}

bool keysInitDma(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
  
  
  // CC1 Event
  //
  hdma_tim1_ch1.Instance                 = DMA2_Stream1;
  hdma_tim1_ch1.Init.Channel             = DMA_CHANNEL_6;
  hdma_tim1_ch1.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_tim1_ch1.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tim1_ch1.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tim1_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_tim1_ch1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
  hdma_tim1_ch1.Init.Mode                = DMA_CIRCULAR;
  hdma_tim1_ch1.Init.Priority            = DMA_PRIORITY_LOW;
  hdma_tim1_ch1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_tim1_ch1) != HAL_OK)
  {
    return false;
  }
  __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_CC1], hdma_tim1_ch1);

  HAL_DMA_Start(&hdma_tim1_ch1, (uint32_t)&row_wr_buf[0], (uint32_t)&GPIOA->ODR, MATRIX_ROWS);
  


  // Update Event
  //
  hdma_tim1_up.Instance                 = DMA2_Stream5;
  hdma_tim1_up.Init.Channel             = DMA_CHANNEL_6;
  hdma_tim1_up.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_tim1_up.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tim1_up.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tim1_up.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_tim1_up.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
  hdma_tim1_up.Init.Mode                = DMA_CIRCULAR;
  hdma_tim1_up.Init.Priority            = DMA_PRIORITY_LOW;
  hdma_tim1_up.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_tim1_up) != HAL_OK)
  {
    return false;
  }
  __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_UPDATE], hdma_tim1_up);

  HAL_DMA_Start(&hdma_tim1_up, (uint32_t)&GPIOB->IDR, (uint32_t)&col_rd_buf[0], MATRIX_ROWS);

  return true;
}

bool keysIsBusy(void)
{
  return false;
}

bool keysUpdate(void)
{
  return true;
}

bool keysReadBuf(uint8_t *p_data, uint32_t length)
{
  return true;
}

bool keysReadColsBuf(uint16_t *p_data, uint32_t rows_cnt)
{
  uint16_t col_buf[MATRIX_ROWS];


  for (int y=0; y<rows_cnt; y++)
  {
    uint16_t col_bit;  
    
    col_bit = 0;
    for (int x=0; x<MATRIX_COLS; x++)
    {
      if (col_rd_buf[y] & col_bit_tbl[x])
      {
        col_bit |= (1<<x);
      }
    }
    col_buf[y] = col_bit;
  }
  memcpy(p_data, col_buf, rows_cnt * sizeof(uint16_t));
  return true;
}

bool keysGetPressed(uint16_t row, uint16_t col)
{
  bool     ret = false;
  uint16_t col_bit;  

  col_bit = col_rd_buf[row];

  if (col_bit & col_bit_tbl[col])
  {
    ret = true;
  }

  return ret;
}

#if CLI_USE(HW_KEYS)
void cliCmd(cli_args_t *args)
{
  bool ret = false;



  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliShowCursor(false);


    while(cliKeepLoop())
    {
      keysUpdate();
      delay(10);

      cliPrintf("     ");
      for (int cols=0; cols<MATRIX_COLS; cols++)
      {
        cliPrintf("%02d ", cols);
      }
      cliPrintf("\n");

      for (int rows=0; rows<MATRIX_ROWS; rows++)
      {
        cliPrintf("%02d : ", rows);

        for (int cols=0; cols<MATRIX_COLS; cols++)
        {
          if (keysGetPressed(rows, cols))
            cliPrintf("O  ");
          else
            cliPrintf("_  ");
        }
        cliPrintf("\n");
      }
      cliMoveUp(MATRIX_ROWS+1);
    }
    cliMoveDown(MATRIX_ROWS+1);

    cliShowCursor(true);
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("keys info\n");
  }
}
#endif

#endif