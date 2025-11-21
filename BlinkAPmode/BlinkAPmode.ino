#include <WiFi.h>
#include <WebServer.h>

// AP credentials
const char* ssid = "ESP32-AP";
const char* password = "12345678";

// Built-in LED pin for many ESP32-S3 devkits (change if needed)
#define LED_BUILTIN 48

WebServer server(80);
bool ledState = false;

const char* htmlTemplate = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32-S3 LED Control</title>
  <style>
    :root{
      --bg: #0f1724;
      --card: #0b1220;
      --accent: #7dd3fc;
      --accent-2: #60a5fa;
      --muted: #94a3b8;
      --success: #22c55e;
      --danger: #ef4444;
      --glass: rgba(255,255,255,0.03);
      font-family: "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    }
    html,body{
      height:100%;
      margin:0;
      background: linear-gradient(180deg, #071028 0%, #00121b 100%);
      color: #e6eef8;
    }
    .wrap{
      min-height:100%;
      display:flex;
      align-items:center;
      justify-content:center;
      padding:24px;
      box-sizing:border-box;
    }
    .card{
      width:100%;
      max-width:520px;
      background: linear-gradient(180deg, rgba(255,255,255,0.02), rgba(255,255,255,0.01));
      border-radius:14px;
      padding:26px;
      box-shadow: 0 8px 30px rgba(2,6,23,0.6);
      border: 1px solid rgba(255,255,255,0.03);
      backdrop-filter: blur(6px);
    }
    h1{
      margin:0 0 8px 0;
      font-size:20px;
      letter-spacing:0.2px;
    }
    p.lead{
      margin:0 0 18px 0;
      color:var(--muted);
      font-size:14px;
    }
    .status {
      display:inline-flex;
      gap:10px;
      align-items:center;
      margin-bottom:18px;
    }
    .badge {
      display:inline-block;
      min-width:84px;
      padding:8px 12px;
      border-radius:999px;
      font-weight:600;
      text-align:center;
      box-shadow: 0 6px 18px rgba(2,6,23,0.5);
      border: 1px solid rgba(255,255,255,0.02);
    }
    .badge.on { background: linear-gradient(90deg,var(--success), #16a34a); color:#022c06; }
    .badge.off { background: linear-gradient(90deg,var(--danger), #dc2626); color:#2a0303; }
    form{ display:flex; gap:12px; align-items:center; flex-wrap:wrap; }
    button.cta {
      -webkit-appearance:none;
      appearance:none;
      border: none;
      padding:14px 22px;
      border-radius:12px;
      font-size:16px;
      font-weight:700;
      cursor:pointer;
      background: linear-gradient(90deg,var(--accent),var(--accent-2));
      color: #022034;
      box-shadow: 0 10px 30px rgba(46, 128, 255, 0.12), inset 0 -2px 6px rgba(0,0,0,0.18);
      transition: transform 120ms ease, box-shadow 120ms ease;
    }
    button.cta:active { transform: translateY(2px); box-shadow: 0 6px 20px rgba(46,128,255,0.08); }
    .small {
      font-size:13px;
      color:var(--muted);
    }
    footer { margin-top:16px; font-size:12px; color:var(--muted); }
    @media (max-width:480px){
      .card{ padding:18px; border-radius:10px; }
      button.cta{ width:100%; padding:12px; }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>ESP32-S3 — LED Control</h1>
      <p class="lead">Connect to the Wi-Fi network <strong>ESP32-AP</strong> (password <em>12345678</em>), open the browser, and use the button below to toggle the built-in LED.</p>

      <div class="status">
        <!-- status badge inserted server-side -->
        %STATUS_BADGE%
        <div class="small">IP: <strong>192.168.4.1</strong></div>
      </div>

      <form action="/toggle" method="POST">
        <button class="cta" type="submit">Toggle LED</button>
      </form>

      <footer>Simple AP webserver • ESP32-S3</footer>
    </div>
  </div>
</body>
</html>
)rawliteral";

void handleRoot() {
  String badge;
  if (ledState) {
    badge = "<div class='badge on'>ON</div>";
  } else {
    badge = "<div class='badge off'>OFF</div>";
  }

  String page = String(htmlTemplate);
  page.replace("%STATUS_BADGE%", badge);
  server.send(200, "text/html", page);
}

void handleToggle() {
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  handleRoot();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Start WiFi AP
  WiFi.softAP(ssid, password);
  Serial.print("AP started. IP: ");
  Serial.println(WiFi.softAPIP()); // usually 192.168.4.1

  // Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_POST, handleToggle);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
