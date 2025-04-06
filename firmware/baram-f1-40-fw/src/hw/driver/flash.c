#include "flash.h"



#ifdef _USE_HW_FLASH
#include "qspi.h"
#include "spi_flash.h"
#include "cli.h"



#define FLASH_SECTOR_MAX          8


typedef struct
{
  uint32_t addr;
  uint32_t length;
} flash_tbl_t;


#if CLI_USE(HW_FLASH)
static void cliFlash(cli_args_t *args);
#endif
static bool flashInSector(uint16_t sector_num, uint32_t addr, uint32_t length);



static flash_tbl_t flash_tbl[FLASH_SECTOR_MAX] =
    {
        {0x8000000,  16*1024},
        {0x8004000,  16*1024},
        {0x8008000,  16*1024},
        {0x800C000,  16*1024},
        {0x8010000,  64*1024},
        {0x8020000, 128*1024},
        {0x8040000, 128*1024},
        {0x8060000, 128*1024},
    };




bool flashInit(void)
{

  logPrintf("[OK] flashInit()\n");

#if CLI_USE(HW_FLASH)
  cliAdd("flash", cliFlash);
#endif
  return true;
}

bool flashInSector(uint16_t sector_num, uint32_t addr, uint32_t length)
{
  bool ret = false;

  uint32_t sector_start;
  uint32_t sector_end;
  uint32_t flash_start;
  uint32_t flash_end;


  sector_start = flash_tbl[sector_num].addr;
  sector_end   = flash_tbl[sector_num].addr + flash_tbl[sector_num].length - 1;
  flash_start  = addr;
  flash_end    = addr + length - 1;


  if (sector_start >= flash_start && sector_start <= flash_end)
  {
    ret = true;
  }

  if (sector_end >= flash_start && sector_end <= flash_end)
  {
    ret = true;
  }

  if (flash_start >= sector_start && flash_start <= sector_end)
  {
    ret = true;
  }

  if (flash_end >= sector_start && flash_end <= sector_end)
  {
    ret = true;
  }

  return ret;
}


bool flashErase(uint32_t addr, uint32_t length)
{
  bool ret = false;
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef init;
  uint32_t page_error;

  int16_t  start_sector_num = -1;
  uint32_t sector_count = 0;



  for (int i=0; i<FLASH_SECTOR_MAX; i++)
  {
    if (flashInSector(i, addr, length) == true)
    {
      if (start_sector_num < 0)
      {
        start_sector_num = i;
      }
      sector_count++;
    }
  }


  if (sector_count > 0)
  {
    HAL_FLASH_Unlock();

    init.TypeErase   = FLASH_TYPEERASE_SECTORS;
    init.Banks       = FLASH_BANK_1;
    init.Sector      = start_sector_num;
    init.NbSectors   = sector_count;
    init.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    status = HAL_FLASHEx_Erase(&init, &page_error);
    if (status == HAL_OK)
    {
      ret = true;
    }

    HAL_FLASH_Lock();
  }

  return ret;
}

bool flashWrite(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  HAL_StatusTypeDef status;


  if (addr%4 != 0)
  {
    return false;
  }

  HAL_FLASH_Unlock();

  for (int i=0; i<length; i+=4)
  {
    uint32_t data;

    data  = p_data[i+0] << 0;
    data |= p_data[i+1] << 8;
    data |= p_data[i+2] <<16;
    data |= p_data[i+3] <<24;

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, (uint64_t)data);
    if (status != HAL_OK)
    {
      ret = false;
      break;
    }
  }

  HAL_FLASH_Lock();

  return ret;
}

bool flashRead(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  uint8_t *p_byte = (uint8_t *)addr;


#ifdef _USE_HW_QSPI
  if (addr >= qspiGetAddr() && addr < (qspiGetAddr() + qspiGetLength()))
  {
    ret = qspiRead(addr - qspiGetAddr(), p_data, length);
    return ret;
  }
#endif
#ifdef _USE_HW_SPI_FLASH
  if (addr >= spiFlashGetAddr() && addr < (spiFlashGetAddr() + spiFlashGetLength()))
  {
    ret = spiFlashRead(addr - spiFlashGetAddr(), p_data, length);
    return ret;
  }
#endif

  for (int i=0; i<length; i++)
  {
    p_data[i] = p_byte[i];
  }

  return ret;
}




#if CLI_USE(HW_FLASH)
void cliFlash(cli_args_t *args)
{
  bool ret = false;
  uint32_t i;
  uint32_t addr;
  uint32_t length;
  uint32_t pre_time;
  bool flash_ret;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("flash addr : 0x%X\n", flash_tbl[0].addr);
    ret = true;
  }

  if(args->argc == 3 && args->isStr(0, "read"))
  {
    uint8_t data;

    addr   = (uint32_t)args->getData(1);
    length = (uint32_t)args->getData(2);

    for (i=0; i<length; i++)
    {
      flash_ret = flashRead(addr+i, &data, 1);

      if (flash_ret == true)
      {
        cliPrintf( "addr : 0x%X\t 0x%02X\n", addr+i, data);
      }
      else
      {
        cliPrintf( "addr : 0x%X\t Fail\n", addr+i);
      }
    }

    ret = true;
  }
    
  if(args->argc == 3 && args->isStr(0, "erase"))
  {
    addr   = (uint32_t)args->getData(1);
    length = (uint32_t)args->getData(2);

    pre_time = millis();
    flash_ret = flashErase(addr, length);

    cliPrintf( "addr : 0x%X\t len : %d %d ms\n", addr, length, (millis()-pre_time));
    if (flash_ret)
    {
      cliPrintf("OK\n");
    }
    else
    {
      cliPrintf("FAIL\n");
    }

    ret = true;
  }
    
  if(args->argc == 3 && args->isStr(0, "write"))
  {
    uint32_t data;

    addr = (uint32_t)args->getData(1);
    data = (uint32_t)args->getData(2);

    pre_time = millis();
    flash_ret = flashWrite(addr, (uint8_t *)&data, 4);

    cliPrintf( "addr : 0x%X\t 0x%X %dms\n", addr, data, millis()-pre_time);
    if (flash_ret)
    {
      cliPrintf("OK\n");
    }
    else
    {
      cliPrintf("FAIL\n");
    }

    ret = true;
  }

  if(args->argc == 3 && args->isStr(0, "check"))
  {
    uint32_t data = 0;
    uint32_t block = 4;


    addr    = (uint32_t)args->getData(1);
    length  = (uint32_t)args->getData(2);
    length -= (length % block);

    do
    {
      cliPrintf("flashErase()..");
      if (flashErase(addr, length) == false)
      {
        cliPrintf("Fail\n");
        break;
      }
      cliPrintf("OK\n");

      cliPrintf("flashWrite()..");
      for (uint32_t i=0; i<length; i+=block)
      {
        data = i;
        if (flashWrite(addr + i, (uint8_t *)&data, block) == false)
        {
          cliPrintf("Fail %d\n", i);
          break;
        }
      }
      cliPrintf("OK\n");

      cliPrintf("flashRead() ..");
      for (uint32_t i=0; i<length; i+=block)
      {
        data = 0;
        if (flashRead(addr + i, (uint8_t *)&data, block) == false)
        {
          cliPrintf("Fail %d\n", i);
          break;
        }
        if (data != i)
        {
          cliPrintf("Check Fail %d\n", i);
          break;
        }
      }  
      cliPrintf("OK\n");


      cliPrintf("flashErase()..");
      if (flashErase(addr, length) == false)
      {
        cliPrintf("Fail\n");
        break;
      }
      cliPrintf("OK\n");  
    } while (0);
    
    ret = true;
  }


  if (ret == false)
  {
    cliPrintf( "flash info\n");
    cliPrintf( "flash read  [addr] [length]\n");
    cliPrintf( "flash erase [addr] [length]\n");
    cliPrintf( "flash write [addr] [data]\n");
    cliPrintf( "flash check [addr] [length]\n");
  }
}
#endif

#endif