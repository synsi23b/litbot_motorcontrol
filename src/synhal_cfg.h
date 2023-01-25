#pragma once

#define SYN_SYSTICK_HERTZ 1000

#define SYN_OS_PRIO_LOW    50
#define SYN_OS_PRIO_NORMAL 100
#define SYN_OS_PRIO_HIGH   150

#define SYN_ENABLE_ADC        0
#define SYN_ENABLE_I2C_1      0 // or 0 for off, 100 for slow, 400 for fast mode
#define SYN_ENABLE_I2C_2      0 // or 0 for off, 100 for slow, 400 for fast mode
#define SYN_ENABLE_UART_1     0
#define SYN_ENABLE_UART_2     0
#define SYN_ENABLE_UART_3     0
#define SYN_ENABLE_SPI_1      0
#define SYN_ENABLE_SPI_2      0
#define SYN_ENABLE_TIMER_IRQ  1
#define SYN_ENABLE_USBRPC     1
#define SYN_ENABLE_EEPROM     0

#define SYN_USBRPC_BUFFSIZE   10 // ammount of packets
#define SYN_USBRPC_USELED     1
#define SYN_USBRPC_STACKSIZE  512
#define SYN_USBRPC_PRIORITY   SYN_OS_PRIO_HIGH

#define SYN_I2C_1_REMAP     0
#define SYN_USART_1_REMAP   0
#define SYN_CAN_1_REMAP     0

// define irq types
// 0 -> off
// 1 -> calling os functions
// 2 -> zero latency
#define SYN_TIMER_1_IRQ_TYPE 2
#define SYN_TIMER_2_IRQ_TYPE 2
#define SYN_TIMER_3_IRQ_TYPE 2
#define SYN_TIMER_4_IRQ_TYPE 2
#define SYN_TIMER_5_IRQ_TYPE 2

#define SYN_EXTI_TYPE_0   0
#define SYN_EXTI_TYPE_1   0
#define SYN_EXTI_TYPE_2   0
#define SYN_EXTI_TYPE_3   0
#define SYN_EXTI_TYPE_4   0
#define SYN_EXTI_TYPE_5   0
#define SYN_EXTI_TYPE_6   0
#define SYN_EXTI_TYPE_7   0
#define SYN_EXTI_TYPE_8   0
#define SYN_EXTI_TYPE_9   0
#define SYN_EXTI_TYPE_10  0
#define SYN_EXTI_TYPE_11  0
#define SYN_EXTI_TYPE_12  0
#define SYN_EXTI_TYPE_13  0
#define SYN_EXTI_TYPE_14  0
#define SYN_EXTI_TYPE_15  0

#define SYN_DMA_IRQ_TYPE_0 0
#define SYN_DMA_IRQ_TYPE_1 0
#define SYN_DMA_IRQ_TYPE_2 0
#define SYN_DMA_IRQ_TYPE_3 0
#define SYN_DMA_IRQ_TYPE_4 0
#define SYN_DMA_IRQ_TYPE_5 0
#define SYN_DMA_IRQ_TYPE_6 0
#define SYN_DMA_IRQ_TYPE_7 0

// these are actually DMA_2 interrupts
#define SYN_DMA_IRQ_TYPE_8  0
#define SYN_DMA_IRQ_TYPE_9  0
#define SYN_DMA_IRQ_TYPE_10 0
#define SYN_DMA_IRQ_TYPE_11 0
#define SYN_DMA_IRQ_TYPE_12 0
#define SYN_DMA_IRQ_TYPE_13 0
#define SYN_DMA_IRQ_TYPE_14 0
#define SYN_DMA_IRQ_TYPE_15 0
