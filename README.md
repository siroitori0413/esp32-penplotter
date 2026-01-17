# esp32-penplotter
これは、いしかわきょーすけさん作のペンプロッタ「ペンプロッタキット2」でHPGL形式のファイルをペンプロット出力するためのプログラムです。
いしかわさん作のペンプロッタのみで動作します。

いしかわきょーすけさん
https://x.com/qx5k_iskw

## Clone方法

`git clone https://github.com/MeemeeLab/esp32-penplotter penplotter`

> [!WARNING]
> Clone先のディレクトリはpenplotterという名前で作成すること

## プログラムの書き込み方法
### config.h
#### それぞれのATOMへの設定切り替え
ATOM(M5Stack ATOM Lite/ATOMS3 Lite) が2つ存在します。

|名前|説明|config.h|
|-|-|-|
|MASTER|Y軸の直動機構（下側の直動機構）のATOM|PENPLOTTER_DEVICE_MASTER|
|CLIENT|X軸の直動機構（上側の直動機構、サーボが付いているほう）のATOM|PENPLOTTER_DEVICE_CLIENT|

プログラムを書き込む際、config.hの下記をそれぞれ有効（コメント削除）にして書き込んでください。下記はCLIENTに書き込むときの設定です。

```c
// #define PENPLOTTER_DEVICE_MASTER
#define PENPLOTTER_DEVICE_CLIENT
```

#### WI-FI設定
ネットワーク経由でデータを送信するためconfig.hにWiFiの設定を行います。

```c
/* Wi-Fi configurations */
#define WIFI_SSID ""
#define WIFI_KEY ""
```
### 必要ライブラリ
ESP32Servo

## 実行方法
プログラムを実行時、シリアルモニタにWiFi接続したAtomのIPアドレスが表示されるので覚えておきます。

上記M5AtomのIPアドレスのTCPポート23番にHPGLファイルを送信することで自動的に描画が開始されます。

そのために、netcatで上記のIPアドレスに向けてファイルを送信します。
（Windowsの場合はnetcatをインストールする必要があります）

Windowsの場合
```
type [HPGLファイル] | ncat [IPアドレス] 23
```

Linux系の場合
```
cat [HPGLファイル] | ncat [IPアドレス] 23
```


