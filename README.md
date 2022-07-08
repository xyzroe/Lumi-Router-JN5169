# Lumi Router

This is fork of [Lumi Router JN5169 firmware](https://github.com/igo-r/Lumi-Router-JN5169) for use with the LM15-SP-WM module used in the Xiaomi Smart Plug (ZNCZ02LM).  
Also it can be used with any other JN5169 based module.

The only difference is the ability to reset the device using a button on DIO 0.  
In the original firmware, all control takes place only through the serial port.

---

## How to flash

1. Download [JN-SW-4107](https://www.nxp.com/downloads/en/software-support/JN-SW-4107.zip) and install
2. Connect using JP1 to your USB-TTL according to table below:

LM15-SP-WM (JP1) | JN5169 | Function | USB-TTL
--- | --- | --- | ---
1 | DIO 28 | TXD0 | RX
2 | DIO 29 | RXD0 | TX
3 |DIO 22 | SPIMISO | GND  
4 |DIO 9/39 | VDD | 3.3V  
5 |DIO 3 | RESETN | ---
6 |DIO 21/39 | VSS | GND  

3. Download firmware binary to `C:\NXP\ProductionFlashProgrammer` and open console at this folder.
4. Run `JN51xxProgrammer.exe -s COM1 -f LumiRouter_20210707.bin -v -V`

---

## Reset and join
The device enters join mode in two cases:
  1. When PDM memory is erased
  2. Right after leaving existing network

Long (> 2000ms) clicking on BTN (DIO0 to GND) erases PDM memory.
The device will restart and then automatically join.

![](https://raw.githubusercontent.com/xyzroe/Lumi-Router-JN5169/master/images/z2m.png)

## Restart
Close JP1_5 (DIO 3) on GND to restart device.

---

### Small prehistory
Some time ago I found such modules based on JNN5169 on TaoBao and ordered a couple.  

![](https://raw.githubusercontent.com/xyzroe/Lumi-Router-JN5169/master/images/taobao.jpg)

Initially, I planned to use them as development boards to learn and play Zigbee SDK by NXP. I soldered just such a comb for easy installation into a breadboard.

![](https://raw.githubusercontent.com/xyzroe/Lumi-Router-JN5169/master/images/module.jpeg)

But since they are not very convenient as a development board, it was decided to make routers out of them to expand the Zigbee network.

---

### Making

![](https://raw.githubusercontent.com/xyzroe/Lumi-Router-JN5169/master/images/start.jpeg)

![](https://raw.githubusercontent.com/xyzroe/Lumi-Router-JN5169/master/images/back_side.jpeg)

![](https://raw.githubusercontent.com/xyzroe/Lumi-Router-JN5169/master/images/finish.jpeg)

![](https://raw.githubusercontent.com/xyzroe/Lumi-Router-JN5169/master/images/installed.jpeg)

---

## Thanks and links
[Original Lumi Router JN5169 firmware](https://github.com/igo-r/Lumi-Router-JN5169) by [Igor Listopad](https://github.com/igo-r)

Lots of info about LM15-SP-WM thanks to [@AFaron0v](https://t.me/AFaron0v)  

[Xiaomi Zigbee (1): Getting to know the hardware](https://alephsecurity.com/2019/07/01/xiaomi-zigbee-1/)
by Lev Aronsky [@levaronsky](https://twitter.com/levaronsky)

[Hello NXP JN5169 World](https://habr.com/ru/post/547374/) by Oleksandr Masliuchenko [@grafalex82](https://t.me/grafalex82)

[Comment 22819072](https://habr.com/ru/post/547374/#comment_22819072) thanks to [@mrG1K](https://t.me/mrG1K)
