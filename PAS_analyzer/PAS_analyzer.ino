/******************************************************************
Created with PROGRAMINO IDE for Arduino - 27.05.2023 | 10:21:23
Project     :
Libraries   :
Author      :  Chris74

Description :  E-bike PAS signal analyzer 

Github, code and info https://github.com/Chris741233/PAS_analyzer


Doc PAS ebikes.ca (Signal Types for Basic PAS Sensors) : 
- https://ebikes.ca/learn/pedal-assist.html
Doc PAS on pedelecforum.de
- https://www.pedelecforum.de/forum/index.php?threads/funktionsweise-digitaler-pas-sensoren-details-fuer-interessierte.9539/


******************************************************************/

// Install these 2 librairy with Arduino IDE librairy manager or manual (see url)

#include <RunningMedian.h>  // filtre median ou moyennage  https://github.com/RobTillaart/RunningMedian 
#include "Streaming.h"      // simplification envois print https://www.arduinolibraries.info/libraries/streaming



// -------- GPIO Arduino Uno/Nano --------------


const int PAS_PIN = 2;   // -- Interrupt : PAS Hall D2, sans resistance (cf INPUT_PULLUP) 

const int LED_PIN = 13;  // -- D13 = LED_BUILTIN


// -------- SETTING CONSTANTS ------------------------

const int  NB_MAGNETS =  6;                 // nb. d'aimants du disque PAS (pour calcul RPM)
const long COEFF_RPM = 60000 / NB_MAGNETS;  // 60000 / 6 magnets  = 10000  (1 minute=60000ms)

// timer loop
const int INTERVAL = 2000;                 // interval affichage Serial en ms, 1000 ou 2000
// 1000 = affichage rapide, 2000 calcul RPM plus stable et précis


// objet statistic median (median filter)
RunningMedian samples_h = RunningMedian(NB_MAGNETS);  // nb. d'echantillon, definir a NB_MAGNETS 
RunningMedian samples_l = RunningMedian(NB_MAGNETS);



// -------- GLOBAL VARIABLES ----------------------


unsigned long previousMillis = 0;  // for timer loop

unsigned int rpm      = 0;         // RPM pedalling 


// -- var Interrupt
volatile unsigned int pulse = 0;   // revolution count : Volatile obligatoire pour interupt si échange avec loop !
unsigned long lastTime = 0;

volatile unsigned int period_h = 0;
volatile unsigned int period_l = 0;


// -------- MAIN PROG ----------------------

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(PAS_PIN, INPUT_PULLUP);  // PAS Hall, sans ajout de résistance !
    
    digitalWrite(LED_PIN, LOW);      // Led  Low au boot
    
    // -- Interrupt pedaling : appel "isr_pas" sur signal CHANGE  
    attachInterrupt(digitalPinToInterrupt(PAS_PIN), isr_pas, CHANGE);  
    
    
    // attention, ne pas mettre d'instruction "delay" ici (et dans le loop egalement) !
    // no delay() here !
    
}


void loop()
{
    unsigned long currentMillis = millis(); // init timer 
    
    // -- remise a zero si pas de pedalage ou RPM trop lent
    if (period_h > 800 || period_l > 800 || rpm <= 0) {
        
        period_h = 0;       // reset var interrupt
        period_l = 0;       // reset var interrupt
        
        samples_h.clear();  // reset median filter
        samples_l.clear();  // reset median filtern
    }
    else {
        // -- Ad stat period : median removes noise from the outliers in the samples.
        samples_h.add(period_h);
        samples_l.add(period_l);
    }
    
    
    
    // -- timer  calcul et affichage Serial
    long check_t = currentMillis - previousMillis;
    
    if (check_t >= INTERVAL) {
        
        calcul_rpm(check_t);  // appel fonction calcul + Serial
        
        previousMillis = currentMillis; // reset timer loop
        
    }
    
    
    // No delay() here !
    
} // end loop



// -------- FUNCTIONS ----------------------


// -- ISR - interrupt PAS
void isr_pas() {
    
    // rester ici le plus concis possible !
    long time = millis();
    
    if (digitalRead(PAS_PIN) == HIGH) {
        digitalWrite(LED_PIN, HIGH);
        period_l = (time - lastTime);  // ms low (inverse)
        
        pulse++;          // increment nb de pulse (pour calcul rpm)    
    }
    else {
        digitalWrite(LED_PIN, LOW);
        period_h = (time - lastTime);  // ms high (inverse)
    }
    
    lastTime = time ; // reset timer interupt
    
    
} // endfunc


void calcul_rpm(long t)
{
    
    detachInterrupt(digitalPinToInterrupt(PAS_PIN)); // (recommended)
    // https://www.arduino.cc/reference/en/language/functions/external-interrupts/detachinterrupt/
    
    // calcul RPM
    rpm = (pulse * COEFF_RPM) / t;
    
    int s_h = samples_h.getMedian(); // signal high in ms (filtre median)
    int s_l = samples_l.getMedian(); // signal low  in ms (filtre median)
    
    int tot_periode = s_h + s_l;                                // period in ms
    float duty_high = float(s_h) / float(tot_periode) * 100;    // % duty_cycle high
    
    
    Serial << "timer= " << t << "ms - pulse= " << pulse << endl;
    Serial << "RPM= " << rpm << " Periode= " << tot_periode << "ms" << endl;
    Serial << "high= " << s_h << "ms - low= " << s_l << "ms" << endl;
    Serial << "duty-cycle+ = " << duty_high << "%" << endl;
    Serial << "----------------------"  << endl;
    
    
    pulse = 0; // reset pulse counter
    
    //interrupts();   // reactiver toutes les interruptions
    attachInterrupt(digitalPinToInterrupt(PAS_PIN), isr_pas, CHANGE);  
    
} //endfunc




