#include "ap.h"
#include "uf2/uf2.h"
#include "boot/boot.h"


static void bootUp(void);
static void cliUpdate(void);


void apInit(void)
{
  #ifdef _USE_HW_CLI  
  cliOpen(HW_UART_CH_CLI, 115200);  
  #endif
  logBoot(false);

  bootUp();

  #ifdef _USE_HW_USB
  uf2Init();
  usbInit();
  #endif

  bootInit();
}

void apMain(void)
{
  uint32_t pre_time;


  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 100)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    cliUpdate();
    #ifdef _USE_HW_USB
    usbUpdate();
    uf2Update();   
    #endif 
  }
}

void cliUpdate(void)
{
  #ifdef _USE_HW_CLI
  cliMain();
  #endif
}

void bootUp(void)
{
  static bool is_run_fw = true;

  uint32_t boot_param;
  uint16_t err_code;

  boot_param = resetGetBootMode();


  if (boot_param & (1<<MODE_BIT_BOOT))
  {
    boot_param &= ~(1<<MODE_BIT_BOOT);
    resetSetBootMode(boot_param);    
    is_run_fw = false;
  }


  uint32_t pre_time = millis();
  while(millis()-pre_time <= 5)
  {
    keysUpdate();
  }
  if (keysGetPressed(0, 0))
  {
    logPrintf("[  ] ESC Pressed\n");
    is_run_fw = false;
  }

  if (boot_param & (1<<MODE_BIT_UPDATE))
  {
    boot_param &= ~(1<<MODE_BIT_UPDATE);
    resetSetBootMode(boot_param);
    
    is_run_fw = true;
  }
  logPrintf("\n");


  if (is_run_fw)
  {  
    err_code = bootJumpFirm();
    if (err_code != OK)
    {
      logPrintf("[  ] bootJumpFirm()\n");
      logPrintf("[E_] err : 0x%04X\n", err_code);
      if (bootVerifyUpdate() == OK)
      {
        logPrintf("[  ] retry update\n");
        if (bootUpdateFirm() == OK)
        {
          err_code = bootJumpFirm();
          if (err_code != OK)
            logPrintf("[E_] err : 0x%04X\n", err_code);
        }
      }      
    }
  }

  logPrintf("\n");
  logPrintf("Boot Mode..\n"); 
}