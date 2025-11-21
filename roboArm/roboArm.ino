#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ==== WiFi AP Setup ====
const char* ssid = "RobotArm_AP";

const char* password = "12345678";

WebServer server(80);

// ==== Servo Setup ====
Servo servoBase;
Servo servoShoulder;
Servo servoElbow;

const int SERVO_BASE_PIN = 15;
const int SERVO_SHOULDER_PIN = 16;
const int SERVO_ELBOW_PIN = 17;

// ==== Robot Arm Geometry ====
const float L1 = 10.0;  // length of first arm segment (cm)
const float L2 = 10.0;  // length of second arm segment (cm)

// Target coords
float targetX = 12.0;
float targetY = 5.0;
float targetZ = 8.0;

// ==== Inverse Kinematics ====
void inverseKinematics(float x, float y, float z, float &thetaBase, float &thetaShoulder, float &thetaElbow) {
  // 1. Base rotation
  thetaBase = atan2(y, x) * 180.0 / PI;

  // 2. Distance in XY plane
  float r = sqrt(x * x + y * y);
  float D = sqrt(r * r + z * z);

  // 3. Elbow
  float cosElbow = (L1*L1 + L2*L2 - D*D) / (2 * L1 * L2);
  cosElbow = constrain(cosElbow, -1.0, 1.0);
  thetaElbow = acos(cosElbow) * 180.0 / PI;

  // 4. Shoulder
  float cosShoulder = (L1*L1 + D*D - L2*L2) / (2 * L1 * D);
  cosShoulder = constrain(cosShoulder, -1.0, 1.0);
  float shoulderAngle = acos(cosShoulder);

  float angleToTarget = atan2(z, r);
  thetaShoulder = (angleToTarget + shoulderAngle) * 180.0 / PI;
}

// ==== Webpage ====
String htmlPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Robot Arm</title>
  <style>
    body { font-family: Arial; text-align: center; background: #f0f0f0; }
    h1 { color: #333; }
    .slider-container { margin: 20px; }
    input[type=range] { width: 80%; }
    .value { font-size: 20px; }
  </style>
</head>
<body>
  <h1>3-Axis Robot Arm Control</h1>

  <div class="slider-container">
    <label>X: <span id="xVal">12</span></label><br>
    <input type="range" min="0" max="20" value="12" id="xSlider" oninput="updateValue('x')">
  </div>

  <div class="slider-container">
    <label>Y: <span id="yVal">5</span></label><br>
    <input type="range" min="0" max="20" value="5" id="ySlider" oninput="updateValue('y')">
  </div>

  <div class="slider-container">
    <label>Z: <span id="zVal">8</span></label><br>
    <input type="range" min="0" max="20" value="8" id="zSlider" oninput="updateValue('z')">
  </div>

  <script>
    function updateValue(axis) {
      let val = document.getElementById(axis + "Slider").value;
      document.getElementById(axis + "Val").innerText = val;
      fetch("/set?" + axis + "=" + val);
    }
  </script>
</body>
</html>
)rawliteral";
  return page;
}

// ==== Handlers ====
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSet() {
  if (server.hasArg("x")) targetX = server.arg("x").toFloat();
  if (server.hasArg("y")) targetY = server.arg("y").toFloat();
  if (server.hasArg("z")) targetZ = server.arg("z").toFloat();

  float thetaBase, thetaShoulder, thetaElbow;
  inverseKinematics(targetX, targetY, targetZ, thetaBase, thetaShoulder, thetaElbow);

  servoBase.write((int)thetaBase);
  servoShoulder.write((int)thetaShoulder);
  servoElbow.write((int)thetaElbow);

  Serial.printf("Target (%.2f, %.2f, %.2f) => Base: %.2f, Shoulder: %.2f, Elbow: %.2f\n",
                targetX, targetY, targetZ, thetaBase, thetaShoulder, thetaElbow);

  server.send(200, "text/plain", "OK");
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);

  // Servo attach
  servoBase.attach(SERVO_BASE_PIN);
  servoShoulder.attach(SERVO_SHOULDER_PIN);
  servoElbow.attach(SERVO_ELBOW_PIN);

  // Start WiFi AP
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
