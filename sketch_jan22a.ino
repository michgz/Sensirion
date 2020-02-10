
#include <string.h>

#include <Wire.h>

#include <SD.h>


static unsigned char in_buff[64];


const bool UseSensirion(void) {return true;}



static bool ReadProdName(void)
{
  delay(100);

  const char str[] =
    "\x7E\x00\xD0\x01\x01\x2D\x7E";

  //Serial.flush();
  Serial.write(str, 7);

  unsigned char x [28];

  unsigned int t = Serial.readBytes(x, 20);


  if (t > 0)
  {
    ::memset(in_buff, 0, sizeof(in_buff));
    ::memcpy(in_buff, x, t);
    return true;
  }

  return false;

}

static bool StartMeasurement(void)
{
  delay(100);

  const char str[] =
    "\x7E\x00\x00\x02\x01\x03\xF9\x7E";

  //Serial.flush();
  Serial.write(str, 8);

  unsigned char x [26];

  unsigned int t = Serial.readBytes(x, 20);


  if (t > 0)
  {
    ::memset(in_buff, 0, sizeof(in_buff));
    ::memcpy(in_buff, x, t);
    return true;
  }

  return false;

}


static bool ReadMeasurement(void)
{
  delay(100);

  const char str[] =
    "\x7E\x00\x03\x00\xFC\x7E";

  //Serial.flush();
  Serial.write(str, 6);

  unsigned char x [64];

  unsigned int t = Serial.readBytes(x, 47 + 4); // Extra 4 bytes for byte stuffing


  if (t > 0)
  {
    ::memset(in_buff, 0, sizeof(in_buff));
    ::memcpy(in_buff, x, t);
    return true;
  }

  return false;

}



static void DispReadData(String c3)
{

  char out1[48];


  ::memset(out1, 0, sizeof(out1));

  unsigned int t;


  // Find the last non-zero element
  t = sizeof(in_buff) - 1;
  while (t > 0 && in_buff[t] == 0)
  {
    t --;
  }

  //Serial.flush();
  Serial.println("");
  Serial.print("COMMAND : "); Serial.println(c3);
  Serial.println("Readback from device :");

  int i, j;

  i = 0;
  j = 0;
  while (i <= t && i < 58)
  {
    if ((j + 3 + 2) > sizeof(out1))
    {
      Serial.println(out1);
      j = 0;
    }
    ::sprintf(&out1[j], "%02X ", in_buff[i]);
    j += 3; i ++;
  }

  Serial.println(out1);

}


static bool IsMeasurementData(void)
{
  
  if (in_buff[0] != 0x7E) return false;
  if (in_buff[1] != 0x00) return false;
  if (in_buff[2] != 0x03) return false;
  if (in_buff[3] != 0x00) return false;
  if (in_buff[4] < 16) return false;

  // First check the checksum
  uint8_t csum;
  int i = in_buff[4];
  uint8_t * a = &in_buff[5];
  uint8_t * b = &in_buff[5];
  uint16_t sum = 0x00 + 0x03 + 0x00 + i;


  while(i > 0)
  {
    uint8_t c = *b;
    b ++;
    if (c == 0x7D)
    {
      c = (*b) ^ 0x20;
      b ++;
    }
    sum += (uint16_t)c;
    i --;
  }

  csum = *b;
  b ++;
  if (csum == 0x7D)
  {
    csum  = (*b) ^ 0x02;
    b ++;
  }

  if (*b != 0x7E) return false;
  b ++;

  if (csum != (0xFF ^ ((uint8_t) (sum & 0x00FF))))
  {
    //Serial.print("Calculated sum = 0x");
    //Serial.println(sum, HEX);
    //Serial.print("Received sum = 0x");
    //Serial.println(csum, HEX);
    return false;
  }

  // Now do an in-place resolution of byte stuffing
  i = in_buff[4];
  a = &in_buff[5];
  b = &in_buff[5];
  while(i > 0)
  {
    uint8_t c = *b;
    b ++;
    if (c == 0x7D)
    {
      c = (*b) ^ 0x20;
      b ++;
    }
    *a = c;
    a ++;
    i --;
  }

  // Do checksum & end of packet
  *a = csum;
  a ++;
  *a = 0x7E;
  a ++;

  // TODO: pad to end with 0x00's

  return true;
}


