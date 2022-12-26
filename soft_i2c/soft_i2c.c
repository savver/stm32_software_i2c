/*--- Includes ------------------------------------------------------------*/

    #include "hw_config.h"
    #include "platform_config.h"
    #include "bits_operate.h"
    #include "soft_i2c.h"

/*--- Defines -------------------------------------------------------------*/

    #define SDA_LOW()   i2c->SDA_Port->BRR   = i2c->SDA_Pin
    #define SDA_HIGH()  i2c->SDA_Port->BSRR  = i2c->SDA_Pin
    
    #define SCL_LOW()   i2c->SCL_Port->BRR   = i2c->SCL_Pin
    #define SCL_HIGH()  i2c->SCL_Port->BSRR  = i2c->SCL_Pin
    
    #define SDA_READ()  i2c->SDA_Port->IDR & i2c->SDA_Pin
    
    #define DELAY()     Delay_Us(i2c->Delay)

/*--- Variables -----------------------------------------------------------*/

// PC.2, PC.0
Soft_I2C_t  Soft_I2C_1 =
{
    GPIOC, GPIO_Pin_2, //SDA
    RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOC,
    
    GPIOC, GPIO_Pin_0,  //SCL
    RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOC,
    
    11
};

// PC.3  PC.1
Soft_I2C_t  Soft_I2C_2 =
{
    GPIOC, GPIO_Pin_3, //SDA
    RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOC,
    
    GPIOC, GPIO_Pin_1,  //SCL
    RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOC,
    
    10
};

// PA.3 PA.1
Soft_I2C_t  Soft_I2C_3 =
{
    GPIOA, GPIO_Pin_3, //SDA
    RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOA,
    
    GPIOA, GPIO_Pin_1,  //SCL
    RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOA,
    
    11
};


/*--- Extern functions ----------------------------------------------------*/

/*--- Private Prototypes --------------------------------------------------*/

    static
    void Delay_Us(uint32_t us);

