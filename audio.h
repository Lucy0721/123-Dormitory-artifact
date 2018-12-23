#include "arduino.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3); // RX, TX

#define AUDIO_PORT mySerial    //Core+
//#define AUDIO_PORT mySerial  //Core

//芯片复位
byte sn_reset[4] =
{
  0x7E, 0x02, 0x0C, 0xEF
};

//指定曲目
byte sn_choose[6] =
{
  0x7E, 0x04, 0x03, 0x00, 0x01, 0xEF
};

//指定音量
byte sn_vol[5] =
{
  0x7E, 0x03, 0x06, 0x18, 0xEF
};

//指定设备
byte sn_device[5] =
{
  0x7E, 0x03, 0x09, 0x01, 0xEF
};

//暂停
byte sn_pause[4] =
{
  0x7E, 0x02, 0x0E, 0xEF
};

//播放
byte sn_play[4] =
{
  0x7E, 0x02, 0x0D, 0xEF
};

//循环播放
byte sn_mode[5] =
{
  0x7E, 0x03, 0x11, 0x00, 0xEF
};

//下一曲
byte sn_down[4] =
{
  0x7E, 0x02, 0x01, 0xEF
};

//上一曲
byte sn_up[4] =
{
  0x7E, 0x02, 0x02, 0xEF
};

//指定EQ（播放风格）
byte sn_eq[5] =
{
  0x7E, 0x03, 0x07, 0x01, 0xEF
};

//-----------------------------
void audio_pause()//暂停
{
  AUDIO_PORT.write(sn_pause, 4);
  delay(50);
}

void audio_play()//播放
{
  AUDIO_PORT.write(sn_play, 4);
  delay(50);
}

//play eq    (Normal/Pop/Rock/Jazz/Classic/Base)  0-5
void audio_eq(byte _audio_eq)//指定音频播放风格
{
  sn_mode[3] = _audio_eq;
  AUDIO_PORT.write(sn_eq, 5);
  delay(100);
}

#define MODE_loopAll 0
#define MODE_loopOne 1
#define MODE_One_END 2
#define MODE_One_STOP  4
//play mode    (ALL/FOL/ONE/RAM/ONE_STOP)  0-4
void audio_mode(byte _audio_mode)//指定循环方式
{
  sn_mode[3] = _audio_mode;
  while (digitalRead(2))
    AUDIO_PORT.write(sn_mode, 5);
  delay(100);
}

#define DEVICE_Flash  5
#define DEVICE_TF  1
//device select    (U/TF/AUX/SLEEP/FLASH)  0-4
void audio_device(byte _audio_device)//指定设备
{
  sn_device[3] = _audio_device;
  while (digitalRead(2))
    AUDIO_PORT.write(sn_device, 5);
  delay(1500);
}

void audio_down()//下一曲
{
  AUDIO_PORT.write(sn_down, 4);
  delay(500);
}

void audio_up()//上一曲
{
  AUDIO_PORT.write(sn_up, 4);
  delay(500);
}

void audio_vol(byte _audio_vol)//指定音量
{
  sn_vol[3] = _audio_vol;
  AUDIO_PORT.write(sn_vol, 5);
  delay(50);
}

void audio_choose(byte _audio_choose)//指定曲目
{
  sn_choose[4] = _audio_choose;
  AUDIO_PORT.write(sn_choose, 6);
  delay(200);
}

void audio_reset()//芯片复位
{
  while (digitalRead(2))
    AUDIO_PORT.write(sn_reset, 4);
  delay(1000);
}

void audio_init(int _audio_init_device, int _audio_init_mode, int _audio_init_vol)//指定相应的设备（_audio_init_device） 相应循环方式（_audio_init_mode）相应音量
{
  AUDIO_PORT.begin(9600);
  delay(1000);
  audio_reset();
  audio_device(_audio_init_device);
  audio_mode(_audio_init_mode);
  audio_vol(_audio_init_vol);
  audio_choose(1);
  audio_pause();
}
