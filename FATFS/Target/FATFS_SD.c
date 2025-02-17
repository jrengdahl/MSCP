/*
 * File: FATFS_SD.c
 * Driver Name: [[ FATFS_SD SPI ]]
 * SW Layer:   MIDWARE
 * Author:     Khaled Magdy
 * -------------------------------------------
 * For More Information, Tutorials, etc.
 * Visit Website: www.DeepBlueMbedded.com
 */
#include "main.h"
#include "diskio.h"
#include "FATFS_SD.h"

#define TRUE  1
#define FALSE 0
#define bool BYTE

static volatile DSTATUS Stat[2] = {STA_NOINIT, STA_NOINIT};  /* Disk Status */
uint16_t Timer1, Timer2; 		/* 1ms Timer Counters */
static uint8_t CardType[2]; 		/* Type 0:MMC, 1:SDC, 2:Block addressing */
static uint8_t PowerFlag[2] = {0, 0};	/* Power flag */

//-----[ SPI Functions ]-----

/* slave select */
static void SELECT(BYTE drv)
{
  HAL_GPIO_WritePin(SD_CS_PORT(drv), SD_CS_PIN(drv), GPIO_PIN_RESET);
}

/* slave deselect */
static void DESELECT(BYTE drv)
{
  HAL_GPIO_WritePin(SD_CS_PORT(drv), SD_CS_PIN(drv), GPIO_PIN_SET);
}

/* SPI transmit a byte */
static void SPI_TxByte(BYTE drv, uint8_t data)
{
  while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD(drv), SPI_FLAG_TXP));
  HAL_SPI_Transmit(HSPI_SDCARD(drv), &data, 1, SPI_TIMEOUT);
}

/* SPI transmit buffer */
static void SPI_TxBuffer(BYTE drv, uint8_t *buffer, uint16_t len)
{
  while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD(drv), SPI_FLAG_TXP));
  HAL_SPI_Transmit(HSPI_SDCARD(drv), buffer, len, SPI_TIMEOUT);
}

/* SPI receive a byte */
static uint8_t SPI_RxByte(BYTE drv)
{
  uint8_t dummy, data;
  dummy = 0xFF;
  while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD(drv), SPI_FLAG_TXP));
  HAL_SPI_TransmitReceive(HSPI_SDCARD(drv), &dummy, &data, 1, SPI_TIMEOUT);
  return data;
}

/* SPI receive a byte via pointer */
static void SPI_RxBytePtr(BYTE drv, uint8_t *buff)
{
  *buff = SPI_RxByte(drv);
}

//-----[ SD Card Functions ]-----

/* wait SD ready */
static uint8_t SD_ReadyWait(BYTE drv)
{
  uint8_t res;
  /* timeout 500ms */
  Timer2 = 500;
  /* if SD goes ready, receives 0xFF */
  do {
    res = SPI_RxByte(drv);
  } while ((res != 0xFF) && Timer2);
  return res;
}

/* power on */
static void SD_PowerOn(BYTE drv)
{
  uint8_t args[6];
  uint32_t cnt = 0x1FFF;
  /* transmit bytes to wake up */
  DESELECT(drv);
  for(int i = 0; i < 10; i++)
  {
    SPI_TxByte(drv,0xFF);
  }
  /* slave select */
  SELECT(drv);
  /* make idle state */
  args[0] = CMD0;   /* CMD0:GO_IDLE_STATE */
  args[1] = 0;
  args[2] = 0;
  args[3] = 0;
  args[4] = 0;
  args[5] = 0x95;
  SPI_TxBuffer(drv, args, sizeof(args));
  /* wait response */
  while ((SPI_RxByte(drv) != 0x01) && cnt)
  {
    cnt--;
  }
  DESELECT(drv);
  SPI_TxByte(drv,0XFF);
  PowerFlag[drv] = 1;
}

/* power off */
static void SD_PowerOff(BYTE drv)
{
  PowerFlag[drv] = 0;
}

/* check power flag */
static uint8_t SD_CheckPower(BYTE drv)
{
  return PowerFlag[drv];
}

/* receive data block */
static bool SD_RxDataBlock(BYTE drv, BYTE *buff, UINT len)
{
  uint8_t token;
  /* timeout 200ms */
  Timer1 = 200;
  /* loop until receive a response or timeout */
  do {
    token = SPI_RxByte(drv);
  } while((token == 0xFF) && Timer1);
  /* invalid response */
  if(token != 0xFE) return FALSE;
  /* receive data */
  do {
    SPI_RxBytePtr(drv, buff++);
  } while(--len);
  /* discard CRC */
  SPI_RxByte(drv);
  SPI_RxByte(drv);
  return TRUE;
}

