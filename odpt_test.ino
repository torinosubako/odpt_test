#include <M5Stack.h>
#include <WiFi.h>
#include "ArduinoJson.h"
#include <HTTPClient.h>
#include <driver/rtc_io.h>
#include <esp_deep_sleep.h>

//ネットワーク設定
const char *ssid = //Your Network SSID//;
const char *password = //Your Network PW//;

//API設定
const String api_key = //Your API Key//;

//東京公共交通オープンデータチャレンジ向け各種設定
const String base_url = "https://api-tokyochallenge.odpt.org/api/v4/odpt:TrainInformation?";
const String odpt_line_saikyo = "odpt:railway=odpt.Railway:JR-East.SaikyoKawagoe";
const String odpt_line_KeihinTohoku = "odpt:railway=odpt.Railway:JR-East.KeihinTohokuNegishi";
const String odpt_line_Yamanote = "odpt:railway=odpt:railway=odpt.Railway:JR-East.Yamanote";
const String odpt_line_Rinkai = "odpt:railway=odpt.Railway:TWR.Rinkai";
String odpt_line_name = "";




//基幹情報
String file_header_base = "";
//00X系
String header_saikyo = "/00";
//01X系
String header_KeihinTohoku = "/01";
//02X系
String header_Yamanote = "/02";
//03X系
//const String header_KeihinTohoku = "/XX";


//更新時間設定(秒)
const int sleeping_time = 300;


void setup() {
  //初期設定
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.clear();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE);

  //初期路線設定
  const String line_name = "京浜東北線" ;

  //　路線設定論理
  if (line_name == "埼京線") {
    //　埼京・川越線
    odpt_line_name = odpt_line_saikyo;
    file_header_base = header_saikyo;
    Serial.println("路線設定:埼京線");
  } else if (line_name == "京浜東北線") {
    //　京浜東北線
    odpt_line_name = odpt_line_KeihinTohoku;
    file_header_base = header_KeihinTohoku;
    Serial.println("路線設定:京浜東北線");
  } else if (line_name == "山手線") {
    //  山手線
    odpt_line_name = odpt_line_Yamanote;
    file_header_base = header_Yamanote;
    Serial.println("路線設定:山手線");
  } else {
    odpt_line_name = odpt_line_saikyo;
    file_header_base = header_saikyo;
    Serial.println("路線設定:初期値(埼京線)に設定しました");
  }

  //  フォント設定（未使用）
  //String font = "genshin-regular-20pt"; // without Ext
  //M5.Lcd.loadFont(font, SD);

  //省電力設定用
  ledcDetachPin(GPIO_NUM_32);

  rtc_gpio_set_level(GPIO_NUM_32, 1);
  rtc_gpio_set_direction(GPIO_NUM_32, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_init(GPIO_NUM_32);

  rtc_gpio_set_level(GPIO_NUM_33, 1);
  rtc_gpio_set_direction(GPIO_NUM_33, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_init(GPIO_NUM_33);

  M5.Power.setPowerBoostKeepOn(true);
  esp_sleep_enable_timer_wakeup(sleeping_time * 1000000LL);

  //起動画面
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, "/400.jpg");
  WiFi.begin(ssid, password);
  delay(1500);
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, "/401.jpg");
  delay(1500);


  while (WiFi.status() != WL_CONNECTED) {
    delay(10000);
    Serial.println("初期リンクを確立しています");
    M5.Lcd.clear(BLACK);
    M5.Lcd.drawJpgFile(SD, "/404.jpg");
  }
  Serial.println("初期リンクを確立しました");
}


void loop() {
  Serial.println("システム定期ルーチン開始");
  String file_header = file_header_base;
  WiFi.begin(ssid, password);

  delay(2000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.println("Connecting to WiFi..");
    M5.Lcd.clear(BLACK);
    M5.Lcd.drawJpgFile(SD, "/404.jpg");
  }

  if ((WiFi.status() == WL_CONNECTED)) {
    Serial.println("データリンク開始");

    HTTPClient http;

    

    http.begin(base_url + odpt_line_name + api_key); //URLを指定
    //http.begin(url); //URLを指定
    int httpCode = http.GET();  //GETリクエストを送信

    if (httpCode > 0) { //返答がある場合
      Serial.println("データリンク成功");
      String payload = http.getString();  //返答（JSON形式）を取得
      Serial.println(base_url + odpt_line_name + api_key);
      Serial.println(httpCode);
      Serial.println(payload);

      //jsonオブジェクトの作成
      String json = payload;
      DynamicJsonDocument besedata(4096);
      deserializeJson(besedata, json);

      //各データを抜き出し
      M5.Lcd.clear(BLACK);
      M5.Lcd.setTextSize(2);
      const char* deta1 = besedata[0]["odpt:trainInformationText"]["en"];
      const char* deta2 = besedata[0]["odpt:trainInformationStatus"]["en"];
      const char* deta3 = besedata[0];
      const String point1 = String(deta1).c_str();
      const String point2 = String(deta2).c_str();
      Serial.println("データ受信結果");
      Serial.println(deta1);
      Serial.println(deta2);
      Serial.println(deta3);

      //Serial.println(deta);
      if (point1 == "Service on schedule") {
        //　平常運転
        file_header.concat("1.jpg");
        Serial.println(file_header);
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, String(file_header).c_str());
      } else if (point2 == "Notice") {
        //　情報有り
        file_header.concat("2.jpg");
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, String(file_header).c_str());
      } else if (point2 == "Delay") {
        //　遅れあり
        file_header.concat("3.jpg");
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, String(file_header).c_str());
      } else if (point2 == "Operation suspended") {
        //　運転見合わせ
        file_header.concat("4.jpg");
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, String(file_header).c_str());
      } else if (point2 == "Direct operation cancellation") {
        //　直通運転中止
        file_header.concat("5.jpg");
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, String(file_header).c_str());
      } else if (point2 == NULL) {
        //取得時間外？
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, "/402.jpg");
        Serial.println("取得時間外?");
      } else {
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, "/403.jpg");
        Serial.println("読み込みエラー?");
      }
    }

    else {
      Serial.println("Error on HTTP request");
      M5.Lcd.clear(BLACK);
      M5.Lcd.drawJpgFile(SD, "/404.jpg");
    }

    http.end(); //リソースを解放
  }
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  delay(2000);
  Serial.println("システム定期ルーチン終了");
  Serial.println("System in sleep!");
  WiFi.mode(WIFI_OFF);
  esp_light_sleep_start();//上部で設定した秒数おきにLight_sleep解除、定期ルーチン開始
}
