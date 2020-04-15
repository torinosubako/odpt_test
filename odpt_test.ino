/*
 * M5 Vision v2.10
 * CodeName:Sanlight
 * Build:2020/04/15
 * Author:torinosubako
 * Github:https://github.com/torinosubako/odpt_test
*/

#include <M5Stack.h>
#include <Ambient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/rtc_io.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include "ArduinoJson.h"
WiFiClient client;
Ambient ambient;


//Wi-Fi設定用基盤情報
const char *ssid = //Your Network SSID//;
const char *password = //Your Network Password//;

//環境センサAmbient連携用基盤情報
unsigned int channelId = //Your Ambient Channel ID//; // AmbientのチャネルID
const char* writeKey = //Your Ambient Write key//; // ライトキー
Adafruit_AM2320 am2320 = Adafruit_AM2320();

//東京公共交通オープンデータチャレンジ向け共通基盤情報
const String api_key = "&acl:consumerKey="//Your API Key//;
const String base_url = "https://api-tokyochallenge.odpt.org/api/v4/odpt:TrainInformation?odpt:railway=odpt.Railway:";

void setup() {
  M5.begin();
  M5.Lcd.clear();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE);
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // hang out until serial port opens
  }

  //起動画面
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, "/img/system/400.jpg");
  WiFi.begin(ssid, password);
  delay(1000);
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, "/img/system/401.jpg");
  delay(1000);

  //無線接続試験
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    M5.Lcd.clear(BLACK);
    M5.Lcd.drawJpgFile(SD, "/img/system/404.jpg");
  }
  Serial.println("初期リンクを確立しました");
  Serial.println(WiFi.localIP());
  ambient.begin(channelId, writeKey, &client);

  //環境センサ用設定
  am2320.begin();
}

void loop() {
  Serial.println("システム定期ルーチン開始");
  int wifi_cont;

  //Wi-Fi接続試験(2sec)
  WiFi.begin(ssid, password);
  delay(2000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    wifi_cont += 1;
    Serial.println("Connecting to WiFi..");
    M5.Lcd.clear(BLACK);
    M5.Lcd.drawJpgFile(SD, "/404.jpg");
    if (wifi_cont > 2) void reset();
  }

  //鉄道運行情報配信プラットフォーム
  Serial.println("データリンク開始");
  //OTIS系統
  String JR_SaikyoKawagoe = odpt_train_info_jr("JR-East.SaikyoKawagoe");
  String JR_Yamanote = odpt_train_info_jr("JR-East.Yamanote");
  String TobuTojo = odpt_train_info_tobu("Tobu.Tojo");
  //画像表示系等(最終コマだけ-10sec)
  display_control(JR_SaikyoKawagoe, 30);
  display_control(JR_Yamanote, 30);
  display_control(TobuTojo, 30);
  display_control(JR_SaikyoKawagoe, 30);
  display_control(JR_Yamanote, 30);
  display_control(TobuTojo, 20);

  //AM2320環境センサプラットフォーム(3sec必須)
  environmental_sensor();
  Serial.println("システム定期ルーチン終了");
}


//関数群
//AM2320環境センサプラットフォーム(3sec)
void environmental_sensor() {
  float temp, humid;
  do {
    delay(2000);
    temp = am2320.readTemperature();
    humid = am2320.readHumidity();
  } while (humid < 1);
  Serial.printf("Temp: %.2f, humid: %.2f\r\n", temp, humid);
  ambient.set(1, temp);
  ambient.set(2, humid);
  ambient.send();
  delay(1000);
}

//JR向け情報取得関数
String odpt_train_info_jr(String line_name) {
  String result; //返答用変数作成
  String file_header = "";
  String file_address = "";

  if (line_name == "JR-East.SaikyoKawagoe") {
    file_header = "/img/JR_JA/";
  } else if (line_name == "JR-East.KeihinTohokuNegishi") {
    file_header = "/img/JR_JK/";
  } else if (line_name == "JR-East.Yamanote") {
    file_header = "/img/JR_JY/";
  } else {
    file_header = "/img/system/403.jpg";
    return file_header;
  }
  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
  //http.begin(url); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    Serial.println("データリンク成功");
    String payload = http.getString();  //返答（JSON形式）を取得
    Serial.println(base_url + line_name + api_key);
    Serial.println(httpCode);
    Serial.println(payload);

    //jsonオブジェクトの作成
    String json = payload;
    DynamicJsonDocument besedata(4096);
    deserializeJson(besedata, json);

    //データ識別・判定
    const char* deta1 = besedata[0]["odpt:trainInformationText"]["en"];
    const char* deta2 = besedata[0]["odpt:trainInformationStatus"]["en"];
    const char* deta3 = besedata[0];
    const String point1 = String(deta1).c_str();
    const String point2 = String(deta2).c_str();
    Serial.println("データ受信結果");
    Serial.println(deta1);
    Serial.println(deta2);
    Serial.println(deta3);

    if (point1 == "Service on schedule") {
      //　平常運転
      file_address = file_header + "01.jpg";
    } else if (point2 == "Notice") {
      //　情報有り
      file_address = file_header + "02.jpg";
    } else if (point2 == "Delay") {
      //　遅れあり
      file_address = file_header + "03.jpg";
    } else if (point2 == "Operation suspended") {
      //　運転見合わせ
      file_address = file_header + "04.jpg";
    } else if (point2 == "Direct operation cancellation") {
      //　直通運転中止
      file_address = file_header + "05.jpg";
    } else if (point2 == NULL) {
      //取得時間外？
      file_address = file_header + "00.jpg";
    } else {
      file_address = "/img/system/403.jpg";
    }
  }
  else {
    Serial.println("Error on HTTP request");
    file_address = "/img/system/404.jpg";
  }
  result = file_address;
  return result;
  http.end(); //リソースを解放
}

//東武向け情報取得関数
String odpt_train_info_tobu(String line_name) {
  String result; //返答用変数作成
  String file_header = "";
  String file_address = "";

  if (line_name == "Tobu.Tojo") {
    file_header = "/img/TB_TJ/";
  } else if (line_name == "Tobu.Ogose") {
    file_header = "/img/TB_OG/";
  } else if (line_name == "Tobu.TobuUrbanPark") {
    file_header = "/img/TB_UP/";
  } else {
    file_header = "/img/system/403.jpg";
    return file_header;
  }
  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
  //http.begin(url); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    Serial.println("データリンク成功");
    String payload = http.getString();  //返答（JSON形式）を取得
    Serial.println(base_url + line_name + api_key);
    Serial.println(httpCode);
    Serial.println(payload);

    //jsonオブジェクトの作成
    String json = payload;
    DynamicJsonDocument besedata(4096);
    deserializeJson(besedata, json);

    //データ識別・判定
    const char* deta1 = besedata[0]["odpt:trainInformationText"]["ja"];
    const char* deta2 = besedata[0]["odpt:trainInformationStatus"]["ja"];
    const char* deta3 = besedata[0];
    const String point1 = String(deta1).c_str();
    const String point2 = String(deta2).c_str();
    Serial.println("データ受信結果");
    Serial.println(deta1);
    Serial.println(deta2);
    Serial.println(deta3);

    //判定論理野（開発中）
    if (point1 == "平常どおり運転しています。") {
      //　平常運転
      file_address = file_header + "01.jpg";
    } else if (point2 == "運行情報あり") {
      if (-1 != point1.indexOf("運転を見合わせています")) {
        //　運転見合わせ
        file_address = file_header + "04.jpg";
      } if (-1 != point1.indexOf("遅れがでています")) {
        // 遅れあり
        file_address = file_header + "03.jpg";
      } else if (-1 != point1.indexOf("直通運転を中止しています")) {
        //　直通運転中止
        file_address = file_header + "05.jpg";
      } else {
        //　情報有り
        file_address = file_header + "02.jpg";
      }
    } else if (point2 == NULL) {
      //取得時間外？
      file_address = file_header + "00.jpg";
    } else {
      file_address = "/img/system/403.jpg";
    }
  }
  else {
    Serial.println("Error on HTTP request");
    file_address = "/img/system/404.jpg";
  }
  result = file_address;
  return result;
  http.end(); //リソースを解放
}

//画像表示制御系
void display_control(String line_code, int interval_time) {
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, String(line_code).c_str());
  interval_time *= 1000;
  delay(interval_time);
}
