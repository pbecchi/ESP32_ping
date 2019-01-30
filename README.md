
# ESP32

## CSI with esp32 console communication
	Python script for communication
	run with -h for help with arguments
	
## Communication between esp and python script

	Errorcode 101 == OK
	
| Action  | Send from  | Sended Data  | Answer  |
|:---:|:---:|:---:|:---:|
| Verify Connection  | Python  | 100 100 | 100 100  | 
| Restart | Python | 100 255| 100 0  
| Start a Wifi Hotspot  | Python  | 100 110 encoded SSID 10 encoded PW 10 channel 10 | 110 110 |   
| start Measuring  | Python  | 100 130 10 pingpong[ 0 oder 1] 10 (count / 10) 10(so max 2550 möglich) | 130 130  |   