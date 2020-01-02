#include <M5Stack.h>
#include <WiFi.h>
#include "ArduinoJson.h"
#include <HTTPClient.h>
#include <esp_deep_sleep.h>
 
const char *ssid = //Your Network SSID//;
const char *password = //Your Network PW//;

const String base_url = "https://api-tokyochallenge.odpt.org/api/v4/odpt:TrainInformation?";
//const String odpt_line_saikyo = "odpt:railway=odpt.Railway:JR-East.SaikyoKawagoe";
//const String odpt_line_Rinkai = "odpt:railway=odpt.Railway:TWR.Rinkai";
//const String odpt_line_Yamanote = "odpt:railway=odpt.Railway:JR-East.Yamanote";
const String odpt_line_name = "odpt:railway=odpt.Railway:JR-East.SaikyoKawagoe";
const String api_key = //Your API Key//;


const int sleeping_time = 300;

void setup() {

  Serial.begin(115200);
  M5.begin();
  M5.Lcd.clear();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE);
  //String font = "genshin-regular-20pt"; // without Ext
  //M5.Lcd.loadFont(font, SD);
  esp_sleep_enable_timer_wakeup(sleeping_time * 1000000LL);

  //起動画面
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, "/400.jpg");
  WiFi.begin(ssid, password);
  delay(1500);
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, "/401.jpg");
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, LOW);
  //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  delay(1500);


  while (WiFi.status() != WL_CONNECTED) {
    delay(10000);
    Serial.println("初期リンクを確立しています");
    Serial.println("Connecting to WiFi..");
    M5.Lcd.clear(BLACK);
    M5.Lcd.drawJpgFile(SD, "/404.jpg");
  }
  Serial.println("初期リンクを確立しました");
  Serial.println("Connected to the WiFi network");
}


void loop() {
  Serial.println("システム定期起動ルーチン開始");
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
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, "/001.jpg");
      } else if (point2 == "Notice") {
        //　情報有り
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, "/002.jpg");
      } else if (point2 == "Delay") {
        //　遅れあり
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, "/003.jpg");
      } else if (point2 == "Operation suspended") {
        //　運転見合わせ
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, "/004.jpg");
      } else if (point2 == "Direct operation cancellation") {
        //　直通運転中止
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, "/005.jpg");
      } else if (point2 == NULL) {
        //取得時間外？
        M5.Lcd.clear(BLACK);
        M5.Lcd.drawJpgFile(SD, "/000.jpg");
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
  Serial.println("システム定期起動ルーチン終了");
  Serial.println("System in sleep!");
  WiFi.mode(WIFI_OFF);
  esp_light_sleep_start();
  //esp_deep_sleep(sleeping_time * 1000000LL);
  //delay(300000);   //300秒おきに更新
}
