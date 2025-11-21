/*
   ESP32 AP Mode + RTOS + 2-Slider PWM Motor Control
   Controls left & right motor speed using PWM.
*/

#include <WiFi.h>
#include <WebServer.h>

// -------- PIN CONFIG (EDIT to match your wiring) --------
const int LEFT_FWD = 16;
const int LEFT_BWD = 17;
const int RIGHT_FWD = 18;
const int RIGHT_BWD = 19;

// PWM channels
#define CH_L 0
#define CH_R 1

// -------- AP CONFIG --------
const char* ap_ssid = "ESP32-RTOS-PWM";
const char* ap_password = "12345678";

WebServer server(80);

// -------- RTOS --------
typedef struct {
  int leftSpeed;   // -255 to 255 (negative = backward)
  int rightSpeed;  // -255 to 255 (negative = backward)
} MotorPWM;

QueueHandle_t pwmQueue;

// ---------------------------------------------------------
// HTML PAGE WITH 2 SLIDERS
// ---------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
body { font-family: Arial; text-align: center; padding-top: 20px; }
.slider { width: 300px; }
label { font-size: 20px; }
</style>
</head>
<body>
<h2>ESP32 RTOS PWM Motor Control</h2>

<label>Left Motor</label><br>
<input type='range' min='-255' max='255' value='0' class='slider' id='left'><br><br>

<label>Right Motor</label><br>
<input type='range' min='-255' max='255' value='0' class='slider' id='right'><br><br>

<script>
function send() {
  let L = document.getElementById("left").value;
  let R = document.getElementById("right").value;
  fetch(`/set?l=${L}&r=${R}`);
}
document.querySelectorAll('.slider').forEach(s => {
  s.oninput = send;
});
</script>

</body>
</html>
)rawliteral";

// ---------------------------------------------------------
// SEND SLIDER VALUES → QUEUE
// ---------------------------------------------------------
void handleSetPWM() {
  MotorPWM cmd;

  cmd.leftSpeed = server.hasArg("l") ? server.arg("l").toInt() : 0;
  cmd.rightSpeed = server.hasArg("r") ? server.arg("r").toInt() : 0;

  xQueueOverwrite(pwmQueue, &cmd);
  server.send(200, "text/plain", "OK");
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

// ---------------------------------------------------------
// MOTOR APPLY
// ---------------------------------------------------------
void applyMotorPWM(int channel, int pinFwd, int pinBwd, int pwmValue) {
  pwmValue = constrain(pwmValue, -255, 255);

  if (pwmValue > 0) {
    digitalWrite(pinBwd, LOW);
    digitalWrite(pinFwd, HIGH);
    ledcWrite(channel, pwmValue);
  } else if (pwmValue < 0) {
    digitalWrite(pinFwd, LOW);
    digitalWrite(pinBwd, HIGH);
    ledcWrite(channel, -pwmValue);
  } else {
    digitalWrite(pinFwd, LOW);
    digitalWrite(pinBwd, LOW);
    ledcWrite(channel, 0);
  }
}

// ---------------------------------------------------------
// TASK: Handle Web Server
// ---------------------------------------------------------
void WebTask(void* pv) {
  server.on("/", handleRoot);
  server.on("/set", handleSetPWM);

  server.begin();
  Serial.println("Web server running.");

  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ---------------------------------------------------------
// TASK: Motor Control (reads queue struct)
// ---------------------------------------------------------
void MotorTask(void* pv) {
  MotorPWM cmd = {0, 0};

  for (;;) {
    if (xQueueReceive(pwmQueue, &cmd, portMAX_DELAY)) {
      applyMotorPWM(CH_L, LEFT_FWD, LEFT_BWD, cmd.leftSpeed);
      applyMotorPWM(CH_R, RIGHT_FWD, RIGHT_BWD, cmd.rightSpeed);

      Serial.printf("L=%d  R=%d\n", cmd.leftSpeed, cmd.rightSpeed);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ---------------------------------------------------------
// SETUP
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(LEFT_FWD, OUTPUT);
  pinMode(LEFT_BWD, OUTPUT);
  pinMode(RIGHT_FWD, OUTPUT);
  pinMode(RIGHT_BWD, OUTPUT);

  // PWM setup
  ledcSetup(CH_L, 20000, 8); // 20 kHz, 8-bit
  ledcSetup(CH_R, 20000, 8);

  ledcAttachPin(LEFT_FWD, CH_L);
  ledcAttachPin(RIGHT_FWD, CH_R);

  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  pwmQueue = xQueueCreate(1, sizeof(MotorPWM));

  xTaskCreatePinnedToCore(WebTask, "WebTask", 6000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(MotorTask, "MotorTask", 4000, NULL, 2, NULL, 0);

  Serial.println("System Ready.");
}

// ---------------------------------------------------------
// LOOP
// ---------------------------------------------------------
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