/*=== Functions ===========================================================*/
void SWI2C_Init(Soft_I2C_t * i2c)
{
    GPIO_InitTypeDef    GPIO_InitStruct;
    
    i2c->SCL_ClockFunc(i2c->SCL_Clock, ENABLE);
    i2c->SDA_ClockFunc(i2c->SDA_Clock, ENABLE); 
    
    GPIO_InitStruct.GPIO_Pin         = i2c->SCL_Pin;
    GPIO_InitStruct.GPIO_Mode        = GPIO_Mode_Out_OD;
    GPIO_InitStruct.GPIO_Speed       = GPIO_Speed_10MHz;
    GPIO_Init(i2c->SCL_Port, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin         = i2c->SDA_Pin;
    GPIO_Init(i2c->SDA_Port, &GPIO_InitStruct);   

    SDA_HIGH();
    SCL_HIGH();    
}
/*----------------------------------------------------------------------------*/
void SWI2C_ToggleTest(Soft_I2C_t * i2c, uint32_t  cnt, uint32_t delay_us)
{
    SDA_HIGH();
    SCL_HIGH();
    
    for(uint32_t i = 0; i < cnt; i++)
    {
        SCL_LOW(); SDA_HIGH();
        Delay_Us(delay_us);
        
        SCL_HIGH(); SDA_LOW();
        Delay_Us(delay_us);
    }
    
    SDA_HIGH();
    SCL_HIGH();
}
/*----------------------------------------------------------------------------*/
// SDA --------
//             |___________
// SCL---------------
//                   |_____
//        1       0     0      
//        1       1     0
//
void SWI2C_Start(Soft_I2C_t * i2c)
{
    SDA_HIGH(); SCL_HIGH(); DELAY();
    
    SDA_LOW();  DELAY();
    SCL_LOW();  DELAY();
}
/*----------------------------------------------------------------------------*/
// before this: SCL = 0, if after data read/write
void SWI2C_Stop(Soft_I2C_t * i2c)
{
    SCL_LOW(); DELAY();
    SDA_LOW(); DELAY();
    
    SCL_HIGH(); DELAY();
    SDA_HIGH();  DELAY();
}
/*----------------------------------------------------------------------------*/
void SWI2C_ReStart(Soft_I2C_t * i2c)
{
    SDA_HIGH(); DELAY();
    SCL_HIGH(); DELAY();
    
    SDA_LOW();  DELAY();
    SCL_LOW();  DELAY();
}
/*----------------------------------------------------------------------------*/
// before this, after start: SDA = 0, SCL = 0
//            data                 data                       ack
//  SDA  ++++++++++++++++    ++++++++++++++++      1  --------------------
//               ________             ________                  ________
//  SCL  _______|        |    _______|        |      __________|        |
//         bit      bit          bit      bit
//          0        1   0        0        1  0
//
// after this, before stop: SDA -> ACK, SCL = 0
//
uint8_t SWI2C_WriteByte(Soft_I2C_t * i2c, uint8_t data)
{
    uint8_t     i;
    uint8_t     ack;
    
    for(i = 0; i < 8; i++)
    {
        if(data & 0x80) SDA_HIGH();
        else            SDA_LOW();
        
        DELAY();
        SCL_HIGH(); 
        DELAY(); 
        SCL_LOW();
        
        data = data << 1;
    }
    SDA_HIGH(); //release line for ACK on other sise
    DELAY(); 
    SCL_HIGH();
    DELAY();
    
    ack = SDA_READ();
    SCL_LOW();
    return ack;
}
/*----------------------------------------------------------------------------*/
// before this, after start: SDA = 0, SCL = 0 (after SCL = 0 was delay, so we can set to 1 immediatly)
//
uint8_t SWI2C_ReadByte(Soft_I2C_t * i2c, uint8_t ack)
{
    uint8_t     i;
    uint8_t     data = 0;
    
    SDA_HIGH(); //release line
    
    for(i = 0; i < 8; i++)
    {
        SCL_HIGH();
        DELAY();
        
        data = data << 1;
        if(SDA_READ()) data |= 1;
        SCL_LOW();
        DELAY();
    }
    if(ack) SDA_LOW();
    else    SDA_HIGH();
    DELAY(); 
    
    SCL_HIGH();
    DELAY();
    SCL_LOW();
    
    SDA_HIGH();
    DELAY();
    return data;
}
/*----------------------------------------------------------------------------*/
uint8_t SWI2C_WriteReg(Soft_I2C_t * i2c, uint8_t   addr, uint8_t  reg_addr, uint8_t  reg_data)
{
    uint8_t  err = 0;
    
    SWI2C_Start(i2c);
    err += SWI2C_WriteByte(i2c, addr);
    err += SWI2C_WriteByte(i2c, reg_addr);
    err += SWI2C_WriteByte(i2c, reg_data);
    SWI2C_Stop(i2c);  

    return err;
}
/*----------------------------------------------------------------------------*/
uint8_t SWI2C_ReadReg(Soft_I2C_t * i2c, uint8_t   addr, uint8_t  reg_addr, uint8_t * err)
{
    //*err = 0;
    
    uint8_t data;
    SWI2C_Start(i2c);
    *err += SWI2C_WriteByte(i2c, addr);
    *err += SWI2C_WriteByte(i2c, reg_addr);
    SWI2C_Stop(i2c);
    SWI2C_Start(i2c);
    *err += SWI2C_WriteByte(i2c, addr + 1);
    data = SWI2C_ReadByte(i2c, 0);
    SWI2C_Stop(i2c);
    
    return data;  
}
/*----------------------------------------------------------------------------*/
uint8_t SWI2C_ReadBuf(Soft_I2C_t * i2c, uint8_t   addr, uint8_t  reg_addr, uint8_t * data, uint8_t cnt)
{
    uint8_t err = 0;

    SWI2C_Start(i2c);
    err += SWI2C_WriteByte(i2c, addr);
    err += SWI2C_WriteByte(i2c, reg_addr);
    SWI2C_Stop(i2c);
    //
    SWI2C_Start(i2c);
    err += SWI2C_WriteByte(i2c, addr + 1);
    for(int i = 0; i < cnt; i++)
        data[i] = SWI2C_ReadByte(i2c, (i != cnt-1) ? 1 : 0);
    SWI2C_Stop(i2c);
    
    return err;  
}
/*----------------------------------------------------------------------------*/
static
void Delay_Us(uint32_t us)
{
    volatile uint8_t        sub_us;

    while(us--)
    {
        for(sub_us = 0; sub_us < SWI2C_1US_NUMBER_TACKTS; sub_us++)
        {}
    }
}
