#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "BluetoothSerial.h"
BluetoothSerial ESP_BT;
byte from_terra[22];
int  from_terra_size = 0;
byte to_terra[16];
int  to_terra_size = 0;
unsigned long terraMillis = 0;
unsigned long terraPing = 0;
int tmp = 0;
byte terra_byte[4];
char terra_num[]="0000000";
bool terra_ping = false;
float terra_result;
float terra_error;
float terra_vcc;
unsigned char terra_resFail = 0;
unsigned char terra_detFail = 0;
unsigned char terra_batLevel = 0;

void printDeviceAddress() {
  const uint8_t* point = esp_bt_dev_get_address();
  for (int i = 0; i < 6; i++) {
    char str[3];
    sprintf(str, "%02X", (int)point[i]);
    Serial.print(str);
    if (i < 5){
      Serial.print(":");
    }
  }
}

void bt_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  //Serial.println(event);
  //Serial.println(param);
  switch(event) {
    case ESP_SPP_INIT_EVT:
      Serial.print("[BT] mac: ");
      printDeviceAddress();
      Serial.println();
      break;
    case ESP_SPP_SRV_OPEN_EVT:
      Serial.println("[BT] Client Connected");
      #ifdef SSD1306
        display.setCursor(0, 24);
        display.print(F("                     "));
        display.setCursor(0, 24);
        display.print(F("BT Client ONLINE"));
        display.display();
      #endif
      break;
    case ESP_SPP_CLOSE_EVT:
      Serial.println("[BT] Client Disconnected");
      #ifdef SSD1306
        display.setCursor(0, 24);
        display.print(F("                     "));
        display.setCursor(0, 24);
        display.print(F("BT Client OFFLINE"));
      #endif
      terra_num[0]=char(48);terra_num[1]=char(48);terra_num[2]=char(48);terra_num[3]=char(48);terra_num[4]=char(48);terra_num[5]=char(48);terra_num[6]=char(48);
      terra_ping = false;
      break;
  };
}

byte crc(byte arrays[], int sizes){
  byte crc = 0;
  for (int i=0; i<sizes; i++){
    int calc = crc + ( arrays[i] & 0xFF);
    if (calc>255){
      crc = calc - 255;
    }else{
      crc = calc;
    };
  }; 
  return crc; 
}

void toTerra(byte arrays[], int sizes){
  ESP_BT.write(arrays, sizes);
  /*
  Serial.print("[BT] to dos[");
  Serial.print(sizes);
  Serial.print("]:");
  for (int i=0; i<sizes; i++){
    //ESP_BT.write(arrays[i]);
    Serial.print(" ");
    Serial.print(arrays[i], HEX);
  }; 
  Serial.println();
  */
}

float TerraFloat(byte a,byte b,byte c,byte d){  
  union u_tag{
    byte bin[4];
    float num;
  } u;
  a= a-1;
  u.bin[0] = d;
  u.bin[1] = c;
  u.bin[2] = (b & 0b01111111) | ( ( a & 0b00000001 ) << 7 );
  u.bin[3] = (a >> 1) | (b & 0b10000000);
  return u.num;
}

