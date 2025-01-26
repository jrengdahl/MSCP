#ifndef QSPI_H
#define QSPI_H

#include <stdint.h>
#include "main.h"
#include "spi.h"
#include "octospi.h"


#ifdef __cplusplus
extern "C" {
#endif

#define QSPI_PAGE_SIZE 256
#define QSPI_LBA_SIZE 512
#define QSPI_BLOCK_SIZE 4096
#define QSPI_TOTAL_SIZE (16 * 1024 * 1024) // 16 MB for example


#define READ_STATUS_REG_1_CMD 0x05
#define READ_STATUS_REG_2_CMD 0x35
#define READ_STATUS_REG_3_CMD 0x15

HAL_StatusTypeDef QSPI_WritePage(OSPI_HandleTypeDef *hospi, uint32_t address, uint8_t *data, uint32_t size);
HAL_StatusTypeDef QSPI_ReadPage(OSPI_HandleTypeDef *hospi, uint32_t address, uint8_t *data, uint32_t size);
HAL_StatusTypeDef QSPI_EraseSector(OSPI_HandleTypeDef *hospi, uint32_t address);
HAL_StatusTypeDef QSPI_EraseChip(OSPI_HandleTypeDef *hospi);
HAL_StatusTypeDef QSPI_ReadStatusReg(OSPI_HandleTypeDef *hospi, uint8_t regCommand, uint8_t *regValue);

#ifdef __cplusplus
}
#endif

#endif // QSPI_H
