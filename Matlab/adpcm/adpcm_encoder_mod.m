function [r_array,adpcm_y] = adpcm_encoder(raw_y,fsmaple)

% This m-file is based on the app note: AN643, Adaptive differential pulse
% code modulation using PICmicro microcontrollers, Microchip Technology
% Inc. The app note is avaialbe from www.microchip.com
% Example:  Y = wavread('test.wav');
%           y = adpcm_encoder(Y);
%           YY = adpcm_decode(y);

IndexTable = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8];

%IndexTable = [-3, -3, -3, -3, 8, 8, 8, 8, -3, -3, -3, -3, 8, 8, 8, 8];         
StepSizeTable = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767];

prevsample = fsmaple;
previndex = 1;
Ns = length(raw_y);
n = 1;
%raw_y = 32767 * raw_y;          % 16-bit operation
r_array = zeros(11,Ns);

%%%%%%%%%%%%%%%%%%%%%
dn  = 0;
gama   =1;
In =0;
code  = In;
%%%%%%%%%%%%%%%%%%%%%

while (n <= Ns)
    
  %%%%%%%%%%%%%%%%%%%%%%%%%  
    In = code;
    if((In+0.5 ) >= 4.5)
        m_In = 8;
    else
        m_In = -3;
    end
    dn = dn + m_In;
    del = exp(dn);
    del_inv = 1/del   ;
    %%%%%%%%%%%%%%%%%%%%%%%%%
    
    predsample = prevsample;
    index = previndex;
    step = StepSizeTable(index);
   % step = int16(del_inv);
    
    diff = raw_y(n) - predsample;
    
    r_array(1,n) = raw_y(n);
    r_array(2,n) = index;
    r_array(3,n) = step;
    r_array(4,n) = diff;
    
    if (diff >= 0)
        code = 0;
    else
        code = 8;
        diff = -diff;
    end

    tempstep = (step);
    if (diff >= tempstep)
        code = bitor(code, 4);
        diff = diff - tempstep;
    end
    tempstep = bitshift(tempstep, -1);
    if (diff >= tempstep)
        code = bitor(code, 2);
        diff = diff - tempstep;
    end
    tempstep = bitshift(tempstep, -1);
    if (diff >= tempstep)
        code = bitor(code, 1);
    end

    diffq = bitshift(step, -3);
    if (bitand(code, 4))
        diffq = diffq + step;
    end
    if (bitand(code, 2))
        diffq = diffq + bitshift(step, -1);
    end
    if (bitand(code, 1))
        diffq = diffq + bitshift(step, -2);
    end

    if (bitand(code, 8))
        predsample = predsample - diffq;
        r_array(5,n) = diffq*(-1);
    else
        predsample = predsample + diffq;
        r_array(5,n) = diffq;
    end

    if (predsample > (32767))%32767
        predsample = 32767;
    elseif (predsample < -32768)%32768
        predsample = -32768;
    end

   r_array(6,n) = predsample;
       
    index = index + IndexTable(code+1);

    r_array(7,n) = index;
    
    if (index < 1)
        index = 1;
    end
    if (index > 89)%89
        index = 89;
    end

    prevsample = predsample;
    previndex = index;

    adpcm_y(n) = bitand(code, 15);
    
    r_array(8,n) = prevsample;
    r_array(9,n) = previndex;
    r_array(10,n) = adpcm_y(n);
    %adpcm_y(n) = code;
    n = n + 1;
end