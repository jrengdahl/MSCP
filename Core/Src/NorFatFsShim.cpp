// SPI-NOR FatFS write shim implementation developed with OpenAI Codex.
// GPT 6 Luna Medium
// Specified and edited by Jonathan Engdahl 2026.10.07

// Be forewarned that running FATFS on a Winbond W25Q128JV SPI-NOR without a wear-leveling layer will shorten its lifespan.
// For the MSCP SSD where the SPI-NOR is only used for boot images with infrequent updates, this does not matter.

// The write algorithm assume that an erase sector can be re-written and modified without being erased, as long as only one-to-zero bit
// transitions are written. This is not actually specified in the WinBond data sheet.

// The static 4K work buffer causes NorFatFs_WriteSectors() to be non-reentrant.


#include "NorFatFsShim.h"

#include <string.h>
#include "diskio.h"

#define NOR_FLASH_ERASE_SECTOR_SIZE QSPI_BLOCK_SIZE                                             // a WinBond SPI-NOR erase sector is 4096 bytes, or 8 FATFS sectors
#define FATFS_LOGICAL_SECTOR_SIZE QSPI_LBA_SIZE

#if (NOR_FLASH_ERASE_SECTOR_SIZE % FATFS_LOGICAL_SECTOR_SIZE) != 0
  #error "Flash erase sector size must be a multiple of the FatFS sector size"
#endif

#if FATFS_LOGICAL_SECTOR_SIZE != (2 * QSPI_PAGE_SIZE)
  #error "FatFS sector must consist of exactly two flash pages"
#endif

#define NOR_FATFS_SECTORS_PER_ERASE (NOR_FLASH_ERASE_SECTOR_SIZE / FATFS_LOGICAL_SECTOR_SIZE)

// 4K Erase sector buffer. Reused for one flash erase sector at a time; keep this off the task stack.

static uint8_t erase_sector_buffer[NOR_FLASH_ERASE_SECTOR_SIZE];

// test whether the region specified by inputs data and size contains any zero bits
// returns true if it does, false if the region is all 0xFF
static bool buffer_region_has_zero_bits(const uint8_t *data, unsigned size)
    {
    for (unsigned i = 0; i < size; i++)
        {
        if (data[i] != 0xFFU)
            {
            return true;
            }
        }
    return false;
    }


// Compare the old data read from flash with new data to be written to flash.
// Return true if the new data can be written over the old data without erasing first.
// A NOR flash write can only flip ones to zeros. If the write would require any zeros to be changed to ones,
// the entire 4K erase block must be erased first.
static bool can_program_without_erase(const uint8_t *old_data,
                                      const uint8_t *new_data,
                                      unsigned size)
    {
    for (unsigned i = 0; i < size; ++i)
        {
        if ((old_data[i] & new_data[i]) != new_data[i])
            {
            return false;
            }
        }
    return true;
    }



// Read one 512-byte FATFS sector from flash
static HAL_StatusTypeDef read_fatfs_sector(OSPI_HandleTypeDef *hospi,
                                           unsigned sector,
                                           uint8_t *destination)
    {
    const uint32_t address = sector * FATFS_LOGICAL_SECTOR_SIZE;

    if(QSPI_ReadPage(hospi, address                 , destination                 , QSPI_PAGE_SIZE) != HAL_OK
    || QSPI_ReadPage(hospi, address + QSPI_PAGE_SIZE, destination + QSPI_PAGE_SIZE, QSPI_PAGE_SIZE) != HAL_OK)
            {
            return HAL_ERROR;
            }
    return HAL_OK;
    }

// Write one FATFS sector to flash, as two single flash pages.
// If the data in a page to be written is all ones, don't bother writing that page.
static HAL_StatusTypeDef program_fatfs_sector(OSPI_HandleTypeDef *hospi,
                                              unsigned sector,
                                              const uint8_t *source)
    {
    uint32_t address = sector * FATFS_LOGICAL_SECTOR_SIZE;

    if((buffer_region_has_zero_bits(source                 , QSPI_PAGE_SIZE) && QSPI_WritePage(hospi, address                 , (uint8_t *)source                 , QSPI_PAGE_SIZE) != HAL_OK)
    || (buffer_region_has_zero_bits(source + QSPI_PAGE_SIZE, QSPI_PAGE_SIZE) && QSPI_WritePage(hospi, address + QSPI_PAGE_SIZE, (uint8_t *)source + QSPI_PAGE_SIZE, QSPI_PAGE_SIZE) != HAL_OK))
        {
        return HAL_ERROR;
        }
    return HAL_OK;
    }


// Write a block of data to flash, starting at first_sector, which is the number of the FATFS sector.
// The sector_count is in FATFS sectors (512 byte chunks).

