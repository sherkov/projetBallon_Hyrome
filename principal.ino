#include "DHT.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <KalmanFilter.h>
#include <elapsedMillis.h>

#include <HP20x_dev.h>

#define DHTPIN A0     // capteur humi+temp_ext 
#define DHTTYPE DHT22   // capteur humi+temp_ext


RTC_PCF8523 rtc;//heure carte SD

long derniereRecupTemps;//timer 
long difference;//timer
 int intervalle= 600;//timer


unsigned char ret = 0;//barometre

//partie capteur oxygene
const float VRefer = 3.3;       // Alimentation du capteur 
const int pinAdc   = A1;        //Branchement du capteur



KalmanFilter t_filter;    //temperature filter
KalmanFilter p_filter;    //pressure filter


const int chipSelect = 10;//capteur oxygene

DHT dht(DHTPIN, DHTTYPE);//capteur humi+temp

File dataFile;//carte SD


#define BUZZER_PIN 4 //pin sur lequel le buzzer est branché

void setup() 
{
    Serial.begin(9600); 
    dht.begin();//debut capteur humi+temp_ext

     pinMode(4, OUTPUT);//buzzer


      //verification de la carte SD
      if (!SD.begin(chipSelect)) {
        Serial.println("Erreur de carte SD");
        while (1) ;
      }
      Serial.println("card initialized.");

     //verification heure de la carte SD
        if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);

      HP20x.begin();//barometre
      delay(100);//barometre
                          }
  
 
}


void loop() 
{

 float h = dht.readHumidity(); // h = variable humiditée
    float temper_ext = dht.readTemperature(); //t = variable température
    DateTime now = rtc.now();       //date de la carte SD
    long Pressure = HP20x.ReadPressure();//barometre pression
    long Temper = HP20x.ReadTemperature();//barometre temp
    int sensorValue;//capteur UV
    long  sum=0;
    float uvindex;
    

    float pression = Pressure/100.0;

    float temper_int = Temper/100.0;



    //capteur UV
    for(int i=0;i<1024;i++)
     {  
        sensorValue=analogRead(A2);
        sum=sensorValue+sum;
        delay(2);
     }   
     
      uvindex = (307*(sum*4980.0/1023.0))/200; // This would be the equivalent UVIndex




                 //début timer
                difference = now.unixtime()-derniereRecupTemps;
    
            if(difference>intervalle)
              {
                  Serial.print("entrée dans la boucle\n");
                  ecritureFichier();
                  derniereRecupTemps=now.unixtime();
              }
                //fin timer
                
                    String heure_p;
                    String minute_p;
                    String seconde_p;
                
                    if(now.hour() < 10)
                    {
                      heure_p = "0" + (String)now.hour();
                    }
                    else
                    {
                      heure_p = (String)now.hour();
                    }
                
                    if(now.minute() < 10)
                    {
                      minute_p = "0" + (String)now.minute();
                    }
                    else
                    {
                      minute_p = (String)now.minute();
                    }
                
                    
                
                    if(now.second() < 10)
                    {
                      seconde_p = "0" + (String)now.second();
                    }
                    else
                    {
                      seconde_p = (String)now.second();
                    }

                
        //Début affichage dans moniteur série
        Serial.print(heure_p);
        Serial.print(':');
        Serial.print(minute_p);
        Serial.print(':');
        Serial.print(seconde_p);
        Serial.print(";");
        Serial.print(temper_int);
        Serial.print(";");
        Serial.print(temper_ext);//temp ext
        Serial.print(";");
        Serial.print(h);//humi ext
        Serial.print(";");
        Serial.print(pression);
        Serial.print(";");
        Serial.print(readConcentration());
        Serial.print(";");
        Serial.print(uvindex);
        Serial.print("\n");

        //Début écrite fichier
        dataFile.print(heure_p);
        dataFile.print(':');
        dataFile.print(minute_p);
        dataFile.print(':');
        dataFile.print(seconde_p);
        dataFile.print(";");
        dataFile.print(temper_int);
        dataFile.print(";");
        dataFile.print(temper_ext);//temp ext
        dataFile.print(";");
        dataFile.print(h);//humi ext
        dataFile.print(";");
        dataFile.print(pression);
        dataFile.print(";");
        dataFile.print(readConcentration());
        dataFile.print(";");
        dataFile.print(uvindex);
        dataFile.print("\n");

        dataFile.flush();


        //Bip court a chaque écriture de mesure
        digitalWrite(4, HIGH);

        delay(50);

        digitalWrite(4, LOW);
        
        
        
        delay(5000);
    }







float readO2Vout() //fonction de lecture pour le capteur oxygene o2
{
    long sum = 0;
    for(int i=0; i<32; i++)
    {
        sum += analogRead(pinAdc);
    }

    sum >>= 5;

    float MeasuredVout = sum * (VRefer / 1023.0);
    return MeasuredVout;
}

float readConcentration() //lecture de la concentration d'oxygene en pourcentage
{
    // Vout samples are with reference to 3.3V
    float MeasuredVout = readO2Vout();

    //float Concentration = FmultiMap(MeasuredVout, VoutArray,O2ConArray, 6);
    //when its output voltage is 2.0V,
    float Concentration = MeasuredVout * 0.21 / 2.0;
    float Concentration_Percentage=Concentration*100;
    return Concentration_Percentage;
}
//fin lecture oxygene


void ecritureFichier()//écriture du fichier csv, avec comme nom l'heure actuelle au format HH:MM:SS
{
    DateTime now = rtc.now();



    String heure_p;
    String minute_p;
    String seconde_p;

      //Verification si le temps relevé est inférieur a 10 -> dans ce cas on ajoute un 0 devant le nombre pour toujours avoir 6 chiffres dans le nom du fichier
    if(now.hour() < 10)
    {
      heure_p = "0" + (String)now.hour();
    }
    else
    {
      heure_p = (String)now.hour();
    }

    if(now.minute() < 10)
    {
      minute_p = "0" + (String)now.minute();
    }
    else
    {
      minute_p = (String)now.minute();
    }

    

    if(now.second() < 10)
    {
      seconde_p = "0" + (String)now.second();
    }
    else
    {
      seconde_p = (String)now.second();
    }

    String filename = heure_p + minute_p + seconde_p + ".csv" ;


    Serial.println(filename);

    dataFile.close();
    
  
    dataFile = SD.open(filename, FILE_WRITE);//Ouverture du fichier, écriture si inexistant
      if (! dataFile) {
        Serial.println("erreur d'ouverture du fichier");
        // Attente tant que on ne peut pas ouvrir le fichier
        while (1) ;
      }


      
          //bip a la fin de la création d'un fichier
          digitalWrite(4, HIGH);
          delay(analogRead(3));

          delay(1);

          digitalWrite(4, LOW);
          delay(analogRead(0));

      Serial.println("écriture du fichier réussie");//Ecriture sur moniteur série

      dataFile.print("Heure;Temperature_int(degres);Temperature_ext(degres);humidite_ext(pourcentage);Pression(hPa);Oxygene(pourcentage);Uv(index)\n");//Ecriture de la première ligne du fichier pour le format des mesures 
      dataFile.flush();
}



