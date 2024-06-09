#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "spi.h"
#include "octospi.h"
#include "QSPI.h"

HAL_StatusTypeDef QSPI_WritePage(OSPI_HandleTypeDef *hospi, uint32_t address, uint8_t *data, uint32_t size)
{
    OSPI_RegularCmdTypeDef sCommand;

    if (size > 256)
        return HAL_ERROR; // Page size is 256 bytes

    // Enable write operations
    sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = 0x06; // Write Enable command
    sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_8_BITS; // Set to valid default value
    sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS; // Set to valid default value
    sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.NbData = 0; // No data for write enable command
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.DummyCycles = 0;
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Configure the command for the page program operation
    sCommand.Instruction = 0x32; // Quad Page Program command
    sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE; // Address in single line mode
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.Address = address;
    sCommand.DataMode = HAL_OSPI_DATA_4_LINES; // Data in quad line mode
    sCommand.NbData = size;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Transmission of the data
    if (HAL_OSPI_Transmit(hospi, data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Wait for the end of the program
    uint8_t reg;
    do
    {
        sCommand.Instruction = 0x05; // Read Status Register command
        sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
        sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
        sCommand.NbData = 1;
        if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return HAL_ERROR;
        }
        if (HAL_OSPI_Receive(hospi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return HAL_ERROR;
        }
    } while (reg & 0x01); // Check the WIP (Write In Progress) bit

    return HAL_OK;
}



HAL_StatusTypeDef QSPI_ReadPage(OSPI_HandleTypeDef *hospi, uint32_t address, uint8_t *data, uint32_t size)
{
    OSPI_RegularCmdTypeDef sCommand;

    if (size > 256)
        return HAL_ERROR; // Page size is 256 bytes

    // Configure the command for the read operation
    sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = 0xEB; // Fast Read Quad I/O command
    sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS; // Ensure correct instruction size
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE; // Disable DTR mode for instruction
    sCommand.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE; // Disable DTR mode for address
    sCommand.Address = address;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
    sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS; // Alternate byte is required
    sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE; // Disable DTR mode for alternate bytes
    sCommand.AlternateBytes = 0x00; // Dummy alternate byte
    sCommand.DataMode = HAL_OSPI_DATA_4_LINES;
    sCommand.NbData = size;
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE; // Disable DTR mode for data
    sCommand.DummyCycles = 4; // Set appropriate dummy cycles
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Reception of the data
    if (HAL_OSPI_Receive(hospi, data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}



HAL_StatusTypeDef QSPI_EraseSector(OSPI_HandleTypeDef *hospi, uint32_t address)
{
    OSPI_RegularCmdTypeDef sCommand;

    // Enable write operations
    sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = 0x06; // Write Enable command
    sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_8_BITS; // Set to valid default value
    sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS; // Set to valid default value
    sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.NbData = 0; // No data for write enable command
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.DummyCycles = 0;
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Configure the command for the sector erase operation
    sCommand.Instruction = 0x20; // Sector Erase command
    sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.Address = address;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.NbData = 0;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Wait for the end of the erase operation
    uint8_t reg;
    do
    {
        sCommand.Instruction = 0x05; // Read Status Register command
        sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
        sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
        sCommand.NbData = 1;
        if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return HAL_ERROR;
        }
        if (HAL_OSPI_Receive(hospi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return HAL_ERROR;
        }
    } while (reg & 0x01); // Check the WIP (Write In Progress) bit

    return HAL_OK;
}

HAL_StatusTypeDef QSPI_EraseChip(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand;

    // Enable write operations
    sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = 0x06; // Write Enable command
    sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_8_BITS; // Set to valid default value
    sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS; // Set to valid default value
    sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.NbData = 0; // No data for write enable command
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.DummyCycles = 0;
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Configure the command for the chip erase operation
    sCommand.Instruction = 0xC7; // Chip Erase command
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.NbData = 0;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Wait for the end of the erase operation
    uint8_t reg;
    do
    {
        sCommand.Instruction = 0x05; // Read Status Register command
        sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
        sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
        sCommand.NbData = 1;
        if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return HAL_ERROR;
        }
        if (HAL_OSPI_Receive(hospi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return HAL_ERROR;
        }
    } while (reg & 0x01); // Check the WIP (Write In Progress) bit

    return HAL_OK;
}

HAL_StatusTypeDef QSPI_ReadStatusReg(OSPI_HandleTypeDef *hospi, uint8_t regCommand, uint8_t *regValue)
{
    OSPI_RegularCmdTypeDef sCommand;

    sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction = regCommand;
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE; // Single line mode
    sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address = 0x00000000; // No address for status register read
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_8_BITS; // Not used, should be set to a valid default
    sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytes = 0x00000000; // Not used
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS; // Not used, should be set to a valid default
    sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
    sCommand.DataMode = HAL_OSPI_DATA_1_LINE; // Single line mode
    sCommand.NbData = 1;
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.DummyCycles = 0;
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_OSPI_Receive(hospi, regValue, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}


