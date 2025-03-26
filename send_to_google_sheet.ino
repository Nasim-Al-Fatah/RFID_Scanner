#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>


//const uint8_t fingerprint[32] = {0xb6, 0xbb, 0xc4, 0x5b, 0xd1, 0x35, 0x97, 0x6e, 0x0d, 0x2c, 0x8f, 0xb2, 0x13, 0x02, 0x8b, 0x4b, 0xa6, 0x51, 0xbf, 0xf9, 0xcf, 0x7d, 0x26, 0x30, 0x4c, 0x59, 0xb6, 0xd4, 0xed, 0x0f, 0x90, 0x0f};
// b6 bb c4 5b d1 35 97 6e 0d 2c 8f b2 13 02 8b 4b a6 51 bf f9 cf 7d 26 30 4c 59 b6 d4 ed 0f 90 0f

#define RST_PIN  D3         // Configurable, see typical pin layout above
#define SS_PIN   D4        // Configurable, see typical pin layout above
#define BUZZER_PIN D2     // Configurable, see typical pin layout above
#define LED_PIN D1       // The ESP8266 pin D1 connected to resistor

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
ESP8266WiFiMulti WiFiMulti;
MFRC522::StatusCode status;      


int blockNum = 2;  

/* Legthn of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData[18];

String data2;
const String data1 = "_WebAppLink_?name=";

void setup() 
{
 
  Serial.begin(9600);
  // Serial.setDebugOutput(true);

  Serial.println();

  /* Set BUZZER & LED as OUTPUT */
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  WiFi.begin("Wifi_name", "Password");
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print("..");
    delay(200);
  }
  Serial.println();
  Serial.println("NodeMCU is Connected!");
  Serial.println(WiFi.localIP());

  SPI.begin();
  digitalWrite(LED_PIN, HIGH);
  
}

void loop()
{
  mfrc522.PCD_Init();
  /* Look for new cards, Reset the loop if no new card is present on RC522 Reader */

  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  /* Read data from the same block */
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));
  ReadDataFromBlock(blockNum, readBlockData);
  /* If you want to print the full memory dump, uncomment the next line */
  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  
  Serial.println();
  Serial.print(F("Last data in RFID:"));
  Serial.print(blockNum);
  Serial.print(F(" --> "));
  for (int j=0 ; j<16 ; j++)
  {
    Serial.write(readBlockData[j]);
  }
  Serial.println();

  tone(BUZZER_PIN, 20, 500);
  delay(500);
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
  
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) 
  {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    //client->setFingerprint(fingerprint);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    client->setInsecure();

    data2 = data1 + String((char*)readBlockData);
    data2.trim();
    Serial.println(data2);
    
    HTTPClient https;
    Serial.print(F("[HTTPS] begin...\n"));
    if (https.begin(*client, (String)data2))
    {  
      Serial.print(F("[HTTPS] GET...\n"));
      int httpCode = https.GET();
    
      if (httpCode > 0) 
      {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        
        tone(BUZZER_PIN, 20, 1000);
        delay(500);
        noTone(BUZZER_PIN);
        digitalWrite(LED_PIN, HIGH);
        
      }
      else 
      {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
      delay(500);
    } 
    else 
    {
      Serial.printf("[HTTPS} Unable to connect\n");
    }
  }
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
     bufferLen = 18;
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  else
  {
    Serial.println("Authentication success");
  }

  /* Reading data from the same Block */
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK)
  {
    bufferLen = 18;
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Block was read successfully");  
  }
}
