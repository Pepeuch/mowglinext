# Hardware Map — `ros_usbnode`

Audit snapshot for preparing a portable HAL refactor. This document maps the current STM32F103 / STM32F401 hardware coupling points without changing firmware behavior.

## Board targets

| PlatformIO env | MCU / board | Compile flag | HAL include path | Notes |
|---|---|---|---|---|
| `Yardforce500` | `genericSTM32F103VC` / STM32F103VCT6 | `BOARD_YARDFORCE500_VARIANT_ORIG=1` | `stm32f1xx_hal*.h` via `include/stm32f_board_hal.h` | Original YardForce 500 board. Uses F1 AFIO remaps and DMA channels. |
| `Yardforce500B` | `genericSTM32F401VC` / STM32F401VCT6 | `BOARD_YARDFORCE500_VARIANT_B=1` | `stm32f4xx_hal*.h` via `include/stm32f_board_hal.h` | YardForce 500B board. Uses F4 GPIO alternate functions and DMA streams. |

## UART usage

| Role | STM32F103 peripheral / pins | STM32F401 peripheral / pins | Code zones | Notes |
|---|---|---|---|---|
| Master / debug UART | `UART4`, RX `PC11`, TX `PC10`, IRQ `UART4_IRQn` when `BOARD_HAS_MASTER_USART=1` | Disabled by `BOARD_HAS_MASTER_USART=0` | `include/board.h`, `src/main.c` | Also owns DMA handles named `hdma_uart4_rx/tx`. Used for debug / optional master link. |
| Drive motors PAC5210 | `USART2`, RX `PD6`, TX `PD5`, IRQ `USART2_IRQn` | `USART2`, RX `PD6`, TX `PD5`, AF `GPIO_AF7_USART2`, IRQ `USART2_IRQn` | `include/board.h`, `src/drivemotor.c`, `src/main.c` callbacks | F103 uses `__HAL_AFIO_REMAP_USART2_ENABLE()`. |
| Blade motor PAC5223 | `USART3`, RX `PB11`, TX `PB10`, IRQ `USART3_IRQn` | `USART6`, RX `PC7`, TX `PC6`, AF `GPIO_AF8_USART6`, IRQ `USART6_IRQn` | `include/board.h`, `src/blademotor.c`, `src/main.c` callbacks | Instance differs between board variants. |
| Panel | `USART1`, RX `PA10`, TX `PA9`, IRQ `USART1_IRQn` | `USART1`, RX `PA10`, TX `PA9`, AF `GPIO_AF7_USART1`, IRQ `USART1_IRQn` | `include/board.h`, `src/panel.c`, `src/main.c` callbacks | Protocol / LED state logic mixed with UART/DMA setup. |

## DMA usage

| Role | STM32F103 DMA | STM32F401 DMA | Code zones | Notes |
|---|---|---|---|---|
| Master UART RX | `DMA2_Channel3` | Not used by current F401 board config | `src/main.c` | Hard-coded in `MASTER_USART_Init()`. |
| Master UART TX | `DMA2_Channel5` | Not used by current F401 board config | `src/main.c` | Hard-coded in `MASTER_USART_Init()`. |
| Drive motor RX | `DMA1_Channel6` | `DMA1_Stream5`, channel `DMA_CHANNEL_4` | `src/drivemotor.c` | HAL DMA handle `hdma_usart2_rx`. |
| Drive motor TX | `DMA1_Channel7` | `DMA1_Stream6`, channel `DMA_CHANNEL_4` | `src/drivemotor.c` | HAL DMA handle `hdma_usart2_tx`. |
| Blade motor RX | `DMA1_Channel3` | `DMA2_Stream1`, channel `DMA_CHANNEL_5` | `src/blademotor.c` | HAL DMA handle `hdma_uart_blade_rx`. |
| Blade motor TX | `DMA1_Channel2` | `DMA2_Stream6`, channel `DMA_CHANNEL_5` | `src/blademotor.c` | HAL DMA handle `hdma_uart_blade_tx`. |
| Panel RX | `DMA1_Channel5` | `DMA2_Stream5`, channel `DMA_CHANNEL_4` | `src/panel.c` | Uses `HAL_UARTEx_ReceiveToIdle_DMA()`. |
| Panel TX | `DMA1_Channel4` | `DMA2_Stream7`, channel `DMA_CHANNEL_4` | `src/panel.c` | Used for panel command frames. |
| Perimeter ADC | `DMA1_Channel1` | Not currently supported | `src/perimeter.c` | Perimeter sensing is explicitly F103-only currently. |
| DMA IRQ setup | `DMA1_Channel1..7`, `DMA2_Channel3`, `DMA2_Channel4_5` | `DMA1_Stream5/6`, `DMA2_Stream1/5/6/7` | `src/main.c::MX_DMA_Init()` | Should move behind a DMA routing table / HAL backend. |

