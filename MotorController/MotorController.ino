#include <pins_arduino.h>
#include <esp_now.h>
#include <WiFi.h>

/**************** Define pins ****************/
#define MLEFT_F 8 //left forward direction
#define MLEFT_B 9 //left backwards direction
#define MRIGHT_F 3 //right forward direction
#define MRIGHT_B 2 //right backwards direction
#define IR_SENSOR 5 //Distance sensor
#define redPin 1 //Red LED
#define bluePin 0 //Blue LED
#define buzzerPin 4 //Passive Buzzer




/**************** Define Variables ****************/
// LED and Buzzer 
const int interval = 75;      //Interval for LED and Buzzer. Defines frequency of turning blue and red
unsigned long prevMillis = 0; //Variable to store miliseconds
int i = 0;                    //Counter
bool sysOn = false;           //Bool to activate or deactivate LEDs and Buzzer

// Motor settings
const int DEADZONE_LOW = 400;   //Lower deadzone for joystick
const int DEADZONE_HIGH = 3700; //Upper deadzone for joystick
const int MAX_SPEEDL = 255;     //Max speed left wheel
const int MAX_SPEEDR = 255;     //Max speed right wheel
const int TURN_SPEED = 200;     //Speed for turns (speed that non dominant wheel will have)
const int HARD_TURN_SPEED = 50; //Speed for hard turns (speed that non dominant wheel will have)

// Telemetry variables (RSSI and obstacle)
float lastRSSI = 0;
unsigned long lastTelemetrySend = 0;
bool currentObstacle = true;




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

DataPacket receivedData; //Create DataPacket for received data

// Info for communication with transmitter
uint8_t transmitterMAC[] = {0x9C, 0x9E, 0x6E, 0x06, 0x55, 0x74}; //MAC address of transmitter ESP32 (uint8_t stands for unsigned 8 bits integer)
esp_now_peer_info_t peerInfo; //Declare peer info structure (pre-defined structure to store information of the transmitter)




/**************** onReceive Function (Function that executes when data is received) ****************/
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  /*
  Note: This function is managed by the esp_now.h library under a callback on the setup function,
  so the arguments are managed by it. These arguments are a structure with info of the transmitter (info), 
  the received data (data), and the length of it (len).
  */

  // Copy the received data into memory address of receivedData
  memcpy(&receivedData, data, sizeof(receivedData));

  // Now, receivedData contains the data received in the datapacket

  // Update sysOn with received data
  sysOn = receivedData.sysOn;
  

  // Pass the raw X and Y directly into the direction processor function
  processMovement(receivedData.x, receivedData.y);

  // Get RSSI.
  /*
  Note: the -> operator access members of a structure or object when you have a pointer to it.
  In this case, *info is a pointer to the structure that stores the info of the transmitter, so 
  by using the -> operator we are able to exctract data from the memory address the pointer is 
  pointing to, in this case, the rssi.
  */
  lastRSSI = info->rx_ctrl->rssi; 
}




/**************** Setup Function ****************/
void setup() {
  Serial.begin(9600); //Setup of serial monitor

  // Setup for all pins
  pinMode(MLEFT_F,OUTPUT);
  pinMode(MLEFT_B,OUTPUT);
  pinMode(MRIGHT_F,OUTPUT);
  pinMode(MRIGHT_B,OUTPUT);
  pinMode(IR_SENSOR, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);



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

  // Print this board's MAC (To paste it into sender and for debugging)
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  // Register transmitter as a peer so we can send telemetry back to it
  memcpy(peerInfo.peer_addr, transmitterMAC, 6); // Copy transmitterMAC into peer_addr pointer
  peerInfo.channel = 0; // Set communication channel (0 for simplicity, but subject to change if interference exists)
  peerInfo.encrypt = false; // Set encryption key (false since encryption is not needed)
  

  // esp_now_add_peer() adds a peer and takes as argument the address to the peer info structure
  // If added succesfully, returns ESP_OK, if not, print debugging message
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add transmitter as peer");
    return;
  }
}




