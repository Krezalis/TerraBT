# TerraBT
Read radiation value from dosimeter

Information how to: https://blog.uaid.net.ua/connect-dosimeter-bluetooth/

![ESP32](https://blog.uaid.net.ua/wp-content/uploads/2019/07/ESP32_terra_WS.jpg)

Usage:
```
void setup() {
  ESP_BT.begin("CHECKPOINT");
  ESP_BT.register_callback(bt_callback);
}
void loop() {
  TerraCon(currentMillis);
}
```
