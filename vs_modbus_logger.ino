//
//  Arduino Projects
//
//  Created by Michael Diehl on 3/8/13.
//
// Modbus Registers
//
// 1000 Controller State uint16_t
//
// All pressure values are 1/16 psi
// 1008 Interstage P  signed short
// 1009 Inlet P signed
// 1010 Plant P signed
// 1011 Separator P signed
// 1012 Reservoir P signed
// 1013 Oil P signed
// 1014 System P signed
//
// VFD Frequencies values are 1/100 Hz, amps are 1/10
// 2018 V1 Current signed
// 2019 V2 Current signed
// 2024 V1 Motor Speed signed
// 2025 V2 Motor Speed signed
//
// 2054 V1 Motor Voltage
// 2055 V2 Motor Volatge
//
// 13000 Total hours * 10 signed long 32 bit
//

#include <SPI.h>
#include <SD.h>
#include <stdlib.h>
#include <ModbusMaster.h>

// Slave address LSB of IP address in Sequence Adjust
#define controllerAddress 2

// SD card select pin 

// Ethernet shield card slot
//#define csPin 4

// Sparkfun MicroSD Shield
#define csPin 8

// Max number of lines in log
#define MAX_SAMPLE_COUNT 360

// instantiate ModbusMaster object
ModbusMaster node(1, controllerAddress, 2000);

// Timer interval for sending the requested position every 10 seconds
const long interval = 10000;

// Previous uptime value for timer.
static long previousMillis = 0;

// Max line count for files
static unsigned int lineCount = 0;

// Log file name suffix.
static unsigned int logSuffix = 1;

// Structs for our data
struct Pressures {
    float interstage;
    float inlet;
    float plant;
    float separator;
    float reservoir;
    float oil;
    float system;
};

struct VFD {
    float v1Current;
    float v2Current;
    float v1MotorSpeed;
    float v2MotorSpeed;
    float v1MotorVoltage;
    float v2MotorVoltage;
};

// Contains our actual data
static Pressures myPressures = (Pressures){ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

static VFD myVFD = (VFD){ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

static float totalHours = 0.0f;

// Setup
void setup()
{
    node.begin(9600);
    SD.begin(csPin);
}

void loop()
{
    // Get the current uptime
    unsigned long currentMillis = millis();

    // Reset previousMillis in the event of a rollover.
    if(previousMillis > currentMillis){
        previousMillis = 0;
    }

    // Check the timer
    if(currentMillis - previousMillis > interval){
        previousMillis = millis();
        updateLog();
    }
}

void updateLog()
{
    getOperatingHours();
    getPressureData();
    getVFDData();

    // Create a data string for log file from values
    String dataString = "";
    char val[8];    // string buffer

    // dtostrf(FLOAT,WIDTH,PRECSISION,BUFFER);
    dataString += dtostrf(myPressures.interstage,5,1,val);
    dataString += ",";
    dataString += dtostrf(myPressures.inlet,5,1,val);
    dataString += ",";
    dataString += dtostrf(myPressures.plant,5,1,val);
    dataString += ",";
    dataString += dtostrf(myPressures.separator,5,1,val);
    dataString += ",";
    dataString += dtostrf(myPressures.reservoir,5,1,val);
    dataString += ",";
    dataString += dtostrf(myPressures.oil,5,1,val);
    dataString += ",";
    dataString += dtostrf(myPressures.system,5,1,val);
    dataString += ",";
    dataString += dtostrf(myVFD.v1Current,5,1,val);
    dataString += ",";
    dataString += dtostrf(myVFD.v2Current,5,1,val);
    dataString += ",";
    dataString += dtostrf(myVFD.v1MotorSpeed,4,0,val);
    dataString += ",";
    dataString += dtostrf(myVFD.v2MotorSpeed,4,0,val);
    dataString += ",";
    dataString += dtostrf(myVFD.v1MotorVoltage,4,0,val);
    dataString += ",";
    dataString += dtostrf(myVFD.v2MotorVoltage,4,0,val);
    dataString += ",";
    dataString += dtostrf(totalHours,5,1,val);

    // Increment the log suffix
    if(lineCount > MAX_SAMPLE_COUNT) {
        lineCount = 0;
        logSuffix++;
    }

    // Construct log filename i.e. vslog_1.csv, vslog_2.csv and so on
    String fileName = "log_";
    fileName = fileName + logSuffix;
    fileName = fileName + ".csv";

    char fileNameCharArray[(fileName.length() + 1)];
    fileName.toCharArray(fileNameCharArray, (fileName.length() + 1));

    // Open the file.
    File dataFile = SD.open(fileNameCharArray, FILE_WRITE);

    // If the file is available, write to it:
    if (dataFile) {
        // Print headers as the first line
        if (lineCount == 0) {
            dataFile.println("InterStage, Inlet, Plant, Separator, Reservoir, Oil, System,"
                             "V1 Amps, V2 Amps, V1 Speed, V2 Speed,"
                             "V1 Motor Voltage, V2 Motor Voltage, Total Hours");
        }
        dataFile.println(dataString);
        dataFile.close();
    }

    // Increment line count
    lineCount++;
}

void getVFDData()
{
    uint8_t result;

    result = node.readHoldingRegisters(2018, 2);
    if (result == node.ku8MBSuccess)
    {
        myVFD.v1Current = (node.getResponseBuffer(0) / 10.0f);
        myVFD.v2Current = (node.getResponseBuffer(1) / 10.0f);
    }
    else{ // Zero out if we don't get a valid response
        myVFD.v1Current = 0.0f;
        myVFD.v2Current = 0.0f;
    }

    result = node.readHoldingRegisters(2024, 2);
    if (result == node.ku8MBSuccess)
    {
        myVFD.v1MotorSpeed = ((float)node.getResponseBuffer(0));
        myVFD.v2MotorSpeed = ((float)node.getResponseBuffer(1));
    }
    else{ // Zero out if we don't get a valid response
        myVFD.v1MotorSpeed = 0.0f;
        myVFD.v2MotorSpeed = 0.0f;
    }

    result = node.readHoldingRegisters(2054, 2);
    if (result == node.ku8MBSuccess)
    {
        myVFD.v1MotorVoltage = ((float)node.getResponseBuffer(0));
        myVFD.v2MotorVoltage = ((float)node.getResponseBuffer(1));
    }
    else{ // Zero out if we don't get a valid response
        myVFD.v1MotorVoltage = 0.0f;
        myVFD.v2MotorVoltage = 0.0f;
    }
}

void getPressureData()
{
    uint8_t result;

    // Use a single readHoldingRegisters for Pressure data
    result = node.readHoldingRegisters(1008, 7);
    if (result == node.ku8MBSuccess)
    {
        myPressures.interstage = ((int16_t)node.getResponseBuffer(0) / 16.0f);
        myPressures.inlet = ((int16_t)node.getResponseBuffer(1) / 16.0f);
        myPressures.plant = ((int16_t)node.getResponseBuffer(2) / 16.0f);
        myPressures.separator = ((int16_t)node.getResponseBuffer(3) / 16.0f);
        myPressures.reservoir = ((int16_t)node.getResponseBuffer(4) / 16.0f);
        myPressures.oil = ((int16_t)node.getResponseBuffer(5) / 16.0f);
        myPressures.system = ((int16_t)node.getResponseBuffer(6) / 16.0f);
    }
    else{ // Zero out if we don't get a valid response
        zeroPressureData();
    }

}

void getOperatingHours()
{
    uint8_t result;

    result = node.readHoldingRegisters(13000, 2);
    if (result == node.ku8MBSuccess)
    {
        uint16_t high = node.getResponseBuffer(0);
        uint16_t low = node.getResponseBuffer(1);

        uint32_t hoursResponse = ((uint32_t)(high) << 16 | (low));
        totalHours = (hoursResponse / 10.0f);
    }
    else{
        totalHours = 0.0f;
    }
}

void zeroVFDData()
{
    VFD myVFD = (VFD){ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

void zeroPressureData()
{
    myPressures = (Pressures){ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
}

