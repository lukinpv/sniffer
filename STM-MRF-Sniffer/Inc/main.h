/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "GUI.h"


#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_ts.h"
#include "stm32f429i_discovery_sdram.h"
#include "../Components/ili9341/ili9341.h" 

/* Exported constants --------------------------------------------------------*/

/* Definition for SPIx clock resources */
#define SPIx                             SPI4
#define SPIx_CLK_ENABLE()                __HAL_RCC_SPI4_CLK_ENABLE()
#define SPIx_SCK_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOE_CLK_ENABLE()
#define SPIx_MISO_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE() 
#define SPIx_MOSI_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE() 
#define SPIx_NSS_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOE_CLK_ENABLE()
     
#define SPIx_FORCE_RESET()               __HAL_RCC_SPI4_FORCE_RESET()
#define SPIx_RELEASE_RESET()             __HAL_RCC_SPI4_RELEASE_RESET()

/* Definition for SPIx Pins */
#define SPIx_SCK_PIN                     GPIO_PIN_2
#define SPIx_SCK_GPIO_PORT               GPIOE
#define SPIx_SCK_AF                      GPIO_AF5_SPI4
#define SPIx_MISO_PIN                    GPIO_PIN_5
#define SPIx_MISO_GPIO_PORT              GPIOE
#define SPIx_MISO_AF                     GPIO_AF5_SPI4
#define SPIx_MOSI_PIN                    GPIO_PIN_6
#define SPIx_MOSI_GPIO_PORT              GPIOE
#define SPIx_MOSI_AF                     GPIO_AF5_SPI4

/* Definition for Olimex Pins */
#define OLIMEX_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOD_CLK_ENABLE()
#define OLMX_GPIO_PORT                  GPIOD

#define OLMX_CS_PIN                     GPIO_PIN_5
#define OLMX_CS_GPIO_PORT               GPIOD
#define OLMX_RST_PIN                    GPIO_PIN_2
#define OLMX_RST_GPIO_PORT              GPIOD
#define OLMX_INT_PIN                    GPIO_PIN_4
#define OLMX_INT_GPIO_PORT              GPIOD

/* Size of buffer */
#define BUFFERSIZE                      1

/* Exported functions ------------------------------------------------------- */

uint8_t spi_sync_exchange(uint8_t tx_data);
void RXPrint(void);

#endif /* __MAIN_H */

