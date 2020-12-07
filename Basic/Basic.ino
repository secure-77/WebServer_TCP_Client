#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266Ping.h>
#include <WiFiUDP.h>
#include <sendemail.h>

// renamed config file with own settings
#include "server-config.h"

// The ESP-12 has a blue LED on GPIO2
#define LED 2

// eMail text buffers, maybe you need to adjust the buffer size
char mail_subject[25];
char mail_text[140];

// TCP Client
WiFiClient client;
String implementedCommands;
String globalResponse;
String loginToken;

// UDP Server
WiFiUDP udpServer;

// WEB Server
ESP8266WebServer webServer(PORT);

// function prototypes for HTTP handlers
void handleRoot();
void handleNotFound();
void handleCommands();
void handleLogin();
void handleRunCommands();

//web server URL text buffer
char secrectURL[25];
char secrectComandURL[25];

// random pin
long randNumber;
char codeArray[3];


/** Runs once at startup */
void setup()
{
  // LED off
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // Initialize the serial port
  Serial.begin(115200);

  // Give the serial monitor of the Arduino IDE time to start
  delay(500);

  // Use an external AP
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  WiFi.config(ip, gateway, subnet);

  // Wait until Wifi is connectet
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected to Wifi");

  // LED On
  digitalWrite(LED, LOW);

  // generatePin
  generatePin();

  // web Server
  webServer.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  webServer.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  // create the secret URL
  sprintf(secrectURL, "/%lu", randNumber);
  Serial.print("secrect url: ");
  Serial.println(secrectURL);
  webServer.on(secrectURL, handleCommands);  // Call the 'handleCommands' function when a request is made to URI
  strcpy(secrectComandURL, secrectURL);
  strcat(secrectComandURL, "/run");
  webServer.on(secrectComandURL, handleRunCommands);  // Call the 'handleRunCommands' function behind the secret url to send data to tcp server
  webServer.on("/cmd_login", HTTP_POST, handleLogin);  // Call the 'handleLogin' function when a POST request is made to URI
  webServer.begin();                           // Actually start the server
  Serial.println("HTTP server started");

  // send mail
  generateMailText();
  sendMail();
  Serial.println(mail_text);
}


/** Main loop, executed repeatedly */
void loop()
{
  check_ap_connection();
  read_tcp_server();
  webServer.handleClient();
}


void handleRoot() {                         // When URI / is requested, send a web page with a button to login
  webServer.send(200, "text/html", "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/><h1>Welcome to COM-Gateway, please login</h1><br><br><form action=\"/cmd_login\" method=\"POST\"><input type=\"text\" name=\"code\" placeholder=\"Code\"><br><input type=\"text\" name=\"loginToken\" placeholder=\"token\"><br><br><button type=\"submit\">Login</button>");
}

void handleLogin() {                          // If a POST request is made to URI /cmd_login
  char codeString[3];
  int retryCount = 0;
  sprintf (codeString, "%lu", randNumber );
  if (webServer.arg("code") == codeString) {

    //connect to tcp server
    loginToken = webServer.arg("loginToken");
    if (loginToken == "") {
      webServer.send(401, "text/plain", "401: Unauthorized, no token entered");
    } else {
      Serial.println("connect to TCP Server...");
      while (!client.connect(serverIP, 7777)) {
        delay(500);
        Serial.print(".");
        retryCount += 1;
        if (retryCount > 3) {
          Serial.println("No connection to tcp server");
          const char *headerMsg = "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/><h1>Server seems to be offline</h1><br><br><form action=\"";
          char page[500];
          strcpy(page, headerMsg);
          strcat(page, secrectComandURL);
          strcat(page, "\" method=\"POST\"><input type=\"submit\" name=\"command\" value=\"wakeup\"/>");
          strcat(page, "<br><br><input type=\"submit\" name=\"command\" value=\"ping\"/></form>");
          webServer.send(200, "text/html", page);
          return;

        }
      }
      Serial.println("connected to TCP Server");

      //redirect to secret page
      webServer.sendHeader("Location", secrectURL);
      webServer.send(303);
    }

  } else {      // code don't match
    webServer.send(401, "text/plain", "401: Unauthorized");
  }
}

// build Commands Page
void handleCommands() {
  const char *headerMsg = "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/><h1>Login successful</h1><br>";
  char page[sizeof(headerMsg) + implementedCommands.length() + 1000];

  // convert commands to char array
  implementedCommands.replace("\n", ",");
  char comms[implementedCommands.length()];
  char result[1000];
  implementedCommands.toCharArray(comms, implementedCommands.length());

  // split commands in tokens to create buttons
  char* token;
  token = strtok(comms, ",");
  while (token != NULL)
  {
    if (!strstr(token, "#### implemented commands ####")) {
      strcat(result, "<input type=\"submit\" name=\"command\" value=\"");
      strcat(result, token);
      strcat(result, "\"/><br>");
    }
    token = strtok (NULL, ",");
  }

  // build HTTP page content
  strcpy(page, headerMsg);
  strcat(page, "#### implemented commands ####<br><br>");
  strcat(page, "<form action=\"");
  strcat(page, secrectURL);
  strcat(page, "/run\" method=\"POST\">");
  strcat(page, result);
  strcat(page, "<br><input type=\"submit\" name=\"command\" value=\"close\"/></form>");

  webServer.send(200, "text/html", page);

}

