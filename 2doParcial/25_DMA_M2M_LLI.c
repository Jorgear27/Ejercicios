/*
Transferir los datos del buffer 1 y 2 al buffer general
*/

#include "LPC17xx.h"
#include "LPC17xx_gpdma.h"

#define BUFFER_SIZE_1 100
#define BUFFER_SIZE_2 500


uint32_t buffer1[10];
uint32_t buffer2[10];
uint32_t buffer_total[10];

void configDMA(){
    GPDMA_LLI_Type LLI_1;
    GPDMA_LLI_Type LLI_2;

    LLI_1.SrcAddr = (uint32_t)buffer1;
    LLI_1.DestAddr = (uint32_t)buffer_total;
    LLI_1.NextLLI = (uint32_t)&LLI_2;
    LLI_1.Control = BUFFER_SIZE_1 // Tamaño de la transferencia
                    | (0x4 << 18) // Ancho de cada dato de 32 bits
                    | (0x4 << 21) // Destination transfer width
                    | (0x1 << 26) // Incrementamos direccion de origen
                    | (0x1 << 27); // Incrementamos direccion de destino

    LLI_2.SrcAddr = (uint32_t)buffer2;
    LLI_2.DestAddr = (uint32_t)buffer_total+BUFFER_SIZE_1*4; //Empezamos de donde termino el buffer anterior teniendo en cuenta que cada palabra de 32 bits ocupa 4 lugares de memoria en la LPC1769
    LLI_2.NextLLI = 0;
    LLI_2.Control = BUFFER_SIZE_2 // Tamaño de la transferencia
                    | (0x4 << 18) // Ancho de cada dato de 32 bits
                    | (0x4 << 21) // Destination transfer width
                    | (0x1 << 26) // Incrementamos direccion de origen
                    | (0x1 << 27); // Incrementamos direccion de destino

    // Habilitamos clock del DMA y reiniciamos todas las configuraciones
    GPDMA_Init();

    // Configuramos el canal 0
    GPDMA_Channel_CFG_Type GPDMACfg;
    GPDMACfg.ChannelNum = 0; //Elegimos Canal 0
    GPDMACfg.TransferSize = BUFFER_SIZE_1+BUFFER_SIZE_2; // Tamaño de la transferencia
    GPDMACfg.TransferWidth = 32; // Ancho de cada dato de 32 bits
    GPDMACfg.SrcMemAddr = (uint32_t)buffer1; // Direccion de memoria del primer buffer 
    GPDMACfg.DstMemAddr = (uint32_t)buffer_total; // Direccion de memoria del buffer total
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2M; // Transferencia de memoria a memoria
    GPDMACfg.SrcConn = 0; // No se selecciona en transferencias M2M
    GPDMACfg.DstConn = 0; // No se selecciona en transferencias M2M
    GPDMACfg.DMALLI = (uint32_t)&LLI_1; // Empezamos por la primera LLI
    GPDMA_Setup(&GPDMACfg);

    // Habilitamos las interrupciones
    NVIC_EnableIRQ(DMA_IRQn);   

    // Habilitamos la transferencia del canal 0
    GPDMA_ChannelCmd(0, ENABLE);
}

void DMA_IRQHandler(){
    GPDMA_ClearIntPending(GPDMA_STAT_INTTC, 0); // limpiamos bandera
    GPDMA_ChannelCmd(0, DISABLE); // Deshabilitamos el canal 0
}

int main(){
    configDMA();
    while(1);
}