/*--- Includes ------------------------------------------------------------*/

    #include "hmc5883.h"
    #include <string.h>
    #include <math.h>

/*--- Defines -------------------------------------------------------------*/

    // HMC5883l - ADDRESS
    #define HMC5883l_ADDRESS           (0x1E << 1)

    // HMC5883l - MSB / LSB ADDRESSES
    #define HMC5883l_REGADDR_CONF_A    (0x00)
    #define HMC5883l_REGADDR_CONF_B    (0x01)
    #define HMC5883l_REGADDR_MODE      (0x02)
    #define HMC5883l_REGADDR_DATAX_MSB (0x03)
    #define HMC5883l_REGADDR_DATAX_LSB (0x04)
    #define HMC5883l_REGADDR_DATAZ_MSB (0x05)
    #define HMC5883l_REGADDR_DATAZ_LSB (0x06)
    #define HMC5883l_REGADDR_DATAY_MSB (0x07)
    #define HMC5883l_REGADDR_DATAY_LSB (0x08)


//---- CONTROL REG A --------------------
// CRA7  CRA6   CRA5   CRA4     CRA3   CRA2   CRA1   CRA0
// (0)   MA1(0) MA0(0) DO2(1)   DO1(0) DO0(0) MS1(0) MS0 (0)
//  0     1      1      0        0      0      0      0
// MA1 to MA0 - Select number of samples averaged (1 to 8) per measurement output.
// 00 = 1(Default); 01 = 2; 10 = 4; 11 = 8
// DO = 100 -> 15 Hz, 101 -> 30 Hz
// MS = 00 - normal
//
    #define HMC5883l_REGVAL_Conf_A (0x70)

//---- CONTROL REG B -------------------
// CRB7   CRB6   CRB5   CRB4 CRB3 CRB2 CRB1 CRB0
// GN2(0) GN1(0) GN0(1) (0) (0)   (0)  (0)  (0)
//  1      1      0
// 110 ± 5.6 Ga
    #define HMC5883l_REGVAL_Conf_B (0xC0)

//---- MODE REGISTER ---------------
    #define HMC5883l_REGVAL_Mode (0x00)

/*--- Variables -----------------------------------------------------------*/

HMC5883_t  HMC5883_1 = 
{
    &Soft_I2C_1, 0, 0, 0,
};

HMC5883_t  HMC5883_2 = 
{
    &Soft_I2C_2, 0, 0, 0, 0,
};

HMC5883_t  HMC5883_3 = 
{
    &Soft_I2C_3, 0, 0, 0,
};

const float PI = 3.141592653589793;

/*--- Extern functions ----------------------------------------------------*/

/*--- Private Prototypes --------------------------------------------------*/