// send commands to tcp server
void handleRunCommands() {

  // wakeup Server
  if (webServer.arg("command") == "wakeup") {
    WOL(mac);
    webServer.send(200, "text/html", "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>sending WOL package...please wait...<br><br><button onclick=\"goBack()\">Go Back</button><script>function goBack() {  window.history.back();}</script>");
    delay(senddelay * 1000);

    // ping Server
  } else if (webServer.arg("command") == "ping") {
    if (checkServerStatus()) {
      webServer.send(200, "text/html", "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/><h1>Server is up and running! please login</h1><br><br><form action=\"/cmd_login\" method=\"POST\"><input type=\"text\" name=\"code\" placeholder=\"Code\"><br><input type=\"text\" name=\"loginToken\" placeholder=\"token\"><br><br><button type=\"submit\">Login</button>");
    } else {
      webServer.send(200, "text/html", "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/><h1>Server is offline! please login</h1><br><br><button onclick=\"goBack()\">Go Back</button><script>function goBack() {  window.history.back();}</script>");
    }
    // close tcp connection
  } else if (webServer.arg("command") == "close") {
    client.stop();
    webServer.sendHeader("Location", "/");
    webServer.send(303);
    // send command to tcp server
  }  else {

    tcp_sendData(webServer.arg("command"));

    // read response
    read_tcp_server();

    // format response for html
    globalResponse.replace("\n", "<br>");
    globalResponse.replace(" ", "&nbsp;");
    globalResponse = "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>" + globalResponse;
    globalResponse += "<br><br><button onclick=\"goBack()\">Go Back</button><script>function goBack() {  window.history.back();}</script>";
    webServer.send(200, "text/html", globalResponse);
  }
}


void handleNotFound() {
  webServer.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}


void tcp_sendData(String data) {

  // login to server
  Serial.print("\nsending data to server: ");
  Serial.println(data);
  if (client.connected()) {
    client.print(data);
  }
}


void read_tcp_server()
{
  delay(1000);
  while (client.available()) {

    String response = client.readStringUntil('>');
    Serial.println(response);
    if (response.length() > 1) {
      globalResponse = response;
    }

    if (response == "#### Connected to Communication Gateway, please Login ####\nLogin Token:") {
      tcp_sendData(loginToken);
    }
    
    if (response.startsWith("#### login sucessfull ####")) {
      tcp_sendData("help");
    }

    if (response.startsWith("#### implemented commands ####")) {
      implementedCommands = response;
    }
  }
}


void check_ap_connection()
{
  static wl_status_t preStatus = WL_DISCONNECTED;
  wl_status_t newStatus = WiFi.status();
  if (newStatus != preStatus)
  {
    if (newStatus == WL_CONNECTED)
    {
      digitalWrite(LED, LOW);

      // Display the own IP address and port
      Serial.print(F("AP connection established, listening on "));
      Serial.print(WiFi.localIP());
      Serial.print(":");
      Serial.println(PORT);
    }
    else
    {
      digitalWrite(LED, HIGH);
      Serial.println(F("AP conection lost"));
    }
    preStatus = newStatus;
  }
}

// generate pin method
void generatePin() {
  randomSeed(RANDOM_REG32);
  randNumber = random(1000, 10000);
  sprintf (codeArray, "%ld", randNumber);
}

// send wol method
void WOL(byte mac[6]) {

  digitalWrite(LED, HIGH);
  //the first 6 bytes of a WoL package are "0xFF"
  byte preamble[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };
  char i;

  //send a UDP package to the broadcast address of the actual configured network
  //to UDP port 7
  udpServer.beginPacket(bc, 7);

  //send preamble
  udpServer.write(preamble, sizeof preamble);

  //now send the MAC address 16 times as recommended by WoL protocol
  for (i = 0; i < 16; i++)
    udpServer.write(mac, 6);

  //finally end the package and send out
  udpServer.endPacket();
  delay(500);
  digitalWrite(LED, LOW);

}

// check server status method
bool checkServerStatus() {

  bool ret = Ping.ping(serverIP, 5);
  int avg_time_ms = Ping.averageTime();

  if (ret) {
    Serial.print(F("Ping ok: "));
    Serial.print(avg_time_ms);
    Serial.println(" ms");
    return true;
  } else {
    Serial.println(F("Ping failed"));
    return false;
  }

}

// send mail method
void sendMail() {
  SendEmail e(smtp_server, smtp_port, smtp_login, smtp_password, smtp_timeout, smtp_ssl);
  e.send(mail_from, mail_to, mail_subject, mail_text);
  Serial.println("E-Mail send");
}

// build email text and subject method
void generateMailText() {
  snprintf(mail_subject, 25, "WOL start code %ld", randNumber);
  snprintf(mail_text, 140, "Web Server is up and running\n http://%s:%u  \nInternal: http://%u.%u.%u.%u:%u Please use the following code: %ld", dynDNS, PORT, ip[0], ip[1], ip[2], ip[3], PORT, randNumber);
}
