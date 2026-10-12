#include <esp_now.h>
#include <WiFi.h>
#include <pins_arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/**************** OLED Parameters ****************/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32    // 128x32 display
#define OLED_RESET    -1    // No reset pin
#define SCREEN_ADDRESS 0x3C // I2C Address of screen

// Initialize screen using Adafruit library
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);




/**************** Define Variables ****************/
// Telemetry variables
float currentRSSI = -1; // -1 means no data received yet
bool currentObstacle = false; // No obstacle yet

// Pins for Joystick
const int pinX = 1;     // X-axis connected to analog pin
const int pinY = 0;     // Y-axis connected to analog pin
const int pinSW = 3;    // Switch (button)
bool sysOn = false;

//Display timer
unsigned long lastDisplayUpdate = 0;


/**************** Data packet structures ****************/
// Note: Must match the transmitter's struct exactly

// Data packet for direction and sysOn (transmitter -> car)
typedef struct {
  int x;
  int y;
  bool sysOn;
} DataPacket;

// Data packet for RSSI (signal strength) and obstacle recognition (car -> transmitter)
typedef struct {
  float RSSI;
  bool obstacle;
} TelemetryPacket;

DataPacket myData; //Create DataPacket for transmitted data

// Info for communication with car
uint8_t receiverMAC[] = {0x9C, 0x9E, 0x6E, 0x06, 0xE9, 0x34}; //MAC address of car ESP32(uint8_t stands for unsigned 8 bits integer)
esp_now_peer_info_t peerInfo; //Declare peer info structure (pre-defined structure to store information of the transmitter)




/**************** onReceive Function (Function that executes when data is received) ****************/
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  /*
  Note: This function is managed by the esp_now.h library under a callback on the setup function,
  so the arguments are managed by it. These arguments are a structure with info of the transmitter (info), 
  the received data (data), and the length of it (len).
  */

  TelemetryPacket telemetry; //Initialize telemetry structure

  // Copy the received data into memory address of telemetry
  memcpy(&telemetry, data, sizeof(telemetry));

  // Store RSSI and obstacle in global variables
  currentRSSI = telemetry.RSSI; 
  currentObstacle = telemetry.obstacle;
}




/**************** Setup Function ****************/
void setup() {
  Serial.begin(9600);           // Start serial communication
  pinMode(pinSW, INPUT);


  /**************** OLED Initialization ****************/
  /*
  The I2C communication protocol is used to communicate with the OLED screen,
  so in order to use it, the protocol is initialized with Wire.begin(), where
  the first argument is the SDA pin and the seconds is the SCL pin.
  */
  Wire.begin(4,5);

  /*
  Initialize the screen communication with display.begin(), where first argument
  is the screen type and second argument is the address. If it doesn't return true,
  the initialization failed and the program is halted.
  */
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 init failed");
    while (true); // Halt if display not found
  }

  // Clear display using function clearDisplay()
  display.clearDisplay();

  // Refresh screen using display() function
  display.display();

  // Use user-defined function to update screen
  updateDisplay(); // Show "Waiting..." on startup



  /**************** Communication Setup ****************/
  /*
  Note: The ESP-NOW Protocol uses the wifi infrastructure of the board to communicate,
  so it is necessary to set the wifi connection mode the board will be using. ESP-NOW
  works with the Sation (STA) mode, so the following line established that mode. 
  */
  WiFi.mode(WIFI_STA);


  //esp_now_init() returns a confirmation value for the initialization of the ESP-NOW protocol.
  //If it hasn't been initialized (!= ESP_OK), print fail status (For debugging)
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return; //Ends the setup() function execution
  }

  //Setup the callback function when data is received. (Function that executes when data is received)
  esp_now_register_recv_cb(onReceive);


  // Register car as a peer so data can be sent to it
  memcpy(peerInfo.peer_addr, receiverMAC, 6); // Copy transmitterMAC into peer_addr pointer
  peerInfo.channel = 0;  // Set communication channel (0 for simplicity, but subject to change if interference exists)
  peerInfo.encrypt = false; // Set encryption key (false since encryption is not needed)

  // esp_now_add_peer() adds a peer and takes as argument the address to the peer info structure
  // If added succesfully, returns ESP_OK, if not, print debugging message
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

/**************** updateDisplay() function ****************/
void updateDisplay() {
  display.clearDisplay(); // Clears the display
  display.setTextSize(1); // Sets the size of the text to 1pt
  display.setTextColor(SSD1306_WHITE); //Sets the color of the text to white

  display.setCursor(0, 0); //Sets cursor in (0,0) position
  display.print("Signal Strength"); //Print "Signal Strength"

  display.setCursor(0, 9); //Sets cursons in (0,9) position
  if (currentRSSI == -1) { //If RSSI is -1, telemetry has not been received yet
    display.print("Waiting..."); //Display Waiting...
  } else {
    // Print RSSI
    display.print("RSSI: ");
    display.print(currentRSSI, 2); // 2 decimal places
    display.print(" dB");

    // If there's currentObstacle is true, print "Obstacle Detected"
    if(currentObstacle){
      display.setCursor(0, 18);
      display.print("Obstacle Detected!");
    }
    // Else, don't print anything
    else{
      display.setCursor(0, 18);
      display.print("");
    }
  }
  
  // Refresh display 
  display.display();
}

void loop() {
  // Read analog values of joystick (0–4095)
  int xValue = analogRead(pinX);
  int yValue = analogRead(pinY);

  // Read button state
  bool buttonPressed = digitalRead(pinSW);

  // If pressed, run toggle()
  if(buttonPressed){
      //Declare last pressed time and current pressed time
      static unsigned long lastPressed = 0;
      unsigned long pressedTime = millis();
      
      // If last time - current time > 200
      if (pressedTime - lastPressed > 200) { // prevents bouncing button
        sysOn = !sysOn;
      }
      lastPressed = pressedTime;// Store current time
  }

  // Assign x and y values to data structure (the data packet that will be sent)
  myData.x = xValue;
  myData.y = yValue;
  myData.sysOn = sysOn;

  //esp_now_send() sends the data packet.
  //First argument: mac address of receiver
  //Second argument: data to be sent. (uint8_t *) forces the cast of that data in the address as a 8 bit unsigned integer
  //Third argument: size of the data
  esp_now_send(receiverMAC, (uint8_t *)&myData, sizeof(myData));

  // Update the OLED display every 300 milliseconds
  if (millis() - lastDisplayUpdate > 300) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  delay(5);
}