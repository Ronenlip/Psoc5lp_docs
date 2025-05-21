#include <project.h>
#include <stdbool.h>
#include <stdio.h>


volatile uint8_t latest_adc_value = 0;
volatile uint8_t adc_ready_flag = 0;
char *uart_ptr;
volatile bool uart_busy = false;
char buffer[35];
uint8 DMA_1_Chan;
uint8 DMA_1_TD[1];
volatile uint8 _adcResult;  // Make sure it's 'volatile'


void dma_config();

uint8_t linearize_adc(uint8_t measured_adc)
{
    const uint8_t adc_min = 0;   // ADC reading at DAC = 10
    const uint8_t adc_max = 210;  // ADC reading at DAC = 255
    const uint8_t dac_min = 1;
    const uint8_t dac_max = 255;

    // Clamp input to valid range
    if (measured_adc <= adc_min) return dac_min;
    if (measured_adc >= adc_max) return dac_max;

    return (uint16_t)(measured_adc - adc_min) * (dac_max - dac_min) / (adc_max - adc_min) + dac_min;
}


void Start_UART_Transmit(char *str) {
    if (!uart_busy) {
        uart_ptr = str;
        uart_busy = true;
        UART_1_PutChar(*uart_ptr++); // Start first byte
    }
}

CY_ISR(UART_TX_ISR) {
    if (*uart_ptr != '\0') {
        UART_1_PutChar(*uart_ptr++);
    } else {
        uart_busy = false;
    }
    isr_UART_TX_ClearPending();
}

int main(void)
{
    CyGlobalIntEnable; // Enable global interrupts
    
    isr_UART_TX_StartEx(UART_TX_ISR);
    UART_1_Start(); // Initialize UART
    
    VDAC8_1_Start();     // Start VDAC
    ADC_SAR_1_Start();   // Start ADC
    ADC_SAR_1_StartConvert();  // Start continuous conversion
   // ADC_isr_StartEx(ADC_EOC_ISR);
    dma_config();  // <--- THIS was missing
    
    uint8_t dac_val = 133;  // Example DAC value (out of 255)
    VDAC8_1_SetValue(dac_val);  // Output voltage
    CyDelay(10);  // Wait a bit for the voltage to settle

    for (;;)
    {   

        if (Status_Reg_1_Read() & 0x01)  // Bit 0 set → DMA done
        {
            latest_adc_value = linearize_adc(_adcResult);
            sprintf(buffer, "The value measured is: %d\r\n",latest_adc_value);
            Start_UART_Transmit(buffer); // Non-blocking UART send
        }

        CyDelay(200); // Wait before next loop
    }
}
void dma_config()
{
    DMA_1_Chan = DMA_1_DmaInitialize(
        1,  // ✅ 1 byte per burst
        1, 
        HI16(CYDEV_PERIPH_BASE), 
        HI16(CYDEV_SRAM_BASE)
    );

    DMA_1_TD[0] = CyDmaTdAllocate();

    CyDmaTdSetConfiguration(
    DMA_1_TD[0],
    1,
    DMA_1_TD[0],  // ✅ Loop to itself
    DMA_1__TD_TERMOUT_EN
    );

    CyDmaTdSetAddress(
        DMA_1_TD[0], 
        LO16((uint32)ADC_SAR_1_SAR_WRK0_PTR),  // ✅ 8-bit result pointer
        LO16((uint32)&_adcResult)
    );

    CyDmaChSetInitialTd(DMA_1_Chan, DMA_1_TD[0]);
    CyDmaChEnable(DMA_1_Chan, 1);
}