/* transmit data block */
#if _USE_WRITE == 1
static bool SD_TxDataBlock(BYTE drv, const uint8_t *buff, BYTE token)
{
  uint8_t resp = 0;
  uint8_t i = 0;
  /* wait SD ready */
  if (SD_ReadyWait(drv) != 0xFF) return FALSE;
  /* transmit token */
  SPI_TxByte(drv, token);
  /* if it's not STOP token, transmit data */
  if (token != 0xFD)
  {
    SPI_TxBuffer(drv, (uint8_t*)buff, 512);
    /* discard CRC */
    SPI_RxByte(drv);
    SPI_RxByte(drv);
    /* receive response */
    while (i <= 64)
    {
      resp = SPI_RxByte(drv);
      /* transmit 0x05 accepted */
      if ((resp & 0x1F) == 0x05) break;
      i++;
    }
    /* recv buffer clear */
    while (SPI_RxByte(drv) == 0);
  }
  /* transmit 0x05 accepted */
  if ((resp & 0x1F) == 0x05) return TRUE;

  return FALSE;
}
#endif /* _USE_WRITE */

/* transmit command */
static BYTE SD_SendCmd(BYTE drv, BYTE cmd, uint32_t arg)
{
  uint8_t crc, res;
  /* wait SD ready */
  if (SD_ReadyWait(drv) != 0xFF) return 0xFF;
  /* transmit command */
  SPI_TxByte(drv, cmd);          /* Command */
  SPI_TxByte(drv, (uint8_t)(arg >> 24));   /* Argument[31..24] */
  SPI_TxByte(drv, (uint8_t)(arg >> 16));   /* Argument[23..16] */
  SPI_TxByte(drv, (uint8_t)(arg >> 8));  /* Argument[15..8] */
  SPI_TxByte(drv, (uint8_t)arg);       /* Argument[7..0] */
  /* prepare CRC */
  if(cmd == CMD0) crc = 0x95; /* CRC for CMD0(0) */
  else if(cmd == CMD8) crc = 0x87;  /* CRC for CMD8(0x1AA) */
  else crc = 1;
  /* transmit CRC */
  SPI_TxByte(drv, crc);
  /* Skip a stuff byte when STOP_TRANSMISSION */
  if (cmd == CMD12) SPI_RxByte(drv);
  /* receive response */
  uint8_t n = 10;
  do {
    res = SPI_RxByte(drv);
  } while ((res & 0x80) && --n);

  return res;
}

//-----[ user_diskio.c Functions ]-----

/* initialize SD */
DSTATUS SD_disk_initialize(BYTE drv)
{
  uint8_t n, type, ocr[4];
  /* no disk */
  if(Stat[drv] & STA_NODISK) return Stat[drv];
  /* power on */
  SD_PowerOn(drv);
  /* slave select */
  SELECT(drv);
  /* check disk type */
  type = 0;
  /* send GO_IDLE_STATE command */
  if (SD_SendCmd(drv, CMD0, 0) == 1)
  {
    /* timeout 1 sec */
    Timer1 = 1000;
    /* SDC V2+ accept CMD8 command, http://elm-chan.org/docs/mmc/mmc_e.html */
    if (SD_SendCmd(drv, CMD8, 0x1AA) == 1)
    {
      /* operation condition register */
      for (n = 0; n < 4; n++)
      {
        ocr[n] = SPI_RxByte(drv);
      }
      /* voltage range 2.7-3.6V */
      if (ocr[2] == 0x01 && ocr[3] == 0xAA)
      {
        /* ACMD41 with HCS bit */
        do {
          if (SD_SendCmd(drv, CMD55, 0) <= 1 && SD_SendCmd(drv, CMD41, 1UL << 30) == 0) break;
        } while (Timer1);

        /* READ_OCR */
        if (Timer1 && SD_SendCmd(drv, CMD58, 0) == 0)
        {
          /* Check CCS bit */
          for (n = 0; n < 4; n++)
          {
            ocr[n] = SPI_RxByte(drv);
          }

          /* SDv2 (HC or SC) */
          type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
        }
      }
    }
    else
    {
      /* SDC V1 or MMC */
      type = (SD_SendCmd(drv, CMD55, 0) <= 1 && SD_SendCmd(drv, CMD41, 0) <= 1) ? CT_SD1 : CT_MMC;
      do
      {
        if (type == CT_SD1)
        {
          if (SD_SendCmd(drv, CMD55, 0) <= 1 && SD_SendCmd(drv, CMD41, 0) == 0) break; /* ACMD41 */
        }
        else
        {
          if (SD_SendCmd(drv, CMD1, 0) == 0) break; /* CMD1 */
        }
      } while (Timer1);
      /* SET_BLOCKLEN */
      if (!Timer1 || SD_SendCmd(drv, CMD16, 512) != 0) type = 0;
    }
  }
  CardType[drv] = type;
  /* Idle */
  DESELECT(drv);
  SPI_RxByte(drv);
  /* Clear STA_NOINIT */
  if (type)
  {
    Stat[drv] &= ~STA_NOINIT;
  }
  else
  {
    /* Initialization failed */
    SD_PowerOff(drv);
  }
  return Stat[drv];
}

/* return disk status */
DSTATUS SD_disk_status(BYTE drv)
{
  return Stat[drv];
}

