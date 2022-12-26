#ifndef HMC5883_H_
#define HMC5883_H_

/*--- Includes -------------------------------------*/

    #include <stdint.h>
    #include "soft_i2c.h"
    
typedef struct 
{
    int32_t    X_summ;
    int32_t    Y_summ;    
    int32_t    Z_summ;
    
    float    X;
    float    Y;    
    float    Z;
    
    uint32_t    cnt;
    uint32_t    max;
}
Avrg_t;
    
typedef struct
{
    Soft_I2C_t *   i2c;
    uint8_t XYZ[6];
    int16_t X;
    int16_t Z;
    int16_t Y;
    
    Avrg_t  XYZ_Avrg;
    
    float X_uT;
    float Z_uT;
    float Y_uT;
    float Field_uT;
    float heading;
    uint32_t  i2c_err;
}
HMC5883_t;

extern HMC5883_t  HMC5883_1;
extern HMC5883_t  HMC5883_2;
extern HMC5883_t  HMC5883_3;
    
/*--- Global Prototypes ------------------------*/

uint8_t HMC5883_Init(HMC5883_t * sens);
uint8_t HMC5883_ReadXYZ(HMC5883_t * sens);
uint32_t HMC5883_TestI2CStability(HMC5883_t * sens);

//-------------------------------------------------------
#endif /* HMC5883_H_ */    
