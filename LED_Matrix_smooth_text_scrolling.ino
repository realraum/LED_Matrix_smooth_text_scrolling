#include "OctoWS2811.h"
#include "font.h"
#include <SPI.h>
#include <UIPEthernet.h>
#include <PubSubClient.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

IPAddress myIP(192,168,0,6);
IPAddress mqttServer(192,168,0,7);
uint16_t mqttPort = 1883;

EthernetClient ethClient;
EthernetUDP udp;

PubSubClient mqttClient;


//fixed values for octoWS2811
const int ledsPerStrip = 20;
const int rows = 8;

DMAMEM int displayMemory[ledsPerStrip * rows];
int drawingMemory[ledsPerStrip * rows];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);


//scrolling text message
char msg[] = "r3.at MakerFaireVienna r3.at realraum.at OpenBioLab Graz OLGA realraum Graz Fun Holz Elektronik Chemie Molekular Biologie Funk  ";
#define LENMSG 128
#define ZUSATZABSTAND 1

//number of colors. Each word gets sequentially colour in a new colour
//First word in msg[] gets first colour, etc
int Colors[][3] = {{0xFF,0x00,0x00},{0x00,0xFF,0x00},{0x00,0x00,0xFF},{0xAA,0x00,0xAA},{0xAA,0xAA,0x00},{0x00,0xAA,0xAA}};
#define NUMCOLORS 6

int OFF = 0x000000;

//DoubleFramebuffer
uint8_t field[rows][ledsPerStrip][3];

void initEth() {
//  Ethernet.begin(mac);

  if (Ethernet.begin(mac) == 0) {
    //TODO: Reset the client  
//    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
//    for (;;)
//      ;
  }
//  Serial.print("localIP: ");
//  Serial.println(Ethernet.localIP());
//  Serial.print("subnetMask: ");
//  Serial.println(Ethernet.subnetMask());
//  Serial.print("gatewayIP: ");
//  Serial.println(Ethernet.gatewayIP());
//  Serial.print("dnsServerIP: ");
//  Serial.println(Ethernet.dnsServerIP());
}

void setup() {
  leds.begin(); // start the octows2811 library
  delay(200);

  //clear screen
  for (int i = 0; i < leds.numPixels(); i++)
  {
    leds.setPixel(i, OFF);
  }
  leds.show();
  
  //init buffer with 0
  for (int a=0; a<rows; a++)
  {
    for (int b=0; b<ledsPerStrip; b++)
    {
      for (int c=0; c<3; c++)
      {
        field[a][b][c]=OFF;
      }
    }
  }

  initEth();

  // setup mqtt client
  mqttClient.setClient(ethClient);
  mqttClient.setServer(mqttServer, mqttPort);
} //==end of setup()==//


void loop() {
  texttest(msg);
} //==end of loop()==//


void texttest(char msg[])
{
  uint8_t curent_word_color=0;

  //==Start of message setup==//
  for (int charIndex = 0; charIndex < (LENMSG - 1); charIndex++) //start of charIndex setup
  {
    int alphabetIndex = msg[charIndex] - ' ';
    if (alphabetIndex < 0)
    {
      alphabetIndex = 0;
    }

    if (msg[charIndex] == ' ')
    {
      curent_word_color++;
      curent_word_color %= NUMCOLORS;
    }

    for (int col = 0; col < 6+ZUSATZABSTAND; col++)
    {
      for (int row = 0; row < rows ; row++)
      {
        if (col < 5 && bitRead( alphabets[alphabetIndex][col], row ) == 1)
        {
          field[row][0][0] = Colors[curent_word_color][0];
          field[row][0][1] = Colors[curent_word_color][1];
          field[row][0][2] = Colors[curent_word_color][2];
        }
        else
        {
          field[row][0][0] = 0x0;
          field[row][0][1] = 0x0;
          field[row][0][2] = 0x0;
        }
      }

      //-- > 10x 1 / 10 weitereschieben ohne das feld zu verändern

      for(int i=0;i<255;i+=3)
      {
        for (int a = 0; a < rows; a++)
        {
          for (int b = 1; b <ledsPerStrip-1; b++)
          {
            int a1 = a * ledsPerStrip;
            leds.setPixel(
              ((a1 + b) - 1),
                ((constrain((field[a][ledsPerStrip-b-1][0])*i + (255-i)*(field[a][ledsPerStrip-b][0]),0,255))   )
              | ((constrain((field[a][ledsPerStrip-b-1][1])*i + (255-i)*(field[a][ledsPerStrip-b][1]),0,255))<<8)
              | ((constrain((field[a][ledsPerStrip-b-1][2])*i + (255-i)*(field[a][ledsPerStrip-b][2]),0,255))<<16)
            );
          }
        }
        //blend pixels from on LED to the next as fast as possible. e.g. without delay
        leds.show();
      }

      //==START OF THE SCROLL METHOD==//
      for (int a = 0; a < rows; a++)
      {
        for (int b = ledsPerStrip - 1; b >= 1; b--)
        {
          field[a][b][0] = field[a][b - 1][0];
          field[a][b][1] = field[a][b - 1][1];
          field[a][b][2] = field[a][b - 1][2];
        }
      } //end of Scroll Method
    } //end of Set Method
  } //end of message setup
} //end of texttest

//------- HELPER FUNCTION------//
// Create a 24 bit color value from R,G,B ////these strips are BRG
unsigned int Color(uint8_t r, uint8_t g, uint8_t b)
{
  //Take the lowest 8 bits of each value and append them end to end
  return ( ((uint32_t)g ) << 16 | ((uint32_t)b ) << 8 | (uint32_t)r );
}
