// Library includes
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>

// Defines buffer size for HTTP clients.
#define REQ_BUF_SZ 60

// Defines ETH things 
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,102);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
EthernetServer server(84);
EthernetClient client;
File webFile;
char HTTP_req[REQ_BUF_SZ] = {0};
char req_index = 0;

EthernetClient testclient;
byte testip[] = { 192, 168, 1, 100 };

String output = "...";

unsigned long startTime;
int auth;

void setup(){
  
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
  pinMode(5, INPUT);

  Serial.begin(9600);
  
  SD.begin(4);
  SD.exists("index.htm");
  
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  server.begin();
  Serial.print("Server is at IP ");
  Serial.println(Ethernet.localIP());
  
  SPI.begin();
}

void loop(){
  
  output = "...";
  
  // Web client AJAX
  EthernetClient client = server.available();
  if (client){
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (req_index < (REQ_BUF_SZ - 1)){
          HTTP_req[req_index] = c;
          req_index++;
        }
        //Serial.println(HTTP_req);
        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          if (StrContains(HTTP_req, "ajax_inputs")) {
            client.println("Content-Type: text/xml");
            client.println("Connection: keep-alive");
            client.println();
            Do();
            Done(client);
          } else {
            client.println("Content-Type: text/html");
            client.println("Connection: keep-alive");
            client.println();
            webFile = SD.open("index.htm");
            if (webFile) {
              while(webFile.available()) {
                client.write(webFile.read());
              }
              webFile.close();
            }
          }
        req_index = 0;
        StrClear(HTTP_req, REQ_BUF_SZ);
        break;
        } 
        if (c == '\n'){
          currentLineIsBlank = true;
        }else if (c != '\r'){
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
  }
}

// Test de connexion
bool isOnlineSSH(int essais, int msec) {
  int i = 0;
  // on boucle autant de d'essais demandés
  while(i < essais) {
    if(testclient.connect(testip, 22)==1) {
      // le serveur répond
      testclient.stop();
      // on s'arrête
      return 1;
    } else {
      delay(msec);
      i++;
    }
  }
  client.println();
  return 0;
}

// Test de connexion
bool isOnline5V() {
  if(digitalRead(5))
    return 0;
  else
    return 1;
}

void Do(void) { 
  if(millis() - startTime > 60000 && auth == 1){
    Serial.println(millis() - startTime);
    output = "-- Auth Required --";
    auth = 0;
  }else if(auth == 1){
    if(output.length() == 0) {
    } else if(StrContains(HTTP_req, "SSH")) {
      // test
      if(isOnlineSSH(2,1000))
        output = "| SSH Machine en fonction";
      else
        output = "| SSH Machine silencieuse";
    } else if(StrContains(HTTP_req, "5V")) {
      // test
      if(isOnline5V())
        output = "| 5V Machine en fonction";
      else
        output = "| 5V Machine silencieuse";
    } else if(StrContains(HTTP_req, "start")) {
      if(isOnline5V()) {
        output = "| Machine deja en fonction";
      } else {
        // allumage
        output = "| Starting";
        digitalWrite(A0, LOW);
        delay(500);
        digitalWrite(A0, HIGH);
      }
    } else if(StrContains(HTTP_req, "reboot")) {
      // forcage
      output = "| Restart";
      digitalWrite(A0, LOW);
      delay(5000);
      digitalWrite(A0, HIGH);
      delay(2500);
      digitalWrite(A0, LOW);
      delay(500);
      digitalWrite(A0, HIGH);
    } else if(StrContains(HTTP_req, "stop")) {
      // forcage
      output = "| Stop";
      digitalWrite(A0, LOW);
      delay(7000);
      digitalWrite(A0, HIGH);
    } else if(StrContains(HTTP_req, "force")) {
        output = "...";
    } else if(HTTP_req[17] != 'n' && HTTP_req[18] != 'o' && HTTP_req[19] != 'c' && HTTP_req[20] != 'a' && HTTP_req[21] != 'c' && HTTP_req[22] != 'h' && HTTP_req[23] != 'e') {
        output = "| Commande non reconnue !";
    } 
  }
  if(StrContains(HTTP_req, "auth")) {
    if(StrContains(HTTP_req, "promete")) {
      startTime = millis();
      auth = 1;
      output = "-- Success --";
    } else {
      output = "-- Password required --";
    }
  }
  if(HTTP_req[17] != 'n' && HTTP_req[18] != 'o' && HTTP_req[19] != 'c' && HTTP_req[20] != 'a' && HTTP_req[21] != 'c' && HTTP_req[22] != 'h' && HTTP_req[23] != 'e' && auth == 0){
    output = "-- Password required --";
  }
  if(output != "..."){
    Serial.println(output);
  }
}

// AJAX things
void Done(EthernetClient cl){
  cl.print("<?xml version = \"1.0\" ?>");
  cl.print("<output>");
  cl.print("<command>");
  if (output.length() != 0) {
    cl.print(output);
  }
  cl.println("</command>");
  cl.print("</output>");
}

void StrClear(char *str, char length){
  for (int i = 0; i < length; i++) {
    str[i] = 0;
  }
}

char StrContains(char *str, const char *sfind){
  char found = 0;
  char index = 0;
  char len;
  len = strlen(str);

  if (strlen(sfind) > len) {
    return 0;
  }
  while (index < len) {
    if (str[index] == sfind[found]) {
      found++;
      if (strlen(sfind) == found) {
        return 1;
      }
    }else{
      found = 0;
    }
    index++;
  }
  return 0;
}

