# Sistema de control de riego automatico con sensor de humedad utilizando el microcontrolador LPC1769
Trabajo final de la materia Electrónica Digital 3 realizado en la FCEFyN UNC

------------

####  Autores:
-  [Gerard Brian](https://github.com/brian1062 "Gerard Brian")
- [Rodriguez Emanuel](https://github.com/Ema-Rodriguez "Rodriguez Emanuel")

------------
#### Funcionamiento:

El trabajo consiste en la utilización de la placa cortex M-3 LPC1769 y sus distintos periféricos ADC - DAC - DMA - TIMER- UART.
Para ello decidimos realizar un sistema que controle los niveles de humedad de una planta y mediante una bomba realizar su posterior regado siempre y cuando haya la suficiente agua para poder realizar el regado y los niveles de humedad de la planta sea bajo.
Utilizamos un Sensor Higrómetro de humedad del suelo el cual envía de forma analógica valores de 0 a 3,3v a nuestro ADC para posteriormente transformar dicho valor en un valor digital.
Una vez digitalizadas dichas medidas procedemos a convertirlas en valores de porcentaje para poder mandarla mediante comunicación UART a nuestra placa arduino en la pc.
Luego nos fijamos si el nivel de humedad se encuentra debajo del 60% (la tierra se encuentra seca), si es así, medimos el nivel de agua de la bomba utilizando un sensor ultrasónico a través de arduino, si dicha medición nos devuelve valores chicos significa que  se encuentra con suficiente agua, por lo que se realiza un regado de 5 seg controlado mediante un timer, luego va a cortar y esperar 1 min para realizar la posterior medición.
Si la bomba no posee el nivel de agua suficiente se activará una flag la cual no activará el riego de la planta (ya que si la bomba funciona sin agua, la misma se quemará) y activaremos un buzzer el cual mediante DAC se enviará una señal senoidal de 4,5 khz para alertarnos que debemos llenar el tanque.
Una vez llenado el dac se desactiva.
Para el envío de datos mediante UART y el envío de los valores de la senoidal al DAC utilizamos DMA.

------------
####  Diagrama de flujo:
![imagen](https://user-images.githubusercontent.com/84784723/200039240-53b1aa00-3e26-4764-b3f1-29433e62761e.png)

------------
####  Materiales:
![imagen](https://user-images.githubusercontent.com/84784723/200039346-d4ded9ad-23f1-4589-a7f3-60db427ed004.png)


------------

####  Configuraciones:
##### GPIO
###### Salida:    
P0.0(Envia señal para activar la bomba)
###### Entradas:     
P0.21(Mediante interrupcion activamos riego manual)
P0.10(Activa flag=1)
P0.11(Activa flag=2)
##### PINSEL    
Func 1:    P0.23 (ADC 0.0); P0.2,P0.3 (UART 0.0);
Func 2:     P0.26(AOUT)  
ADC0.0:Convierte valores analogicos recibidos de el sensor de humedad a digital.
##### DAC: 
Convierte valores de un arreglo que contiene una señal senoidal.
##### TIMER0: 
Es el cual activara nuestro adc configurado para togglear el mat0.1 cada 30 segundos
##### TIMER2:
Tiene 2 match el primero en 5 seg el cual apagará la bomba y el siguiente en 15 segundo el cual apagara el timer2 y encendera el timer0 para volver a manejar el adc.
#####DMA:
Chanel 0: Encargado de enviar el arreglo de la señal senoidal a nuestro DAC
Chanel 1: Encargado de enviar nuestro mensaje y porcentaje de humedad a nuestro UART 
