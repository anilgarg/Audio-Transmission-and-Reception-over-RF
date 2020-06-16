clear all
%[Y,Fs] = audioread('test.wav');
%Y = wavread('test.wav');
n  = [1:100];
f= 80;
Fs = 8000;
Ns = Fs/f;
N = 1:100;
Y1 = 2048+ 2048* sin(2*pi*N/Ns-pi/4);
Y2 = 2048+ 2048* .1*sin(2*pi*N/Ns-pi/4);
Y3 = 2048+ 2048* .3*sin(2*pi*N/Ns-pi/4);
%Y = int16(Y1);

 Y= [Y1 Y1 Y1];
 Y = int16(Y);


%Y = 2047+ Y*2047;
[re,en1] = adpcm_encoder_mod(Y,Y(1));
[dre,YY] = adpcm_decoder_mod(en1,Y(1));

%[re,en2] = adpcm_encoder_mod(Y,Y(100));
%YY2 = adpcm_decoder_mod(en2,Y(100));

%[re,en3] = adpcm_encoder_mod(Y3,Y2(100));
%YY = adpcm_decoder_mod(en3,Y2(100));
%Xin = [Y1 Y2 Y3]
%X = [YY1 YY2 YY]
%figure(3);
%plot(X)
%title('Test ')
%grid on;

%filename = 'out.wav';
%audiowrite(filename,YY,Fs);
%YY = YY *32767; 
% figure(1);
% plot(Y)
% figure(2);
% plot(YY)
L = length( re(3,:) )
N= 1:L;
t= N/Fs;

figure(1);
plot(t,Y)
%plot(Y)
title('Input Signal before encoding Fs= 8000 SPS ')
xlabel('Time(sec)') 
ylabel('Input Signal')
grid on;


figure(2);
plot(t,re(4,:))
title('Signal before quantization ')
xlabel('Time(sec)') 
ylabel('IP Signal')
grid on;

figure(3);
plot(t,re(10,:))
title('Signal after quantization ')
xlabel('Time(sec)') 
ylabel('encoded value')
grid on;

figure(3);
plot(t,re(10,:))
title('Signal after quantization ')
xlabel('Time(sec)') 
ylabel('encoded value')
grid on;

figure(4);
plot(t,dre(1,:))
title('Signal after dequantization ')
xlabel('Time(sec)') 
ylabel('dequantization Signal')
grid on;


figure(5);
plot(t,dre(2,:))
title('decoded samples ')
xlabel('Time(sec)') 
ylabel('Output Signal')
grid on;

inp = re(1,:) ;
 for r = 1:L
%         H(r,c) = 1/(r+c-1);
         err(r) = (inp(r) - YY(r) );
 end
 err = err*100/max(Y)
figure(6);
plot(t,err);
title('IP-OP Error ')
xlabel('Time(sec)') 
ylabel('% Error')
grid on;

% be(1,:) = re(10,:);
% 
% 
% figure(4);
% %plot(t,re(3,:))
% %hold on;
% plot(t,be(1,:),'color',[0.75 0.75 0.75])
% %hold off
% title('Quantization during encoding vs input signal ')
% xlabel('Time(sec)','FontWeight','bold') 
% ylabel('Quantization step size and input signal')
% legend({'y = step size','y = input signal'},'Location','southwest')
% grid on;



