#include <Arduino.h>

#include <Adafruit_MCP4728.h>
#include <Wire.h>

#define DEFAULT_OUTPUT_VALUE 1024

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
void recvWithStartEndMarkers();
void parseData();
void showParsedData();
void sendDACCommands();

boolean newData = false;

//============

void setup() {
    Serial.begin(9600);
    while(!Serial);
    Serial.println("This demo expects 3 pieces of data - text, an integer and a floating point value");
    Serial.println("Enter data in this style [s,1024,2000,3000,4000] ");
    Serial.println();

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
    switch(mychar) {
        case 's' :
            Serial.println("Setting values");
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_A, integer0FromPC);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_B, integer1FromPC);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_C, integer2FromPC);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_D, integer3FromPC);
            break;
        case 'c' :
            Serial.print("Clearing values to ");
            Serial.println(DEFAULT_OUTPUT_VALUE);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_A, DEFAULT_OUTPUT_VALUE);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_B, DEFAULT_OUTPUT_VALUE);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_C, DEFAULT_OUTPUT_VALUE);
            // mcp_dac.setChannelValue(MCP4728_CHANNEL_D, DEFAULT_OUTPUT_VALUE);
            break;
        default:
            Serial.println("Message non recognized");
            break;
    }
}