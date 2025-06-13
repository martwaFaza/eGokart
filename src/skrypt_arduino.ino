#include <Wire.h>
#include <TFLI2C.h> 
#include <Arduino.h>
#include <Adafruit_NeoPixel.h> //do

#define WYJSCIE_UART 2 
#define HAMOWANIE_PRZECIWPRADEM 3 
//#define TRIGGER_POMARANCZOWY 10 // PWM  
#define WYJSCIE_HAMULEC_HAMOWANIE 5
#define WYJSCIE_HAMULEC_ODHAMOWANIE 13
#define KRANCOWKA 12
#define WLACZNIK_AUTOMATU 8 
//#define PIN_HAMOWANIE_PRZECIWPRAWDEM 3 // domyślnie 1
#define WYJSCIE_PWM_PREDKOSC 11
#define HALLOTRON_SKRET A0
// #define CZUJNIK_ODLEGLOSCI_LEWA_STRONA A1
// #define CZUJNIK_ODLEGLOSCI_SRODEK_LEWA A2
// #define CZUJNIK_ODLEGLOSCI_SRODEK_PRAWA A3
// #define CZUJNIK_ODLEGLOSCI_PRAWA_STRONA A4
#define CZUJNIK_PRADU A5
#define HALLOTRON_WEJSCIE A6
#define ODLEGLOSC_ZWALNIANIA 450
#define POCZATKOWA_WARTOSC_ZADANA 25
#define ODLEGLOSC_HAMOWANIA_CZLOWIEK 500
#define ILOSC_DIOD 37
#define PIN_STEROWANIE_DIODY A15
#define WYPELNIENIE_HAMOWANIE_CZERWONE 255
#define WYPELNIENIE_CZERWONE_JAZDA 30


//#define PI 3.1415
#define TAB_SIZE 5
#define CZAS_ODHAMOWANIA 200
#define MAX_ZAKRES_PRZETWORNIK 1024
#define MAX_WYPELNIENIE_PWM 255
//#define WYPELNIENIE_TRIGGER 30

volatile bool dataReceived = false; 
String receivedData = "";

String komenda = "";
int i = 0;
int wartosc_czujnika_test;

int flaga_czlowiek; // flaga potwierdzajaca rozpoznanie czlowieka 
int flaga_hamulec = 0; // flaga od hamulca odpoweidzialna za manewr hamowania
int pomiar_pradu;
int pomiary[TAB_SIZE];
int srednia_pomiar;
unsigned long timer1;
unsigned long timer2;
int pomiary_odleglosci[TAB_SIZE];

//const int iloscImpulsow = 11;
//volatile int licznik = 0;
int timer3;
int timer4;
//float predkoscObrotowa = 0.0;
//const float obwodKola = 0.82;
//float promienKola;
//float predkoscLiniowa = 0;
//float predkoscLiniowaDrukowanie;
//float konwersjaDoKm_h = 6.0/10000.0;
//int wartosc_czujnik_odleglosci_srodek_prawa; 
//int wartosc_czujnik_odleglosci_srodek_lewa; 
//int wartosc_czujnik_odleglosci_prawa_strona; 
//int wartosc_czujnik_odleglosci_lewa_strona;
//unsigned long czas_trwania_lewa_srodek;
//unsigned long czas_trwania_prawa_srodek;
//float odleglosc_funkcja_lewa_srodek;
//float odleglosc_funkcja_prawa_srodek; 
float wartosc_zadana_wyjscie_driver;
int wartosc_z_HALLA;
float ograniczenie_predkosci_mnoznik = 1.00; 
int zmienna_automat;
unsigned long timer5 = 0;

TFLI2C tflI2C;
Adafruit_NeoPixel diody = Adafruit_NeoPixel(ILOSC_DIOD, PIN_STEROWANIE_DIODY, NEO_GRB + NEO_KHZ800);

int16_t  tfDist = 0;    // distance in centimeters
int16_t  tfAddr = TFL_DEF_ADR;  // Use this default I2C address

int srednia_z_pomiarow(int rozmiar_tablicy, int wartosc);
void manewr_hamowania();
void manewr_odhamowywania();
//float obliczaniePredkosci(int liczbaImpulsow);
float odczyt_czujnika_halla(int pin_czujnika);