extern "C"
DRESULT NorFatFs_WriteSectors(OSPI_HandleTypeDef *hospi,
                              const uint8_t *data,              // pointer to data to be written
                              unsigned first_sector,            // the first FATFS sector number to write the data to
                              unsigned sector_count)            // the number of FATFS sectors to write
{
    if (hospi == 0 || (sector_count != 0U && data == 0))
        {
        return RES_PARERR;
        }

    if (sector_count == 0U)
        {
        return RES_OK;
        }

    if (first_sector >= QSPI_TOTAL_SIZE / FATFS_LOGICAL_SECTOR_SIZE
    ||  sector_count >  QSPI_TOTAL_SIZE / FATFS_LOGICAL_SECTOR_SIZE - first_sector)
        {
        return RES_PARERR;
        }

    const unsigned end_sector = first_sector + sector_count;    // one past the last sector
    unsigned next_sector = first_sector;                                                                        // the current FATFS sector
    while (next_sector < end_sector)
        {
        const unsigned erase_first_sector = next_sector - (next_sector % NOR_FATFS_SECTORS_PER_ERASE);          // the first FATFS sector of the current erase sector
        const unsigned erase_end_sector = erase_first_sector + NOR_FATFS_SECTORS_PER_ERASE;                     // one past the last FATFS sector of the current erase sector
        const unsigned write_end_sector = (end_sector < erase_end_sector) ? end_sector : erase_end_sector;      // one past the last FATFS sector to be written to the current erase sector
        const unsigned write_count = write_end_sector - next_sector;                                            // the number of FATFS sectors to be written to the current erase sector

        bool erase_required = false;

        // Read all requested old sectors first, into their erase-sector offsets in the 4K buffer.
        for (unsigned sector = next_sector; sector < write_end_sector; sector++)
            {
            const unsigned erase_buffer_offset = (sector - erase_first_sector) * FATFS_LOGICAL_SECTOR_SIZE;     // byte offset into erase_sector_buffer of the old FATFS sector
            const unsigned write_buffer_offset = (sector - first_sector) * FATFS_LOGICAL_SECTOR_SIZE;           // byte offset into user's write request buffer of the new FATFS sector

            // read one old sector into the corresponding offset in the erase sector buffer
            if (read_fatfs_sector(hospi, sector, &erase_sector_buffer[erase_buffer_offset]) != HAL_OK)
                {
                return RES_ERROR;
                }

            // see if an erase will be required to write the new data
            if (!can_program_without_erase(&erase_sector_buffer[erase_buffer_offset], &data[write_buffer_offset], FATFS_LOGICAL_SECTOR_SIZE))
                {
                erase_required = true;
                }
            }

        // no erase required: copy the data directly from the user's write buffer to flash, quick and easy
        if (!erase_required)
            {
            for (unsigned sector = next_sector; sector < write_end_sector; sector++)
                {
                const unsigned erase_buffer_offset = (sector - erase_first_sector) * FATFS_LOGICAL_SECTOR_SIZE;
                const unsigned write_buffer_offset = (sector - first_sector) * FATFS_LOGICAL_SECTOR_SIZE;

                if(memcmp(&erase_sector_buffer[erase_buffer_offset], &data[write_buffer_offset], FATFS_LOGICAL_SECTOR_SIZE) != 0
                && program_fatfs_sector(hospi, sector, &data[write_buffer_offset]) != HAL_OK)
                    {
                    return RES_ERROR;
                    }
                }
            }

        else // an erase is required
            {
            // read in old contents of sectors not covered by this write chunk, so the old data that won't be over-written is not lost
            for (unsigned sector = erase_first_sector; sector < erase_end_sector; sector++)
                {
                if (sector >= next_sector && sector < write_end_sector)continue;  // skip the read if it has already been read

                const unsigned erase_buffer_offset = (sector - erase_first_sector) * FATFS_LOGICAL_SECTOR_SIZE;
                if (read_fatfs_sector(hospi, sector, &erase_sector_buffer[erase_buffer_offset]) != HAL_OK)
                    {
                    return RES_ERROR;
                    }
                }

            // erase the entire erase sector
            if (QSPI_EraseSector(hospi, erase_first_sector * FATFS_LOGICAL_SECTOR_SIZE) != HAL_OK)
                {
                return RES_ERROR;
                }

            // Restore untouched sectors from scratch and targets directly from FatFS.
            for (unsigned sector = erase_first_sector; sector < erase_end_sector; sector++)
                {
                const uint8_t *source;

                // if the data is within the write region, choose the source from user data buffer
                if (sector >= next_sector && sector < write_end_sector)
                    {
                    const unsigned write_buffer_offset = (sector - first_sector) * FATFS_LOGICAL_SECTOR_SIZE;
                    source = &data[write_buffer_offset];
                    }

                // else if the data is preserved old data, choose the source from the erase sector buffer
                else
                    {
                    const unsigned erase_buffer_offset = (sector - erase_first_sector) * FATFS_LOGICAL_SECTOR_SIZE;
                    source = &erase_sector_buffer[erase_buffer_offset];
                    }

                // write either old or new data to the now-blank erase sector, but only if the data is not all ones
                if(buffer_region_has_zero_bits(source, FATFS_LOGICAL_SECTOR_SIZE)
                && program_fatfs_sector(hospi, sector, source) != HAL_OK)
                    {
                    return RES_ERROR;
                    }
                }
            }

        next_sector += write_count;
        }

    return RES_OK;
    }