void TerraCon(unsigned long currentMillis){
  //Terra integration
    if (ESP_BT.available()){
      from_terra[from_terra_size]=ESP_BT.read();
      from_terra_size ++;
      terraMillis=currentMillis;
    };
    
    if (from_terra_size > 0){
      if (currentMillis - terraMillis >= 7) {
        /*
        Serial.print("[BT] data [");
        Serial.print(from_terra_size);
        Serial.print("]:");
        for (int i=0; i<from_terra_size; i++){
          Serial.print(" ");
          Serial.print(from_terra[i], HEX);
        };
        Serial.println();
        */

        if (from_terra[0]==0x55 and from_terra[1]==0xAA){
          if (from_terra[2]==0b00100000 and from_terra_size==9 ){
            Serial.print("[BT] dosimeter init: ");   
            terra_byte[0]=from_terra[3];
            terra_byte[1]=from_terra[4];
            terra_byte[2]=from_terra[5];
            terra_byte[3]=from_terra[6];
            terra_num[0]=char( (terra_byte[3] & 0b00001111) +48 );
            terra_num[1]=char( (terra_byte[2] >> 4) +48    );
            terra_num[2]=char( (terra_byte[2] & 0b00001111) +48 );
            terra_num[3]=char( (terra_byte[1] >> 4) +48 );
            terra_num[4]=char( (terra_byte[1] & 0b00001111) +48 );
            terra_num[5]=char( (terra_byte[0] >> 4) +48 );
            terra_num[6]=char( (terra_byte[0] & 0b00001111) +48 ); 
            Serial.print(terra_num); 
            Serial.print(" type: "); 
            Serial.println(terra_byte[3] >> 4); 
              
            to_terra[0] = 0x55;
            to_terra[1] = 0xAA;
            to_terra[2] = 0b00100000;
            to_terra[3] = terra_byte[0];
            to_terra[4] = terra_byte[1];
            to_terra[5] = terra_byte[2];
            to_terra[6] = terra_byte[3];
            to_terra[7] = crc(to_terra,7);
            toTerra(to_terra,8);
          };
          if (from_terra[2]==0b00100000 and from_terra_size==8 and terra_ping==false ){
            //MED mode
            to_terra[0] = 0x55;
            to_terra[1] = 0xAA;
            to_terra[2] = 0x01;
            to_terra[3] = 0x00;
            to_terra[4] = 0x00;
            to_terra[5] = 0x00;
            to_terra[6] = 0x00;
            to_terra[7] = 0x02;
            to_terra[8] = crc(to_terra,8);
            toTerra(to_terra,9); 
            terra_ping = true;
          };
          if (from_terra[2]==0x00 and from_terra_size==22){
            terra_result=TerraFloat(from_terra[8],from_terra[7],from_terra[10],from_terra[9]);
            terra_error=TerraFloat(from_terra[12],from_terra[11],from_terra[14],from_terra[13]);
            terra_vcc=TerraFloat(from_terra[18],from_terra[17],from_terra[20],from_terra[19]);

            terra_resFail=(from_terra[16] & 0b10000000) >> 7;
            terra_detFail=(from_terra[16] & 0b00000010) >> 1;

            if (((from_terra[16] & 0b00100000) == 0b00100000) and ((from_terra[16] & 0b01000000) == 0b00000000)) terra_batLevel = 75;
            if (((from_terra[16] & 0b00100000) == 0b00000000) and ((from_terra[16] & 0b01000000) == 0b01000000)) terra_batLevel = 50;
            if (((from_terra[16] & 0b00100000) == 0b00100000) and ((from_terra[16] & 0b01000000) == 0b01000000)) terra_batLevel = 25;
            if ((from_terra[16] & 0b00000001) == 1) terra_batLevel = 0;
            
            Serial.print("[br] radiation: ");
            Serial.print(terra_result);
            if (from_terra[15]==0){
              Serial.print(" mkSv ");  
            }else{
              Serial.print(" beta ");
            };
            Serial.print(terra_error);
            Serial.print(" % ");
            Serial.print(terra_vcc);
            Serial.print(" V ");
            Serial.print(terra_batLevel);
            Serial.print(" %");
            if (terra_resFail) {Serial.print(" NOT VALID");};
            if (terra_detFail) {Serial.print(" DETECTOR FAIL");};
            Serial.println();
            srvsend("rad " + String(terra_result) + "|" + String(terra_error) + "|" + String(terra_vcc)+ "|" + String(terra_batLevel)+ "|" + String(terra_resFail)+ "|" + String(terra_detFail) );

            #ifdef ST7789V
              tft.setTextColor(TFT_YELLOW, TFT_BLACK);
              tft.drawString("RADIATION " + String(terra_result) + " " + String(terra_error) + "%           ", 20, 48, 2);
            #endif
          };
          
        };        
        from_terra_size = 0;
        terraPing=currentMillis;
      };
    };
    if ( (currentMillis - terraPing) >= 4000 and (terra_ping == true ) ) {
      if (ESP_BT.hasClient() == true){
        //request result
        to_terra[0] = 0x55;
        to_terra[1] = 0xAA;
        to_terra[2] = 0x00;
        to_terra[3] = 0x00;
        to_terra[4] = 0x00;
        to_terra[5] = 0x00;
        to_terra[6] = 0x00;
        to_terra[7] = 0x00;
        to_terra[8] = 0x00;
        to_terra[9] = crc(to_terra,9);
        toTerra(to_terra,10);
      };
      terraPing=currentMillis;
    };
}
