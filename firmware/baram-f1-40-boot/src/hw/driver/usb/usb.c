#include "usb.h"

#include "tusb.h"
#include "usb_desc.h"


static void usbInitPhy(void);




bool usbInit(void)
{
  usbInitPhy();

  tusb_init();

  return true;
}

void usbDeInit(void)
{
  __HAL_RCC_USB_OTG_FS_CLK_DISABLE();

  HAL_NVIC_DisableIRQ(OTG_FS_IRQn);  
}

void usbUpdate(void)
{
  tud_task();
}

void tud_mount_cb(void)
{
  logPrintf("tud_mount_cb()\n");
}

void tud_umount_cb(void)
{
  logPrintf("tud_umount_cb()\n");
}

void usbInitPhy(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  /**USB_OTG_FS GPIO Configuration
  PA11     ------> USB_OTG_FS_DM
  PA12     ------> USB_OTG_FS_DP
  */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

  // Disable VBUS sense
  USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
}

void OTG_FS_IRQHandler(void)
{
  tud_int_handler(BOARD_TUD_RHPORT);
}