void setup() {
  Serial.begin(115200);
  timer3 = millis();

  diody.begin(); //Inicjalizacja
  diody.show();

  //promienKola = (obwodKola/(2*PI)); 
  
  pinMode(KRANCOWKA, INPUT_PULLUP);
  pinMode(WLACZNIK_AUTOMATU, INPUT_PULLUP);
  pinMode(WYJSCIE_UART, OUTPUT);
  pinMode(WYJSCIE_HAMULEC_HAMOWANIE, OUTPUT);
  pinMode(WYJSCIE_HAMULEC_ODHAMOWANIE, OUTPUT);
  //pinMode(PIN_HAMOWANIE_PRZECIWPRAWDEM, INPUT_PULLUP);

  //pinMode(CZUJNIK_ODLEGLOSCI_PRAWA_STRONA, INPUT);
  //pinMode(CZUJNIK_ODLEGLOSCI_LEWA_STRONA, INPUT);
  //pinMode(CZUJNIK_ODLEGLOSCI_SRODEK_PRAWA, INPUT);
  //pinMode(CZUJNIK_ODLEGLOSCI_SRODEK_LEWA, INPUT);
  //pinMode(TRIGGER_POMARANCZOWY, OUTPUT);
  pinMode(HAMOWANIE_PRZECIWPRADEM, OUTPUT);
  pinMode(WYJSCIE_PWM_PREDKOSC, OUTPUT);
  //pinMode(TRIGGER_POMARANCZOWY, OUTPUT);

  timer1 = millis();
  timer2 = millis();
  digitalWrite(WYJSCIE_HAMULEC_HAMOWANIE, LOW);
  analogWrite(WYJSCIE_PWM_PREDKOSC, POCZATKOWA_WARTOSC_ZADANA);

  //analogWrite(TRIGGER_POMARANCZOWY, WYPELNIENIE_TRIGGER);

  Wire.begin();
}


void loop()
{
    if(Serial.available()>0)
    {
      while (Serial.available() > 0)
      {
        komenda += char(Serial.read());
        if(komenda == "person"){
          break;
        }
      }

      if(komenda == "person")
      {
        digitalWrite(WYJSCIE_UART, HIGH);
        if(flaga_czlowiek == 0 )
        {
          flaga_czlowiek = 1;
        }
        timer1 = millis();
        Serial.println(komenda);
        delay(100);
        digitalWrite(WYJSCIE_UART, LOW);
      }
      

      delay(100);
      komenda = "";
      Serial.println(komenda);
    }
    delay(50);
    

    
    if(millis() - timer1 > 2000)
      {
        flaga_czlowiek = 0;
      }

    // Sygnał do hamowania
    if(flaga_czlowiek==1 && digitalRead(WLACZNIK_AUTOMATU)==0 && flaga_hamulec==0 && tfDist < ODLEGLOSC_HAMOWANIA_CZLOWIEK)
    {
      flaga_hamulec = 1;
    }

    //Manewr hamoawnia przy zwarciu wejscia do masy i pojawia sie czlowiek
    if(flaga_hamulec == 1)
    {
      manewr_hamowania();
    }
    
    if(flaga_hamulec == 2)
    {
      manewr_odhamowywania();
    }

  //if(millis() - timer3 > 1000)
  //{
  //  predkoscLiniowaDrukowanie = obliczaniePredkosci(licznik);
    //timer3 = millis();
    //licznik = 0;
  //}

if(digitalRead(KRANCOWKA) == 0)
{
  for(i = 0; i < ILOSC_DIOD; i++)
  {
    diody.setPixelColor(i, diody.Color(WYPELNIENIE_CZERWONE_JAZDA, 0, 0));
  }  
  diody.show();
  delay(10);
}

else if(digitalRead(KRANCOWKA) == 1)
{
  for(i = 0; i < ILOSC_DIOD; i++)
  {
    diody.setPixelColor(i, diody.Color(WYPELNIENIE_HAMOWANIE_CZERWONE,0,0));
  }
  diody.show();
  delay(10);
}


    // Pomiar prądu i średnia z 4 kolejnych pomiarów
    pomiar_pradu=analogRead(CZUJNIK_PRADU);
    srednia_pomiar = srednia_z_pomiarow(TAB_SIZE, pomiar_pradu);

    // czas_trwania_lewa_srodek = pulseIn(CZUJNIK_ODLEGLOSCI_SRODEK_LEWA, HIGH);
    // odleglosc_funkcja_lewa_srodek = (czas_trwania_lewa_srodek *.0343)/2;

    if(tflI2C.getData(tfDist, tfAddr))
    {
      Serial.println(String(tfDist)+" cm / " + String(tfDist/2.54)+" inches");
    }
    delay(20);

    if(tfDist >= 900)
    {
      tfDist = 900; 
    }

    else if(tfDist <= 0)
    {
      tfDist = 900;
    }

zmienna_automat = digitalRead(WLACZNIK_AUTOMATU);

if(zmienna_automat == 0)
{
    if(tfDist < ODLEGLOSC_ZWALNIANIA)
    {
      ograniczenie_predkosci_mnoznik = ograniczenie_predkosci_mnoznik - 0.3;
      if(ograniczenie_predkosci_mnoznik < 0.00)
      {
        ograniczenie_predkosci_mnoznik = 0.00;
      }
    }

    else if(tfDist >= ODLEGLOSC_ZWALNIANIA)
    {
        ograniczenie_predkosci_mnoznik = ograniczenie_predkosci_mnoznik + 0.1;
        if(ograniczenie_predkosci_mnoznik > 1.00)
        {
          ograniczenie_predkosci_mnoznik = 1.00; 
        }

        else if(ograniczenie_predkosci_mnoznik < 0.00)
        {
          ograniczenie_predkosci_mnoznik = 0.00;
        }
    }


  wartosc_z_HALLA = analogRead(HALLOTRON_WEJSCIE);
  wartosc_zadana_wyjscie_driver = ograniczenie_predkosci_mnoznik * (wartosc_z_HALLA)/2048.0 * 255.00;
  analogWrite(WYJSCIE_PWM_PREDKOSC, (int)wartosc_zadana_wyjscie_driver);
}

else if(zmienna_automat == 1){
  ograniczenie_predkosci_mnoznik = 1.00;
  flaga_hamulec = 0;
}

  wartosc_z_HALLA = analogRead(HALLOTRON_WEJSCIE);
  wartosc_zadana_wyjscie_driver = ograniczenie_predkosci_mnoznik * (wartosc_z_HALLA)/2048.0 * 255.00;
  analogWrite(WYJSCIE_PWM_PREDKOSC, (int)wartosc_zadana_wyjscie_driver);
  
    diody.clear();

    Serial.println("Odleglosc:");
    Serial.println(tfDist);
    Serial.println("Czujnik pradu:");
    Serial.println(analogRead(CZUJNIK_PRADU));
    Serial.println("Flaga hamulec:");
    Serial.println(flaga_hamulec);
    Serial.println("Krancowka:");
    Serial.println(digitalRead(KRANCOWKA));
}

