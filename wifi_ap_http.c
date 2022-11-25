/* This file contain functions used for generate access point with web page */

#include "apWebSerwer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "openssl_server.h"
#include "rtc.h"

uint32_t g_version=0x0100;

connection_info_t connectionInfo;
EventGroupHandle_t wifi_event_group;
//------------------------------------------------------------------------------
extern esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
//------------------------------------------------------------------------------
static uint8_t getDataFromBuffer(char *readBuffer, char *writeBuffer, char *stringToFind, uint8_t position);
static uint8_t decodeValues(char* buffer, uint16_t bufferLength);
static void saveConnectionInfo(connection_info_t *pConnectionInfo);
static void writeDataToConnectionInfoStruct(const char *SSID, const char *Pass, const char *Ip, const char *gateWay, const char *netMask, const char *dateTime);
/* @brief: clear pass buffer */
static void clearPassBuffer(char *bufferToClear, uint8_t bufferLength);
static void decodeDatTimeBuf(char *dateTime);
//------------------------------------------------------------------------------
/*
 * @brief: become an access point, set AP with all necessary data
 */
void becomeAccessPoint(void)
{
	/* disable default wifi logging */
	esp_log_level_set("wifi", ESP_LOG_NONE);

	wifi_event_group = xEventGroupCreate();

	/* Initialize tcpIp adapter, inside function it also initialize TCPIP stack */
	tcpip_adapter_init();

    /* stop DHCP server */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));

    /* Assign static ip to AP interface */
    tcpip_adapter_ip_info_t ipInfo;
    memset(&ipInfo, 0, sizeof(ipInfo));

    IP4_ADDR(&ipInfo.ip, 192,168,10,1);
    IP4_ADDR(&ipInfo.gw, 192,168,10,1);
    IP4_ADDR(&ipInfo.netmask, 255,255,255,0);

	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo));

    /* start DHCP server after disable it */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

    /* Initialize wifi event handler */
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));	 /* Register a function that will be called
    																		when the ESP32 detect certain types
    																		of wifi related events */
    /* Initialize wifi stack as access point, config store in RAM */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));					    	 /* Initialize wifi subsystem */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

	wifi_config_t apConfig = {
			.ap = {
					.ssid="Lock Data",
					.ssid_len=0,
					.password="hskdata1",
					.channel=0,
					.authmode=WIFI_AUTH_WPA2_PSK,
					.ssid_hidden=0,
					.max_connection=4,
					.beacon_interval=100
			}
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apConfig));

	/* start wifi */
	ESP_ERROR_CHECK(esp_wifi_start());
}
//-----------------------------------------------------------------------------------------------
/* Print info about connected devices */
void printConnectedStations()
{
	printf("Display connected stations:\n");

	wifi_sta_list_t wifi_sta_list;
	tcpip_adapter_sta_list_t tcpip_adapter;

	memset(&wifi_sta_list, 0 ,sizeof(wifi_sta_list));
	memset(&tcpip_adapter, 0, sizeof(tcpip_adapter));

	ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&wifi_sta_list));
	ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&wifi_sta_list, &tcpip_adapter));

	for(uint8_t i=0; i<tcpip_adapter.num; i++)
	{
		tcpip_adapter_sta_info_t sta_info = tcpip_adapter.sta[i];
		printf("Stat: %d - mac: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X - IP: %s\n\n", i + 1,
				sta_info.mac[0], sta_info.mac[1], sta_info.mac[2], sta_info.mac[3],
				sta_info.mac[4], sta_info.mac[5], ip4addr_ntoa(&(sta_info.ip)));
	}
}
//-----------------------------------------------------------------------------------------------
/* Display information if there is new device connected, or old device disconnected */
void displInfoAboutConnSta()
{
	EventBits_t staBits = xEventGroupWaitBits(wifi_event_group, BIT0 | BIT1,
													pdTRUE, pdFALSE, portMAX_DELAY);
	if((staBits & BIT0) != 0)
	{
		printf("New device connected\n");
	}
	else
	{
		printf("A station disconnected\n");
	}
}
//-----------------------------------------------------------------------------------------------
/* Task for display information about Connection status and connected stations to ESP32 AP*/
void printfAPConnDev()
{
	while(1)
	{
		displInfoAboutConnSta();
		printConnectedStations();
		vTaskDelay(10000/portTICK_RATE_MS);
	}
}
//-----------------------------------------------------------------------------------------------
/*
 * @brief: create simple Http server
 */
