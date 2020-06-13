/*
 * M5 Vision v2.14
 * CodeName:Sunlight_Refresh+2
 * Build:2020/06/13
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


//Wi-Fi設定用基盤情報(2.4GHz帯域のみ)
const char *ssid = //Your Network SSID//;
const char *password = //Your Network Password//;

//環境センサAmbient連携用基盤情報
unsigned int channelId = //Your Ambient Channel ID//; // AmbientのチャネルID
const char* writeKey = //Your Ambient Write key//; // ライトキー
Adafruit_AM2320 am2320 = Adafruit_AM2320();

//東京公共交通オープンデータチャレンジ向け共通基盤情報
const String api_key = "&acl:consumerKey="//Your API Key//;
const String base_url = "https://api-tokyochallenge.odpt.org/api/v4/odpt:TrainInformation?odpt:railway=odpt.Railway:";


//M5Stackリフレッシュ用変数
int reno_cont;
int reno_limit = 20;

void setup() {
  M5.begin();
  M5.Lcd.clear();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE);
  Serial.begin(9600);
  int wifi_cont = 0;
  while (!Serial) {
    delay(10); // hang out until serial port opens
  }

  //起動画面
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, "/img/system/400.jpg");
  M5.Lcd.clear(BLACK);
  M5.Lcd.drawJpgFile(SD, "/img/system/401.jpg");

  //Wi-Fi接続試験(2sec)
  WiFi.begin(ssid, password);
  delay(2000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    wifi_cont ++;
    Serial.println("Connecting to WiFi..");
    Serial.println( wifi_cont);
    M5.Lcd.clear(BLACK);
    M5.Lcd.drawJpgFile(SD, "/img/system/404.jpg");
    if (wifi_cont >= 3){
      M5.Power.reset();
    }
  }
  Serial.println("初期リンクを確立しました");
  Serial.println(WiFi.localIP());
  ambient.begin(channelId, writeKey, &client);

  //環境センサ用設定
  am2320.begin();
}

void loop() {
  int wifi_cont = 0;

  //自動リセット
  if (reno_cont >= reno_limit){
    M5.Power.reset();
  }
  reno_cont ++;
  Serial.println(reno_cont);
  

  //Wi-Fi接続試験(2sec)
  WiFi.begin(ssid, password);
  delay(2000);
  Serial.println(wifi_cont);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    wifi_cont ++;
    Serial.println("Connecting to WiFi..");
    Serial.println(wifi_cont);
    M5.Lcd.clear(BLACK);
    M5.Lcd.drawJpgFile(SD, "/img/system/404.jpg");
    if (wifi_cont >= 5){
      M5.Power.reset();
    }
  }
  
  //AM2320環境センサプラットフォーム(3sec必須)
  environmental_sensor();

  //鉄道運行情報配信プラットフォーム
  Serial.println("データリンク開始");
  //OTIS(データ取得・整形)系統
  String JR_SaikyoKawagoe = odpt_train_info_jr("JR-East.SaikyoKawagoe");
  String JR_Yamanote = odpt_train_info_jr("JR-East.Yamanote");
  String TobuTojo = odpt_train_info_tobu("Tobu.Tojo");

  //WiFi切断  
  WiFi.disconnect();
  
  //画像表示系等(最終コマだけ-10sec)
  display_control(TobuTojo, 30);
  display_control(JR_SaikyoKawagoe, 30);
  display_control(JR_Yamanote, 30);
  display_control(TobuTojo, 30);
  display_control(JR_SaikyoKawagoe, 30);
  display_control(JR_Yamanote, 20);

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

//JR向け情報取得関数(vr.200611)
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
  } else if (line_name == "JR-East.ChuoRapid ") { //画像未実装
    file_header = "/img/JR_JC/";
  } else if (line_name == "JR-East.ChuoSobuLocal ") { //画像未実装
    file_header = "/img/JR_JB/";
  } else if (line_name == "JR-East.JobanRapid ") { //画像未実装
    file_header = "/img/JR_JJ/";
  } else if (line_name == "JR-East.JobanLocal ") { //画像未実装
    file_header = "/img/JR_JL/";
  } else if (line_name == "JR-East.Keiyo ") { //画像未実装
    file_header = "/img/JR_JE/";
  } else if (line_name == "JR-East.Musashino ") { //画像未実装
    file_header = "/img/JR_JM/";
  } else if (line_name == "JR-East.SobuRapid ") { //画像未実装
    file_header = "/img/JR_JOS/";
  } else if (line_name == "JR-East.Yokosuka  ") { //画像未実装
    file_header = "/img/JR_JOY/";
  } else if (line_name == "JR-East.Tokaido ") { //画像未実装
    file_header = "/img/JR_JT/";
  } else if (line_name == "JR-East.Utsunomiya ") { //画像未実装
    file_header = "/img/JR_JU/";
  } else if (line_name == "JR-East.Takasaki ") { //画像未実装
    file_header = "/img/JR_JUT/";
  } else {
    file_header = "/img/system/403.jpg";
    return file_header;
  }
  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    String payload = http.getString();  //返答（JSON形式）を取得
    //Serial.println(base_url + line_name + api_key);
    //Serial.println(payload);

    //jsonオブジェクトの作成
    String json = payload;
    DynamicJsonDocument besedata(4096);
    deserializeJson(besedata, json);

    //データ識別・判定
    const char* deta1 = besedata[0]["odpt:trainInformationText"]["en"];
    const char* deta2 = besedata[0]["odpt:trainInformationStatus"]["en"];
    const char* deta3 = besedata[0];
    const char* deta4 = besedata[0]["odpt:trainInformationStatus"]["ja"];
    const String point1 = String(deta1).c_str();
    const String point2 = String(deta2).c_str();
    const String point4 = String(deta4).c_str();

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
    } else if (point4 == "運転見合わせ") {
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
  } else if (line_name == "Tobu.TobuSkytree") {
    file_header = "/img/TB_TS/";
  } else if (line_name == "TobuSkytreeBranch") { //東武スカイツリーライン(押上-曳舟)
    file_header = "/img/TB_TSB/";
  } else if (line_name == "Tobu.Kameido") {
    file_header = "/img/TB_TSK/";
  } else if (line_name == "Tobu.Daishi") {
    file_header = "/img/TB_TSD/";    
  } else if (line_name == "Tobu.Nikko") {
    file_header = "/img/TB_TN/";
  } else if (line_name == "Tobu.Kinugawa") {
    file_header = "/img/TB_TNK/";
  } else if (line_name == "Tobu.Utsunomiya") {
    file_header = "/img/TB_TNU/";
  } else if (line_name == "Tobu.Isesaki") {
    file_header = "/img/TB_TI/";
  } else if (line_name == "Tobu.Sano") {
    file_header = "/img/TB_TIS/";
  } else if (line_name == "Tobu.Kiryu") {
    file_header = "/img/TB_TII/";
  } else if (line_name == "Tobu.Koizumi") {
    file_header = "/img/TB_TIO/";
  } else if (line_name == "Tobu.KoizumiBranch") { //東武小泉線(東小泉-太田)
    file_header = "/img/TB_TIO/";
  } else {
    file_header = "/img/system/403.jpg";
    return file_header;
  }
  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
//  http.begin(url); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    String payload = http.getString();  //返答（JSON形式）を取得
    //Serial.println(base_url + line_name + api_key);
    //Serial.println(httpCode);
    //Serial.println(payload);

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
    //Serial.println("データ受信結果");
    //Serial.println(deta1);
    //Serial.println(deta2);
    //Serial.println(deta3);

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
