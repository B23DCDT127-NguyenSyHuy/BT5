#include <stdint.h>
extern uint32_t _sdata, _edata, _sbss, _ebss, _sidata;
extern int main(void);
extern void SVC_Handler(void);
extern void PendSV_Handler(void);
extern void SysTick_Handler(void);
extern void EXTI0_IRQHandler(void); // Khai báo ngắt ngoài PB0

void Default_Handler(void) { while(1); }

void Reset_Handler(void) {
    uint32_t *pSrc = &_sidata;
    uint32_t *pDest = &_sdata;
    while(pDest < &_edata) { *pDest++ = *pSrc++; }
    pDest = &_sbss;
    while(pDest < &_ebss) { *pDest++ = 0; }
    main();
    while(1);
}

__attribute__((used, section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))0x20005000, Reset_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, 0, 0, 0, 0,
    SVC_Handler, Default_Handler, 0, PendSV_Handler, SysTick_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, EXTI0_IRQHandler /* IRQ6 cho EXTI0 */
};
