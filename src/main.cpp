/*
 * Home Automation
 * Core function:
 *  - read push-buttons (via I2C)
 *  - create events single/double/.. click on pushbuttons
 *  - translate events into relay toggle/on/off actions
 *  - execute the action to set the relays (via I2C)
 * Convience function:
 *  - communicate the pushbuttons and relays statuses on MQTT for Home Assistant app
 *  - control relays using MQTT (from Home Assistant app)
 *  - provide (fail-safe) website:
 *      - to view pushbuttons status
 *      - to view relays status
 *      - without SD-card present
 *      - when internet is down; but home-network is still present
 *      - no need for Home Assistant
 *  [- collect logs on SD-card]
 *  [- update firmware via web-server or SD-card]
 *  [- update web-page via web-server or SD-card]
   RAM1: variables:36992, code:225780, padding:3596   free for local variables:257920
   RAM2: variables:55744  free for malloc/new:468544
 */

#include <Wire.h>                   // I2C
#include <arduino-timer.h>          // timer
#include <EventResponder.h>         // timer (during yield function)
#include <cppQueue.h>
#include <TimeLib.h>                // Time/RTC
#include <LittleFS.h>               // LittleFS 
#include <ArduinoJson.h>            // JSON file format


#include <qnethernet/QNDNSClient.h>             // ethernet/MQTT/webserver

#include "relay_button.h"
#include "events.h"
#include "eth.h"
#include "web_service.h"
#include "utils.h"
#include "mqtt.h"
#include "config.h"
#include "heartbeat.h"
#include "power.h"
#include "ntp_lib.h"
#include "ntp_app.h"
#include "teensy_rtc15.h"
#include "CronAlarms.h"

#include <PubSubClient.h> // MQTT client library


using namespace qindesign::network; // ethernet/MQTT/webserver




// Little FS
LittleFS_Program g_little_fs;

// JSON document
JsonDocument g_json; 
char g_buffer[2048];



// arduino-timer global variables declaration section
Timer<8, micros> g_timer_us;
Timer<64, millis> g_timer_ms;

// EventResponder: ==> add the timers to the yield function
EventResponder g_event_responder;



void
timer_responder(EventResponderRef event_responder)
{
  // do the thing!
  g_timer_us.tick();
  g_event_responder.triggerEvent();
}


bool print_datetime(void *)
{
  char buffer[64];
  rtc15_now_str(buffer);
  Serial.printf("now: %s\n", buffer);
  return true;
}


extern uint8_t g_mqtt_is_reconnecting;

bool test_mqtt_reconnecting_timer_us(void *)
{
  if (g_mqtt_is_reconnecting) {
    Serial.printf("test_mqtt_reconnecting_timer_us\n");
  }
  return true;
}



void timer_cancel_all_tasks()
{
  g_timer_us.cancel();
  g_timer_ms.cancel();  
}


static time_t rtc_get_local()
{
  uint64_t rtc15_local = rtc15_to_local(rtc15_get());
  return rtc15_to_sec(rtc15_local);
}


void print_clock(const void *data) 
{
  char buffer[128];
  rtc15_now_str(buffer);
  Serial.printf("now: %s --> '%s'\n", buffer, (const char *)data);
}

#define PIN_SPECIAL_BUTTON 4


