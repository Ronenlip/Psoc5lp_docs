#include "project.h"
#include <stdbool.h>
#include <stdio.h>

int8 aState;
int8 aLastState;
int Counter = 0;
char buffer[35];
bool flag = true;

#define DEBOUNCE_THRESHOLD 5000  // Adjust as needed
#define SYSTICK_MAX 16777215     // Default 24-bit counter

CY_ISR(encoder_handler)
{
        aState = TD_Read();
        
        if (aState != aLastState)
        {
            if (clk_Read() != aState)
            {
                Counter++;  // Clockwise rotation
            }
            else
            {
                Counter--;  // Counter-clockwise rotation
            }
        
        }

        aLastState = aState;
        flag =true;
        isr_B_ClearPending();
        TD_ClearInterrupt();
}


int main(void)
{
    CyGlobalIntEnable; // Enable global interrupts

    UART_1_Start(); // Initialize UART

    // Initialize encoder state
    aLastState = TD_Read();
    isr_B_StartEx(encoder_handler);

    for (;;)
    {
        if (flag){
            sprintf(buffer, "Counter value : %d\r\n", Counter);
            UART_1_PutString(buffer); // Blocking transmit
            flag = false;
        }
     
    }
}