/*=== Functions ===========================================================*/
uint8_t HMC5883_Init(HMC5883_t * sens)
{
    uint8_t err = 0;
    uint8_t conf_A, conf_B, mode;
    
    SWI2C_Init(sens->i2c);
    SWI2C_Stop(sens->i2c); 
    
    sens->XYZ_Avrg.max = 10;
    
    err += SWI2C_WriteReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_CONF_A, HMC5883l_REGVAL_Conf_A );
    err += SWI2C_WriteReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_CONF_B, HMC5883l_REGVAL_Conf_B );
    err += SWI2C_WriteReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_MODE,   HMC5883l_REGVAL_Mode );    
    
    if(err == 0)
    {
        conf_A = SWI2C_ReadReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_CONF_A, &err );
        conf_B = SWI2C_ReadReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_CONF_B, &err );
        mode   = SWI2C_ReadReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_MODE, &err );
        
        if(err == 0)
        {
            if((conf_A == HMC5883l_REGVAL_Conf_A) &&
               (conf_B == HMC5883l_REGVAL_Conf_B) &&
               (mode == HMC5883l_REGVAL_Mode))
            return 0;
        }
    }
    return 1;
}
/*----------------------------------------------------------------------------*/
uint32_t HMC5883_TestI2CStability(HMC5883_t * sens)
{
    uint8_t  local_i2c_err;
    uint32_t i2c_err = 0;
    uint32_t read_err = 0;
    uint32_t ok = 0;
    uint8_t conf_A, conf_B, mode;

    for(int i = 0; i < 50; i++)
    {
        local_i2c_err = 0;
        conf_A = conf_B = mode = 0;
        conf_A = SWI2C_ReadReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_CONF_A, &local_i2c_err );
        conf_B = SWI2C_ReadReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_CONF_B, &local_i2c_err );
        mode   = SWI2C_ReadReg( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_MODE, &local_i2c_err );
        i2c_err += local_i2c_err;
        
        if((conf_A == HMC5883l_REGVAL_Conf_A) &&
           (conf_B == HMC5883l_REGVAL_Conf_B) &&
           (mode == HMC5883l_REGVAL_Mode))
            ok++;
        else
            read_err++;
    }
    return read_err + i2c_err;
}
/*----------------------------------------------------------------------------*/
uint8_t HMC5883_ReadXYZ(HMC5883_t * sens)
{
    uint8_t err = 0;
    uint16_t val;
#if 1
    err = SWI2C_ReadBuf( sens->i2c, HMC5883l_ADDRESS, HMC5883l_REGADDR_DATAX_MSB,
                         sens->XYZ, 6);
#else   
    uint8_t  start_regaddr = HMC5883l_REGADDR_DATAX_MSB;
    for(int i = 0; i < 6; i++)
        sens->XYZ[i] = SWI2C_ReadReg( sens->i2c, HMC5883l_ADDRESS, start_regaddr + i, &err );
#endif 
    sens->i2c_err += err;
    
    val = (sens->XYZ[0] << 8) | sens->XYZ[1];
    memcpy( &sens->X, &val, 2);
    
    val = (sens->XYZ[2] << 8) | sens->XYZ[3];
    memcpy( &sens->Z, &val, 2);
    
    val = (sens->XYZ[4] << 8) | sens->XYZ[5];
    memcpy( &sens->Y, &val, 2);
    
    sens->XYZ_Avrg.X_summ += sens->X;
    sens->XYZ_Avrg.Y_summ += sens->Y;
    sens->XYZ_Avrg.Z_summ += sens->Z;
    sens->XYZ_Avrg.cnt++;
    
    if(sens->XYZ_Avrg.cnt < sens->XYZ_Avrg.max)
        return err;
    
    sens->XYZ_Avrg.X = 1.0*sens->XYZ_Avrg.X_summ/sens->XYZ_Avrg.max;
    sens->XYZ_Avrg.Y = 1.0*sens->XYZ_Avrg.Y_summ/sens->XYZ_Avrg.max;
    sens->XYZ_Avrg.Z = 1.0*sens->XYZ_Avrg.Z_summ/sens->XYZ_Avrg.max;
    sens->XYZ_Avrg.cnt = 0;
    sens->XYZ_Avrg.X_summ = sens->XYZ_Avrg.Y_summ = sens->XYZ_Avrg.Z_summ = 0;
    
    sens->X_uT = (sens->XYZ_Avrg.X * 3.03f)/10.0;
    sens->Y_uT = (sens->XYZ_Avrg.Y * 3.03f)/10.0;
    sens->Z_uT = (sens->XYZ_Avrg.Z * 3.03f)/10.0;  
    sens->heading = atan2(sens->Y_uT, sens->X_uT);  
    sens->Field_uT = sqrt(sens->X_uT * sens->X_uT + sens->Y_uT * sens->Y_uT + + sens->Z_uT * sens->Z_uT);
    
    // Correct for when signs are reversed.
    if(sens->heading < 0)
		sens->heading += 2*PI;

    // Check for wrap due to addition of declination.
    if(sens->heading > 2*PI)
		sens->heading -= 2*PI;

    // Convert radians to degrees for readability.
    sens->heading = sens->heading * 180/PI; 
    
    return err;
}
