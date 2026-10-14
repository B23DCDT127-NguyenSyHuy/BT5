#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n) { char *d = dest; const char *s = src; while (n--) *d++ = *s++; return dest; }
void *memset(void *s, int c, size_t n) { unsigned char *p = s; while (n--) *p++ = (unsigned char)c; return s; }

#define SCB_VTOR     (*((volatile uint32_t *)0xE000ED08))
#define RCC_APB2ENR  (*((volatile uint32_t *)0x40021018))
#define RCC_APB1ENR  (*((volatile uint32_t *)0x4002101C))

#define GPIOA_CRL    (*((volatile uint32_t *)0x40010800))
#define GPIOB_CRH    (*((volatile uint32_t *)0x40010C04))
#define GPIOB_IDR    (*((volatile uint32_t *)0x40010C08))
#define GPIOB_ODR    (*((volatile uint32_t *)0x40010C0C))

// TIM2 (PWM)
#define TIM2_CR1     (*((volatile uint32_t *)0x40000000))
#define TIM2_CCMR1   (*((volatile uint32_t *)0x40000018))
#define TIM2_CCMR2   (*((volatile uint32_t *)0x4000001C))
#define TIM2_CCER    (*((volatile uint32_t *)0x40000020))
#define TIM2_PSC     (*((volatile uint32_t *)0x40000028))
#define TIM2_ARR     (*((volatile uint32_t *)0x4000002C))
#define TIM2_CCR1    (*((volatile uint32_t *)0x40000034))
#define TIM2_CCR2    (*((volatile uint32_t *)0x40000038))
#define TIM2_CCR3    (*((volatile uint32_t *)0x4000003C))

QueueHandle_t xQueueLED[3];

void Hardware_Init(void) {
    SCB_VTOR = 0x08000000;
    
    // 1. Cấp xung nhịp cho GPIOA, GPIOB, AFIO và TIM2
    RCC_APB2ENR |= (1 << 0) | (1 << 2) | (1 << 3); // Bit 0 là AFIO, Bit 2 là GPIOA, Bit 3 là GPIOB
    RCC_APB1ENR |= (1 << 0); // TIM2 clock

    // 2. PA0, PA1, PA2: Alternate Function Push-Pull (Chế độ 0x0B cho tốc độ 50MHz, AF PP)
    GPIOA_CRL &= ~(0xFFF << 0);
    GPIOA_CRL |=  (0xBBB << 0);

    // 3. PB12, PB13, PB14: Input Pull-up cho nút nhấn
    GPIOB_CRH &= ~((0xF << 16) | (0xF << 20) | (0xF << 24));
    GPIOB_CRH |=  ((0x8 << 16) | (0x8 << 20) | (0x8 << 24));
    GPIOB_ODR |=  (1 << 12) | (1 << 13) | (1 << 14);

    // 4. Cấu hình TIM2 tạo PWM 1kHz
    TIM2_PSC = 8 - 1;       
    TIM2_ARR = 1000 - 1;    
    
    // PWM Mode 1 cho Channel 1, 2, 3
    TIM2_CCMR1 = (0x6 << 4) | (0x6 << 12);
    TIM2_CCMR2 = (0x6 << 4);
    
    // Kích hoạt ngõ ra PWM
    TIM2_CCER = (1 << 0) | (1 << 4) | (1 << 8);
    TIM2_CCR1 = 200; TIM2_CCR2 = 500; TIM2_CCR3 = 800; // Đặt độ sáng mẫu ban đầu
    
    TIM2_CR1 |= (1 << 0); // Bật Timer
}

// Task nhận dữ liệu từ Queue để đổi độ sáng LED
void vTask_LED(void *pvParameters) {
    uint32_t led_id = (uint32_t)pvParameters;
    uint16_t duty;
    while(1) {
        if (xQueueReceive(xQueueLED[led_id], &duty, portMAX_DELAY) == pdTRUE) {
            if (led_id == 0) TIM2_CCR1 = duty;
            else if (led_id == 1) TIM2_CCR2 = duty;
            else if (led_id == 2) TIM2_CCR3 = duty;
        }
    }
}

// Task đọc 3 nút nhấn ở PB12, PB13, PB14
void vTask_Button(void *pvParameters) {
    uint8_t target_led = 0;
    uint16_t duty[3] = {200, 500, 800};
    uint8_t last_b1 = 1, last_b2 = 1, last_b3 = 1;

    while(1) {
        uint8_t b1 = (GPIOB_IDR & (1 << 12)) ? 1 : 0;
        uint8_t b2 = (GPIOB_IDR & (1 << 13)) ? 1 : 0;
        uint8_t b3 = (GPIOB_IDR & (1 << 14)) ? 1 : 0;

        // Nút PB12: Chuyển đổi LED mục tiêu điều khiển
        if (b1 == 0 && last_b1 == 1) {
            target_led = (target_led + 1) % 3;
        }
        // Nút PB13: Tăng độ sáng
        if (b2 == 0 && last_b2 == 1) {
            if (duty[target_led] <= 900) duty[target_led] += 100;
            xQueueSend(xQueueLED[target_led], &duty[target_led], 0);
        }
        // Nút PB14: Giảm độ sáng
        if (b3 == 0 && last_b3 == 1) {
            if (duty[target_led] >= 100) duty[target_led] -= 100;
            xQueueSend(xQueueLED[target_led], &duty[target_led], 0);
        }

        last_b1 = b1; last_b2 = b2; last_b3 = b3;
        vTaskDelay(40); // Chống dội phím
    }
}

int main(void) {
    Hardware_Init();

    xQueueLED[0] = xQueueCreate(1, sizeof(uint16_t));
    xQueueLED[1] = xQueueCreate(1, sizeof(uint16_t));
    xQueueLED[2] = xQueueCreate(1, sizeof(uint16_t));

    xTaskCreate(vTask_LED, "L1", 128, (void*)0, 1, NULL);
    xTaskCreate(vTask_LED, "L2", 128, (void*)1, 1, NULL);
    xTaskCreate(vTask_LED, "L3", 128, (void*)2, 1, NULL);
    xTaskCreate(vTask_Button, "BTN", 128, NULL, 1, NULL);

    vTaskStartScheduler();
    while(1) {}
    return 0;
}
