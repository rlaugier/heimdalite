#include <Arduino.h>

#include <Adafruit_MCP4728.h>
#include <Wire.h>

#define DEFAULT_OUTPUT_VALUE 1024
#define TWOPI 6.2832
#define PI 3.1416
#define VALUE 0
#define SINE 1
#define TRIANGLE 2
#define RED_LED_PIN 11
#define YELLOW_LED_PIN 5
#define SWITCH_PIN 7

Adafruit_MCP4728 mcp_dac;

// Example 5 - Receive with start- and end-markers combined with parsing

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

      // variables to hold the parsed data
char messageFromPC[numChars] = {0};
int integer0FromPC = DEFAULT_OUTPUT_VALUE;
int integer1FromPC = DEFAULT_OUTPUT_VALUE;
int integer2FromPC = DEFAULT_OUTPUT_VALUE;
int integer3FromPC = DEFAULT_OUTPUT_VALUE;
float floatFromPC = 0.0;

int signalMode = VALUE;
long period_ms = 1000;
int sig_amplitude = 1600;
int sig_offset = 2048;
bool verbose = true;

void recvWithStartEndMarkers();
void parseData();
void showParsedData();
void sendDACCommands();
float getPhase();
float signalWave();
void sendSignal();
float triangleWave();

boolean newData = false;

//============

void setup() {
    Serial.begin(9600);
    while(!Serial);
    Serial.println("This demo expects 3 pieces of data - text, an integer and a floating point value");
    Serial.println("Enter data in this style [s,1024,2000,3000,4000] ");
    Serial.println("For a signal generator (sawtooth) [g,period1,period2,half-amp,offset] ");
    Serial.println("e.g. [g,1000,1000,2000,2000]");
    Serial.println("");


    // Try to initialize!
    int mcp_status = mcp_dac.begin(0x64);
    Serial.println(mcp_status);
    if (!mcp_status) {
        Serial.println("Failed to find MCP4728 chip");
        while (1) {
        delay(10);
        }
    }

    mcp_dac.setChannelValue(MCP4728_CHANNEL_A, 2048, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    mcp_dac.setChannelValue(MCP4728_CHANNEL_B, 2048, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    mcp_dac.setChannelValue(MCP4728_CHANNEL_C, 2048, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    mcp_dac.setChannelValue(MCP4728_CHANNEL_D, 2048, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);

    // mcp_dac.setChannelValue(MCP4728_CHANNEL_A, 4095);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_B, 2048);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_C, 1024);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_D, 0);

    // Testing LEDs
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN,HIGH);
    delay(500);
    digitalWrite(RED_LED_PIN,LOW);
    digitalWrite(YELLOW_LED_PIN,HIGH);
    delay(5);
    digitalWrite(YELLOW_LED_PIN,LOW);

    pinMode(SWITCH_PIN, INPUT);


}

//============

void loop() {
    recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        parseData();
        showParsedData();
        sendDACCommands();
        newData = false;
    }
    if (signalMode == SINE) {
        sendSignal();
    }
    else if (signalMode == TRIANGLE) {
        sendSignal();
    }
}

//============

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '[';
    char endMarker = ']';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

//============

void parseData() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC
 
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    integer0FromPC = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    integer1FromPC = atoi(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    integer2FromPC = atoi(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    integer3FromPC = atoi(strtokIndx);


}

//============

void showParsedData() {
    Serial.print("Message ");
    Serial.println(messageFromPC);
    Serial.print("Integer ");
    Serial.println(integer0FromPC);
    Serial.print("Integer ");
    Serial.println(integer1FromPC);
    Serial.print("Integer ");
    Serial.println(integer2FromPC);
    Serial.print("Integer ");
    Serial.println(integer3FromPC);
}

void sendDACCommands(){
    int mychar = int(messageFromPC[0]) ;
    Serial.println("==================");
    Serial.flush();
    switch(mychar) {
        case 's' :
            signalMode = VALUE;
            Serial.println("Setting values");
            mcp_dac.setChannelValue(MCP4728_CHANNEL_A, integer0FromPC, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
            mcp_dac.setChannelValue(MCP4728_CHANNEL_B, integer1FromPC, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
            mcp_dac.setChannelValue(MCP4728_CHANNEL_C, integer2FromPC, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
            mcp_dac.setChannelValue(MCP4728_CHANNEL_D, integer3FromPC, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
            Serial.println(0);

            if (verbose == true) {
                digitalWrite(YELLOW_LED_PIN, HIGH);
                delay(50);
                digitalWrite(YELLOW_LED_PIN, LOW);
            }
            break;
        case 'c' :
            signalMode = VALUE;
            Serial.print("Clearing values to ");
            Serial.println(DEFAULT_OUTPUT_VALUE);
            mcp_dac.setChannelValue(MCP4728_CHANNEL_A, DEFAULT_OUTPUT_VALUE, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
            mcp_dac.setChannelValue(MCP4728_CHANNEL_B, DEFAULT_OUTPUT_VALUE, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
            mcp_dac.setChannelValue(MCP4728_CHANNEL_C, DEFAULT_OUTPUT_VALUE, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
            mcp_dac.setChannelValue(MCP4728_CHANNEL_D, DEFAULT_OUTPUT_VALUE, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
            Serial.println(0);

            if (verbose == true) {
                digitalWrite(YELLOW_LED_PIN, HIGH);
                delay(100);
                digitalWrite(YELLOW_LED_PIN, LOW);
            }
            break;
        case 'g':
            signalMode = SINE;
            period_ms = long(integer0FromPC * integer1FromPC);
            sig_amplitude = int(integer2FromPC);
            sig_offset = int(integer3FromPC);
            Serial.println("Signal generator sinewave");
            Serial.print("Period (ms = )");
            Serial.println(period_ms);
            Serial.print("Signal amplitude = ");
            Serial.println(sig_amplitude);
            Serial.print("Signal offset = ");
            Serial.println(sig_offset);
            Serial.println(0);
            if (verbose == true) {
                digitalWrite(YELLOW_LED_PIN, HIGH);
                delay(100);
                digitalWrite(YELLOW_LED_PIN, LOW);
            }
            break;
        case 't':
            signalMode = TRIANGLE;
            period_ms = long(integer0FromPC * integer1FromPC);
            sig_amplitude = int(integer2FromPC);
            sig_offset = int(integer3FromPC);
            Serial.println("Signal generator triangle");
            Serial.print("Period (ms = )");
            Serial.println(period_ms);
            Serial.print("Signal amplitude = ");
            Serial.println(sig_amplitude);
            Serial.print("Signal offset = ");
            Serial.println(sig_offset);
            Serial.println(0);

            if (verbose == true) {
                digitalWrite(YELLOW_LED_PIN, HIGH);
                delay(100);
                digitalWrite(YELLOW_LED_PIN, LOW);
            }
            break;
        case 'v':
            if (integer0FromPC == 0) {
                verbose = false;
            }
            else 
            {
                verbose = true;
            }

            if (verbose == true) {
                digitalWrite(YELLOW_LED_PIN, HIGH);
                delay(10);
                digitalWrite(YELLOW_LED_PIN, LOW);
            }
            else {
                digitalWrite(RED_LED_PIN, HIGH);
                delay(10);
                digitalWrite(RED_LED_PIN, LOW);
            }
            break;
        default:
            Serial.println("Message non recognized");
            Serial.println(signalMode);
            Serial.println(1);

            if (verbose == true) {
                digitalWrite(RED_LED_PIN, HIGH);
                delay(500);
                digitalWrite(RED_LED_PIN, LOW);
            }
            break;
    }
}

float getPhase(){
    unsigned long time = micros();
    int relative = time % (period_ms*1000);
    float phase = TWOPI * float(relative) / float(period_ms*1000);
    // Serial.println(phase,3);
    return phase;
}



float signalWave(){
    float value = sig_offset + sig_amplitude * sin(getPhase());
    return int(round(value));
}

float triangleWave(){
    unsigned long time = micros();
    uint32_t period = period_ms*1000;
    uint32_t relative = time % period;
    // int halfperiod = period_ms / 2;
    float valrel = abs(float(relative) / float(period)- 0.5) * 4.0 - 1.0;
    // Serial.println(valrel, 2);
    float value = sig_offset + valrel * float(sig_amplitude);
    return int(round(value));
}

void sendSignal(){
    int myval = 0;
    if (signalMode == SINE) {
        myval = signalWave();
    }
    else if (signalMode == TRIANGLE) {
        myval = triangleWave();
    }
    Serial.print(millis());
    Serial.print("  ");
    Serial.println(myval);
    // mcp_dac.fastWrite(myval, myval, myval, myval);
    mcp_dac.setChannelValue(MCP4728_CHANNEL_A, myval, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_B, myval, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_C, myval, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_D, myval, MCP4728_VREF_INTERNAL, MCP4728_GAIN_2X);

    delay(10);
}