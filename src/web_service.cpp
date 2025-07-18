#include <algorithm>                // ethernet/MQTT/webserver
#include <cstdio>                   // ethernet/MQTT/webserver
#include <utility>                  // ethernet/MQTT/webserver
#include <vector>                   // ethernet/MQTT/webserver
#include <string>                   // ethernet/MQTT/webserver

#include <Arduino.h>
#include <QNEthernet.h>             // ethernet/MQTT/webserver
#include <LittleFS.h>               // LittleFS 
#include <ArduinoJson.h>            // JSON file format

#include "web_service.h"
#include "relay_button.h"

using namespace qindesign::network; // ethernet/MQTT/webserver


extern LittleFS_Program g_little_fs;
extern StaticJsonDocument<4096> g_json;

// Keeps track of state for a single client.
struct ClientState {
  ClientState(EthernetClient client)
      : client(std::move(client)) {}

  EthernetClient client;
  bool closed = false;

  // For timeouts.
  uint32_t lastRead = millis();  // Mark creation time

  // For half closed connections, after "Connection: close" was sent
  // and closeOutput() was called
  uint32_t closedTime = 0;    // When the output was shut down
  bool outputClosed = false;  // Whether the output was shut down

  // Parsing state
  bool emptyLine = false;
  bool is_not_first_line = false;
  char first_line_request[256] = {};
};


// Ethernet/MQTT/Webserver global variables declaration section
std::vector<ClientState> g_clients;   // webserver
EthernetServer g_web_server{WEB_SERVER_PORT}; // webserver

uint8_t
web_service_begin()
{
  g_web_server.begin();
  return g_web_server ? 1 : 0;
}


void handle_GET_file(EthernetClient &client, const char *file)
{
  bool did_write = false;

  char buffer[128];
  strcpy(buffer, "/www");
  strcpy(buffer+strlen(buffer), file);
  char *ext = strrchr(buffer, '.');

  char cache_control[128];
  strcpy(cache_control, "max-age=0");

  char content_type[32];
  strcpy(content_type, "text/plain");
  if (ext)
  {
    ext++;
    if (!strcasecmp(ext, "ico"))  strcpy(content_type, "image/x-icon");
    if (!strcasecmp(ext, "html")) strcpy(content_type, "text/html");
    if (!strcasecmp(ext, "jpg"))  strcpy(content_type, "image/jpeg");
    if (!strcasecmp(ext, "svg"))  strcpy(content_type, "image/svg+xml");
    if (!strcasecmp(ext, "png"))  strcpy(content_type, "image/png");
    if (!strcasecmp(ext, "css"))  strcpy(content_type, "text/css");
    if (!strcasecmp(ext, "js"))   strcpy(content_type, "text/javascript");
    if (!strcasecmp(ext, "json")) strcpy(content_type, "application/json");
    if (!strcasecmp(ext, "ico")  ||
        !strcasecmp(ext, "html") ||
        !strcasecmp(ext, "jpg")  ||
        !strcasecmp(ext, "svg")  ||
        !strcasecmp(ext, "png")  ||
        !strcasecmp(ext, "css")  ||
        !strcasecmp(ext, "js")   ||
        !strcasecmp(ext, "json")
        )
    {
      strcpy(cache_control, "max-age=604800"); // one week!
    }
  }

  if (g_little_fs.exists(buffer))
  {
    File dataFile = g_little_fs.open(buffer, FILE_READ);
    if (dataFile) 
    {
      uint32_t stop, start = micros();
      Serial.println("reading from g_little_fs");
      client.write("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: ");
      client.write(content_type);
      client.write("\r\n"
                    "Cache-Control: ");
      client.write(cache_control);
      client.write("\r\n"
                    "\r\n"
                    );
      uint32_t counter = 100;
      while (dataFile.available()) 
      {
        client.write(dataFile.read());
        counter++;
        if ((counter % 4096) == 0)
        {
          client.flush();
        }
        if ((counter % 256) == 0)
        {
          stop = micros();        
          if ((stop-start) > 2000) // every 2 ms call yield...
          {
            start = stop;
            yield();
          }
        }
      }
      dataFile.close();
      client.flush();
      did_write = true;
    }
  }
  if (!did_write)
  {
    client.writeFully("HTTP/1.1 200 OK\r\n"
                      "Connection: close\r\n"
                      "Content-Type: text/plain\r\n"
                      "\r\n"
                      "Hello Client! ==> Sorry file not found...\r\n");
    client.flush();
  }
}


