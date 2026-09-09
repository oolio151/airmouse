#include <Arduino.h>
#include <Wire.h>
#include <BleMouse.h>

// stuff that should be tweaked the most commonly
#define MOUSE_FREQUENCY 200 // in hertz
#define SENSITIVITY 0.08f;
#define MOUSE_LEFT_PIN 15
#define MOUSE_RIGHT_PIN 5
#define MOUSE_UNLOCK_PIN 18

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;


constexpr uint8_t GYRO_ADDR = 0x6B;

// L3GD20H registers
constexpr uint8_t WHO_AM_I   = 0x0F;
constexpr uint8_t CTRL1      = 0x20;
constexpr uint8_t CTRL4      = 0x23;

constexpr uint8_t OUT_X_L    = 0x28;
constexpr uint8_t OUT_Y_L    = 0x2A;
constexpr uint8_t OUT_Z_L    = 0x2C;


BleMouse bleMouse("airmouse","oolio151",100);


float offsetX = 0.0f;
float offsetY = 0.0f;
float offsetZ = 0.0f;


// filters out smaller movements
constexpr float DEADZONE = 1.5f;

// used to retain fractional mouse movement
float accumulatedX = 0.0f;
float accumulatedY = 0.0f;

void writeRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(GYRO_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}


uint8_t readRegister(uint8_t reg)
{
    Wire.beginTransmission(GYRO_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(GYRO_ADDR, (uint8_t)1);

    if (Wire.available())
        return Wire.read();
    
    return 0;
}


int16_t readAxis(uint8_t lowRegister)
{
    Wire.beginTransmission(GYRO_ADDR);
    Wire.write(lowRegister | 0x80);
    Wire.endTransmission(false);
    Wire.requestFrom(GYRO_ADDR, (uint8_t)2);

    if (Wire.available() < 2)
        return 0;

    uint8_t low  = Wire.read();
    uint8_t high = Wire.read();

    return static_cast<int16_t>(
        (static_cast<uint16_t>(high) << 8) | low
    );
}


void readGyro(float &gx, float &gy, float &gz)
{
    int16_t rawX = readAxis(OUT_X_L);
    int16_t rawY = readAxis(OUT_Y_L);
    int16_t rawZ = readAxis(OUT_Z_L);

    constexpr float SCALE = 0.00875f;

    gx = rawX * SCALE;
    gy = rawY * SCALE;
    gz = rawZ * SCALE;
}

void calibrateGyro()
{
    constexpr int SAMPLE_COUNT = 500;

    float sumX = 0;
    float sumY = 0;
    float sumZ = 0;

    Serial.println();
    Serial.println("Calibrating gyro...");
    Serial.println("KEEP THE DEVICE STILL.");

    delay(1000);

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float gx;
        float gy;
        float gz;

        readGyro(gx, gy, gz);

        sumX += gx;
        sumY += gy;
        sumZ += gz;

        delay(2);
    }

    offsetX = sumX / SAMPLE_COUNT;
    offsetY = sumY / SAMPLE_COUNT;
    offsetZ = sumZ / SAMPLE_COUNT;

    Serial.println("Calibration finished.");

    Serial.printf(
        "Offsets: X=%.3f Y=%.3f Z=%.3f\n",
        offsetX,
        offsetY,
        offsetZ
    );
}


void setup()
{
    Serial.begin(115200);

    pinMode(MOUSE_LEFT_PIN, INPUT_PULLUP);
    pinMode(MOUSE_RIGHT_PIN, INPUT_PULLUP);
    pinMode(MOUSE_UNLOCK_PIN, INPUT_PULLUP);

    delay(1000);

    Serial.println();
    Serial.println("starting mouse...");

    Wire.begin(SDA_PIN, SCL_PIN);

    uint8_t whoAmI = readRegister(WHO_AM_I);

    Serial.printf(
        "L3GD20H WHO_AM_I = 0x%02X\n",
        whoAmI
    );

    writeRegister(CTRL1, 0x0F);
    writeRegister(CTRL4, 0x00);

    delay(100);

    calibrateGyro();

    // start the bluetooth stuff
    Serial.println("starting bluetooth");

    bleMouse.begin();
}


bool previousLeftPressed = false;
bool previousRightPressed = false;
  
void loop()
{
    if (digitalRead(MOUSE_UNLOCK_PIN) == LOW)
    {
        float gx;
        float gy;
        float gz;

        readGyro(gx, gy, gz);

        gx -= offsetX;
        gy -= offsetY;
        gz -= offsetZ;

        if (fabs(gx) < DEADZONE)
            gx = 0;

        if (fabs(gy) < DEADZONE)
            gy = 0;

        if (fabs(gz) < DEADZONE)
            gz = 0;

        float mouseX = -gx * SENSITIVITY;
        float mouseY =  gy * SENSITIVITY;

        accumulatedX += mouseX;
        accumulatedY += mouseY;

        int dx = static_cast<int>(accumulatedX);
        int dy = static_cast<int>(accumulatedY);

        accumulatedX -= dx;
        accumulatedY -= dy;

        if (bleMouse.isConnected() && (dx != 0 || dy != 0))
        {
            bleMouse.move(dx, dy);
        }
    }

    bool leftPressed  = digitalRead(MOUSE_LEFT_PIN) == LOW;
    bool rightPressed = digitalRead(MOUSE_RIGHT_PIN) == LOW;

    if (bleMouse.isConnected())
    {
        if (leftPressed != previousLeftPressed)
        {
            if (leftPressed)
            {
                bleMouse.press(MOUSE_LEFT);
                Serial.println("left pressed");
            }
            else
            {
                bleMouse.release(MOUSE_LEFT);
                Serial.println("left released");
            }

            previousLeftPressed = leftPressed;
        }

        if (rightPressed != previousRightPressed)
        {
            if (rightPressed)
            {
                bleMouse.press(MOUSE_RIGHT);
                Serial.println("right pressed");
            }
            else
            {
                bleMouse.release(MOUSE_RIGHT);
                Serial.println("right released");
            }

            previousRightPressed = rightPressed;
        }
    }

    delay(1000 / MOUSE_FREQUENCY);
}