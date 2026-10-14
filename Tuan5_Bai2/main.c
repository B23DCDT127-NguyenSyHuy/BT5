#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n) { char *d = dest; const char *s = src; while (n--) *d++ = *s++; return dest; }
void *memset(void *s, int c, size_t n) { unsigned char *p = s; while (n--) *p++ = (unsigned char)c; return s; }

#define SCB_VTOR        (*((volatile uint32_t *)0xE000ED08))
#define RCC_APB2ENR     (*((volatile uint32_t *)0x40021018))
#define GPIOA_CRH       (*((volatile uint32_t *)0x40010804))
#define GPIOB_CRL       (*((volatile uint32_t *)0x40010C00))
#define GPIOB_ODR       (*((volatile uint32_t *)0x40010C0C))
#define GPIOC_CRH       (*((volatile uint32_t *)0x40011004))
#define GPIOC_ODR       (*((volatile uint32_t *)0x4001100C))

#define AFIO_EXTICR1    (*((volatile uint32_t *)0x40010008))
#define EXTI_IMR        (*((volatile uint32_t *)0x40010400))
#define EXTI_FTSR       (*((volatile uint32_t *)0x4001040C))
#define EXTI_PR         (*((volatile uint32_t *)0x40010414))
#define NVIC_ISER0      (*((volatile uint32_t *)0xE000E100))
#define NVIC_IPR        ((volatile uint8_t *)0xE000E400)

#define USART1_SR       (*((volatile uint32_t *)0x40013800))
#define USART1_DR       (*((volatile uint32_t *)0x40013804))
#define USART1_BRR      (*((volatile uint32_t *)0x40013808))
#define USART1_CR1      (*((volatile uint32_t *)0x4001380C))

QueueHandle_t xButtonQueue;
volatile uint32_t press_count = 0;

void UART1_SendChar(char c) { while (!(USART1_SR & (1 << 7))); USART1_DR = c; }
void UART1_SendString(const char *str) { while (*str) UART1_SendChar(*str++); }
void UART1_SendNumber(uint32_t num) {
    char buf[12]; int i = 0;
    if (num == 0) { UART1_SendChar('0'); return; }
    while (num > 0) { buf[i++] = (num % 10) + '0'; num /= 10; }
    while (i > 0) UART1_SendChar(buf[--i]);
}

void Hardware_Init(void) {
    SCB_VTOR = 0x08000000;
    RCC_APB2ENR |= (1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 14);

    // PC13 làm đèn báo trạng thái nút bấm
    GPIOC_CRH &= ~(0xF << 20); GPIOC_CRH |= (0x3 << 20); GPIOC_ODR |= (1 << 13);
    
    // UART1 TX(PA9), RX(PA10)
    GPIOA_CRH &= ~((0xF << 4) | (0xF << 8));
    GPIOA_CRH |=  ((0xB << 4) | (0x8 << 8));
    USART1_BRR = 0x0341; // 9600 baud với thạch anh 8MHz
    USART1_CR1 = (1 << 13) | (1 << 3) | (1 << 2);

    // PB0 làm Input Pull-up
    GPIOB_CRL &= ~(0xF << 0); GPIOB_CRL |= (0x8 << 0); GPIOB_ODR |= (1 << 0);

    // Ngắt ngoài EXTI0 nối vào PB0
    AFIO_EXTICR1 &= ~(0xF << 0); AFIO_EXTICR1 |= (0x1 << 0);
    EXTI_IMR |= (1 << 0); EXTI_FTSR |= (1 << 0);
    
    // Cấu hình mức ưu tiên ngắt an toàn cho FreeRTOS
    NVIC_IPR[6] = (6 << 4);
    NVIC_ISER0 |= (1 << 6);
}

void EXTI0_IRQHandler(void) {
    if (EXTI_PR & (1 << 0)) {
        EXTI_PR = (1 << 0); // Xóa cờ ngắt
        press_count++;
        uint32_t val = press_count;
        
        GPIOC_ODR ^= (1 << 13); // Đảo trạng thái đèn báo PC13
        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(xButtonQueue, &val, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void vTask_UART(void *pvParameters) {
    uint32_t count;
    UART1_SendString("=== He thong khoi dong: Bai 2 (EXTI + UART) ===\r\n");
    while (1) {
        if (xQueueReceive(xButtonQueue, &count, portMAX_DELAY) == pdTRUE) {
            UART1_SendString("So lan nhan nut: ");
            UART1_SendNumber(count);
            UART1_SendString("\r\n");
        }
    }
}

int main(void) {
    Hardware_Init();
    xButtonQueue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(vTask_UART, "UART", 128, NULL, 2, NULL);
    vTaskStartScheduler();
    while (1) {} return 0;
}