void handle_GET_api_json(EthernetClient &client, const char *what, const char *params)
{
  const char *on_relays_str = "on_relays";
  const char *on_pushbuttons_str = "on_pushbuttons";
  const char *do_relay_str = "do_relay";
  g_json.clear(); // start with an empty document.

  bool handled = false;
  if (!strcmp(what, on_relays_str))
  {
    handled = true;
    JsonArray on_relays = g_json.createNestedArray(on_relays_str);
    for (uint8_t rel = 0; rel<128; rel++)
    {
      if (rel_get_state(rel))
      {
        on_relays.add(rel);
      }    
    }
  } else if (!strcmp(what, on_pushbuttons_str))
  {
    handled = true;
    JsonArray on_pushbuttons = g_json.createNestedArray(on_pushbuttons_str);
    for (uint8_t but = 0; but<64; but++)
    {
      if (pushbutton_get_state(but))
      {
        on_pushbuttons.add(but);
      }    
    }
  } else if (!strcmp(what, do_relay_str))
  {
    char *rel = strstr(params, "relay=");
    if (rel)
    {
      int16_t rel_no = atoi(rel+strlen("relay="));
      if ((0 <= rel_no) && (rel_no < 128))
      {
        char *action = strstr(params, "action=");
        if (action)
        {
          action += strlen("action=");
          int16_t ra = -1;
          if (!strcasecmp(action, "on")) ra = A_ON;
          if (!strcasecmp(action, "off")) ra = A_OFF;
          if (!strcasecmp(action, "toggle")) ra = A_TOGGLE;
          if (ra > 0)
          {
            handled = true;
            uint8_t new_state = rel_update(rel_no, ra);
            g_json["relay"] = rel_no;
            g_json["new_state"] = new_state;
          }
        }
      }
    }
  }

  client.write("HTTP/1.1 200 OK\r\n"
                "Connection: close\r\n"
                "Content-Type: application/json\r\n"
                "Cache-Control: max-age: 0\r\n"
                "\r\n"
                );
  if (handled)
  {
    serializeJson(g_json, client);
  } else
  {
    client.write("{}");
  }
  client.flush();
}

void
handle_GET_url(EthernetClient &client, const char *url, const char *parameters)
{
  Serial.printf("handle_GET_url: '%s' =?=> '%s'\n", url, parameters);

  if (!strcmp(url, "/")) return handle_GET_file(client, "/index.html");
  if (!strcmp(url, "/favicon.ico")) return handle_GET_file(client, "/assets/site/favicon.ico");
  if (!strcmp(url, "/config.json")) return handle_GET_file(client, "/../config.json");
  if (!strcmp(url, "/api/on_relays.json")) return handle_GET_api_json(client, "on_relays", parameters);
  if (!strcmp(url, "/api/on_pushbuttons.json")) return handle_GET_api_json(client, "on_pushbuttons", parameters);
  if (!strcmp(url, "/api/do_relay.json")) return handle_GET_api_json(client, "do_relay", parameters);
  if (!strcmp(url, "/api/config.json")) return handle_GET_api_json(client, "do_relay", parameters);
  return handle_GET_file(client, url);
}


