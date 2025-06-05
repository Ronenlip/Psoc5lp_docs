#include <project.h> // Include necessary headers for PSoC or your microcontroller
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// volatile Global variables
volatile int push = 0; // In Used
volatile int option = 0; //In Used: 0 -freq , 1- amp
volatile int AmpValue=0; //In Used
volatile float GloabalFreq = 110; //In Used
volatile float GloabalAmp = 1.0; //In Used
volatile int Scale=1; //In Used
volatile int ch=1;//In Used
volatile int8 aState;//In Used
volatile int8 aLastState;//In Used
volatile int LastValueFreq=0;//In Used
volatile int LastValueAmp=0;//In Used

//Global variables
float correctionFactor = 1.0345; //In Used
float FreqCorrectionFactor = 1.191; //In Used
int Value=0; //In Used
int temp; //In Used
int p=0; //In Used
uint8 waveform[256];//In Used


// Function prototypes
uint8_t readDebouncedPin(uint8_t (*readFunc)(void), uint16_t debounceDelay);
void main_FSM(int ch);
void control_FSM(int ch);
int control_scale(int ch);

//Interrupts:

CY_ISR(rotery_Handler) {
    aState = readDebouncedPin(clk_Read, 5); // Read the current state of the CLK pin with debounce
    if (aState != aLastState) { // Check if the state has changed
        if (readDebouncedPin(TD_Read, 5) != aState) { // If the direction pin state is different from the CLK pin state
            
            ch=ch+Scale; // Increment the counter for clockwise rotation
            
        } else {
            if (!(option == 1 && LastValueAmp + ch <= 0)) {
                ch -= Scale; // Decrement the counter for counterclockwise rotation
            }
            
        }
        aLastState = aState; // Update the last state to the current state
    }
    clk_ClearInterrupt(); // Clear the interrupt flag for the CLK pin
    clk_isr_ClearPending(); // Clear the pending interrupt for the rotary encoder
}

CY_ISR(SW_Handler) {
    if (push==1 && option==1){
        while(p<256){             //offset=1/2
            waveform[p] = (uint8)(124 + (AmpValue*correctionFactor) * sin(2 * M_PI * p / 256.0));  // Generate a sine wave with 0.2Vpp around 2.08V
            p++;
            if (p==256){

                WaveDAC8_Wave1Setup(waveform, sizeof(waveform));// Set the waveform to the WaveDAC
                
            }     
        }
      p=0; 
      Counter_WritePeriod((int)(10E+6/(GloabalFreq*256)*FreqCorrectionFactor)); 
    }
    else if(push==1 && option==0){
        Counter_WritePeriod(Value);
    }
    else if(push==1 && option==2){Scale=temp;}
    CyDelay(30);
    //LCD_I2C_clear();
    push++; // Set the push flag to indicate the button was pressed
    push%=2;
    ch=0;
}

/*----------------------------Main Code------------------------------------------------*/

int main(void) {
    CyGlobalIntEnable; // Enable global interrupts if required by your MCU
    Opamp_1_Start();
    WaveDAC8_Start();
    I2C_Start();
    LCD_I2C_start();
    LCD_I2C_clear();
    LCD_I2C_setCursor(0,0);
    LCD_I2C_print("SineWave Ctrl");
    Counter_Start();
    aLastState = readDebouncedPin(clk_Read, 5); // Read initial state of the CLK pin with debounce
    clk_isr_StartEx(rotery_Handler); // Start the ISR for the rotary encoder
    SW_int_StartEx(SW_Handler); // Start the ISR for the button
    

    for (;;) {
        if (push==0){main_FSM(ch);}
        else if(push==1 && option == 2){temp = control_scale(ch);}
        else if(push==1){control_FSM(ch);}
        
       
    }
}


/*----------------------------------Function------------------------------*/

uint8_t readDebouncedPin(uint8_t (*readFunc)(void), uint16_t debounceDelay) {
    //debouce for encoder
    
    uint8_t state = readFunc();
    CyDelayUs(debounceDelay); // Delay for debouncing
    if (state == readFunc()) {
        return state;
    }
    return !state;
}

