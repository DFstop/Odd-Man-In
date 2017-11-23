/*
   Sends instant messages via pubnub based on a buttonpress

  Button 1 lights up LED 1,
  Button 2 lights up LED 2,
  Button 3 lights up LED 3,



*/
#include <ArduinoJson.h>
#include <SPI.h>

#include <WiFi101.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>

//OCADU Wifi credentials below
static char ssid[] = "ocadu-embedded";      //SSID of the wireless network
static char pass[] = "internetofthings";    //password of that network
int status = WL_IDLE_STATUS;                // the Wifi radio's status


//Chris's pubnub account keys below
const static char pubkey[] = "pub-c-31315745-29a7-4863-bc2a-97a528df3c93";  //get this from your PUbNub account
const static char subkey[] = "sub-c-ac854de4-c885-11e7-9695-d62da049879f";  //get this from your PubNub account

//COuld have user1 publishing to channel1, and listening to channel 2, with user2 publishing to channel 2, listening to channel 1
const static char pubChannel[] = "channel1"; //choose a name for the channel to publish messages to
const static char subChannel[] = "channel1"; //choose a name for the channel to publish messages to

int ledPin1 = 10;
int ledPin2 = 11;
int ledPin3 = 12; //13 rather than 12 because pin 13 is already connected to the onboard LED

int buttonPin1 = 5;
int buttonPin2 = 6;
int buttonPin3 = 9;
int buttonPrev1 = 1;
int buttonPrev2 = 1;
int buttonPrev3 = 1;
int buttonVal1;
int buttonVal2;
int buttonVal3;

unsigned long lastRefresh = 0;
int publishRate = 2000;

int sensorPin1 = A0;
int myMsg;
int yourMsg;

void setup()
{
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  Serial.begin(9600);
  connectToServer();
}

void loop()
{
  
  buttonVal1 = digitalRead(buttonPin1);
  buttonVal2 = digitalRead(buttonPin2);
  buttonVal3 = digitalRead(buttonPin3);

  if ((buttonVal1 == 0) && (buttonPrev1 == 1)) //trigger the feed update with a button, uses both current and prev value to only change on the switch
  {
    myMsg = 1;
    publishToPubNub(myMsg);
  }
  if ((buttonVal2 == 0) && (buttonPrev2 == 1)) //trigger the feed update with a button, uses both current and prev value to only change on the switch
  {
    myMsg = 2;
    publishToPubNub(myMsg);
  }
  if ((buttonVal3 == 0) && (buttonPrev3 == 1)) //trigger the feed update with a button, uses both current and prev value to only change on the switch
  {
    myMsg = 3;
    publishToPubNub(myMsg);
  }

  if (millis() - lastRefresh >= publishRate) //theTimer to trigger the reads
  {
    readFromPubNub();
    Serial.print("message: ");
    Serial.println();

  switch (myMsg) {
    case 0:
      
      break;
    case 1:
      analogWrite(ledPin1, HIGH);
      analogWrite(ledPin2, LOW);
      analogWrite(ledPin3, LOW);      
      break;
    case 2:
      analogWrite(ledPin1, LOW);
      analogWrite(ledPin2, HIGH);
      analogWrite(ledPin3, LOW);
      break;
    case 3:
      analogWrite(ledPin1, LOW);
      analogWrite(ledPin2, LOW);
      analogWrite(ledPin3, HIGH);
      break;
  }
  
    
//    Serial.print("randoVal2 ");
//    Serial.println(yourVal2);

    lastRefresh = millis();
  }

  buttonPrev1 = buttonVal1; //store the value of this cycle to compare next loop
  buttonPrev2 = buttonVal2; 
  buttonPrev3 = buttonVal3; 
}


void connectToServer()
{
  WiFi.setPins(8, 7, 4, 2); //This is specific to the feather M0

  status = WiFi.begin(ssid, pass);                    //attempt to connect to the network
  Serial.println("***Connecting to WiFi Network***");


  for (int trys = 1; trys <= 10; trys++)                 //use a loop to attempt the connection more than once
  {
    if ( status == WL_CONNECTED)                        //check to see if the connection was successful
    {
      Serial.print("Connected to ");
      Serial.println(ssid);

      PubNub.begin(pubkey, subkey);                      //connect to the PubNub Servers
      Serial.println("PubNub Connected");
      break;                                             //exit the connection loop
    }
    else
    {
      Serial.print("Could Not Connect - Attempt:");
      Serial.println(trys);

    }

    if (trys == 10)
    {
      Serial.println("I don't this this is going to work");
    }
    delay(1000);
  }
}


void publishToPubNub(int messageNum)
{
  WiFiClient *client;
  StaticJsonBuffer<800> messageBuffer;                    //create a memory buffer to hold a JSON Object
  JsonObject& pMessage = messageBuffer.createObject();    //create a new JSON object in that buffer

  ///the imporant bit where you feed in values
  pMessage["message"] = messageNum;                      //add a new property and give it a value
  pMessage["randoVal2"] = "test";                     //add a new property and give it a value

//  pMessage.prettyPrintTo(Serial);   //uncomment this to see the messages in the serial monitor

  int mSize = pMessage.measureLength() + 1;                   //determine the size of the JSON Message
  char msg[mSize];                                            //create a char array to hold the message
  pMessage.printTo(msg, mSize);                              //convert the JSON object into simple text (needed for the PN Arduino client)

  client = PubNub.publish(pubChannel, msg);                      //publish the message to PubNub

  if (!client)                                                //error check the connection
  {
    Serial.println("client error");
    delay(1000);
    return;
  }

  if (PubNub.get_last_http_status_code_class() != PubNub::http_scc_success)  //check that it worked
  {
    Serial.print("Got HTTP status code error from PubNub, class: ");
    Serial.print(PubNub.get_last_http_status_code_class(), DEC);
  }

  while (client->available())
  {
    Serial.write(client->read());
  }
  client->stop();
  Serial.println("Successful Publish");
}


void readFromPubNub()
{
  StaticJsonBuffer<1200> inBuffer;                    //create a memory buffer to hold a JSON Object
  WiFiClient *sClient = PubNub.history(subChannel, 1);

  if (!sClient) {
    Serial.println("message read error");
    delay(1000);
    return;
  }

  while (sClient->connected())
  {
    while (sClient->connected() && !sClient->available()) ; // wait
    char c = sClient->read();
    JsonObject& sMessage = inBuffer.parse(*sClient);

    if (sMessage.success())
    {
//    sMessage.prettyPrintTo(Serial); //uncomment to see the JSON message in the serial monitor
      yourMsg = sMessage["message"];  //
      Serial.print("message: ");
      Serial.println(yourMsg);
    }
  }
  sClient->stop();
}