/**************** processMovement Function ****************/
void processMovement(int x, int y) {
  /*
  This function process the movement given the x and y values received from the potentiometer.
  */

  // Declare direction of joystick based on deadzones
  bool isUp = y > DEADZONE_HIGH;
  bool isDown = y < DEADZONE_LOW;
  bool isRight = x > DEADZONE_HIGH;
  bool isLeft = x < DEADZONE_LOW;

  //Declare speed of motors as 0.
  int leftMotor = 0;
  int rightMotor = 0;

  /*
  Note: each if will declare the speed of the wheels using speed constants (see motor settins section)
  based on the direction the joystick is pointing
  */
  /**** Diagonals (Gentle arcing turns) ****/
  if (isUp && isRight) { //Northeast
    leftMotor = MAX_SPEEDL;
    rightMotor = TURN_SPEED; 
  } 
  else if (isUp && isLeft) { //Northwest
    leftMotor = TURN_SPEED;
    rightMotor = MAX_SPEEDR;
  } 
  else if (isDown && isRight) { //Southeast
    leftMotor = -MAX_SPEEDL;
    rightMotor = -TURN_SPEED;
  } 
  else if (isDown && isLeft) { //Southwest
    leftMotor = -TURN_SPEED;
    rightMotor = -MAX_SPEEDR;
  } 
  
  /**** Pure Cardinal Directions ****/
  else if (isUp) { //North
    leftMotor = MAX_SPEEDL;
    rightMotor = MAX_SPEEDR;
  } 
  else if (isDown) { //South
    leftMotor = -MAX_SPEEDL;
    rightMotor = -MAX_SPEEDR;
  } 
  else if (isRight) { //East
    leftMotor = MAX_SPEEDL;
    rightMotor = HARD_TURN_SPEED;
  } 
  else if (isLeft) { //West
    leftMotor = HARD_TURN_SPEED;
    rightMotor = MAX_SPEEDR;
  } 
  
  /**** Center Deadzone ****/
  else {
    leftMotor = 0;
    rightMotor = 0;
  }

  // Once speed is declared, send to motor driver
  MotorControl(leftMotor, rightMotor);
}




/**************** MotorControl Function ****************/
void MotorControl(int speedLeft, int speedRight){
  /*
  Sets the speed of the right and left motor based on the speed given
  (negatives mean counterclockwise)
  speedLeft: -255 to 255
  speedRight: -255 to 255
  */


  // --- LEFT MOTOR ---
  if (speedLeft > 0) {
    // Going Forward
    analogWrite(MLEFT_B, 0);            // Strictly turn off backward PWM first
    analogWrite(MLEFT_F, speedLeft);    // Apply forward PWM
  } 
  else if (speedLeft < 0) {
    // Going Backward
    analogWrite(MLEFT_F, 0);              // Strictly turn off forward PWM first
    analogWrite(MLEFT_B, abs(speedLeft)); // Apply backward PWM
  } 
  else {
    // Stopped (Neutral)
    analogWrite(MLEFT_F, 0);
    analogWrite(MLEFT_B, 0);
  }

  // --- RIGHT MOTOR ---
  if (speedRight > 0) {
    // Going Forward
    analogWrite(MRIGHT_B, 0);           // Strictly turn off backward PWM first
    analogWrite(MRIGHT_F, speedRight);  // Apply forward PWM
  } 
  else if (speedRight < 0) {
    // Going Backward
    analogWrite(MRIGHT_F, 0);               // Strictly turn off forward PWM first
    analogWrite(MRIGHT_B, abs(speedRight)); // Apply backward PWM
  } 
  else {
    // Stopped (Neutral)
    analogWrite(MRIGHT_F, 0);
    analogWrite(MRIGHT_B, 0);
  }
}





/**************** Main Loop ****************/
void loop() {
  /*
  Note: Since all the movement is controlled by the callback function onReceive(), the main loop
  only includes code for the telemetry send and LED and Buzzer control
  */

  /**** Telemetry ****/
  // If 1 second hasn't passed since last telemetry communication, don't send telemetry
  if (millis() - lastTelemetrySend > 1000) {
    TelemetryPacket telemetry; //Declare telemetry structure packet

    telemetry.RSSI = lastRSSI; //Store RSSI in telemtry packet
    
    // Check for obstacle using IR Sensor and store in local variable
    /*
    Note: LOW output (0) means there's an obstacle in front of it, while HIGH (1) means
    there's nothing
    */
    if(digitalRead(IR_SENSOR) == 0){
      currentObstacle = true;
      telemetry.obstacle = true;
    }else{
      telemetry.obstacle = false;
      currentObstacle = false;
    }

    // Send telemetry data to receiver using esp_now_send(function)
    esp_now_send(transmitterMAC, (uint8_t *)&telemetry, sizeof(telemetry));
    
    lastTelemetrySend = millis(); // Reset the timer
    
  }


  /**** LED and Buzzer system ****/
  //When sysOn is true:
  if (sysOn) {
    // Check current time
    unsigned long nowMillis = millis();

    // If the time passed since prevMilis es greater or equal to interval:
    if (nowMillis - prevMillis >= interval) {
      prevMillis = nowMillis; //Save current milis() as prevMilis

      i++; //Increase counter
      if (i % 2 == 0) { // odd checks to toggle lights
        digitalWrite(redPin, HIGH);
        digitalWrite(bluePin, LOW);
      } else {
        digitalWrite(redPin, LOW);
        digitalWrite(bluePin, HIGH);
      }
      if (i == 6) { // toggles the buzzer every 6 loops (450ms)
        tone(buzzerPin, 900);
      } 
      else if (i >= 12) {
        tone(buzzerPin, 700);
        i = 0; // Reset the cycle
      }
    }
  }
  else { // If sysOn is false, turns LED and Buzzer off
    digitalWrite(redPin, LOW);
    digitalWrite(bluePin, LOW);
    noTone(buzzerPin);
    i = 0;
  }

  delay(5);
}