## Timer usage

| Timer | Role | STM32F103 details | STM32F401 details | Code zones | Notes |
|---|---|---|---|---|---|
| `TIM1` | Charge PWM, CH1/CH1N | Uses AFIO remap `__HAL_AFIO_REMAP_TIM1_ENABLE()` for charge pins | Uses AF `GPIO_AF1_TIM1` | `src/charger.c` | Direct write to `TIM1->CCR1`; needs PWM abstraction first. |
| `TIM2` | ADC charging trigger | `ADC_EXTERNALTRIGCONV_T2_CC2`; prescaler assumes 72 MHz clock | Same logical trigger, with F4-specific ADC init fields | `src/adc.c` | Sampling timing depends on clock tree. |
| `TIM3` | Beeper PWM, CH4 on `PB1` | GPIO AF push-pull | AF `GPIO_AF2_TIM3` | `src/main.c::TIM3_Init()`, `chirp()` | Direct write to `TIM3_Handle.Instance->CCR4`. |
| `TIM4` | Buzzer PWM, CH3 on `PD14` | Uses `__HAL_AFIO_REMAP_TIM4_ENABLE()` | AF `GPIO_AF2_TIM4` | `src/main.c::TIM4_Init()`, `chirp()` | Direct write to `TIM4_Handle.Instance->CCR3`. |
| `IWDG` / `WWDG` | Watchdogs | STM32 HAL watchdogs | STM32 HAL watchdogs | `src/main.c::WATCHDOG_vInit()`, `WATCHDOG_Refresh()` | Should move behind `hal_watchdog`. |

## ADC usage

| ADC role | STM32F103 | STM32F401 | Channels / pins | Code zones | Notes |
|---|---|---|---|---|---|
| Charging / battery monitor | `ADC2`, IRQ `ADC1_2_IRQn`, calibration via `HAL_ADCEx_Calibration_Start()` | `ADC1`, IRQ `ADC_IRQn`, no equivalent calibration currently implemented | `PA1` charge current, `PA2` charge voltage, `PA3` battery voltage, `PA7` charger input, `PC2` blade NTC | `src/adc.c`, `include/perimeter.h`, `src/charger.c` | ADC peripheral differs per board. Conversion math is mixed with HAL sampling. |
| Perimeter sense | `ADC1` + `DMA1_Channel1` | Not supported; comment notes ADC1 conflict with charging on 500B | `PA6` perimeter sense, `PB8/PB9` coil selection | `src/perimeter.c` | Good candidate for later optional driver module. |

## I2C usage

| Bus | Role | Pins / peripheral | Code zones | Notes |
|---|---|---|---|---|
| Hardware I2C | Onboard LIS3DH accelerometer for tilt protection | `I2C1`, `PB6/PB7`, AF `GPIO_AF4_I2C1` on F401 | `src/i2c.c`, `include/i2c.h`, `include/i2c_lis3dh.h` | Driver directly exposes LIS3DH-specific helpers; split into generic `hal_i2c` + LIS3DH driver. |
| Software I2C | External IMU bus on J18 | `PB3/PB4` from `SOFT_I2C_*` board macros | `src/soft_i2c.c`, `include/soft_i2c.h`, `src/imu/*` | F103 disables JTAG via direct `RCC->APB2ENR` / `AFIO->MAPR`; timing is CPU-loop based and non-portable. |

## USB / IRQ usage

| Area | STM32F103 | STM32F401 | Code zones | Notes |
|---|---|---|---|---|
| USB CDC device | USB FS device using F1 Cube USB stack | USB OTG FS style via F4 proxy files | `src/usb_device.c`, `src/usbd_conf.c`, `src/usbd_cdc_if.c`, `include/usbd_*`, `CDC/*`, `src/proxy_inc/*` | Transport should be separated from Mowgli protocol framing before ESP32 ports. |
| USB IRQ | `USB_LP_CAN1_RX0_IRQn` / `USB_LP_CAN1_RX0_IRQHandler` | `OTG_FS_IRQn` / F4 handlers in proxy include | `include/stm32f1xx_it.h`, `include/stm32f4xx_it.h`, `src/stm32f_it.c`, `src/proxy_inc/stm32f1`, `src/proxy_inc/stm32f4` | `src/stm32f_it.c` includes family-specific `.c` files directly. |
| UART callbacks | HAL global callbacks dispatch by `huart->Instance` | Same pattern | `src/main.c::HAL_UART_*Callback()` | Dispatch should move to a UART registry / HAL callback adapter. |
| ADC / DMA callbacks | HAL callback style, family-specific IRQ names | Same style with different IRQs | `src/adc.c`, `src/perimeter.c`, `src/main.c`, proxy IRQ files | IRQ routing is currently implicit through HAL globals. |

## Other GPIO / board-controlled signals

