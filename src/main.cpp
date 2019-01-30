#include <Arduino.h>

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/semphr.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_vfs_fat.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/inet.h"

#define MOUNT_PATH "/data"
#define PORT 17000;

//HANDELS
wl_handle_t wl_handle = WL_INVALID_HANDLE;
EventGroupHandle_t s_wifi_event_group;
SemaphoreHandle_t Mutex;

//Configs
wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT()
;

//Global Variables
char buf[100];
int p;
int soc;
int port = PORT
;
bool AP = false;
bool Csi = false;
bool master = true;
bool socketBinded = false;
sockaddr_in* deviceList;

esp_err_t event_handler(void *ctx, system_event_t *event) {

	switch (event->event_id) {

	case SYSTEM_EVENT_STA_START:
		ESP_LOGV(TAG,"Station Started");
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_STOP:
		ESP_LOGV(TAG,"Station Stopped");
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		ESP_LOGV(TAG,"Station Connected to SSID: %s" , &event->event_info.connected.ssid);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		ESP_LOGV(TAG,"Station Disconnected");
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG,"Connected with IP Adress: %s" , ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip) );
		break;
	case SYSTEM_EVENT_AP_START:
		ESP_LOGV(TAG,"AP Started sucessfully");
		break;
	case SYSTEM_EVENT_AP_STOP:
		ESP_LOGV(TAG,"AP Stopped sucessfully");
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG,"Station connected with MAC: " MACSTR , MAC2STR(&event->event_info.sta_connected.mac) );
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG,"Station disconnected with MAC: " MACSTR , MAC2STR(&event->event_info.sta_disconnected.mac) );
		break;
	case SYSTEM_EVENT_AP_STAIPASSIGNED:
		ESP_LOGI(TAG,"Station with mac got ip assigned");
		break;
	default:
		ESP_LOGE(TAG, "Unhandeled Event: %d", event->event_id);
		break;
	}
	return ESP_OK;
}

/*
 * Init the non volatile Storage system
 *
 * @author Robin Simon
 */
bool init_nvs() {
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "NVS FLASH INIT FAILED WITH ERROR: %s",
				esp_err_to_name(err));
		return false;
	}
	return true;
}
/*
 * Init the Filesystem
 *
 * @author Robin Simon
 */
bool init_filesystem() {
	const esp_vfs_fat_mount_config_t mount_config = { .format_if_mount_failed =
	true, .max_files = 4, .allocation_unit_size = CONFIG_WL_SECTOR_SIZE };
	esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage",
			&mount_config, &wl_handle);

	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to mount FATFS WITH ERROR: %s",
				esp_err_to_name(err));
		return false;
	}

	return true;
}
/*
 * Init the necessary components
 *
 * @author Robin Simon
 */
bool esp_init() {

	tcpip_adapter_init();

	if (!init_nvs()) {
		return false;
	}

	if (!init_filesystem()) {
		return false;
	}

	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	//differ the default config to activate Csi
	wifi_init_config.csi_enable = 1;
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

	//Nom WIFI Mode selected yet
	esp_wifi_set_mode(WIFI_MODE_NULL);

	s_wifi_event_group = xEventGroupCreate();
	return true;
}
/*
 * Start the AP
 *
 * @author Robin Simon
 */
bool startAP(const char* ssid, const char* pw, uint8_t channel) {
	if (AP) {
		return true;
	}
	wifi_config_t wifi_config = { };
	wifi_config.ap = {};

	strcpy((char *) wifi_config.ap.ssid, ssid);
	wifi_config.ap.ssid_len = strlen(ssid);
	strcpy((char *) wifi_config.ap.password, pw);

	wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
	wifi_config.ap.channel = channel;
	wifi_config.ap.max_connection = 10;

	esp_wifi_set_mode(WIFI_MODE_AP);

	esp_err_t err_wifi_config = (esp_wifi_set_config(ESP_IF_WIFI_AP,
			&wifi_config));
	if (err_wifi_config != ESP_OK) {
		ESP_LOGE(TAG, "Could not set Wifi Config WITH ERROR: %s",
				esp_err_to_name(err_wifi_config));
		return false;
	}

	esp_err_t err_wifi_start = esp_wifi_start();
	if (err_wifi_start != ESP_OK) {
		ESP_LOGE(TAG, "Could not start Wifi WITH ERROR: %s",
				esp_err_to_name(err_wifi_start));
		return false;
	}

	ESP_LOGI(TAG, "wifi_init_softap finished.\nSSID: %s password: %s channel: %d",ssid , pw , channel);
	AP = true;
	return true;;
}

/*
 * Connect to an AP
 *
 * @author Robin Simon
 */
bool connAP(const char* ssid, const char* pw) {

	wifi_config_t wifi_config = { };
	wifi_config.sta = {};

	strcpy((char *) wifi_config.sta.ssid, ssid);
	strcpy((char *) wifi_config.sta.password, pw);

	esp_wifi_set_mode(WIFI_MODE_STA);

	esp_err_t err_wifi_config = (esp_wifi_set_config(ESP_IF_WIFI_STA,
			&wifi_config));
	if (err_wifi_config != ESP_OK) {
		ESP_LOGE(TAG, "Could not set Wifi Config WITH ERROR: %s",
				esp_err_to_name(err_wifi_config));
		return false;
	}

	esp_err_t err_wifi_start = esp_wifi_start();
	if (err_wifi_start != ESP_OK) {
		ESP_LOGE(TAG, "Could not start Wifi WITH ERROR: %s",
				esp_err_to_name(err_wifi_start));
		return false;
	}

	ESP_LOGI(TAG, "Connect to SSID: %s password: %s ",ssid , pw );

	return true;;
}

void setup() {
	Serial.begin(115200);
	p = 0;
	esp_init();
	Mutex = xSemaphoreCreateMutex();
}

void readAP() {
	char ssid[128], pw[128];
	int ch = 0, p = 0;
	do {
		ssid[p] = Serial.read();
		p++;
	} while (ssid[p - 1] != 10);
	ssid[p - 1] = 0;
	p = 0;
	do {
		pw[p] = Serial.read();
		p++;
	} while (pw[p - 1] != 10);
	pw[p - 1] = 0;
	p = 0;
	ch = Serial.read() - 32;
	p = -1;
	if (startAP(ssid, pw, ch)) {
		Serial.write(10);
		Serial.write(110);
		Serial.write(110);
		Serial.write(10);
	}
}

void connectAP() {
	char ssid[128], pw[128];
	int p = 0;
	do {
		ssid[p] = Serial.read();
		p++;
	} while (ssid[p - 1] != 10);
	ssid[p - 1] = 0;
	p = 0;
	do {
		pw[p] = Serial.read();
		p++;
	} while (pw[p - 1] != 10);
	pw[p - 1] = 0;
	p = -1;
	if (connAP(ssid, pw)) {
		Serial.write(10);
		Serial.write(110);
		Serial.write(110);
		Serial.write(10);
	}
}

void readMesVar(bool *pingpong, int *count, int *devices) {

	char b;

	b = Serial.read();

	while (b == 10) {
		b = Serial.read();
	}

	if (b == 200) {
		*pingpong = true;
	} else {
		*pingpong = false;
	}

	if (Serial.read() == 10) {
		*count = Serial.read() * 10;
	}

	if (Serial.read() == 10) {
		*devices = Serial.read();
	}

	Serial.write(10);
	Serial.write(130);
	Serial.write(130);
	Serial.write(10);
	p = -1;
	ESP_LOGI(TAG,"pingpong is: %d count is %d devicecount is %d",*pingpong,*count,*devices);

}

static void csi_handler(void *ctx, wifi_csi_info_t *data) {

	ESP_LOGI(TAG,"%d",data->rx_ctrl.timestamp);
	//TODO: Handler

}

bool startCsi() {
	if (Csi) {
		return true;
	}
	wifi_csi_config_t csi_config = { };
	csi_config.htltf_en = false;
	csi_config.lltf_en = true;
	csi_config.manu_scale = false;
	csi_config.shift = 0;
	csi_config.stbc_htltf2_en = false;
	ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
	ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(csi_handler , NULL));
	ESP_ERROR_CHECK(esp_wifi_set_csi(true));

	Csi = true;
	return true;
}

bool startSocket() {
	if (socketBinded) {
		ESP_LOGI(TAG, "Socket binded");
		return true;
	}
	struct sockaddr_in sockAddr;
	struct timeval t;

	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(port);
	sockAddr.sin_len = sizeof(sockAddr);

	t.tv_sec = 100;
	t.tv_usec = 0;

	if ((soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		ESP_LOGE(TAG, "Socket Could not be created");
		return false;
	}

	if (setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t)) < 0) {
		closesocket(soc);
		ESP_LOGE(TAG, "Socketopt timeout could not be set");
		return false;
	}

	if ((bind(soc, (struct sockaddr *) &sockAddr, sizeof(sockAddr))) < 0) {
		ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
		closesocket(soc);
	}ESP_LOGI(TAG, "Socket binded");

	socketBinded = true;
	return true;
}

bool waitForDevices(int amount, int timetowait) {
	char input[50], addrString[128];
	int ak_count = 0;
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);

	long time = millis();
	do {
		int len = recvfrom(soc, (void*) input, 50, 0, (sockaddr*) &from,
				&fromlen);

		if (len > 0) {

			if (len == 5 && input[0] == 100 && input[1] == 101
					&& input[2] == 102 && input[3] == 103 && input[4] == 104) {
				memcpy((void*) &deviceList[ak_count], (void*) &from,
						sizeof(sockaddr_in));
				ak_count++;
			}ESP_LOGI(TAG,"input: %d %s",len,input);
		}ESP_LOGI(TAG,"%d",ak_count);
	} while (ak_count < amount && (millis() - time < timetowait));

	for (int i = 0; i < ak_count; i++) {
		inet_ntoa_r(from.sin_addr.s_addr, addrString, sizeof(addrString) - 1);ESP_LOGI(TAG,"Position %d: IpAdress: %s",i,addrString);
	}
	return true;
}

bool prepareMes(bool pingpong, int count, int devices) {
	startCsi();
	startSocket();
	deviceList = (sockaddr_in*) malloc(devices * sizeof(sockaddr_in));
	waitForDevices(devices, 100000);
	return true;
}

void sniffer_cb(void *buf, wifi_promiscuous_pkt_type_t type) {

	wifi_promiscuous_pkt_t* paket = (wifi_promiscuous_pkt_t *) buf;
	int len = paket->rx_ctrl.sig_len;
	if (len > 0) {

		for (int i = 0; i < len; i++) {
			Serial.write(paket->payload[i]);
		}
		Serial.write(10);

	}
}

void sendData(char *data, size_t datalen, ip4_addr_t ipaddr) {

	struct sockaddr_in destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_len = sizeof(destAddr);
	destAddr.sin_port = htons(port);
	destAddr.sin_addr.s_addr = ip4_addr_get_u32(&ipaddr);

	int err = sendto(soc, data, datalen, 0, (struct sockaddr *) &destAddr,
			sizeof(destAddr));
	if (err < 0) {
		ESP_LOGE(TAG, "Error occured during sending errno %d", errno);
		return;
	}

	ESP_LOGI(TAG,"DATA send");
}
void sendData(char *data, size_t datalen, sockaddr_in ipaddr) {

	struct sockaddr_in destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_len = sizeof(destAddr);
	destAddr.sin_port = htons(port);
	destAddr.sin_addr = ipaddr.sin_addr;

	int err = sendto(soc, data, datalen, 0, (struct sockaddr *) &destAddr,
			sizeof(destAddr));
	if (err < 0) {
		ESP_LOGE(TAG, "Error occured during sending errno %d", errno);
		return;
	}

	ESP_LOGI(TAG,"DATA send");
}

bool startMes(int dev) {

	for (int i = 0; i < dev; i++) {
		char data[9] = "TestData";
		sendData(data, sizeof(char) * 9, deviceList[0]);
	}

	return true;
}

void readInput() {
	if (Serial.available() > 0) {
		buf[p] = Serial.read();
		if (buf[p] == 10) {
			switch (p) {
			case 0:
				p = -1;
				break;
			case 2:
				if (buf[0] == 100 && buf[1] == 100) {
					Serial.write(10);
					Serial.write(100);
					Serial.write(100);
					Serial.write(10);
					p = -1;
				}
				if (buf[0] == 100 && buf[1] == 110) {
					readAP();
				}
				if (buf[0] == 100 && buf[1] == 111) {
					//connect to Ap and register with the master (should always be on 192.168.4.1)
					connectAP();
					startSocket();
					sleep(10);
					char data[5] = { 100, 101, 102, 103, 104 };
					ip4_addr_t ipaddr;
					IP4_ADDR(&ipaddr, 192, 168, 4, 1);
					sendData(data, sizeof(char) * 5, ipaddr);

					char input[10];
					do {
						int len = recv(soc, (void*) input, 10, 0);

						if (len > 0) {
							ESP_LOGI(TAG,"input: %d %s",len,input);
						}

					} while (true);
				}
				if (buf[0] == 100 && buf[1] == 130) {
					bool pingpong;
					int count;
					int devices;
					//master starts the measurement
					readMesVar(&pingpong, &count, &devices);
					prepareMes(pingpong, count, devices);
					//signal to skript all requirements are met and Measurement starts
					Serial.write(10);
					Serial.write(130);
					Serial.write(140);
					Serial.write(10);

					startMes(devices);
				}

				break;
			case 3:
				if (buf[0] == 200 && buf[1] == 200 && buf[2] == 100) {
					wifi_promiscuous_filter_t filter = { };
					filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;
					esp_wifi_set_promiscuous_filter(&filter);

					esp_wifi_set_promiscuous_rx_cb(sniffer_cb);
					esp_wifi_set_promiscuous(true);

					while (true) {
					};
					p = -1;
				}
				break;
			default:
				p = -1;
			}
		}
		p++;
	}
}

void loop() {

	readInput();

}

