#include <stdint.h>
#include <stddef.h>


/*  sample value and index buffer declaration*/
typedef struct  __attribute__((packed))
{
    int16_t prevsample;
    int8_t previndex;

}S_STATE;
S_STATE adpcm_state;


/* adpcm table declaration*/
int8_t IndexTable[] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
int16_t StepSizeTable[] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

/* function declaration */
void init_adpcm(void);
void ADPCMDecoder(uint8_t * p_ui8_inputsample,uint16_t * p_ui16_outsample,uint16_t prevSample, uint8_t buffer_size);

/* adpcm initialization*/
void init_adpcm(void)
{
    adpcm_state.prevsample = 0;
    adpcm_state.previndex = 0;;
}

/* ADPCM decoder algorithm */
void ADPCMDecoder(uint8_t * p_ui8_inputsample,uint16_t * p_ui16_outsample,uint16_t prevSample, uint8_t buffer_size)
{
    int8_t  index = 0,code,int8_evensample =0;
    uint8_t  ui8_count = 0,ui8_count_codeindex = 0,uint8_codeTemp =0;
    int16_t  predsample,step,diffq ;

    // scaled frame sample value from 8 bits to 12 bits
    adpcm_state.prevsample = ((int16_t)(prevSample & 0x00FF))<<4;

    // frame index value for initial step size
    adpcm_state.previndex = (prevSample >>8) & 0xff;

    // looping up to 100 samples
    for(ui8_count = 0; ui8_count < buffer_size ; ui8_count ++)
    {
        /* previous sample and step size calculation */
        predsample = adpcm_state.prevsample;
        index = adpcm_state.previndex;
        step = StepSizeTable[index];

        /* current sample code extraction from lower and upper nibble*/
        int8_evensample++;
       if(int8_evensample >= 2)
       {
           int8_evensample = 0;
           code = (uint8_codeTemp>>4) & 0x0f;
           ui8_count_codeindex++;
        }
        else
        {
            uint8_codeTemp = p_ui8_inputsample[ui8_count_codeindex];
            code = (uint8_codeTemp & 0x0f);

        }

       /*  De-quantization of calculated step size*/
        diffq = step >> 3;
        if(code & 4)
             diffq += step;
        if(code & 2)
             diffq += step >> 1;
        if(code & 1)
             diffq += step >> 2;

        /*  calculate original sample from quantization sample code and calculated step size*/
        if(code & 8)
             predsample -= diffq;
        else
             predsample += diffq;

        /* Precaution in case of higher sample limit*/
        if(predsample > 32767)
             predsample = 32767;
        else if(predsample < -32768)
             predsample = -32768;

        /* calculate new index*/
        index += IndexTable[code];

        /* Precaution in case of higher index limit*/
        if(index < 0)
             index = 0;
        if(index > 88)
             index = 88;


        /*  buffer decoded sample in array*/
        if(predsample & 0x8000 )
        {
            p_ui16_outsample[ui8_count] = 0;// Limit for negative sample
            predsample = 0;
            index = 0;
        }
        else if(predsample>4025)
        {
            p_ui16_outsample[ui8_count] = 4026;// Limit for higher sample
            predsample = 4026;
            index = 66;
        }
        else
        {
            p_ui16_outsample[ui8_count] = predsample;// Buffer the sample value

        }

        /* pass calculated prev sample and index for further new sample calculation */
        adpcm_state.prevsample = predsample;
        adpcm_state.previndex = index;
    }
}



