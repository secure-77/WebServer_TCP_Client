# ESP8266 WebServer TCP Client / GUI

This little Web TCP Client provides a GUI to control a text based tcp server. The code is highly specified to this [python tcp server](https://github.com/secure-77/PythonTCPServer)

#### On start

1. auto connect to your wifi
2. generate an OTP and send it via email
3. provides a webserver on the wifi ip with login mask

#### Login

4. check if OTP was correct
5. check if tcp server is online
5. login to the tcp server or (if server is offline: provides a web page with ping and WOL function)
6. if server is online: login to tcp-server with token
7. retrieve available commands from tcp-server
8. provides the commands on a web-site as buttons
9. on pressing a command button, send the command to the server and show the result


## Features

- OTP via E-Mail
- check server status via ping
- can sent WOL packages
- simple webserver authentication
- simple tcp-server authentication
- send commands to a tcp server
- receive responses from a tcp server

## Requirements
- Arduino board with ESP8266  i used ![](https://img.shields.io/badge/ESP8266-v%202.3.0-green) https://github.com/esp8266/Arduino
- These libs (you can find them in the libraries folder): 
https://github.com/gpepe/esp8266-sendemail
https://github.com/dancol90/ESP8266Ping
https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer


## Screenshots

### Login Mask
[![](https://github.com/secure-77/WebServer_TCP_Client/blob/main/login.png)](https://github.com/secure-77/WebServer_TCP_Client/blob/main/login.png)

## no connection to tcp-server
[![](https://github.com/secure-77/WebServer_TCP_Client/blob/main/ping%20and%20wol.png)](https://github.com/secure-77/WebServer_TCP_Client/blob/main/ping%20and%20wol.png)

## sending WOL
[![](https://github.com/secure-77/WebServer_TCP_Client/blob/main/wol.png)](https://github.com/secure-77/WebServer_TCP_Client/blob/main/wol.png)

## commands
[![](https://github.com/secure-77/WebServer_TCP_Client/blob/main/commands.png)](https://github.com/secure-77/WebServer_TCP_Client/blob/main/commands.png)

## response
[![](https://github.com/secure-77/WebServer_TCP_Client/blob/main/response.png)](https://github.com/secure-77/WebServer_TCP_Client/blob/main/response.png)