void http_server(void *pvParameters)
{
  struct netconn *conn_net, *newconn;
  err_t err;

  printf("Start server set\n\n");

  /* Creates a new connection and returns a pointer to netconn stract */
  conn_net = netconn_new(NETCONN_TCP);

  netconn_bind(conn_net, IP_ADDR_ANY, 80);

  netconn_listen(conn_net);

  do
  {
     err = netconn_accept(conn_net, &newconn);

     if (err == ERR_OK)
     {
       http_server_netconn(newconn);
       netconn_delete(newconn);
     }
   } while(err == ERR_OK);

  netconn_close(conn_net);
  netconn_delete(conn_net);
}
//-----------------------------------------------------------------------------------------------
/*
 * @brief: Function use for data communication
 */
void http_server_netconn(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  uint16_t buflen;
  err_t err;
  char data[20];

  err = netconn_recv(conn, &inbuf);

  printf("http_server_netconn_serve");

  if (err == ERR_OK)
  {
    netbuf_data(inbuf, (void**)&buf, &buflen);

    /* Display buffer in the screen */
    printf("buffer = %s \n", buf);

    /* Check if there was any GET */
    if(buflen>=5 && buf[0]=='G' && buf[1]=='E' && buf[2]=='T' && buf[3]==' ' && buf[4]=='/' )
    {
        printf("buf[5] = %c \n", buf[5]);

       /*
        * Send the HTML header
        */

        printf("netconn_write \n\n");
        netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
        netconn_write(conn, http_index_hml, sizeof(http_index_hml)-1, NETCONN_NOCOPY);
    }
    /* If there was a post msg */
    else if(buf[0] == 'P' && buf[1] == 'O' && buf[2] == 'S' && buf[3] == 'T' && buf[4]==' ' && buf[5]=='/')
    {
        Uart_Send_Data(1, (uint8_t*)"Data receive\r\n");
        Uart_Send_Data(1, (uint8_t*)buf);

        char *ptr = strstr(buf, "ssid=");

        if(ptr)
        {
        	Uart_Send_Data(1, (uint8_t*)"found ssid\r\n");
        	/* Decode receive msg */
        	decodeValues(ptr, 200);
        }
        netconn_write(conn, http_index_post_enterData, sizeof(http_index_post_enterData)-1, NETCONN_NOCOPY);
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete buffer from netbuf */
  netbuf_delete(inbuf);
}
//-----------------------------------------------------------------------
/*
 * 	@brief:	function use to get information that come
 * 				from basic HTTP web page works as AP.
 * 				It looks for stringToFind in readBuffer then buffer that we need
 * 				is write into writeBuffer
 *	@param:	*readBuffer: 	buffer that contain receive data
 *	@param:	*writeBuffer: 	buffer were seperate information from read Buffer will be write
 *	@param: *stringToFind:	pointer to buffer that contain
 *	@param:	*position:		size of stringToFind
 *	@ret:	operation status, 1 when string was found and there are data to write, 0 if string could not be find,
 *				or there is no data to write
 * */
static uint8_t getDataFromBuffer(char *readBuffer, char *writeBuffer, char *stringToFind, uint8_t position)
{
	uint8_t status = 0;
	char *pointer;

	pointer = strstr(readBuffer, stringToFind);

    if(pointer)
    {
		for(uint8_t loopSSID = 0; loopSSID<40; loopSSID++)
		{
			if(pointer[position] != '&' && pointer[position] != '\0')
			{
				writeBuffer[loopSSID] = pointer[position];
				position++;
				status = 1;
			}
			else
			{
				break;
			}
		}
    }

    if(status == 1)
    {
    	return 1;
    }
    return 0;
}
//-----------------------------------------------------------------------
/*
 * @brief:	decode values that was pass from http web page in AP mode
 * @param:	*buffer: buffer with pass data
 * @param:	bufferLength: length of pass buffer
 */
static uint8_t decodeValues(char* buffer, uint16_t bufferLength)
{
	char tmpSSID[32]	= {0};
	char tmpPass[64] 	= {0};
	char tmpIp[16] 		= {0};
	char tmpGw[16] 		= {0};
	char tmpNetMask[16] = {0};
	char tmpTimeAndDate[20] = {0};
	uint8_t receiveData	= 0;
	char data[80] = {0};

	/* Get SSID */
    if(getDataFromBuffer(buffer, tmpSSID, "ssid=", 5) == 1)
    {
		sprintf(data, "ssid: %s\r\n", tmpSSID);
		Uart_Send_Data(1, (uint8_t*)data);
		clearPassBuffer(data, sizeof(data));
		receiveData++;
    }

    /* Get password */
    if(getDataFromBuffer(buffer, tmpPass, "password=", 9) == 1)
    {
    	sprintf(data, "password: %s\r\n", tmpPass);
    	Uart_Send_Data(1, (uint8_t*)data);
    	clearPassBuffer(data, sizeof(data));
    	receiveData++;
    }

    /* Get Ip*/
    if(getDataFromBuffer(buffer, tmpIp, "ip=", 3) == 1)
    {
    	sprintf(data, "ip: %s\r\n", tmpIp);
    	Uart_Send_Data(1, (uint8_t*)data);
    	clearPassBuffer(data, sizeof(data));
    	receiveData++;
    }

    /* Get gw */
    if(getDataFromBuffer(buffer, tmpGw, "gw=", 3) == 1)
    {
    	sprintf(data, "gw: %s\r\n", tmpGw);
    	Uart_Send_Data(1, (uint8_t*)data);
    	clearPassBuffer(data, sizeof(data));
    	receiveData++;
    }

    /* Get netmask */
    if(getDataFromBuffer(buffer, tmpNetMask, "netmask=", 8) == 1)
    {
    	sprintf(data, "netMask: %s\r\n", tmpNetMask);
    	Uart_Send_Data(1, (uint8_t*)data);
        clearPassBuffer(data, sizeof(data));
        receiveData++;
    }

    /* Get time and date */
    if(getDataFromBuffer(buffer, tmpTimeAndDate, "time=", 5) == 1)
    {
    	sprintf(data, "DateTime: %s\r\n", tmpTimeAndDate);
    	Uart_Send_Data(1, (uint8_t*)data);
    	clearPassBuffer(data, sizeof(data));
    	receiveData++;
    }

    decodeDatTimeBuf(tmpTimeAndDate);

	//writeDataToConnectionInfoStruct(tmpSSID, tmpPass, tmpIp, tmpGw, tmpNetMask);

	return receiveData;
}
//-----------------------------------------------------------------------
/*
    * writes connection information into NVS
*/
static void saveConnectionInfo(connection_info_t *pConnectionInfo)
{
	nvs_handle handle;
	ESP_ERROR_CHECK(nvs_open(BOOTWIFI_NAMESPACE, NVS_READWRITE, &handle));
	ESP_ERROR_CHECK(nvs_set_blob(handle, KEY_CONNECTION_INFO, pConnectionInfo, sizeof(connection_info_t)));
	ESP_ERROR_CHECK(nvs_set_u32(handle, KEY_VERSION, g_version));
	ESP_ERROR_CHECK(nvs_commit(handle));
	nvs_close(handle);
}
//-----------------------------------------------------------------------
/* @brief:	decode dateTime buffer from HttpWebPage
 * 			dd.mm.rr-gg.mm
 * @param:  string with data
 * */
static void decodeDatTimeBuf(char *dateTime)
{
	uint8_t table[5] = {0};
	uint8_t i = 0;
	uint8_t j = 0;

	uint8_t day = 0;
	uint8_t month = 0;
	uint16_t year = 0;

	uint8_t hour = 0;
	uint8_t minutes = 0;

	char data[20] = {0};
	char *eptr;
	char tmpBuf[2] = {0};
	char tmpBufYear[4] = {0};

	//------------------------------------
	/* Get day */
	while(dateTime[i] != '.')
	{
		tmpBuf[j] = dateTime[i];
		i++;
		j++;
	}

	day = strtol(tmpBuf, &eptr, 10);

	sprintf(data, "day: %u\r\n",day);
	Uart_Send_Data(1, (uint8_t*)data);
	clearPassBuffer(data, sizeof(data));
	clearPassBuffer(tmpBuf, sizeof(tmpBuf));

	i++;
	j=0;
	//------------------------------------
	/* get month */
	while(dateTime[i] != '.')
	{
		tmpBuf[j] = dateTime[i];
		i++;
		j++;
	}

	month = strtol(tmpBuf, &eptr, 10);

	sprintf(data, "month: %u\r\n",month);
	Uart_Send_Data(1, (uint8_t*)data);
	clearPassBuffer(data, sizeof(data));
	clearPassBuffer(tmpBuf, sizeof(tmpBuf));

	i++;
	j=0;
	//------------------------------------
	/* get year */
	while(dateTime[i] != '-')
	{
		tmpBufYear[j] = dateTime[i];
		i++;
		j++;
	}

	year = strtol(tmpBufYear, &eptr, 10);
	year -=2000;

	sprintf(data, "year: %u\r\n",year);
	Uart_Send_Data(1, (uint8_t*)data);
	clearPassBuffer(data, sizeof(data));
	clearPassBuffer(tmpBufYear, sizeof(tmpBufYear));
	i++;
	j=0;
	//------------------------------------
	/* get hour */
	while(dateTime[i] != '.')
	{
		tmpBuf[j] = dateTime[i];
		i++;
		j++;
	}

	hour = strtol(tmpBuf, &eptr, 10);

	sprintf(data, "hour: %u\r\n",hour);
	Uart_Send_Data(1, (uint8_t*)data);
	clearPassBuffer(data, sizeof(data));
	clearPassBuffer(tmpBuf, sizeof(tmpBuf));

	i++;
	j=0;
	//------------------------------------
	/* get minutes */
	while(dateTime[i] != '\0')
	{
		tmpBuf[j] = dateTime[i];
		i++;
		j++;
	}

	minutes = strtol(tmpBuf, &eptr, 10);

	sprintf(data, "minutes: %u\r\n",minutes);
	Uart_Send_Data(1, (uint8_t*)data);
	clearPassBuffer(data, sizeof(data));
	clearPassBuffer(tmpBuf, sizeof(tmpBuf));

	writeValueString(hour, minutes, 1, day, month, year);
}
//-----------------------------------------------------------------------
/*
 * @brief: write data with info to struct
 */
static void writeDataToConnectionInfoStruct(const char *SSID, const char *Pass, const char *Ip, const char *gateWay, const char *netMask, const char *dateTime)
{
	char data[70];
	/* Write SSID to main buffer */
	for(uint8_t loop = 0; loop<32; loop++)	{	connectionInfo.ssid[loop] = SSID[loop];		}

	/* Write password to main buffer */
	for(uint8_t loop = 0; loop<64; loop++)	{	connectionInfo.password[loop] = Pass[loop];	}

	if(strcmp(Ip, "") != 0) 				{	inet_pton(AF_INET, Ip, &connectionInfo.ipInfo.ip); 				}
	else									{	connectionInfo.ipInfo.ip.addr = 0;								}

	if(strcmp(gateWay, "") != 0)			{	inet_pton(AF_INET, gateWay, &connectionInfo.ipInfo.gw);			}
	else									{	connectionInfo.ipInfo.gw.addr = 0;								}

	if(strcmp(netMask, "") != 0)			{	inet_pton(AF_INET, netMask, &connectionInfo.ipInfo.netmask);	}
	else									{	connectionInfo.ipInfo.netmask.addr = 0;							}

	/* write data into I2C clock */
	if(strcmp(dateTime, "") != 0)			{ decodeDatTimeBuf(dateTime); }

	saveConnectionInfo(&connectionInfo);
}
//-----------------------------------------------------------------------
/*
 * @brief:	Clears buffer that was pass to this function
 * @param:	*bufferToClear: pointer to buffer that needs to be clear
 * @param:	bufferLength: length of buffer ro clear
 * @ret:	none
 * */
static void clearPassBuffer(char *bufferToClear, uint8_t bufferLength)
{
	for(uint8_t i = 0; i<bufferLength; i++)
	{
		*(bufferToClear + i) = 0;
	}
}
