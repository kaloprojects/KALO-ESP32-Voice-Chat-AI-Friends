# KALO AI Board Summary

Sharing a self constructed PCB (printed circuit board) as a DIY template for your projects, based on an ESP32-S3 (DFRobot FireBeetle 2).

Why this DFRobot ESP32-S3?: The [FireBeetle 2](https://wiki.dfrobot.com/SKU_DFR0975_FireBeetle_2_Board_ESP32_S3) full fills all needs for my mobile ESP32-AI-Device: ESP32-S3 performance (not ESP32 only), 16 MB PSRAM (flawless audio recording & and STT, SD card no longer needed, latest libraries supported), lot of GPIO pins, USB-C connector, 2 versions available (with internal & external wlan antenna), camera and display port (not needed for my project, but might be of interest for other projects) and last not least: the onboard LiPo battery charger & connector allows standalone (mobile) devices.

Sharing Gerber data (for 1:1 production requests) and my PCB CAD (.lay6) source file (enabling user to add their own modifications). Gerber data (.zip) allow ordering production from any PCB vendor (e.g. [PCBWay](https://www.pcbway.com/orderonline.aspx), [JLCPCB](https://cart.jlcpcb.com/quote)). PCB size is 100 x 100 mm (for low production costs). Detail: Left third of the PCB is constructed as perforated 2.54 matrix grid board, allowing to add additional components (or alternatively as carrier for a internal speaker). The optional cutting line reduces the PCB size to 100 x 62 mm (in case a smaller PCB with separate speaker preferred). I also kept a SD Card module slot on my PCB, not mandatory (as audio recording and STT is using PSRAM), just added for future ideas (e.g. for playing .mp3 files via voice request).   

The PCB was constructed with the PCB Design application [Sprint Layout 6.0](https://www.electronic-software-shop.com/lng/en/electronic-software/sprint-layout-60.html). 
<br>I publish my **.lay6** source file too, so any interested user can modify or extend the layout and circuits to their needs.

![2026-01 KALO AI Board - IMG_0375 Power ON](https://github.com/user-attachments/assets/d51bb915-bc10-47a9-b8d9-039f197fa445)


.
.
. <br>
# PCB Layout (front / back)

<img width="5198" height="2717" alt="2026-01 KALO AI Board - PCB Layout front   back view" src="https://github.com/user-attachments/assets/6390f184-a396-467a-a79e-b5aaf11906ed" />

![2026-01 KALO AI Board - PCB Real front   back view](https://github.com/user-attachments/assets/0cabb674-11b5-4fe2-abae-23114222ff82)

# BOM
|Component|BOM id# link|
|:---|:---|
|ESP32-S3 DFRobot FireBeetle2 (DFR0975/ DFR0975-U) - [WIKI](https://wiki.dfrobot.com/SKU_DFR0975_FireBeetle_2_Board_ESP32_S3) | [id#](https://www.digikey.de/de/products/filter/evaluierungsboards/evaluierungsboards-hf-rfid-und-wireless/1165?s=N4IgTCBcDaICIDEBKAGAnAdgKwgLoF8g)|
|I2S Microphone INMP441	| [id#](https://www.amazon.co.uk/s?k=INMP441)|
|I2S Amplifier Module MAX 98357	| [id#](https://www.amazon.co.uk/s?k=I2S+MAX98357)|
|LiPo 3.7V battery, e.g. 350mAh (PH2.0 connector)	| [id#](https://www.amazon.co.uk/s?k=Lipo+3.7v+350mA+PH2.0)|
|SD Card Module (optional)	| [id#](https://www.amazon.co.uk/s?k=ESP32+SD+card+module)|
|RGB LED 5mm (common Anode)	| [id#](https://www.digikey.de/de/products/detail/kingbright/WP154A4SEJ3VBDZGW-CA/6569334?s=N4IgTCBcDaIOoAUCMBWALAQTQZQKICkBmANQCEARALQHE4QBdAXyA)|
|B103 Poti Wheel 3-pin 10K	| [id#](https://www.amazon.co.uk/s?k=Potentiometer+B103+wheel)|
|Tactile button switch (round, 1/5“ grid)	| [id#](https://www.amazon.co.uk/s?k=pioneer+CDJ+button+switch)|
|3 x SMD resistor (size 1206) ideal: 330 / 470 / 680 Ohm (or alternative 3x 470)	| [id#](https://www.amazon.co.uk/s?k=sms+resistor+1206)|
|Power switch (e.g. E-Switch EG1224)	| [id#](https://www.digikey.de/en/products/detail/e-switch/EG1224/502052)|
|Speaker 3 Watt 8 Ohm (JST-PH2.0 connector), 13mm spacer	| [id#](https://www.amazon.co.uk/s?k=3+Watt+8+Ohm+JST-PH2.0+speaker)|
|Alternative: (small) Hi-End full range speaker (chassis mounted) Visatron SC 4.6 FL 8 Ohm	| [id#](https://www.digikey.de/de/products/detail/visaton-gmbh-co-kg/SC-4-6-FL-8-OHM/10650540)|
|2 x JST-PH2.0 connector right angle (2 pin) | [id#](https://www.digikey.de/en/products/detail/jst-sales-america-inc/S2B-PH-K-S/926626)|
|Male Jumper Pins & Pin Caps | [id#](https://www.amazon.co.uk/s?k=2.54+Jumper+caps)|

# PCB Circuits (GPIO Pin assignments)

Sharing the GPIO assignments (I did not create any separate circuit diagrams, my favorite PCB application Sprint Layout supports the direct drawing workflow). Latest code update (KALO_ESP32_Voice_Chat_AI_Friends_20260106.ino) supports these assignments by default:

|Group|Description|GPIO|
|:---|:---|:---|
|I2S microphone INMP441|I2S_WS|8|
||I2S_SD  |11|
||I2S_SCK |10|
||I2S_LR  |GND|
|I2S Amplifier Module MAX 98357|I2S_DOUT|   4|        
||I2S_LRC|    5|        
||I2S_BCLK|   6|        
|RGB Status led (connected via serial resistors)|pin_LED_RED|    0|        
||pin_LED_GREEN|  9|       
||pin_LED_BLUE|   18|  
|User controls |pin_RECORD_BTN| 38|       
||pin_TOUCH|      7|        
||pin_VOL_POTI|   3|        
|SD Card Module (optional)|SD_CS|          15|       
||SD_SCK|         14|          
||SD_MISO|        12|        
||SD_MOSI|        13|       

# Instruction details/comments

- **Battery**: Always check the _polarity (red +, black-)_ of any LiPo batteries!. The FireBeetle connector polarity is (unfortunately) _mirrored_ to most common batteries. Simply swap the cables _inside_ the battery PH2.0 connector if needed, the PCB tells you the correct red (+) and black (-) cable positions. I added 2 connectors for the battery (PH2.0 size). Left default connector (above speaker) routes thru an additional Power on/off switch, the 2nd connector (close ESP32) was added just in case the user wants to cut the pcb to a smaller size (100 x 62 mm) with an external speaker and power on/off switch inside battery cable (1:1 backside alternative to the battery connector on FireBeetle PH2.0 board)
- **SD Card module** is optional and located on backside. Solder the INMP441 microphone on frontside first, adding the SD Card module later on if needed
- **Wheel poti** (for audio volume) can be mounted on front or back side
- **RGB LED and resistors**: The RGB LED is connected via serial resistors. Ideal values are: 470 Ohm (R) - 680 Ohm (G) - 330 Ohm (B), reason: GREEN is typically brighter and BLUE is less bright than RED. No big deal if you decide to use same values (e.g. 470 Ohm) for all 3 colors. Ordering LED: Ensure that you choose a version with common _Anode_ (not GND). Resistors: I decided for SMD size _1206_ as this size can still be soldered manually. Hint for SMD soldering newbie’s: just add a tiny solder drop on one pad only, dive in the resistor, when connected then solder the other pad
- **Bridges**: Don't forget to add the 3 Jumper pin caps for Power On: INMP on / SD on / MAX on. Reason i added bridges: Allowing disconnecting the components in case not needed. Also don't forget to solder 2 tiny cable bridges on frontside (marked white connection line between + and - pair) crossing the optional pcb cutting line. The 3pin connector left of the MAX Amplifier was added for adjusting the default volume GAIN (typically connecting a bridge to upper GND pin, boosting to +12dB) 
- **I2S MAX 98357**: The (odd) standard screw connector is no longer needed, speaker outout of the module is connected with 2 single pins to the pcb, so any speaker with PH2.0 connector can connected more easy. **INMP441**: I am using two 3pin houses, this allows to plug-in the IMP441 with standard pins. On picture above I use extended pins (for best matching to the chassis height)  
- [**Sprint Layout 6.0**](https://www.electronic-software-shop.com/lng/en/electronic-software/sprint-layout-60.html) hints for SL6 newbie’s: Most connections in PCB are grouped, just right click to ungroup elements or hold _Alt_ to select single components. Creating own trace connections: I recommend to use the keyboard _Space_ key to toggle thru angles on the fly. My measure recommendations: 0.3mm for standard traces, 1.7/0.9 mm for plated through-holes, grid 25 mil.

Enjoy the Gerber files or create your own modified PCB with my .lay6 source file ! :thumbsup:
