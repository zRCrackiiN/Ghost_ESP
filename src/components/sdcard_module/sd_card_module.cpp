#include "sd_card_module.h"

#ifdef SD_CARD_CS_PIN

SDCardModule::SDCardModule() : csPin(SD_CARD_CS_PIN) {}

bool SDCardModule::init() {
    Initlized = SD.begin(csPin);

    if (!Initlized) {
        Serial.println("SD Card initialization failed!");
        return false;
    }
    Serial.println("SD Card initialized.");

    const char* dirPath = "/logs";
    if (!SD.exists(dirPath)) {
        if (SD.mkdir(dirPath)) {
        } else {
        Serial.println("Directory creation failed");
        }
    } else {
        Serial.println("Directory already exists");
    }

    unsigned int maxNum = 0;
    File root = SD.open("/logs");
    
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }
        if (strstr(entry.name(), "boot_") == entry.name()) {
            unsigned int fileNum;
            int scanned = sscanf(entry.name(), "boot_%u_", &fileNum);
            if (scanned == 1) {
                maxNum = max(maxNum, fileNum);
            }
        }
        entry.close();
    }
    root.close();

    BootNum = maxNum + 1;

    char newLogFileName[128];
    sprintf(newLogFileName, "/logs/boot_%u_%s.txt", BootNum, "GhostESP");

    writeFile(newLogFileName, "Begin Ghost ESP Log");
    return true;
}

bool SDCardModule::logMessage(const char *logFileName, const char* foldername, String message)
{
    if (Initlized)
    {
        unsigned long timeSinceBoot = millis();
        unsigned long hours = (timeSinceBoot / (3600000UL)) % 24;
        unsigned long minutes = (timeSinceBoot / (60000UL)) % 60;
        unsigned long seconds = (timeSinceBoot / 1000UL) % 60;

        char timeString[64];
        sprintf(timeString, "[%02lu:%02lu:%02lu] ", hours, minutes, seconds);
        
        char newLogFileName[128];
        sprintf(newLogFileName, "/%s/boot_%u_%s", foldername, BootNum, logFileName);

        String fullMessage = String(timeString) + message;

        if (!SD.exists("/" + String(foldername)))
        {
            SD.mkdir("/" + String(foldername));
        }

        if (!SD.exists(newLogFileName))
        {
            writeFile(newLogFileName, "Begin File");
        }
        
        return appendFile(newLogFileName, fullMessage.c_str());
    }
    else
    {
        return false;
    }
}

bool SDCardModule::writeFile(const char *path, const char *message) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }
    if (file.println(message)) {

    } else {
        Serial.println("Write failed");
    }
    file.close();
    return true;
}

bool SDCardModule::readFile(const char *path) {
    File file = SD.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }
    Serial.print("Read from file: ");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
    return true;
}

bool SDCardModule::appendFile(const char *path, const char *message) {
    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return false;
    }
    if (file.println(message)) {

    } else {
        Serial.println("Append failed");
    }
    file.close();
    return true;
}

bool SDCardModule::deleteFile(const char *path) {
    if (SD.remove(path)) {
        return true;
    } else {
        Serial.println("Delete failed");
        return false;
    }
}

#endif // SD_CARD_CS_PIN