void
setup() 
{
  power_begin();
  Serial.begin(1000000);
  unsigned long serial_start = millis();
  while (!Serial && (millis() - serial_start < 4000)) {
    delay(10); // Wait up to 4 seconds for Serial host to connect
  }
  Serial.println("Home automation starting...");

  pinMode(PIN_SPECIAL_BUTTON, INPUT_PULLUP);

  Serial.printf("init I2C\n");
  uint clk = 100000;
  Wire.setClock(clk);
  Wire.begin();
  Wire.setClock(clk);

  clk = 400000;
  Wire1.setClock(clk);
  Wire1.begin();
  Wire1.setClock(clk);
  // by default the IO expanders are in disconnect state, let's connect them.
  io_expanders_reconnect_handler(NULL);


  Serial.println("Init the timers!");
  g_timer_us.every(  100'000, heartbeat_led);  // every 100ms
  g_timer_us.every(   10'000, handle_io_expanders); // every 10ms
  g_timer_us.every(      950, detect_AC); // every 1ms
  g_timer_us.every(   20'000, test_mqtt_reconnecting_timer_us);

  // g_timer.every(10'231'124, handle_brownout_detector); // every 10.2... sec
  //g_timer.every(5'004'123, io_expanders_reconnect_handler);

  g_timer_ms.every(5'001, mqtt_reconnect_handler); // every 5 sec check for reconnect to MQTT

  //g_timer_ms.every(5'002, handle_reconnect_sd); // every 5 sec check for reconnect to SD
  g_timer_ms.every(20'002, print_datetime); // every 2 sec print the time
  //g_timer_ms.every(120'000, ntp_app_timer);  // every 2 minutes
  g_timer_ms.every(1000, ntp_app_timer);  // every 2 minutes
  g_timer_ms.every(200, ntp_app_LED);  // every 200ms update the status

  Serial.println("Init the file system");
  if (!g_little_fs.begin(6*1024*1024))
  {
    Serial.printf("Error starting %s\n", "Program FLASH DISK");
  }

  Serial.println("Read the config files");
  read_config_json();
  read_and_restore_relay_states();
  read_connect_links();
  rel_but_config_print();

  // once the timers are setup, we going to add the tick to the yield function using EventResponder
  g_event_responder.attach(&timer_responder);
  g_event_responder.triggerEvent();

  Serial.println("Core functions are life now!");

  // show PROGRAM FS  
  Serial.printf("little FS: used %llu / total %llu bytes\n", g_little_fs.usedSize(), g_little_fs.totalSize());
  {
    File root = g_little_fs.open("/"); 
    print_directory(root, 0);
    root.close();
  }

  // todo: only copy SD/www when a button is pressed! (and also when no 'www' directory is available on little FS)
  // todo: move it before reading the configuration files; but only when we read the button is pressed @ start up.  
  // if (builtin_sd_card_begin())
  // {
  //   // read SD card 'www' directory and copy all files from it.
  //   Serial.println("copy 'www' from SD card...");      
  //   g_little_fs.mkdir("/www");
  //   File www = SD.open("/www");
  //   copy_directory_to_little_fs(www, "/www");
  //   www.close();

  //   copy_file_to_little_fs("config.json");
  // }

  Serial.printf("Init ethernet/MQTT/Webserer\n");


  if (init_ethernet()) {
    Serial.println("Ethernet initialized, now MQTT...");
    mqtt_begin();

    // Start the server
    Serial.printf("Starting server on port %u...", WEB_SERVER_PORT);
    uint8_t ret = web_service_begin();
    Serial.printf("%s\r\n", (ret) ? "Done." : "FAILED!");
  }

  
  ntp_begin();
  // rtc15_set_active_timezone(rtc_CE); // set the timezone to Belgium
  ntp_app_begin();
  //ntp_app_trigger_synch(); // no trigger, 
  // it will be triggered by the cron job automatically
  // also there is a backup battery, so the RTC is not lost

  // store_connect_links();
  // copy_file_to_SD("connect_links.bin");

  Serial.println("Setting up cron alarms...");

  // create the alarms, to trigger at specific times
  // Edit cron entry at https://crontab.guru/
  // Be aware here we have a first number indicating the seconds!

  Cron.setGetTimeFunction(rtc_get_local);

  // Cron.create("0 00 10 * * *", MorningAlarm, false, (void *)"MorningAlarm");  // 8:30am every day
  // Cron.create("0 45 17 * * *", EveningAlarm, false); // 5:45pm every day
  // Cron.create("30 30 8 * * 6", WeeklyAlarm, false);  // 8:30:30 every Saturday
  Cron.create("13 00 3 * * *", ntp_app_cron, false);  // 03:0:13 every day
  // Cron.create("00 00 20 * * *", ntp_app_cron, false);  // 03:0:13 every day


  // create timers, to trigger relative to when they're created
  Cron.create("*/15 * * * * *", print_clock, false, (void *)"<<15 sec timer>>");           // timer for every 15 seconds
  Serial.println("Ending cron setup...");
  

  Serial.printf("Init finished!\n");
}


void
loop()
{
  unsigned long start_time = micros();

  // Note:
  // core functionality is in yield (via EventResponder)

  // comfort functionality is here

  { // todo: timer ms
    elapsedMicros em = 0; 
    g_timer_ms.tick();
    if (int32_t(em) > 100)
    {
      Serial.printf("g_timer_ms loop: %d us\n", int32_t(em));
    }    
  }
  if (10) {
    elapsedMicros em = 0;     
    mqtt_loop();
    if (int32_t(em) > 100)
    {
      Serial.printf("mqtt_loop: %d us\n", int32_t(em));
    }    
  }
  {
    elapsedMicros em = 0; 
    handle_web_requests(); // WEB
    if (int32_t(em) > 100)
    {
      Serial.printf("handle_web_requests: %d us\n", int32_t(em));
    }    
  }
  {
    elapsedMicros em = 0; 
    static uint8_t special_button_state = 0;
    if (digitalRead(PIN_SPECIAL_BUTTON) == 0)
    {
      if (special_button_state == 0)
      {
        Serial.printf("SPECIAL Button pressed\n");
        ntp_app_trigger_synch();
        special_button_state = 1;
      }
    } else
    {
      if (special_button_state != 0)
      {
        Serial.printf("SPECIAL Button released\n");
        special_button_state = 0;
      }

    }
    if (int32_t(em) > 100)
    {
      Serial.printf("special button: %d us\n", int32_t(em));
    }    
  }
  {
    elapsedMicros em = 0; 
    ntp_app_loop();
    if (int32_t(em) > 100)
    {
      Serial.printf("NTP loop: %d us\n", int32_t(em));
    }
  }
  { 
    elapsedMicros em = 0; 
    Cron.loop();
    if (int32_t(em) > 100)
    {
      Serial.printf("cron loop: %d us\n", int32_t(em));
    }
  }

  {
    elapsedMicros em = 0; 
    EventSignal s;
    memset(&s, 0, sizeof(s));
    if (g_mqtt_queue_event_signals.pop(&s))
    {
      if (s.cmd_ == C_BUTTON)
      {
        const char *state = "unknown";
        if (s.state_ == S_PRESSED) state = "pressed";
        if (s.state_ == S_RELEASED) state = "released";
        if (s.state_ == S_SINGLE_CLICK) state = "single_click";
        if (s.state_ == S_DOUBLE_CLICK) state = "double_click";
        if (s.state_ == S_TRIPLE_CLICK) state = "triple_click";
        if (s.state_ == S_LONG_PRESS) state = "long_press";
        mqtt_publish_button(s.nr_, state);
        g_timer_ms.in(1000, mqtt_publish_button_idle, (void *)int(s.nr_));
      } 
      if (s.cmd_ == C_RELAY)
      {
        const char *state = "0";
        if (s.state_ == S_ON) state = "1";
        mqtt_publish_relay(s.nr_, state);
      }


      //if (debug)    
      {
        const char *cmd = "Unknown";
        if (s.cmd_ == C_BUTTON) cmd = "BUTTON";
        if (s.cmd_ == C_RELAY) cmd = "RELAY";
        const char *state = "unknown";
        if (s.state_ == S_PRESSED) state = "pressed";
        if (s.state_ == S_RELEASED) state = "released";
        if (s.state_ == S_SINGLE_CLICK) state = "single_click";
        if (s.state_ == S_DOUBLE_CLICK) state = "double_click";
        if (s.state_ == S_TRIPLE_CLICK) state = "triple_click";
        if (s.state_ == S_LONG_PRESS) state = "long_press";
        if (s.state_ == S_ON) state = "on";
        if (s.state_ == S_OFF) state = "off";

        Serial.printf("M:-> signal %s:%-3d %s\n", cmd, s.nr_, state);      
      }
    }
    if (int32_t(em) > 100)
    {
      Serial.printf("signal event loop: %d us\n", int32_t(em));
    }
  }

  static unsigned int timer = millis();
  if((millis()-timer) >= 60000)
  {
    timer = millis();
    unsigned long publish_time = micros();
    char buf[32];
    sprintf(buf, "%d", timer);
    mqtt_publish_topic_value("mqtt_test/hello", buf);
    Serial.printf("My timer! %d us\n", (unsigned int)(micros() - publish_time));
  }

  unsigned long stop_time = micros();
  if ((stop_time - start_time) > 100)
  {
    Serial.print("main loop: ");
    Serial.print(stop_time - start_time);
    Serial.println(" us");
  }
}
