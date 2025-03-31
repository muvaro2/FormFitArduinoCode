#include <ArduinoBLE.h>
#include <Adafruit_LSM6DSOX.h>

// Create sensor instance
Adafruit_LSM6DSOX sox = Adafruit_LSM6DSOX();

// Define custom UUIDs for your service and characteristic
const char* serviceUuid      = "12345678-1234-5678-1234-56789abcdef0";
const char* sensorCharUuid   = "12345678-1234-5678-1234-56789abcdef1";

// Create a service and one characteristic (6 floats = 24 bytes)
BLEService sensorService(serviceUuid);
BLECharacteristic sensorCharacteristic(sensorCharUuid, BLERead | BLENotify, 24);

void setup() {
  Serial.begin(115200);
  while (!Serial);  // wait for serial monitor
  Serial.println("Starting BLE sensor peripheral");

  // initialize BLE
  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1);
  }

  // Initialize the LSM6DSOX sensor (assuming I2C)
  if (!sox.begin_I2C()) {
    Serial.println("Failed to find LSM6DSOX sensor!");
    while (1);
  }
  // Optionally set sensor ranges (adjust as desired)
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);

  // Set BLE device name and advertise our service
  BLE.setLocalName("FormFit");
  BLE.setAdvertisedService(sensorService);

  // Add our characteristic to the service, then add service
  sensorService.addCharacteristic(sensorCharacteristic);
  BLE.addService(sensorService);

  // Initialize characteristic value to zero
  uint8_t initData[24] = {0};
  sensorCharacteristic.writeValue(initData, 24);

  // Start advertising
  BLE.advertise();
  Serial.println("BLE device is now advertising...");
}

void loop() {
  // Wait for a central to connect
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    
    // While the central is connected, read and update sensor data
    while (central.connected()) {
      sensors_event_t accel, gyro, temp;
      sox.getEvent(&accel, &gyro, &temp);
      
      // Pack sensor data: [gyroX, gyroY, gyroZ, accelX, accelY, accelZ]
      float sensorData[6] = {
        gyro.gyro.x, gyro.gyro.y, gyro.gyro.z,
        accel.acceleration.x, accel.acceleration.y, accel.acceleration.z
      };

      // Copy the 6 floats (24 bytes) into a byte array
      uint8_t data[24];
      memcpy(data, sensorData, sizeof(sensorData));

      // Update the BLE characteristic with the new data
      sensorCharacteristic.writeValue(data, sizeof(data));
      
      delay(100);  // update at ~10 Hz
    }
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
