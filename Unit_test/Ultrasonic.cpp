/*Author: Carson Sun and Terence Leung
  Date last modified: 14/04/2024
  Functionality: Motor control
  Version: us7 
  Course: Realtime embedded programming
  Belongs to: Group 18 */

#include "pigpio.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>


//Global constant for I2C address on PCA9685 
const int PCA9685_ADDR = 0x40;
const int TRIG_PIN = 27;
const int ECHO_PIN = 22;
const float SOUND_SPEED = 34300;


class PCA9685 {
private:
    int i2cHandle;
    int servo_channel=8;

public:
    PCA9685(int handle) : i2cHandle(handle) {}

    void initialise() {
        i2cWriteByteData(i2cHandle, 0x00, 0x20);
        int freq = 1000;
        int prescale = (25000000 / (4096 * freq)) - 1;
        i2cWriteByteData(i2cHandle, 0xFE, prescale); // Set PWM frequency to 1000 Hz
    }

    void setPWM(int channel, int on, int off) {
        int regBase = 0x06 + 4*channel;
        i2cWriteByteData(i2cHandle, regBase, on & 0xFF);
        i2cWriteByteData(i2cHandle, regBase + 1, on >> 8);
        i2cWriteByteData(i2cHandle, regBase + 2, off & 0xFF);
        i2cWriteByteData(i2cHandle, regBase + 3, off >> 8);
    }

    void setServoPWM( float angle) {
        float pulse = (angle / 180.0 * (2500 - 500)) + 500; // Map 0-180 degrees to 500-2500 microseconds
        int on = 0; // PWM signal starts at the beginning of the cycle
        int off = static_cast<int>((pulse / 20000.0) * 4096); // Convert microseconds to off tick for 50 Hz
        setPWM(servo_channel, on, off);
    }
};


class Ultrasonic {
private:
    int trigPin, echoPin;
    PCA9685& pca;
    int servoChannel;

public:
    // Constructor that initializes pin numbers
    Ultrasonic(int triggerPin, int echoPin, PCA9685& pcaController, int servoCh) :
        trigPin(triggerPin), echoPin(echoPin), pca(pcaController), servoChannel(servoCh) {
        gpioSetMode(trigPin, PI_OUTPUT);
        gpioSetMode(echoPin, PI_INPUT);
        gpioWrite(trigPin, PI_LOW);
        std::this_thread::sleep_for(std::chrono::seconds(2)); // settle sensor
    }

    /*void setAngle(float angle) {
        pca.setServoPWM( angle);
    }

    float sweepAndGetDistance() {
        float distances[3];
        float angles[] = {45, 90, 125}; // Sweep angles: Left, Center, Right

        for (int i = 0; i < 3; i++) {
            setAngle(angles[i]);
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for servo to reach position
            distances[i] = getDistance();
        }

        setAngle(90); // Reset to center
        return *std::min_element(distances, distances + 3); // Return the smallest distance
    }*/

    void setup() {
        gpioSetMode(trigPin, PI_OUTPUT);
        gpioSetMode(echoPin, PI_INPUT);
        gpioWrite(trigPin, PI_LOW);
    }

    float getDistance() {
        gpioWrite(trigPin, PI_HIGH);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        gpioWrite(trigPin, PI_LOW);

        auto start = std::chrono::high_resolution_clock::now();
        while (gpioRead(echoPin) == PI_LOW);
        auto stop = std::chrono::high_resolution_clock::now();

        while (gpioRead(echoPin) == PI_HIGH);
        stop = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = stop - start;
        return (elapsed.count() * SOUND_SPEED) / 2.0; // Speed of sound in cm/s at room temp
    }
};


class Motor {
private:
    PCA9685& pca;

public:
    Motor(PCA9685& pcaController) : pca(pcaController) {}

    void Forward() {
        pca.setPWM(0, 0, 0);
        pca.setPWM(1, 0, 1024);
        pca.setPWM(3, 0, 0);
        pca.setPWM(2, 0, 1024);
        pca.setPWM(4, 0, 0);
        pca.setPWM(5, 0, 1024);
        pca.setPWM(6, 0, 0);
        pca.setPWM(7, 0, 1024);
    }

    void Stop() {
        pca.setPWM(0, 0, 0);
        pca.setPWM(1, 0, 0);
        pca.setPWM(3, 0, 0);
        pca.setPWM(2, 0, 0);
        pca.setPWM(4, 0, 0);
        pca.setPWM(5, 0, 0);
        pca.setPWM(6, 0, 0);
        pca.setPWM(7, 0, 0);
    }
};


int main() {
    if (gpioInitialise() < 0) {
        std::cerr << "Pigpio initialisation failed." << std::endl;
        return 1;
    }

    int i2cHandle = i2cOpen(1, PCA9685_ADDR, 0);
    if (i2cHandle < 0) {
        std::cerr << "Failed to open I2C communication." << std::endl;
        return 1;
    }

    PCA9685 pca(i2cHandle);
    pca.initialise();

    Motor motor(pca);
    Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN, pca, 0);
    ultrasonic.setup();

    while (true) {
        float distance = ultrasonic.getDistance();
        std::cout << "Distance: " << distance << "mm" << std::endl;

        if (distance != -1 && distance < 85) {
            motor.Stop();
        } else {
            motor.Forward();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    gpioTerminate();
    return 0;
}
