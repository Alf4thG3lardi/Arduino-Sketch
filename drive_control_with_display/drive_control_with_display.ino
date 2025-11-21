#include <Arduino.h>
#include <U8g2lib.h>

#define MOTOR_IN1  32
#define MOTOR_IN2  33
#define MOTOR_PWM  25

#define ENCODER_A  23
#define ENCODER_B  18
#define POT_PIN    26   // Potentiometer (ADC)

#define MOTOR_PWM_CHANNEL 0
#define MOTOR_PWM_FREQ 20000      // 20 kHz → aman untuk motor
#define MOTOR_PWM_RES 8    

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

volatile int menuIndex = 0;
const char *modes[] = {"Eco", "Normal", "Performance"};

int accelScale = 50; // default Eco
int gasValue = 0;

/* ------------------- TASK: ENCODER ------------------- */
void encoderTask(void *pv)
{
    int lastA = digitalRead(ENCODER_A);

    for (;;)
    {
        int a = digitalRead(ENCODER_A);

        if (a != lastA)
        {
            if (digitalRead(ENCODER_B) != a) menuIndex++;
            else menuIndex--;

            if (menuIndex < 0) menuIndex = 2;
            if (menuIndex > 2) menuIndex = 0;

            lastA = a;
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

/* ------------------- TASK: POTENSIO GAS ------------------- */
void gasTask(void *pv)
{
    for (;;)
    {
        int raw = analogRead(POT_PIN);      // 0–4095
        raw = map(raw, 0, 4095, 0, 255);    // 0–255

        int speed = 0;

        // === NON-LINEAR SCALING PER MODE ===
        if (menuIndex == 0) {
            // ECO → lambat naiknya (kurva kuadrat)
            speed = (raw * raw) / 255;      
        }
        else if (menuIndex == 1) {
            // NORMAL → linear
            speed = raw;
        }
        else if (menuIndex == 2) {
            // PERFORMANCE → sangat sensitif (kurva akar)
            speed = sqrt(raw) * 16;          // kasi boost
        }

        // batas 0–255
        gasValue = constrain(speed, 0, 255);

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

/* ------------------- TASK: MOTOR ------------------- */
void motorTask(void *pv)
{
  ledcSetup(MOTOR_PWM_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcAttachPin(MOTOR_PWM, MOTOR_PWM_CHANNEL);

  while(1) {
    int pot = analogRead(POT_PIN);     // 0 – 4095
    int speed = map(pot, 0, 4095, 0, 255);

    ledcWrite(MOTOR_PWM_CHANNEL, speed);

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/* ------------------- TASK: OLED ------------------- */
void oledTask(void *pv)
{
    u8g2.begin();

    for (;;)
    {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_mr);

        u8g2.drawStr(0, 10, "Mode:");

        for (int i = 0; i < 3; i++)
        {
            if (menuIndex == i)
                u8g2.drawStr(0, 24 + i * 12, ">");

            u8g2.drawStr(14, 24 + i * 12, modes[i]);
        }

        char buf[20];
        sprintf(buf, "Gas: %d", gasValue);
        u8g2.drawStr(0, 60, buf);

        u8g2.sendBuffer();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

/* ------------------- SETUP ------------------- */
void setup()
{
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    pinMode(ENCODER_A, INPUT);
    pinMode(ENCODER_B, INPUT);

    digitalWrite(MOTOR_IN1, HIGH);   // forward
    digitalWrite(MOTOR_IN2, LOW);    // forward only

    analogReadResolution(12); // ESP32 max 4095

    xTaskCreate(encoderTask, "ENC", 2048, NULL, 1, NULL);
    xTaskCreate(gasTask, "GAS", 2048, NULL, 1, NULL);
    xTaskCreate(motorTask, "MOTOR", 4096, NULL, 1, NULL);
    xTaskCreate(oledTask, "OLED", 3072, NULL, 1, NULL);
}

void loop() {}
