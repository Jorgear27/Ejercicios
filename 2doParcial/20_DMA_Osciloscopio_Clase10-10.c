/*
Implementar un osciloscopio básico para audio frecuencia que soporte entradas de -1.5 a +1.5V con el 
LPC1769 utilizando DMA para transferir datos del ADC a memoria (P2M). Calcular el periodo y valor
maximo de la señal. Suponer que la señal tiene offset para tener rango de 0 a 3 V
*/

#include "LPC17xx.h"
#include "LPC17xx_pinsel.h"
#include "LPC17xx_adc.h"
#include "LPC17xx_gpdma.h"

#define RATE 100000
#define BUFFER_SIZE 1024

volatile bool flag = false;
volatile uint16_t buffer[BUFFER_SIZE];

void configADC(){
    LPC_SC->PCONP |= (1<<12); // Encender el periferico de ADC
    LPC_SC->PCLKSEL0 &= ~(3<<24); // PCLK_ADC = CCLK/4+

    // Configurar el pin de ADC P0.23
    PINSEL_CFG_Type pincfg;
    pincfg.Portnum = 0;
    pincfg.Pinnum = 23;
    pincfg.Funcnum = 1;
    pincfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    pincfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pincfg);

    // Configuramos el ADC con una frecuencia de muestreo de 100kHz
    ADC_Init(LPC_ADC0, RATE);
    
    // Habilitamos el modo burst
    ADC_BurstCmd(LPC_ADC0, ENABLE);

    // Habilitamos para que convierta permanentemente
    ADC_StartCmd(LPC_ADC0, ADC_START_CONTINUOS);  
    
    // habilitamos la conversion del canal 0
    ADC_ChannelCmd(LPC_ADC0, 0, ENABLE);
}

void configDMA(){
    // habilitamos el clock del DMA y reiniciamos todas las configuraciones
    GPDMA_Init(LPC_GPDMACH0);

    GPDMA_Channel_CFG_Type GPDMACfg;
    GPDMACfg.ChannelNum = 0; //Elegimos Canal 0
    GPDMACfg.TransferSize = BUFFER_SIZE; // Tamaño de la transferencia
    GPDMACfg.TransferWidth = 0; // No se selecciona en transferencias P2M
    GPDMACfg.SrcMemAddr = 0; // No se selecciona en transferencias P2M
    GPDMACfg.DstMemAddr = (uint32_t)buffer; // Direccion de memoria del buffer
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M; // Transferencia de periferico a memoria
    GPDMACfg.SrcConn = GPDMA_CONN_ADC; // Conexion del periferico
    GPDMACfg.DstConn = 0; // No se selecciona en transferencias P2M
    GPDMACfg.DMALLI = 0; // No necesitamos usar LLI
    GPDMA_Setup(&GPDMACfg);

    // Habilitamos la transferencia del canal 0 y la interrupcion de transferencia completa
    GPDMA_ChannelCmd(0, ENABLE);
}

void DMA_IRQHandler(){
    flag = true; //habilitamos el procesamiento de los datos
    GPDMA_ClearIntPending(GPDMA_STAT_INTTC, 0); // limpiamos bandera
    LPC_GPDMACH0->DMACCDestAddr = (uint32_t)buffer ;//volvemos a llenar el buffer
    GPMA_ChannelCmd(0, ENABLE);
}

int main(){
    configADC();
    configDMA();

    while(1){
        // Esperamos a que se complete la transferencia
        if(flag){
            flag = false;
            // Calculamos el valor maximo y minimo de la señal
            uint16_t max = 0;
            uint16_t min = 4095;
            uint16_t i;
            for(i = 0; i < BUFFER_SIZE; i++){
                if(buffer[i] > max){
                    max = buffer[i];
                }
                if(buffer[i] < min){
                    min = buffer[i];
                }
            }

            // Calculamos el periodo de la señal
            uint16_t periodo = 0;
            uint16_t primer_cruce = 0;
            uint16_t segundo_cruce = 0;
            uint8_t cruces = 0;
            for(i = 0; i < BUFFER_SIZE; i++){
                if (i>0){
                    if (buffer[i-1] < (max + min) / 2 && buffer[i] >= (max + min) / 2 ||
                        buffer[i-1] > (max + min) / 2 && buffer[i] <= (max + min) / 2){// Detectamos los cruces por cero 
                        if (cruces==0){ //Si es el primer cruce
                            primer_cruce = i;
                            cruces++;
                        } else {//Si es el segundo cruce
                            segundo_cruce = i;
                            cruces=0;
                            periodo = segundo_cruce - primer_cruce;
                        }
                    }
                }
            }
        }
        
        
    }
}