| Signal | Pins / macros | Code zones | Notes |
|---|---|---|---|
| Status LED | `PB2` | `include/board.h`, `src/main.c` | Simple first candidate for GPIO HAL. |
| 24 V switch `TF4` | `PC5` | `include/board.h`, `src/main.c`, `src/charger.c` | Safety-relevant power path. |
| Blade reset | `PE14` | `include/board.h`, `src/blademotor.c` | Board-specific active level. |
| Drive reset / enable | `PE15`, plus `PD7/PD8` | `include/board.h`, `src/drivemotor.c` | Board-specific motor controller glue. |
| Charge high/low side | `PE8/PE9` | `include/board.h`, `src/charger.c` | Coupled to `TIM1` complementary PWM. |
| Stop buttons | `PC0`, `PC8` | `include/board.h`, `src/emergency.c`, `src/panel.c` | Input polarity should be declarative. |
| Tilt input | `PA8` | `include/board.h`, `src/emergency.c` | Safety input. |
| Wheel lift | `PD0/PD1` | `include/board.h`, `src/emergency.c` | Safety input. |
| Play / Home buttons | `PC7/PC9` or `PC9`, `PB13` | `include/board.h`, `src/panel.c` | `PLAY_BUTTON_PIN` differs by variant. |
| Rain sensor | `PE2` | `include/board.h`, `src/main.c` | Input polarity active-low. |
| Hall stop / bumper | `PD2/PD3` | `include/board.h`, `src/main.c` | Conditional via `OPTION_BUMPER`. |

## Main hardware-coupled code zones

| File | Coupling type |
|---|---|
| `include/board.h` | Board selection, pin map, peripheral instances, IRQ names, feature flags. |
| `include/stm32f_board_hal.h` | Family-specific STM32 HAL includes. |
| `src/main.c` | Clock tree, DMA IRQ setup, GPIO init, PWM init, watchdogs, debug UART, callbacks. |
| `src/adc.c` | ADC peripheral choice, trigger timer, channels, IRQ, RTC backup storage. |
| `src/charger.c` | TIM1 PWM, complementary output, direct CCR write, RTC backup writes. |
| `src/drivemotor.c` | UART/DMA setup, AF remap, motor reset GPIO. |
| `src/blademotor.c` | UART/DMA setup, AF remap, blade reset GPIO. |
| `src/panel.c` | GPIO buttons, panel UART/DMA setup, panel protocol. |
| `src/i2c.c` | I2C1 setup and LIS3DH-specific bus access. |
| `src/soft_i2c.c` | Bit-banged I2C, direct F1 AFIO/JTAG register manipulation, CPU-loop timing. |
| `src/perimeter.c` | F103-only ADC/DMA perimeter sampling. |
| `src/stm32f_it.c`, `src/proxy_inc/*` | Family-specific interrupt handlers/startup glue. |
| `src/usb_device.c`, `src/usbd_*`, `CDC/*` | STM32 USB CDC transport. |

## HAL abstraction priorities

| Priority | Abstraction | Why first | Suggested first users |
|---|---|---|---|
| 1 | GPIO / pinmux / clock enable | Lowest risk, unlocks board pin tables, removes many `GPIOx` macros from app code. | LED, rain, play/home, emergency inputs. |
| 2 | Time / delay / tick | Needed for STM32 + ESP32 portability and testability. | `HAL_GetTick()`, `HAL_Delay()`, soft I2C delays, debounce logic. |
| 3 | UART + DMA | Most duplicated F1/F4 logic and essential for motors/panel. | Panel first, then blade motor, then drive motor. |
| 4 | PWM / timer output | Removes direct `TIMx->CCR` writes and family-specific AF remaps. | Charge PWM, beeper, buzzer. |
| 5 | ADC | Family-specific peripheral/channel/trigger logic is concentrated and safety-relevant. | Charging/battery monitor first; perimeter later. |
| 6 | I2C bus | Lets IMU drivers become bus-agnostic. | Hardware LIS3DH, then soft-I2C external IMUs. |
| 7 | Persistent storage | Current RTC backup register usage will not port to ESP32. | Charge offset and amp-hour accumulator. |
| 8 | USB / transport | Needed before ESP32-S3/P4; separate Mowgli protocol from USB CDC. | `mowgli_comms`, `usbd_cdc_if`, ROS/custom serial bridge. |
| 9 | Watchdog | STM32 IWDG/WWDG and ESP task watchdog differ significantly. | Main loop watchdog refresh. |
| 10 | CAN / FDCAN / TWAI | Not central today, but required for future controller variants. | Future motor/controller buses. |

## Suggested migration rule

Keep `board.h` as the source of truth during the first migration, but move hardware facts into structured board descriptors incrementally. Each PR should preserve the current F103/F401 binaries' behavior and only redirect one peripheral class through a thin HAL wrapper.