void manewr_odhamowywania()
{
  if(flaga_czlowiek == 0 && digitalRead(KRANCOWKA) == 1)
  {
    digitalWrite(WYJSCIE_HAMULEC_ODHAMOWANIE, HIGH);
  }

  if(digitalRead(KRANCOWKA) == 0)
  {
    digitalWrite(WYJSCIE_HAMULEC_ODHAMOWANIE, LOW);
    flaga_hamulec = 0;
  }
}

void manewr_hamowania()
{
  if(digitalRead(KRANCOWKA) == 0)
  {
    digitalWrite(WYJSCIE_HAMULEC_HAMOWANIE, HIGH); // 
  }

  if(analogRead(CZUJNIK_PRADU) <= 350 && digitalRead(KRANCOWKA) == 1)
  {
    digitalWrite(WYJSCIE_HAMULEC_HAMOWANIE, LOW);
    flaga_hamulec = 2; 
  }
}


int srednia_z_pomiarow(int rozmiar_tablicy, int wartosc)
{
int srednia = 0;
int suma = 0;

//przesuniecie pomiarów w tablicy
for(int i = 0; i < rozmiar_tablicy-1; i++)
{
  pomiary[rozmiar_tablicy-i-1]=pomiary[rozmiar_tablicy-i-2];
}

pomiary[0]=wartosc;
//liczenie sumy
for(int i = 0; i < rozmiar_tablicy; i++)
{
  suma = suma + pomiary[i]; 
}
//licznie sredniej
srednia = suma / rozmiar_tablicy;
//zwracanie sredniej
return srednia;
}

//float obliczaniePredkosci(int liczbaImpulsow)
//{
  //predkoscObrotowa = (liczbaImpulsow/iloscImpulsow) * 60.00;
 // predkoscLiniowa = (predkoscObrotowa * promienKola);
  //predkoscLiniowa = predkoscLiniowa * konwersjaDoKm_h;
  //licznik = 0;
 // return predkoscLiniowa;
//}

float odczyt_czujnika_halla(int pin_czujnika) 
{
  float odczyt_wartosci = analogRead(pin_czujnika);

  return odczyt_wartosci; 
}
