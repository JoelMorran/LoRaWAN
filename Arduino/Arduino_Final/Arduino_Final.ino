//Include required libraries
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU9250.h"
#include "LoRaWan.h"
#include "TinyGPS++.h"

//Initialise required variables
char * deviceId = "46AAC86800430028";
char * devAddr = "0228B1B1";
char * appSKey = "2B7E151628AED2A6ABF7158809CF4F3C";
char * nwkSKey = "3B7E151628AED2A6ABF7158809CF4F3C";

_data_rate_t dr = DR3;
_physical_type_t physicalType = US915HYBRID;

unsigned char data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA,};
char buffer[256];

MPU9250 accelgyro;
I2Cdev   I2C_M;

uint8_t buffer_m[6];

int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t   mx, my, mz;

float heading;
float tiltheading;

float Axyz[3];
float Gxyz[3];
float Mxyz[3];

#define sample_num_mdate  5000

volatile float mx_sample[3];
volatile float my_sample[3];
volatile float mz_sample[3];

static float mx_centre = 0;
static float my_centre = 0;
static float mz_centre = 0;

volatile int mx_max = 0;
volatile int my_max = 0;
volatile int mz_max = 0;

volatile int mx_min = 0;
volatile int my_min = 0;
volatile int mz_min = 0;

TinyGPSPlus gps;

char c;

char finalStr[1000];
char placeholderStr[100];

void setup(void)
{
  //Initialise USB Serial
  SerialUSB.begin(115200);
  while (!SerialUSB);

  //Setup LoRaWAN connection
  lora.init();

  lora.setId(devAddr, deviceId, NULL);
  lora.setKey(nwkSKey, appSKey, NULL);

  lora.setDeciveMode(LWABP);
  lora.setDataRate(dr, physicalType);

  lora.setAdaptiveDataRate(false);

  lora.setDutyCycle(false);
  lora.setJoinDutyCycle(false);

  lora.setPower(14);

  //Initialise I2C bridge
  Wire.begin();

  //Initialise serial
  Serial.begin(9600);

  //Initialise accelerometer
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();

  //Test accelerometer connection
  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU9250 connection successful" : "MPU9250 connection failed");

  delay(1000);
}

void loop(void)
{
  if (Serial.available() > 0)
  {
    if (gps.encode(c = Serial.read()))
    {
      if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid())
      {
        //Append device ID to final string
        strcpy (finalStr, deviceId);
        strcat (finalStr, ",");

        //Append month to final string
        sprintf(placeholderStr, "%d", gps.date.month());
        strcat (finalStr, placeholderStr);
        strcat (finalStr, "/");

        //Append day to final string
        sprintf(placeholderStr, "%d", gps.date.day());
        strcat (finalStr, placeholderStr);
        strcat (finalStr, "/");

        //Append year to final string
        sprintf(placeholderStr, "%d", gps.date.year());
        strcat (finalStr, placeholderStr);
        strcat (finalStr, " ");

        //Append hour to final string
        if (gps.time.hour() < 10)
        {
          strcat (finalStr, "0");
        }
        sprintf(placeholderStr, "%d", gps.time.hour() - 1);
        strcat (finalStr, placeholderStr);
        strcat (finalStr, ":");

        //Append minute to final string
        if (gps.time.minute() < 10)
        {
          strcat (finalStr, "0");
        }
        sprintf(placeholderStr, "%d", gps.time.minute());
        strcat (finalStr, placeholderStr);
        strcat (finalStr, ":");

        //Append second to final string
        if (gps.time.second() < 10)
        {
          strcat (finalStr, "0");
        }
        sprintf(placeholderStr, "%d", gps.time.second());
        strcat (finalStr, placeholderStr);
        strcat (finalStr, ".");

        //Append centisecond to final string
        if (gps.time.centisecond() < 10)
        {
          strcat (finalStr, "0");
        }
        sprintf(placeholderStr, "%d", gps.time.centisecond());
        strcat (finalStr, placeholderStr);
        strcat (finalStr, " PM,");

        //Retrieve accelerometer data
        getAccel_Data();

        //Append X to final string
        sprintf(placeholderStr, "%f", Axyz[0]);
        strcat (finalStr, placeholderStr);
        strcat (finalStr, ",");

        //Append Y to final string
        sprintf(placeholderStr, "%f", Axyz[1]);
        strcat (finalStr, placeholderStr);
        strcat (finalStr, ",");

        //Append Z to final string
        sprintf(placeholderStr, "%f", Axyz[2]);
        strcat (finalStr, placeholderStr);
        strcat (finalStr, ",");

        //Append latitude to final string
        sprintf(placeholderStr, "%f", gps.location.lat());
        strcat (finalStr, placeholderStr);
        strcat (finalStr, ",");

        //Append longitude to final string
        sprintf(placeholderStr, "%f", gps.location.lng());
        strcat (finalStr, placeholderStr);
        
        //Display final string to serial monitor
        SerialUSB.println(finalStr);
        SerialUSB.println("================================================================");

        //Transmit final string via LoRaWAN
        lora.transferPacket(finalStr, 3);
      }
    }
  }
}