// The simplest possible (very non-compliant) HTTP server. Respond to
// any input with an HTTP/1.1 response.
void 
process_client_data(ClientState &state) {
  // Loop over available data until an empty line or no more data
  // Note that if emptyLine starts as false then this will ignore any
  // initial blank line.
  // unsigned long stop_time, start_time = micros();

  while (true) {
    int avail = state.client.available();
    if (avail <= 0) {
      return;
    }

    state.lastRead = millis();
    char c = state.client.read();
    // Serial.print(c);
    if (!state.is_not_first_line)
    {
      if ((c != '\n') && (c != '\r'))
      {
        char buf_c[2];
        buf_c[0] = c;
        buf_c[1] = 0;
        if (strlen(state.first_line_request) < (sizeof(state.first_line_request) - 2))
        {
          strcat(state.first_line_request, buf_c);
        }
      }
    }
    if (c == '\n') {
      state.is_not_first_line = true;
      if (state.emptyLine) {
        break;
      }
      // Start a new empty line
      state.emptyLine = true;
    } else if (c != '\r') {
      // Ignore carriage returns because CRLF is a likely pattern in
      // an HTTP request
      state.emptyLine = false;
    }
  }

  // parse the first line, and answers to GET requests.
  char *url = strchr(state.first_line_request, ' ');
  if (url) // ignore when no space is found!
  {
    *url = '\0';
    url++;
    char *p2 = strchr(url, ' '); // end of URL
    if (p2)
    {
      *p2 = '\0';
      if (!strcmp(state.first_line_request, "GET"))
      {
        char *params = strchr(url, '?');
        if (params != NULL)
        {
          *params = '\0';
          params++;
        } else
        {
          params = (char *)"";
        }
        handle_GET_url(state.client, url, params);
        state.client.flush();
      }
    }
  }

  state.first_line_request[0] = '\0';
  memset(state.first_line_request, 0, sizeof(state.first_line_request));
  state.is_not_first_line = false; // reset for potential next request

  // Half close the connection, per
  // [Tear-down](https://datatracker.ietf.org/doc/html/rfc7230#section-6.6)
  state.client.closeOutput();
  state.closedTime = millis();
  state.outputClosed = true;
}


void handle_web_requests()
{
  unsigned long stop_time, start_time = micros();
  EthernetClient client = g_web_server.accept();
  if (client) {
    // We got a connection!
    // IPAddress ip = client.remoteIP();
    // Serial.printf("Client connected: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
    g_clients.emplace_back(std::move(client));
    // Serial.printf("Client count: %zu\r\n", g_clients.size());
  }

  // Process data from each client
  for (ClientState &state : g_clients) {  // Use a reference so we don't copy
    if (!state.client.connected()) {
      state.closed = true;
      continue;
    }

    // Check if we need to force close the client
    if (state.outputClosed) {
      if (millis() - state.closedTime >= WEB_CLIENT_SHUTDOWN_TIME) {
        // IPAddress ip = state.client.remoteIP();
        // Serial.printf("Client shutdown timeout: %u.%u.%u.%u\r\n",
        //        ip[0], ip[1], ip[2], ip[3]);
        state.client.close();
        state.closed = true;
        continue;
      }
    } else {
      if (millis() - state.lastRead >= WEB_CLIENT_TIMEOUT) {
        // IPAddress ip = state.client.remoteIP();
        // Serial.printf("Client timeout: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
        state.client.close();
        state.closed = true;
        continue;
      }
    }

    process_client_data(state);
  }

  // Clean up all the closed clients
  size_t size = g_clients.size();
  g_clients.erase(std::remove_if(g_clients.begin(), g_clients.end(),
                               [](const auto &state) { return state.closed; }),
                g_clients.end());
  if (g_clients.size() != size) {
    // Serial.printf("New client count: %zu\r\n", g_clients.size());
  }
  stop_time = micros();
  if ((stop_time - start_time) > 200)
  {
    Serial.print("handle_web_requests: ");
    Serial.print(stop_time - start_time);
    Serial.println(" us");
  }
}
