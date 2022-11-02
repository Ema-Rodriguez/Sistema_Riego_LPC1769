#include <SoftwareSerial.h>
const int Trigger = 2;   //Pin digital 2 para el Trigger del sensor
const int Echo = 3;      //Pin digital 3 para el Echo del sensor
char dato;               //Dato proveniente de LPC1769
const int Salida = 10;   //Salida de nivel de agua
const int Salida1= 11;   //Salida nivel de agua
int flag=0;


void setup() {

  Serial.begin(9600);          //iniciailzamos la comunicación
  pinMode(Trigger, OUTPUT);    //pin como salida
  pinMode(Echo, INPUT);        //pin como entrada
  digitalWrite(Trigger, LOW);  //Inicializamos el pin con 0
  pinMode(Salida, OUTPUT);     //pin como salida
  pinMode(Salida1, OUTPUT);    //pin como salida
  digitalWrite(Salida, LOW);   //Inicializamos el pin con 0
  digitalWrite(Salida1, LOW);  //Inicializamos el pin con 0
  Serial.flush();
}

void loop()
{
  if (Serial.available()){
    dato = Serial.read();            //leeemos la opcion

    if (dato=='Q'){                   //significa que quiere medir el nivel del agua
        long t;                       //tiempo que demora en llegar el eco
        long d;                       //distancia en centimetros
      
        digitalWrite(Trigger, HIGH);
        delayMicroseconds(10);        //Enviamos un pulso de 10us
        digitalWrite(Trigger, LOW);
        
        t = pulseIn(Echo, HIGH);      //obtenemos el ancho del pulso
        d = t/59;                     //escalamos el tiempo a una distancia en cm
        
        Serial.print("Distancia: ");
        Serial.print(d);              //Enviamos serialmente el valor de la distancia
        Serial.print("cm");
        Serial.println();
        if(d>13){                     //significa que NO alcanza el agua
          digitalWrite(Salida1, HIGH);//Ponemos el pin 11 en 1
          }
        else{                       //significa que hay agua
          digitalWrite(Salida, HIGH); //Ponemos el pin 10 en 1
          }
        flag=1;
        delay(1);
        digitalWrite(Salida, LOW);    //Ponemos el pin 10 en 0
        digitalWrite(Salida1, LOW);   //Ponemos el pin 11 en 0
    }
    else if(flag==0){
        Serial.print(dato);
      }
    else if(flag==1 && dato=='\n'){
      flag=0;
    }
    }

}