void main_FSM(int ch){
    /*
    Main screen
    */
    char Scale_str[20];
    char Freq_str[20];
    char Amp_str[20];
    
    int state=ch%3;
    if (state==0){
        LCD_I2C_setCursor(0,1);
        sprintf(Freq_str,">>Change Freq: %.1f",GloabalFreq);
        LCD_I2C_print(Freq_str);
        LCD_I2C_setCursor(0,2);
        sprintf(Amp_str,"  Change AMP: %.1f",GloabalAmp);
        LCD_I2C_print(Amp_str);
        LCD_I2C_setCursor(0,3);
        sprintf(Scale_str,"  Scale:  %d",Scale);
        LCD_I2C_print(Scale_str);
        option=0;  //freq
    }
    else if(state==1){
        LCD_I2C_setCursor(0,1);
        sprintf(Freq_str,"  Change Freq: %.1f",GloabalFreq);
        LCD_I2C_print(Freq_str);
        LCD_I2C_setCursor(0,2);
        sprintf(Amp_str,">>Change AMP: %.1f",GloabalAmp);
        LCD_I2C_print(Amp_str);
        LCD_I2C_setCursor(0,3);
        sprintf(Scale_str,"  Scale:  %d",Scale);
        LCD_I2C_print(Scale_str);
        option=1;  //amp
    }
    else{
        LCD_I2C_setCursor(0,1);
        sprintf(Freq_str,"  Change Freq: %.1f",GloabalFreq);
        LCD_I2C_print(Freq_str);;
        LCD_I2C_setCursor(0,2);
        sprintf(Amp_str,"  Change AMP: %.1f",GloabalAmp);
        LCD_I2C_print(Amp_str);
        LCD_I2C_setCursor(0,3);
        sprintf(Scale_str,">>Scale:  %d",Scale);
        LCD_I2C_print(Scale_str);
        option=2;  //scale
    }
    if (push == 1){LCD_I2C_clear();}
    return;
}

void control_FSM(int ch){
    char str[20];
    float y;
    if (option==0 && LastValueFreq + ch>-1000){
        int k = LastValueFreq + ch;
        Value=k+1000;  //start freq is 100HZ
        y=10.0E+6/(256*Value)*1.1915; // Counter_clk/(samples*clk_divider)
        GloabalFreq = y;
        sprintf(str,"Freq: %.1f [HZ]",y);
        LCD_I2C_setCursor(0,1);
        LCD_I2C_print(str); 
        
    }
    else{
        int k = LastValueAmp + ch;
        if (k>0){
            AmpValue=k;
            y=4.08/256*k;
            GloabalAmp = y;
            sprintf(str,"AMP: %.3f [V]",y);
            LCD_I2C_setCursor(0,1);
            LCD_I2C_print(str); 
        }
        else{
           AmpValue=0; 
           GloabalAmp = 0;
           sprintf(str,"AMP: %.3f [V]",0.000);
           LCD_I2C_setCursor(0,1);
           LCD_I2C_print(str); 
        }
    }
    if (push == 0 && option==0 ){LCD_I2C_clear(); LastValueFreq  = LastValueFreq + ch;;}
    if (push == 0 && option==1 ){LCD_I2C_clear(); LastValueAmp = LastValueAmp + ch;}
}

int control_scale(int ch){
    int Op= ch%3;
    if(Op==0){
        LCD_I2C_setCursor(0,1);
        LCD_I2C_print(">>Scale = 1");
        LCD_I2C_setCursor(0,2);
        LCD_I2C_print("  Scale = 5"); 
        LCD_I2C_setCursor(0,3);
        LCD_I2C_print("  Scale = 10");
        Op = 1;
        
    }
    else if(Op==1){
        LCD_I2C_setCursor(0,1);
        LCD_I2C_print("  Scale = 1");
        LCD_I2C_setCursor(0,2);
        LCD_I2C_print(">>Scale = 5"); 
        LCD_I2C_setCursor(0,3);
        LCD_I2C_print("  Scale = 10");
        Op = 5;
        
    }
    else if(Op==2){
        LCD_I2C_setCursor(0,1);
        LCD_I2C_print("  Scale = 1");
        LCD_I2C_setCursor(0,2);
        LCD_I2C_print("  Scale = 5"); 
        LCD_I2C_setCursor(0,3);
        LCD_I2C_print(">>Scale = 10");
        Op = 10;
        
    }
    return Op;
}
