#include <SD.h>
#include <string.h>
#include <ctype.h>
#include <NewPing.h>
#define  TRIGGER_PIN  11
#define  ECHO_PIN     10
#define MAX_DISTANCE 200
#include <TinyGPS.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <dht.h>
#define dht_dpin A0 //no ; here. Set equal to channel sensor is on
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 
int DistanceIn;
int DistanceCm;
Adafruit_BMP085 bmp;
dht DHT;
TinyGPS gps;
static void gpsdump(TinyGPS &gps);
static bool feedgps();
static void print_float(float val, float invalid, int len, int prec);
static void print_date(TinyGPS &gps);
int CS_pin = 53;
float refresh_rate = 0.0;
void setup()
{
  Serial.begin(9600);
  Serial3.begin(9600);
   if (!bmp.begin()) {
  //	Serial.println("Could not find a valid BMP085 sensor, check wiring!");
	while (1) {}
  }  
  //SD card
  Serial.println("Initializing Card");
  //CS Pin is an output
  pinMode(CS_pin, OUTPUT);
  if (!SD.begin(CS_pin))
  {
      Serial.println("Card Failure");
      return;
  }
  Serial.println("Card Ready");
  
  //Read the Configuration information (COMMANDS.txt)
  File commandFile = SD.open("COMMANDS.txt");
  if (commandFile)
  {
    Serial.println("Reading Command File");
    
    float decade = pow(10, (commandFile.available() - 1));
    while(commandFile.available())
    {
      float temp = (commandFile.read() - '0');
      refresh_rate = temp*decade+refresh_rate;
      decade = decade/10;
    }
    Serial.print("Refresh Rate = ");
    Serial.print(refresh_rate);
    Serial.println("ms");
  }
  else
  {
    Serial.println("Could not read command file.");
    return;
  }
  

}

void loop()
{
  bool newdata = false;
  unsigned long start = millis();
  while (millis() - start < 1000)
  {
    if (feedgps())
      newdata = true;
  }
  
  gpsdump(gps);

}

static void gpsdump(TinyGPS &gps)
{
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  //  static const float LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
  //SMS
    delay(7000);
    Serial.println("AT+CMGF=1\r");
    delay(3000);
    Serial.println("AT+CMGS=\"7875237077\"" );
    delay(3000);
//    Serial.println("-------------");
    //GPS
    gps.f_get_position(&flat, &flon, &age);
    print_date(gps);
    Serial.print("@1,");
//    Serial.print("Latitude  : ");
    print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 9, 6);
    Serial.print(",");
//    Serial.print("Longitude : ");
    print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
    Serial.print(",");
//    Serial.println("To know location click on the link : ");
//    Serial.print("https://maps.google.co.in/?q=");
//    print_float(flat,TinyGPS::GPS_INVALID_F_ANGLE,9,6);
//    Serial.print(",");
//    print_float(flon,TinyGPS::GPS_INVALID_F_ANGLE,10,6);
//    Serial.println("");
//    Serial.print("flat");

    //DHT Sensor
    DHT.read11(dht_dpin);
//    Serial.print("Humidity : ");
    Serial.print(DHT.humidity);
//    Serial.println("%  ");   
    Serial.print(",");
//    Serial.print("Temperature : ");
    Serial.print(DHT.temperature); 
    Serial.print(",");
//    Serial.println("C  ");
    delay(800);
    //ultrasonic
//    Serial.print("UltraSonic Distance Measurement : ");
    delay(2000);// Wait 100ms between pings (about 10 pings/sec). 29ms should be the shortest delay between pings.
    DistanceCm = sonar.ping_cm();
//    Serial.print("Ping : ");
    Serial.print(DistanceCm); 
    Serial.print(",");
//    Serial.print(" cm   ");  
//    delay(1000);// Wait 100ms between pings (about 10 pings/sec). 29ms should be the shortest delay between pings.
//    DistanceIn = sonar.ping_in();
//    Serial.print("Ping : ");
//    Serial.print(DistanceIn); // Convert ping time to distance and print result 
                            // (0 = outside set distance range, no ping echo)
//    Serial.println(" in     ");
    delay(100);// Wait 100ms between pings (about 10 pings/sec). 29ms should be the shortest delay between pings.
    //pressure
//    Serial.print("Pressure : ");
    Serial.print(bmp.readPressure());
    Serial.print(",");
//    Serial.println(" Pa"); 
//    Serial.print("Altitude : ");
    Serial.print(bmp.readAltitude());
    Serial.print(",");
//    Serial.println(" meters");
//    Serial.println("---------------");
    delay(10000);
    Serial.print((char)26);  
//    Serial.println("----------------");  
    Serial.println("");
  File logFile = SD.open("LOG.txt", FILE_WRITE);
  if (logFile)
   {

    logFile.println("Latitude  : ");
    logFile.println("");
    logFile.println("");
    logFile.println("To know location click on the link : ");  
    logFile.print("https://maps.google.co.in/?q=");
    logFile.println(",");
    logFile.println("");
    logFile.print("Humidity  : ");
    logFile.print(DHT.humidity);
    logFile.println("%  ");
    logFile.print("Temperature : ");
    logFile.print(DHT.temperature);
    logFile.println("C  ");
    logFile.print("UltraSonic Distance Measurement : ");
    logFile.print("Ping : ");
    logFile.print(DistanceCm);
    logFile.print(" cm   ");
    logFile.print("Ping : ");
    logFile.print(DistanceIn);
    logFile.println(" in     ");
    logFile.print("Pressure : ");
    logFile.print(bmp.readPressure());
    logFile.println(" Pa");
    logFile.print("Altitude : ");
    logFile.print(bmp.readAltitude());
    logFile.println(" meters");
    logFile.println("---------------");
    logFile.close();
   }
  else
   {
    Serial.println("LOG.txt");
    Serial.println("Couldn't open log file");
   }
  delay(refresh_rate);
 }

static void print_float(float val, float invalid, int len, int prec)
{
  char sz[32];
  if (val == invalid)
  {
    strcpy(sz, "*******");
    sz[len] = 0;
        if (len > 0) 
          sz[len-1] = ' ';
    for (int i=7; i<len; ++i)
        sz[i] = ' ';
    Serial.print(sz);
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1);
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(" ");
  }
  feedgps();

}

static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print("*******    *******    ");
  else
  {
    char sz[32];
//    Serial.print("Date : ");
//    sprintf(sz, "%02d/%02d/%02d ",day, month, year);
//    Serial.print(sz);
//    Serial.println("");
//    Serial.print("Time : ");
    hour=hour+5;
    minute=minute+30;
    if(minute>60)
     {
      minute=minute-60;
      hour=hour+1;
     }
    if(hour>23)
     {
      hour=hour-24;
     }   
//    sprintf(sz, "%02d:%02d:%02d   ",hour, minute, second);
//    Serial.print(sz);
//    Serial.println("");
    File logFile = SD.open("LOG.txt", FILE_WRITE);
  if (logFile)
   {
    logFile.println("-------------");
    logFile.print("Date : ");
    sprintf(sz, "%02d/%02d/%02d ",day, month, year);
    logFile.print(sz);
    logFile.println("");
    logFile.print("TIME : "); 
    sprintf(sz, "%02d:%02d:%02d   ",hour, minute, second); 
    logFile.println(sz);       
    logFile.close();
   }
  else
   {
    Serial.println("LOG.txt");
    Serial.println("Couldn't open log file");
   }
  delay(refresh_rate); 
 
  }
  feedgps();

}

static bool feedgps()
{
  while (Serial3.available())
  {
    if (gps.encode(Serial3.read()))
      return true;
  }
  return false;}
