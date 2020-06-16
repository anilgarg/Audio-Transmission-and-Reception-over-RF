#if 1

#include <stdint.h>
#include <stddef.h>

typedef struct  __attribute__((packed))
{
    int16_t prevsample;
    int8_t previndex;
    uint8_t prevsampleRF;
    uint8_t previndexRF;
}S_STATE;
S_STATE adpcm_state;


/* variable and function declaration */
int8_t IndexTable[] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
int16_t StepSizeTable[] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

extern uint16_t int16_prevSampleForRF;
void ADPCMEncoder(uint16_t * p_ui16_inputsample,uint8_t * p_ui8_outsample, int16_t prevsample,uint8_t buffer_size);
void init_adpcm(void);

/**************************************************************************************************/
/* Function Name : init_adpcm                                                  */
/* Description   : initialization of adpcm variables*/

/**************************************************************************************************/
void init_adpcm(void)
{
    adpcm_state.prevsample = 0;
    adpcm_state.previndex=0;
}

/**************************************************************************************************/
/* Function Name : ADPCMEncoder                                                  */
/* Description   : ADPCMEncoder
* initialization and while function */
/* Parameters    :   sample array as input with size                                                      */
/*                                                                                                */
/*                                                                                               */
/* Return        : compressed code value                                                                           */
/*                                                                                                */
/**************************************************************************************************/
void ADPCMEncoder(uint16_t * p_ui16_inputsample,uint8_t * p_ui8_outsample, int16_t prevsample,uint8_t buffer_size)
{
    int8_t  index = 0,code,int8_evensample,int8_lock = 0 ;
    uint8_t  ui8_count = 0,uint8_codeTemp = 0 ,ui8_count_codeindex = 0;
    int16_t  predsample,diff,step,tempstep,diffq,sample;


    int8_evensample = 0; // use for combined upper and lower code nibble in one byte
    for(ui8_count = 0; ui8_count < buffer_size ; ui8_count ++) // loop till 100 samples
    {
        // lock initial index as well as sample value for synchronization with transmitter during decoding
        if(int8_lock == 0)
        {
            int8_lock = 1;
            int16_prevSampleForRF = adpcm_state.previndex ;
            int16_prevSampleForRF = (int16_prevSampleForRF <<8)+ (uint8_t)(adpcm_state.prevsample >>4) ;
        }

        predsample = adpcm_state.prevsample; // buffer previous sampled value
        index = adpcm_state.previndex;      // index for step size
        step = StepSizeTable[index];        // quantization step size

        sample = p_ui16_inputsample[ui8_count];  // buffer current sampled value
        diff =  sample- predsample; // calculate difference between  current and previous sampled value

        /* quantization of difference value as per size defined in lookup table */
        if(diff >= 0)
             code = 0;
        else
        {
             code = 8;
             diff = - diff;
        }
        tempstep = step;
        if(diff >= tempstep)
        {
             code |= 4;
             diff -= tempstep;
        }
        tempstep >>= 1;
        if(diff >= tempstep)
        {
             code |= 2;
             diff -= tempstep;
        }
        tempstep >>= 1;
        if(diff >= tempstep)
             code |= 1;

        /* de-quantization of code as per size defined in lookup table */
        diffq = step >> 3;
        if(code & 4)
             diffq += step;
        if(code & 2)
             diffq += step >> 1;
        if(code & 1)
             diffq += step >> 2;

        /* find the original value from decoded code */
        if(code & 8)
             predsample -= diffq;
        else
             predsample += diffq;
        /* define the maximum and minimum cap */
        if(predsample > 32767)
             predsample = 32767;
        else if(predsample < -32768)
             predsample = -32768;

        /* define the maximum and minimum cap */
        index += IndexTable[code];

        /* define the maximum and minimum cap of index */
        if(index < 0)
             index = 0;
        if(index > 88)
             index = 88;


        if(predsample & 0x8000 ) // minimum value in case of negative
        {
          predsample = 0;
        }
        else if(predsample>4095) // maximum value in case of sample diff value is higher then 12 bits
        {
          predsample = 4095;
        }
        else
        {
           // nothing
        }
        /* buffer current difference sampled value and index  */
        adpcm_state.prevsample = predsample;
        adpcm_state.previndex = index;
        int8_evensample++;

        // use for combined upper and lower code nibble in one byte
       if(int8_evensample >= 2)
       {
           uint8_codeTemp = uint8_codeTemp + ((code & 0x0f)<<4);
           int8_evensample = 0;
           p_ui8_outsample[ui8_count_codeindex] = uint8_codeTemp;
           ui8_count_codeindex++;
        }
        else
        {
            uint8_codeTemp = (code & 0x0f);

        }

    }
}


#endif

