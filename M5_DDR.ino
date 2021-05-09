/* Simplified Thrustmaster T.16000M FCS Joystick Report Parser */

#include <M5Stack.h>
#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>


#include <Toio.h>
Toio toio;  // Toio オブジェクト生成
ToioCore* toiocore = nullptr; // 発見された ToioCore オブジェクトのポインタ変数を定義

int8_t steering=0;
int8_t throttle=0;

// ライト ON/OFF 状態
static bool light_on = false;

// MIDI データ (チャルメラ)
static const uint8_t CHARUMERA_LEN = 39;
static uint8_t CHARUMERA_DATA[CHARUMERA_LEN] = {
  3,             // Type of Sound control (MIDI)
  1,             // Repeat count
  12,            // Number of operations
  14,  69, 255,  // 140ms, A5
  14,  71, 255,  // 140ms, B5
  56,  73, 255,  // 560ms, C#6
  14,  71, 255,  // 140ms, B5
  14,  69, 255,  // 140ms, A5
  114, 128, 255, // no sound
  14,  69, 255,  // 140ms, A5
  14,  71, 255,  // 140ms, B5
  14,  73, 255,  // 560ms, C#6
  14,  71, 255,  // 140ms, B5
  14,  69, 255,  // 140ms, A5
  56,  71, 255   // 560ms, B5
};

void displayCaptionButtonA(String caption) {
  M5.Lcd.setCursor(30, 215, 2);
  M5.Lcd.print("[" + caption + "]");
}

void displayCaptionButtonB(String caption) {
  M5.Lcd.setCursor(125, 215, 2);
  M5.Lcd.print("[" + caption + "]");
}



// Thrustmaster T.16000M HID report
struct GamePadEventData
{
  uint16_t	buttons;
  uint8_t		hat;
  uint16_t	x;
  uint16_t	y;
  uint8_t		twist;
  uint8_t		slider;
}__attribute__((packed));

class JoystickEvents
{
  public:
    virtual void OnGamePadChanged(const GamePadEventData *evt);
};

#define RPT_GAMEPAD_LEN	sizeof(GamePadEventData)

class JoystickReportParser : public HIDReportParser
{
  JoystickEvents		*joyEvents;

  uint8_t oldPad[RPT_GAMEPAD_LEN];

  public:
  JoystickReportParser(JoystickEvents *evt);

  virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};


JoystickReportParser::JoystickReportParser(JoystickEvents *evt) :
  joyEvents(evt)
{}

void JoystickReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf)
{
  // Checking if there are changes in report since the method was last called
  bool match = (sizeof(oldPad) == len) && (memcmp(oldPad, buf, len) == 0);

  // Calling Game Pad event handler
  if (!match && joyEvents) {
    joyEvents->OnGamePadChanged((const GamePadEventData*)buf);
    memcpy(oldPad, buf, len);
  }
}

void JoystickEvents::OnGamePadChanged(const GamePadEventData *evt)
{
  M5.Lcd.fillScreen(TFT_NAVY);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, 44, TFT_BLUE);
  M5.Lcd.println("ToioDan!");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 45);
  M5.Lcd.setTextColor(TFT_YELLOW);
  //M5.Lcd.print("X: "); M5.Lcd.println(evt->x, HEX);
  //M5.Lcd.print("Y: "); M5.Lcd.println(evt->y, HEX);
  M5.Lcd.print("X: "); M5.Lcd.println(evt->x,BIN);
  M5.Lcd.print("Y: "); M5.Lcd.println(evt->y,BIN);

  boolean left = bitRead(evt->y,5-1);
  boolean down = bitRead(evt->y,6-1);
  boolean up = bitRead(evt->y,7-1);
  boolean right = bitRead(evt->y,8-1);
  boolean triangle = bitRead(evt->y,9-1);
  boolean square= bitRead(evt->y,10-1);
  boolean circle = bitRead(evt->y,12-1);
  boolean x = bitRead(evt->y,11-1);
  boolean select_ = bitRead(evt->y,13-1);
  boolean start = bitRead(evt->y,14-1);

  //left and right で前後移動
  if(left && !right) steering = -50;
  else if(!left && right) steering = +50;
  else steering = 0;

  //up and down で左右回転制御
  if(up && !down) throttle = +50;
  else if(!up && down) throttle = -50;
  else throttle = 0;

  //スピードアップ
  if(circle) throttle += +40;

  //スピードダウン
  if(select_) if(throttle<=100) throttle += -40; //スピードアップ

  //チャルメラ
  if(start) toiocore->playSoundRaw(CHARUMERA_DATA, CHARUMERA_LEN);

  //Stay Cool! button
  if(bitRead(evt->x,16-1)==1){
    if (light_on) {
      toiocore->turnOffLed(); // 点灯中なら消灯
    } else {
      toiocore->turnOnLed(255, 255, 255); // 消灯中なら白で点灯
    }
    light_on = !light_on;
  }

  M5.Lcd.print("throttle : "); M5.Lcd.println(throttle);
  M5.Lcd.print("steering : "); M5.Lcd.println(steering);
  toiocore->drive(throttle, steering);
}

USB                                             Usb;
USBHub                                          Hub(&Usb);
HIDUniversal                                    Hid(&Usb);
JoystickEvents                                  JoyEvents;
JoystickReportParser                            Joy(&JoyEvents);




void setup()
{
  M5.begin();
  M5.Lcd.fillScreen(TFT_NAVY);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 31);
  M5.Lcd.setTextColor(TFT_YELLOW);

  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
    Serial.println("OSC did not start.");

  delay( 200 );

  if (!Hid.SetReportParser(0, &Joy))
    ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  );


  //========Toio Setup===========
  // 3 秒間 Toio Core Cube をスキャン
  M5.Lcd.setCursor(0, 50, 2);
  M5.Lcd.print("Scanning your toio core...");
  std::vector<ToioCore*> toiocore_list = toio.scan(3);

  // 見つからなければ終了
  size_t n = toiocore_list.size();
  if (n == 0) {
    M5.Lcd.setCursor(0, 50, 2);
    M5.Lcd.print("No toio Core Cube was found. Turn on your Toio Core Cube, then press the reset button of your Toio Core Cube.");
    return;
  }

  // 最初に見つかった Toio Core Cube の ToioCore オブジェクト
  toiocore = toiocore_list.at(0);

  // Toio　Core のデバイス名と MAC アドレスを表示
  M5.Lcd.setCursor(0, 50, 2);
  M5.Lcd.print(String(toiocore->getName().c_str()) + " (" + String(toiocore->getAddress().c_str()) + ")");

  // M5Stack のボタン A/B/C のキャプションを表示
  displayCaptionButtonA(" Connect  ");
  displayCaptionButtonB("  Light   ");

  // Connection イベントのコールバックをセット
  toiocore->onConnection([](bool state) {
    displayCaptionButtonA(state ? "Disconnect" : " Connect  ");
  });

    
}

void loop()
{
  Usb.Task();
  
  M5.update();
  
  // M5Stack のボタン A が押されたときの処理 (接続/切断)
  displayCaptionButtonA(" Connect  ");
  if (M5.BtnA.wasReleased()) {
    // Toio Core Cube と BLE 接続中かどうかをチェック
    if (toiocore->isConnected()) {
      toiocore->disconnect(); // 接続中なら切断
    } else {
      toiocore->connect(); // 切断中なら接続
    }
    return;
  }
  displayCaptionButtonA(" Connect  ");
  
  toiocore->drive(throttle, steering);
}
