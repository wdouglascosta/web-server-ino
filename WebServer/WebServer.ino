#include <dht.h>


#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   60

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39                                                                                                                                                                                                                                                                                                                                                                                                                                           
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978


int speakerPin = 5;


// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 20); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
File webFile;               // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
boolean LED_state[2] = {0}; // stores the states of the LEDs

dht DHT; // Cria um objeto da classe dht
uint32_t timer = 0;

void setup()
{
    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    
    Serial.begin(9600);       // for debugging
    
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");
    // switches
    pinMode(2, INPUT);
    pinMode(3, INPUT);
    // LEDs
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);
    
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients

    pinMode(speakerPin, OUTPUT);  
}

void loop()
{
  dhtSensor();
    EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        SetLEDs();
                        // send XML file containing input states
                        XML_response(client);
                    }
                    else {  // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send web page
                        webFile = SD.open("index.htm");        // open web page file
                        if (webFile) {
                            while(webFile.available()) {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }
                    // display received HTTP request on serial port
                    Serial.print(HTTP_req);
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
    
    // read buttons and debounce
    ButtonDebounce();
}

// function reads the push button switch states, debounces and latches the LED states
// toggles the LED states on each push - release cycle
// hard coded to debounce two switches on pins 2 and 3; and two LEDs on pins 6 and 7
// function adapted from Arduino IDE built-in example:
// File --> Examples --> 02.Digital --> Debounce
void ButtonDebounce(void)
{
    static byte buttonState[2]     = {LOW, LOW};   // the current reading from the input pin
    static byte lastButtonState[2] = {LOW, LOW};   // the previous reading from the input pin
    
    // the following variables are long's because the time, measured in miliseconds,
    // will quickly become a bigger number than can be stored in an int.
    static long lastDebounceTime[2] = {0};  // the last time the output pin was toggled
    long debounceDelay = 50;         // the debounce time; increase if the output flickers
  
    byte reading[2];
    
    reading[0] = digitalRead(2);
    reading[1] = digitalRead(3);
    
    for (int i = 0; i < 2; i++) {
        if (reading[i] != lastButtonState[i]) {
            // reset the debouncing timer
            lastDebounceTime[i] = millis();
        }
      
        if ((millis() - lastDebounceTime[i]) > debounceDelay) {
            // whatever the reading is at, it's been there for longer
            // than the debounce delay, so take it as the actual current state:
        
            // if the button state has changed:
            if (reading[i] != buttonState[i]) {
                buttonState[i] = reading[i];
          
                // only toggle the LED if the new button state is HIGH
                if (buttonState[i] == HIGH) {
                    LED_state[i] = !LED_state[i];
                }
            }
        }
    } // end for() loop
    
    // set the LEDs
    digitalWrite(12, LED_state[0]);
    digitalWrite(13, LED_state[1]);
      
    // save the reading.  Next time through the loop,
    // it'll be the lastButtonState:
    lastButtonState[0] = reading[0];
    lastButtonState[1] = reading[1];
}

// checks if received HTTP request is switching on/off LEDs
// also saves the state of the LEDs
void SetLEDs(void)
{
    // LED 1 (pin 6)
    if (StrContains(HTTP_req, "LED1=1")) {
        LED_state[0] = 1;  // save LED state
        digitalWrite(12, HIGH);
        //song();
    }
    else if (StrContains(HTTP_req, "LED1=0")) {
        LED_state[0] = 0;  // save LED state
        digitalWrite(12, LOW);
    }
    // LED 2 (pin 7)
    if (StrContains(HTTP_req, "LED2=1")) {
        LED_state[1] = 1;  // save LED state
        digitalWrite(13, HIGH);
        //song1();
    }
    else if (StrContains(HTTP_req, "LED2=0")) {
        LED_state[1] = 0;  // save LED state
        digitalWrite(13, LOW);
    }
}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{
    int analog_val;            // stores value read from analog inputs
    int count;                 // used by 'for' loops
    int sw_arr[] = {2, 3};  // pins interfaced to switches
    
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    // checkbox LED states
    // LED1
        cl.print("<TEMP>");
        cl.print(DHT.temperature);
        cl.print("</TEMP>");

        cl.print("<HUM>");
        cl.print(DHT.humidity);
        cl.print("</HUM>");
    
    cl.print("<LED>");
    if (LED_state[0]) {
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
    cl.println("</LED>");
    // button LED states
    // LED3
    cl.print("<LED>");
    if (LED_state[1]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</LED>");
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

void dhtSensor(){
  if(millis() - timer>= 2000)
  {
 
    DHT.read11(A1); // chama método de leitura da classe dht,
                    // com o pino de transmissão de dados ligado no pino A1
 
    // Exibe na serial o valor de umidade
    Serial.print(DHT.humidity);
    Serial.println(" %");
 
    // Exibe na serial o valor da temperatura
    Serial.print(DHT.temperature);
    Serial.println(" Celsius");
 
    timer = millis(); // Atualiza a referência
  }
}
void song1(){
  int X = 300;
  int Y = X/2;
  beep(speakerPin, NOTE_G5, X);
  beep(speakerPin, NOTE_C5, X);
  beep(speakerPin, NOTE_E5, Y);
  beep(speakerPin, NOTE_F5, Y);
  
  beep(speakerPin, NOTE_G5, X);
  beep(speakerPin, NOTE_C5, X);
  beep(speakerPin, NOTE_E5, Y);
  beep(speakerPin, NOTE_F5, Y);

  beep(speakerPin, NOTE_G5, X);
  beep(speakerPin, NOTE_C5, X);
  beep(speakerPin, NOTE_E5, Y);
  beep(speakerPin, NOTE_F5, Y);
  
  beep(speakerPin, NOTE_G5, X);
  beep(speakerPin, NOTE_C5, X);
  beep(speakerPin, NOTE_E5, Y);
  beep(speakerPin, NOTE_F5, Y);

  beep(speakerPin, NOTE_G5, X);
  beep(speakerPin, NOTE_C5, X);
  beep(speakerPin, NOTE_DS5, Y);
  beep(speakerPin, NOTE_F5, Y);

  beep(speakerPin, NOTE_G5, X);
  beep(speakerPin, NOTE_C5, X);
  beep(speakerPin, NOTE_DS5, Y);
  beep(speakerPin, NOTE_F5, Y);

  beep(speakerPin, NOTE_G5, X);
  beep(speakerPin, NOTE_C5, X);
  beep(speakerPin, NOTE_DS5, Y);
  beep(speakerPin, NOTE_F5, Y);

  beep(speakerPin, NOTE_G5, X*3);
  beep(speakerPin, NOTE_C5, X*3);
  beep(speakerPin, NOTE_E5, X/2);
  beep(speakerPin, NOTE_F5, X/2);
  beep(speakerPin, NOTE_G5, X*2);
  beep(speakerPin, NOTE_C5, X*2);
  beep(speakerPin, NOTE_E5, X/2);
  beep(speakerPin, NOTE_F5, X/2);
  beep(speakerPin, NOTE_D5, X);

  beep(speakerPin, NOTE_G4, X);
  beep(speakerPin, NOTE_B4, X);
  beep(speakerPin, NOTE_C5, Y);
  beep(speakerPin, NOTE_D5, Y);

    beep(speakerPin, NOTE_G4, X);
  beep(speakerPin, NOTE_B4, X);
  beep(speakerPin, NOTE_C5, Y);
  beep(speakerPin, NOTE_D5, Y);

    beep(speakerPin, NOTE_G4, X);
  beep(speakerPin, NOTE_B4, X);
  beep(speakerPin, NOTE_C5, Y);
  beep(speakerPin, NOTE_D5, Y);

    beep(speakerPin, NOTE_G4, X);
  beep(speakerPin, NOTE_B4, X);
  beep(speakerPin, NOTE_C5, Y);
  beep(speakerPin, NOTE_D5, Y);

    beep(speakerPin, NOTE_F5, X*3);
  beep(speakerPin, NOTE_B4, X*3);
  beep(speakerPin, NOTE_E5, X/2);
  beep(speakerPin, NOTE_D5, X/2);
  beep(speakerPin, NOTE_F5, X*2);
  beep(speakerPin, NOTE_B5, X*2);
  beep(speakerPin, NOTE_E5, X/2);
  beep(speakerPin, NOTE_D5, X/2);
  beep(speakerPin, NOTE_C4, X*3);

  
  delay(1800);
  
}

void beep (unsigned char speakerPin, int frequencyInHertz, long timeInMilliseconds)  //code for working out the rate at which each note plays and the frequency.
{
  int x;      
  long delayAmount = (long)(1000000/frequencyInHertz);
  long loopTime = (long)((timeInMilliseconds*1000)/(delayAmount*2));
  for (x=0;x<loopTime;x++)    
  {    
    digitalWrite(speakerPin,HIGH);
    delayMicroseconds(delayAmount);
    digitalWrite(speakerPin,LOW);
    delayMicroseconds(delayAmount);
  }    

}        

 
void song()  //here is where all the notes for the song are played.
{        
  int X = 130;
  int Y = X*2;
  int x = 0;
  beep(speakerPin, NOTE_G5 - x, X); 
  beep(speakerPin, NOTE_D6 - x, X); 
  beep(speakerPin, NOTE_E6 - x, X);
  beep(speakerPin, NOTE_D6 - x, Y*2);
  delay(Y);
  beep(speakerPin, NOTE_G5 - x, X); 
  beep(speakerPin, NOTE_D6 - x, X); 
  beep(speakerPin, NOTE_E6 - x, X);
  beep(speakerPin, NOTE_D6 - x, Y*2);
  delay(Y);
  beep(speakerPin, NOTE_C6 - x, X);
  beep(speakerPin, NOTE_C6 - x, X);
  delay(20);
  beep(speakerPin, NOTE_C6 - x, X);
  beep(speakerPin, NOTE_C6 - x, X);
  delay(20);
  beep(speakerPin, NOTE_C6 - x, X);
  beep(speakerPin, NOTE_C6 - x, X);
  delay(20);
  beep(speakerPin, NOTE_D6 - x, X);
  beep(speakerPin, NOTE_E6 - x, Y);
  delay(30);
  beep(speakerPin, NOTE_D6 - x, Y);
  delay(30);
  beep(speakerPin, NOTE_G5 - x, Y);
  delay(30);
  beep(speakerPin, NOTE_E5 - x, Y);
  delay(30);
  beep(speakerPin, NOTE_G5 - x, Y);
  delay(500);
  
  
  
  
}