//Gets the GPS heading
void getAccel_Data(void)
{
  accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
  Axyz[0] = (double) ax / 16384;//16384  LSB/g
  Axyz[1] = (double) ay / 16384;
  Axyz[2] = (double) az / 16384;
}

//NOTE: The following functions aren't used however the program won't run without them...
//=======================================================================================

//Gets the GPS heading
void getHeading(void)
{
  heading = 180 * atan2(Mxyz[1], Mxyz[0]) / PI;
  if (heading < 0) heading += 360;
}

//Retrieves GPS tilt heading
void getTiltHeading(void)
{
  float pitch = asin(-Axyz[0]);
  float roll = asin(Axyz[1] / cos(pitch));

  float xh = Mxyz[0] * cos(pitch) + Mxyz[2] * sin(pitch);
  float yh = Mxyz[0] * sin(roll) * sin(pitch) + Mxyz[1] * cos(roll) - Mxyz[2] * sin(roll) * cos(pitch);
  float zh = -Mxyz[0] * cos(roll) * sin(pitch) + Mxyz[1] * sin(roll) + Mxyz[2] * cos(roll) * cos(pitch);
  tiltheading = 180 * atan2(yh, xh) / PI;
  if (yh < 0)    tiltheading += 360;
}

//Calibrates GPS
void Mxyz_init_calibrated ()
{

  SerialUSB.println(F("Before using 9DOF,we need to calibrate the compass frist,It will takes about 2 minutes."));
  SerialUSB.print("  ");
  SerialUSB.println(F("During  calibratting ,you should rotate and turn the 9DOF all the time within 2 minutes."));
  SerialUSB.print("  ");
  SerialUSB.println(F("If you are ready ,please sent a command data 'ready' to start sample and calibrate."));
  while (!SerialUSB.find("ready"));
  SerialUSB.println("  ");
  SerialUSB.println("ready");
  SerialUSB.println("Sample starting......");
  SerialUSB.println("waiting ......");

  get_calibration_Data ();

  SerialUSB.println("     ");
  SerialUSB.println("compass calibration parameter ");
  SerialUSB.print(mx_centre);
  SerialUSB.print("     ");
  SerialUSB.print(my_centre);
  SerialUSB.print("     ");
  SerialUSB.println(mz_centre);
  SerialUSB.println("    ");
}

//Retrieve calibration data
void get_calibration_Data ()
{
  for (int i = 0; i < sample_num_mdate; i++)
  {
    get_one_sample_date_mxyz();
    
    if (mx_sample[2] >= mx_sample[1])mx_sample[1] = mx_sample[2];
    if (my_sample[2] >= my_sample[1])my_sample[1] = my_sample[2]; //find max value
    if (mz_sample[2] >= mz_sample[1])mz_sample[1] = mz_sample[2];

    if (mx_sample[2] <= mx_sample[0])mx_sample[0] = mx_sample[2];
    if (my_sample[2] <= my_sample[0])my_sample[0] = my_sample[2]; //find min value
    if (mz_sample[2] <= mz_sample[0])mz_sample[0] = mz_sample[2];
  }

  mx_max = mx_sample[1];
  my_max = my_sample[1];
  mz_max = mz_sample[1];

  mx_min = mx_sample[0];
  my_min = my_sample[0];
  mz_min = mz_sample[0];

  mx_centre = (mx_max + mx_min) / 2;
  my_centre = (my_max + my_min) / 2;
  mz_centre = (mz_max + mz_min) / 2;
}

//Retrieve a GPS sample
void get_one_sample_date_mxyz()
{
  getCompass_Data();
  mx_sample[2] = Mxyz[0];
  my_sample[2] = Mxyz[1];
  mz_sample[2] = Mxyz[2];
}

//Retrieve gyro data
void getGyro_Data(void)
{
  accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
  Gxyz[0] = (double) gx * 250 / 32768;//131 LSB(��/s)
  Gxyz[1] = (double) gy * 250 / 32768;
  Gxyz[2] = (double) gz * 250 / 32768;
}

//Retrieve compass data
void getCompass_Data(void)
{
  I2C_M.writeByte(MPU9150_RA_MAG_ADDRESS, 0x0A, 0x01); //enable the magnetometer
  delay(10);
  I2C_M.readBytes(MPU9150_RA_MAG_ADDRESS, MPU9150_RA_MAG_XOUT_L, 6, buffer_m);

  mx = ((int16_t)(buffer_m[1]) << 8) | buffer_m[0] ;
  my = ((int16_t)(buffer_m[3]) << 8) | buffer_m[2] ;
  mz = ((int16_t)(buffer_m[5]) << 8) | buffer_m[4] ;

  //Mxyz[0] = (double) mx * 1200 / 4096;
  //Mxyz[1] = (double) my * 1200 / 4096;
  //Mxyz[2] = (double) mz * 1200 / 4096;
  Mxyz[0] = (double) mx * 4800 / 8192;
  Mxyz[1] = (double) my * 4800 / 8192;
  Mxyz[2] = (double) mz * 4800 / 8192;
}

//Retrieve compass date
void getCompassDate_calibrated ()
{
  getCompass_Data();
  Mxyz[0] = Mxyz[0] - mx_centre;
  Mxyz[1] = Mxyz[1] - my_centre;
  Mxyz[2] = Mxyz[2] - mz_centre;
}