static void DispReadMeasurementData(char * out, const int len)
{
  if (IsMeasurementData())
  {


    int i;
    int j = 0;
    int t = in_buff[4] / 4;

    ::memset(out, 0, len);

    i = 0;
    while (i < t && i < 10)
    {
      char vv [20];
      ::memset(vv, 0, sizeof(vv));
      
      unsigned char ffs [8];
      ffs[0] = in_buff[5 + 4 * i + 3];
      ffs[1] = in_buff[5 + 4 * i + 2];
      ffs[2] = in_buff[5 + 4 * i + 1];
      ffs[3] = in_buff[5 + 4 * i + 0];
      float ff = *((float *) ffs);
      dtostrf(ff, 3, 6, vv);
      ::strcpy(&out[j], vv);
      j += ::strlen(vv);
      out[j++] = ','; 
      if (j > (len - 16)) break;
      i ++;
    }

    //Serial.flush();
    //Serial.println("ABCD");
    //Serial.println("ABCD");
    //Serial.println(out);

  }
  else
  {
    // Fall back to hex output

    int i;
    int j = 0;
    int t = in_buff[4];

    ::memset(out, 0, len);

    i = 0;
    while (i < t)
    {
      ::sprintf(&out[j], "%02X ", in_buff[5 + i]);
      j += 3; 
      if (j > (len - 16)) break;
      i ++;
    }

  }

}


#define DS1307_ADDRESS  0x68

void setup() {
  // put your setup code here, to run once:

#if 0   // Change to 1 to set RTC.

  delay(1000);

  Wire.begin();
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x00);   // Start address
  Wire.write(0x00);   // Seconds in BCD
  Wire.write(0x45);   // Minutes in BCD
  Wire.write(0x21);   // Hours (24-hour format) in BCD
  Wire.write(0x01);   // Day-of-the-week. I think it doesn't matter what is put here
  Wire.write(0x01);   // Day-of-the-month in BCD.
  Wire.write(0x01);   // Month in BCD
  Wire.write(0x20);   // Year in BCD
  Wire.endTransmission();

  while(true);

#endif // 0


  Serial.begin(115200);
  while (!Serial) ;

  Serial.println("Starting...");


  pinMode(13, OUTPUT);

  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);
  delay(1000);
  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);



  delay(2000);


  if (UseSensirion())
  {

    int j = 3;

    while (j > 0)
    {
      if (ReadProdName())
      {
        digitalWrite(13, HIGH);
  
        delay(500);
  
        DispReadData("Product Name");
  
        break;  // Succeeded. Now can move on with other things
      }
  
      delay(2000);
      digitalWrite(13, LOW);
      j --;
    }
  
    delay (3000);
    digitalWrite(13, LOW);
  
    if (StartMeasurement())
    {
      digitalWrite(13, HIGH);
  
      delay(500);
  
      DispReadData("Start Measurement");
    }
    else
    {
      // Failed. Ignore
    }

  }

  delay (3000);
  digitalWrite(13, LOW);

}

const int chipSelect = 10;
const int cardDetect = 7;  // Unused. Not sure if it's right..

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }


static void DoOne(void)
{

  // Get the current time
  
  unsigned char rtc [6];

  Wire.begin();
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  rtc[0] = bcd2bin(Wire.read() & 0x7F);   // Seconds
  rtc[1] = bcd2bin(Wire.read());   // Minutes
  rtc[2] = bcd2bin(Wire.read());   // Hours
  (void) Wire.read();   // discard day-of-week
  rtc[3] = bcd2bin(Wire.read());   // Day
  rtc[4] = bcd2bin(Wire.read());   // Month
  rtc[5] = bcd2bin(Wire.read());   // Year

  char c1 [32] = "                    ";
    
  ::sprintf(c1, "%02d/%02d/%04d %02d:%02d:%02d,", rtc[3], rtc[4], rtc[5] + 2000, rtc[2], rtc[1], rtc[0]);
  Serial.print("Current time : ");
  Serial.println(c1);


  // Read from the Sensirion
  char out[128];

  if (UseSensirion())
  {
    if (ReadMeasurement())
    {
      DispReadMeasurementData(out, 128);
  
      digitalWrite(13, HIGH);

    }
    else
    {
      ::strcpy(out, "Can't read");
    }
  }
  else
  {
    ::strcpy(out, "- Empty -");
  }


  Serial.println("String to write :");
  Serial.println(out);


  // Write to the SD card
  
  SD.begin(chipSelect);

  File dataFile;

  {
    char c2 [16];
    ::sprintf(c2, "%04d%02d%02d.CSV", 2000 + rtc[5], rtc[4], rtc[3]);

    Serial.print("Opening file : ");
    Serial.println(c2);
    dataFile = SD.open(c2, FILE_WRITE);
  }

  if (dataFile) {
    dataFile.print(c1);
    dataFile.println(out);
    
    dataFile.close();
    Serial.println("Wrote to file");
  }
  else
  {
    Serial.println("Could not open file");
  }

  
}


void loop() {
  // put your main code here, to run repeatedly:

  DoOne();

  delay (500);
  digitalWrite(13, LOW);

  delay(4500);
}
