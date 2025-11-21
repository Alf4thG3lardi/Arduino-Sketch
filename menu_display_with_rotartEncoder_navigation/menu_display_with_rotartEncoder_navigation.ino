#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// =================== DISPLAY ======================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_HEIGHT, SCREEN_WIDTH, &Wire);

// ================= ROTARY PIN =====================
#define PIN_CLK 34
#define PIN_DT  35
#define PIN_SW  32

TaskHandle_t TaskEncoderHandle;

// =============== ENCODER STATE =====================
volatile int movementCount = 0;
volatile unsigned long lastMovementTime = 0;
unsigned long timeoutGap = 500;     // 0.5 detik

// Filter
const int sampleFilter = 4;
int stableCLK = 1, stableDT = 1;
int lastCLK = 1;

// =================== MENU ==========================
int menuIndex = 0;
String menuList[4] = {
  "Status",
  "Settings",
  "Info",
  "MotorCtrl"
};
int menuCount = 4;

// =================== DRAW MENU =====================
void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);

  for (int i = 0; i < menuCount; i++) {
    if (i == menuIndex) {
      display.fillRect(0, i * 16, 128, 16, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }

    display.setCursor(5, i * 16 + 4);
    display.print(menuList[i]);
  }

  display.display();
}

// =============== MENU ACTION ===================
void runMenuFunction(int index) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(5, 60);
  display.print("Running: ");
  display.println(menuList[index]);

  display.display();
  delay(800);

  drawMenu();
}

// =================================================
//         FILTERED ROTARY READ FUNCTION
// =================================================
void readEncoderFiltered() {
  static int clkSamples = 0;
  static int dtSamples = 0;

  int rawCLK = digitalRead(PIN_CLK);
  int rawDT  = digitalRead(PIN_DT);

  // Filter sample stabil
  if (rawCLK != stableCLK) {
    if (++clkSamples >= sampleFilter) {
      stableCLK = rawCLK;
      clkSamples = 0;
    }
  } else clkSamples = 0;

  if (rawDT != stableDT) {
    if (++dtSamples >= sampleFilter) {
      stableDT = rawDT;
      dtSamples = 0;
    }
  } else dtSamples = 0;

  // deteksi tepi turun CLK
  if (stableCLK != lastCLK && stableCLK == 0) {

    if (stableDT == 1)
      movementCount++;
    else
      movementCount--;

    lastMovementTime = millis();
  }

  lastCLK = stableCLK;
}

// =================================================
//                 ENCODER TASK
// =================================================
void TaskEncoder(void *pvParams) {
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT,  INPUT_PULLUP);
  pinMode(PIN_SW,  INPUT_PULLUP);

  lastCLK = digitalRead(PIN_CLK);
  lastMovementTime = millis();

  while (1) {

    readEncoderFiltered();
    unsigned long now = millis();

    // Jika batch selesai
    if ((now - lastMovementTime) > timeoutGap && movementCount != 0) {

      int total = movementCount;
      movementCount = 0;

      Serial.print("Batch Movement: ");
      Serial.println(total);

      if (abs(total) == 1) {
        // --- PINDAH MENU ---
        if (total > 0) menuIndex++;
        else menuIndex--;

        if (menuIndex < 0) menuIndex = menuCount - 1;
        if (menuIndex >= menuCount) menuIndex = 0;

        drawMenu();
      } 
      else {
        // --- JALANKAN FUNGSI MENU ---
        runMenuFunction(menuIndex);
      }

      lastMovementTime = millis();
    }

    // Tombol SW
    if (digitalRead(PIN_SW) == LOW) {
      display.clearDisplay();
      display.setCursor(10, 60);
      display.print("SW Pressed!");
      display.display();
      delay(300);
      drawMenu();
      while (digitalRead(PIN_SW) == LOW);
    }

    vTaskDelay(1);
  }
}

// =================================================
//                      SETUP
// =================================================
void setup() {
  Serial.begin(115200);

  // OLED init
  display.begin(0x3C, true);  // I2C Address 0x3C
  display.clearDisplay();
  display.display();

  display.setRotation(0);

  // Show loading
  display.setCursor(10, 60);
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.println("OLED Ready");
  display.display();
  delay(600);

  drawMenu();

  // RTOS
  xTaskCreatePinnedToCore(
    TaskEncoder,
    "TaskEncoder",
    4096,
    NULL,
    2,
    &TaskEncoderHandle,
    1
  );
}

void loop() {
  // kosong, RTOS yang bekerja
}