/* read sector */
DRESULT SD_disk_read(BYTE drv, BYTE* buff, DWORD sector, UINT count)
{
  /* pdrv should be 0 */
  if (!count) return RES_PARERR;

  /* no disk */
  if (Stat[drv] & STA_NOINIT) return RES_NOTRDY;

  /* convert to byte address */
  if (!(CardType[drv] & CT_SD2)) sector *= 512;

  SELECT(drv);

  if (count == 1)
  {
    /* READ_SINGLE_BLOCK */
    if ((SD_SendCmd(drv, CMD17, sector) == 0) && SD_RxDataBlock(drv, buff, 512)) count = 0;
  }
  else
  {
    /* READ_MULTIPLE_BLOCK */
    if (SD_SendCmd(drv, CMD18, sector) == 0)
    {
      do {
        if (!SD_RxDataBlock(drv, buff, 512)) break;
        buff += 512;
      } while (--count);

      /* STOP_TRANSMISSION */
      SD_SendCmd(drv, CMD12, 0);
    }
  }

  /* Idle */
  DESELECT(drv);
  SPI_RxByte(drv);

  return count ? RES_ERROR : RES_OK;
}

/* write sector */
#if _USE_WRITE == 1
DRESULT SD_disk_write(BYTE drv, const BYTE* buff, DWORD sector, UINT count)
{
  if (!count) return RES_PARERR;

  /* no disk */
  if (Stat[drv] & STA_NOINIT) return RES_NOTRDY;

  /* write protection */
  if (Stat[drv] & STA_PROTECT) return RES_WRPRT;

  /* convert to byte address */
  if (!(CardType[drv] & CT_SD2)) sector *= 512;

  SELECT(drv);

  if (count == 1)
  {
    /* WRITE_BLOCK */
    if ((SD_SendCmd(drv, CMD24, sector) == 0) && SD_TxDataBlock(drv, buff, 0xFE))
      count = 0;
  }
  else
  {
    /* WRITE_MULTIPLE_BLOCK */
    if (CardType[drv] & CT_SD1)
    {
      SD_SendCmd(drv, CMD55, 0);
      SD_SendCmd(drv, CMD23, count); /* ACMD23 */
    }

    if (SD_SendCmd(drv, CMD25, sector) == 0)
    {
      do {
        if(!SD_TxDataBlock(drv, buff, 0xFC)) break;
        buff += 512;
      } while (--count);

      /* STOP_TRAN token */
      if(!SD_TxDataBlock(drv, 0, 0xFD))
      {
        count = 1;
      }
    }
  }

  /* Idle */
  DESELECT(drv);
  SPI_RxByte(drv);

  return count ? RES_ERROR : RES_OK;
}
#endif /* _USE_WRITE */

/* ioctl */
DRESULT SD_disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
  DRESULT res;
  uint8_t n, csd[16], *ptr = buff;
  WORD csize;

  res = RES_ERROR;

  if (ctrl == CTRL_POWER)
  {
    switch (*ptr)
    {
    case 0:
      SD_PowerOff(drv);    /* Power Off */
      res = RES_OK;
      break;
    case 1:
      SD_PowerOn(drv);   /* Power On */
      res = RES_OK;
      break;
    case 2:
      *(ptr + 1) = SD_CheckPower(drv);
      res = RES_OK;   /* Power Check */
      break;
    default:
      res = RES_PARERR;
    }
  }
  else
  {
    /* no disk */
    if (Stat[drv] & STA_NOINIT){
    	return RES_NOTRDY;
    }
    SELECT(drv);
    switch (ctrl)
    {
    case GET_SECTOR_COUNT:
      /* SEND_CSD */
      if ((SD_SendCmd(drv, CMD9, 0) == 0) && SD_RxDataBlock(drv, csd, 16))
      {
        if ((csd[0] >> 6) == 1)
        {
          /* SDC V2 */
          csize = csd[9] + ((WORD) csd[8] << 8) + 1;
          *(DWORD*) buff = (DWORD) csize << 10;
        }
        else
        {
          /* MMC or SDC V1 */
          n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
          csize = (csd[8] >> 6) + ((WORD) csd[7] << 2) + ((WORD) (csd[6] & 3) << 10) + 1;
          *(DWORD*) buff = (DWORD) csize << (n - 9);
        }
        res = RES_OK;
      }
      break;
    case GET_SECTOR_SIZE:
      *(WORD*) buff = 512;
      res = RES_OK;
      break;
    case CTRL_SYNC:
      if (SD_ReadyWait(drv) == 0xFF) res = RES_OK;
      break;
    case MMC_GET_CSD:
      /* SEND_CSD */
      if (SD_SendCmd(drv, CMD9, 0) == 0 && SD_RxDataBlock(drv, ptr, 16)) res = RES_OK;
      break;
    case MMC_GET_CID:
      /* SEND_CID */
      if (SD_SendCmd(drv, CMD10, 0) == 0 && SD_RxDataBlock(drv, ptr, 16)) res = RES_OK;
      break;
    case MMC_GET_OCR:
      /* READ_OCR */
      if (SD_SendCmd(drv, CMD58, 0) == 0)
      {
        for (n = 0; n < 4; n++)
        {
          *ptr++ = SPI_RxByte(drv);
        }
        res = RES_OK;
      }
    default:
      res = RES_PARERR;
    }
    DESELECT(drv);
    SPI_RxByte(drv);
  }
  return res;
}
