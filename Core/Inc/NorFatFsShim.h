// NOR FatFS write shim implementation developed with OpenAI Codex.
// GPT 6 Luna Medium
// Directed and edited by Jonathan Engdahl 2026.10.07

#ifndef NOR_FATFS_SHIM_H
#define NOR_FATFS_SHIM_H

#include <stdint.h>
#include "QSPI.h"
#include "diskio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Write FatFS logical sectors while preserving NOR erase-sector contents. */
DRESULT NorFatFs_WriteSectors(OSPI_HandleTypeDef *hospi,
                              const uint8_t *data,
                              unsigned first_sector,
                              unsigned sector_count);

#ifdef __cplusplus
}
#endif

#endif /* NOR_FATFS_SHIM_H */
