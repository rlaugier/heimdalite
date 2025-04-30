#include <Arduino.h>

#include <Adafruit_MCP4728.h>
#include <Wire.h>

#define DEFAULT_OUTPUT_VALUE 1024
#define TWOPI 6.2832
#define PI 3.1416
#define VALUE 0
#define SINE 1
#define TRIANGLE 2

// Adafruit_MCP4728 mcp_dac;

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
int long period_micros = 1000000;
int sig_amplitude = 1600;
int sig_offset = 2048;

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

// 
    // Try to initialize!
    // if (!mcp_dac.begin()) {
    //     Serial.println("Failed to find MCP4728 chip");
    //     while (1) {
    //     delay(10);
    //     }
    // }

    // mcp_dac.setChannelValue(MCP4728_CHANNEL_A, 4095);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_B, 2048);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_C, 1024);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_D, 0);
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
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_A, integer0FromPC);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_B, integer1FromPC);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_C, integer2FromPC);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_D, integer3FromPC);
            Serial.flush();
            break;
        case 'c' :
            signalMode = VALUE;
            Serial.print("Clearing values to ");
            Serial.println(DEFAULT_OUTPUT_VALUE);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_A, DEFAULT_OUTPUT_VALUE);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_B, DEFAULT_OUTPUT_VALUE);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_C, DEFAULT_OUTPUT_VALUE);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_D, DEFAULT_OUTPUT_VALUE);
            Serial.println(signalMode);
            Serial.flush();
            break;
        case 'g':
            signalMode = SINE;
            period_micros = int(integer0FromPC * integer1FromPC);
            sig_amplitude = int(integer2FromPC);
            sig_offset = int(integer3FromPC);
            Serial.println("Signal generator sinewave");
            Serial.print("Period (microseconds = )");
            Serial.println(period_micros);
            Serial.print("Signal amplitude = ");
            Serial.println(sig_amplitude);
            Serial.print("Signal offset = ");
            Serial.println(sig_offset);
            Serial.println(signalMode);
            Serial.flush();
            break;
        case 't':
            signalMode = TRIANGLE;
            period_micros = int(integer0FromPC * integer1FromPC);
            sig_amplitude = int(integer2FromPC);
            sig_offset = int(integer3FromPC);
            Serial.println("Signal generator triangle");
            Serial.print("Period (microseconds = )");
            Serial.println(period_micros);
            Serial.print("Signal amplitude = ");
            Serial.println(sig_amplitude);
            Serial.print("Signal offset = ");
            Serial.println(sig_offset);
            Serial.println(signalMode);
            Serial.flush();
            break;
        default:
            Serial.println("Message non recognized");
            Serial.println(signalMode);
            Serial.flush();
            break;
    }
}

float getPhase(){
    unsigned long time = micros();
    int relative = time % period_micros;
    float phase = TWOPI * float(relative) / float(period_micros);
    // Serial.println(phase,3);
    return phase;
}



float signalWave(){
    float value = sig_offset + sig_amplitude * sin(getPhase());
    return int(round(value));
}

float triangleWave(){
    unsigned long time = micros();
    int relative = time % period_micros;
    int halfperiod = period_micros / 2;
    int intvalue = sig_offset + abs(relative % halfperiod - halfperiod);
    // Serial.println(intvalue);
    float value = float(intvalue) * float(sig_amplitude) / float(relative) ;
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
    // Serial.println(myval);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_A, myval);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_B, myval);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_C, myval);
    // mcp_dac.setChannelValue(MCP4728_CHANNEL_D, myval);
}