#include <LGPRS.h>
#include <LGPRSClient.h>
#include <LGPRSServer.h>
#include <PubSubClient.h>
#include <LGPS.h>


LGPRSClient GPRSclient;

#define PUBLISH_TOPICNAME "MyAsset"
#define ASSET_RFID "123456789abcdef123456789"
#define mqttBroker "iot.eclipse.org"
#define MQTTportNumber 1883
#define CLIENT_ID "Link It ONE CLIENT 1"

#define TIME_DELAY_BETWEEN_POSTS 60000u

static PubSubClient mqttClient(GPRSclient);
static unsigned long lastSend;
static gpsSentenceInfoStruct info;
static char buff[256];
static double latitude;
static double longitude;
static int tmp, hour, minute, second, num ;

void setup()
{
  // setup Serial port
  delay(5000);
  Serial.begin(115200);
  LGPS.powerOn();
  Serial.println("LGPS Power on, and waiting ...");
  delay(3000);
  Serial.println("Starting. Trying to connect");

  // while (!LGPRS.attachGPRS("TATA.DOCOMO.INTERNET","",""))
  while (!LGPRS.attachGPRS())
  {
    delay(500);
    Serial.println("Attach to GPRS network by auto-detect APN setting");
  }



  // if you get a connection, report back via serial:
  Serial.print("Connect to ");
  Serial.println(mqttBroker);

  mqttClient.setServer( mqttBroker, MQTTportNumber );
  mqttClient.setCallback( callback );

  lastSend = 0;

}

void loop()
{
  if ( millis() - lastSend > TIME_DELAY_BETWEEN_POSTS )
  { // Send an update only after 5 seconds
    if ( !mqttClient.connected() )
    {
      reconnectToBroker();
    }
    publishStatus(TRUE, PUBLISH_TOPICNAME);
    lastSend = millis();
    delay(5000);
    disconnectFromBroker();
  }

}

void disconnectFromBroker()
{
  if (mqttClient.connected() == TRUE)
  {
    Serial.println("Disconnecting from broker");
    mqttClient.disconnect();
  }
}

void reconnectToBroker()
{
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker ...");
    // Attempt to connect
    if ( mqttClient.connect(CLIENT_ID) )
    { // Better use some random name
      Serial.println( "[DONE]" );
      // Publish a message on topic "outTopic"
      mqttClient.publish( "outTopic", "Hello, This is LinkIt One" );
      // Subscribe to topic "inTopic"
      mqttClient.subscribe( "inTopic" );
    }
    else
    {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( mqttClient.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

void callback( char* topic, byte* payload, unsigned int length ) {
  Serial.print( "Recived message on Topic:" );
  Serial.print( topic );
  Serial.print( "    Message:");
  for (int i = 0; i < length; i++) {
    Serial.print( (char)payload[i] );
  }
  Serial.println();
}
void parseGPGGA(const char* GPGGAstr)
{
  /* Refer to http://www.gpsinformation.org/dale/nmea.htm#GGA
   * Sample data: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
   * Where:
   *  GGA          Global Positioning System Fix Data
   *  123519       Fix taken at 12:35:19 UTC
   *  4807.038,N   Latitude 48 deg 07.038' N
   *  01131.000,E  Longitude 11 deg 31.000' E
   *  1            Fix quality: 0 = invalid
   *                            1 = GPS fix (SPS)
   *                            2 = DGPS fix
   *                            3 = PPS fix
   *                            4 = Real Time Kinematic
   *                            5 = Float RTK
   *                            6 = estimated (dead reckoning) (2.3 feature)
   *                            7 = Manual input mode
   *                            8 = Simulation mode
   *  08           Number of satellites being tracked
   *  0.9          Horizontal dilution of position
   *  545.4,M      Altitude, Meters, above mean sea level
   *  46.9,M       Height of geoid (mean sea level) above WGS84
   *                   ellipsoid
   *  (empty field) time in seconds since last DGPS update
   *  (empty field) DGPS station ID number
   *  *47          the checksum data, always begins with *
   */

  if (GPGGAstr[0] == '$')
  {
    tmp = getComma(1, GPGGAstr);
    hour     = (GPGGAstr[tmp + 0] - '0') * 10 + (GPGGAstr[tmp + 1] - '0');
    minute   = (GPGGAstr[tmp + 2] - '0') * 10 + (GPGGAstr[tmp + 3] - '0');
    second    = (GPGGAstr[tmp + 4] - '0') * 10 + (GPGGAstr[tmp + 5] - '0');

    sprintf(buff, "UTC timer %2d-%2d-%2d", hour, minute, second);
    Serial.println(buff);

    tmp = getComma(2, GPGGAstr);
    latitude = getDoubleNumber(&GPGGAstr[tmp]);


    tmp = getComma(4, GPGGAstr);
    longitude = getDoubleNumber(&GPGGAstr[tmp]);
    longitude /= 100;
    latitude /= 100;

    longitude -= 0.22;
    latitude += 0.357147;


    sprintf(buff, "Co-ordinates %10.4f, %10.4f", latitude, longitude);
    Serial.println(buff);

    tmp = getComma(7, GPGGAstr);
    num = getIntNumber(&GPGGAstr[tmp]);
    sprintf(buff, "satellites number = %d", num);
    Serial.println(buff);
  }
  else
  {
    Serial.println("Not get data");
  }
}


static unsigned char getComma(unsigned char num, const char *str)
{
  unsigned char i, j = 0;
  int len = strlen(str);
  for (i = 0; i < len; i ++)
  {
    if (str[i] == ',')
      j++;
    if (j == num)
      return i + 1;
  }
  return 0;
}

static double getDoubleNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;

  i = getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev = atof(buf);
  return rev;
}

static double getIntNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;

  i = getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev = atoi(buf);
  return rev;
}



void publishStatus(bool parkingLotStatus, const char *topic)
{


  String payload;
  payload = ASSET_RFID;
  Serial.println( "Publishing Status as :");
  LGPS.getData(&info);
  parseGPGGA((const char*)info.GPGGA);


  payload += ":";
  payload += latitude;
  payload += ",";
  payload += longitude;
  payload += ":";
  payload += num;

  // Send payload
  char convData[500];
  payload.toCharArray( convData, 500 );
  mqttClient.publish( topic , convData );
}



