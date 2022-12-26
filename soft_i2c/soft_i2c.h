/**
  *****************************************************************************
  @file     soft_i2c.h
  @author   savver (Anton Sap., savver.engineer@gmail.com)
  @version  0.1.
  @date     16.12.2022
  @brief    Software implementation of the I2C interface 
            using GPIO in Open-Drain mode. You can create 
            multiple interfaces
  *****************************************************************************
 */

#ifndef SOFT_I2C_H_
#define SOFT_I2C_H_

/*--- Includes --------------------------------------------------------------*/

    #include <stdint.h>
    #include "stm32f10x_conf.h"
   
/*--- User Config -----------------------------------------------------------*/

    #define SWI2C_1US_NUMBER_TACKTS   20  
    
/*--- Defines, Types --------------------------------------------------------*/

typedef void (*GPIO_RCC_CLK)(uint32_t RCC_APB2Periph, FunctionalState NewState);

typedef struct
{
    GPIO_TypeDef* SDA_Port;
    uint16_t      SDA_Pin;
    GPIO_RCC_CLK  SDA_ClockFunc;
    uint32_t      SDA_Clock;
    
    GPIO_TypeDef* SCL_Port;
    uint16_t      SCL_Pin;
    GPIO_RCC_CLK  SCL_ClockFunc;
    uint32_t      SCL_Clock;
    
    uint32_t      Delay;
}
Soft_I2C_t;

/*--- Extern Variables ------------------------------------------------------*/

    extern Soft_I2C_t  Soft_I2C_1;
    extern Soft_I2C_t  Soft_I2C_2;
    extern Soft_I2C_t  Soft_I2C_3;
    
/*--- Global Prototypes -----------------------------------------------------*/

    void SWI2C_Init(Soft_I2C_t * i2c);
    void SWI2C_ToggleTest(Soft_I2C_t * i2c, uint32_t  cnt, uint32_t delay_us);

    void SWI2C_Start(Soft_I2C_t * i2c);
    void SWI2C_Stop(Soft_I2C_t * i2c);
    void SWI2C_ReStart(Soft_I2C_t * i2c);

    uint8_t SWI2C_WriteByte(Soft_I2C_t * i2c, uint8_t data);
    uint8_t SWI2C_ReadByte( Soft_I2C_t * i2c, uint8_t ack);

    uint8_t SWI2C_WriteReg(Soft_I2C_t * i2c, uint8_t   addr, uint8_t  reg_addr, uint8_t  reg_data);
    uint8_t SWI2C_ReadReg( Soft_I2C_t * i2c, uint8_t   addr, uint8_t  reg_addr, uint8_t * err);
    uint8_t SWI2C_ReadBuf( Soft_I2C_t * i2c, uint8_t   addr, uint8_t  reg_addr, uint8_t * data, uint8_t cnt);

//-------------------------------------------------------
#endif /* SOFT_I2C_H_ */    
