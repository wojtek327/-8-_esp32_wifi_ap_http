#ifndef MAIN_APWEBSERWER_H_
#define MAIN_APWEBSERWER_H_

#include "main.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "freertos/portmacro.h"
#include "freertos/event_groups.h"

#define SSID_SIZE (32)			/* Maximum SSID size for wifi 		*/
#define PASSWORD_SIZE (64)		/* Maximum Password size for wifi 	*/

/* @brief: keys for stored data value about connection information in nvs */
#define KEY_CONNECTION_INFO "connectionInfo"
#define BOOTWIFI_NAMESPACE 	"bootwifi"
#define KEY_VERSION 		"version"

const static char http_html_hdr[] =
    "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

const static char http_index_hml[] = "<!DOCTYPE html>"
      "<html>"
      "<head>"
	  "	<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"UTF-8\""
	  "	<style>"
	  "	</style>"
	  "<title>Panel logowania</title>\n"
	  "</head>"
	  "<body bgcolor=\"#001F3F\">"
	  "<div style=\"zoom: 100%\">"
	  "<h1><font color = \"#FFFFFF\">Ustaw parametry sieci wifi:</hi>"
	  "<form action=\"enterData\" method=\"post\">"
	  "<table width=\"65%\" cellspacing=\"0\" cellpadding=\"10\">"
	  "<tbody>"
	  "<tr style=\"color: #FFFFFF\">"
	  "<td>SSID:</td>"
	  "<td><input type=\"text\" autocorrect=\"off\" autocapitalize=\"none\" name=\"ssid\" /></td>"
	  "</tr>"
	  "<tr style=\"color: #FFFFFF\">"
	  "<td>Password:</td>"
	  "<td><input type=\"text\" autocorrect=\"off\" autocapitalize=\"none\" name=\"password\" /></td>"
	  "</tr>"
	  "<tr style=\"color: #FFFFFF\">"
	  "<td>Adres Ip:</td>"
	  "<td><input type=\"text\" autocorrect=\"off\" autocapitalize=\"none\" name=\"ip\" /></td>"
	  "</tr>"
	  "<tr style=\"color: #FFFFFF\">"
	  "<td>Brama domyślna:</td>"
	  "<td><input type=\"text\" autocorrect=\"off\" autocapitalize=\"none\" name=\"gw\" /></td>"
	  "</tr>"
	  "<tr style=\"color: #FFFFFF\">"
	  "<td>Maska podsieci:</td>"
	  "<td><input type=\"text\" autocorrect=\"off\" autocapitalize=\"none\" name=\"netmask\" /></td>"
	  "</tr>"
	  "<tr style=\"color: #FFFFFF\">"
	  "<td>Data i godzina:</td>"
	  "<td><input type=\"text\" autocorrect=\"off\" autocapitalize=\"none\" name=\"time\" /></td>"
	  "</tr>"
	  "</tbody>"
	  "</table>"
	  "<p>"
	  "<input type=\"submit\" value=\"Ustaw parametry sieci w urządzeniu\" style=\"font-weigth:bold; font-size:32px; width:50%; height:100px\">"
	  "</p>"
	  "</form>"
	  "</div>"
	  "</body>"
	  "</html>";

const static char http_index_post_enterData[] = "<!DOCTYPE html>"
	      "<html>"
	      "<head>"
		  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\""
		  "<style>"
		  "</style>"
		  "<title>Wgrywanie zakończone</title>\n"
		  "</head>"
		  "<body bgcolor=\"#001F3F\">"
		  "<div style=\"zoom: 100%;\">"
		  "<h1><font color = \"#FFFFFF\">Parametry sieci ustawione</hi>"
		  "</div>"
		  "</body>"
		  "</html>";

extern EventGroupHandle_t wifi_event_group;

typedef struct{
	char ssid[SSID_SIZE];
	char password[PASSWORD_SIZE];
	tcpip_adapter_ip_info_t ipInfo;
}connection_info_t;

//-----------------------------------------------------------------------------------------------
void becomeAccessPoint(void);
//-----------------------------------------------------------------------------------------------
void http_server_netconn(struct netconn *conn);
//-----------------------------------------------------------------------------------------------
void http_server(void *pvParameters);
//-----------------------------------------------------------------------------------------------
void printfAPConnDev();
//-----------------------------------------------------------------------------------------------

#endif /* MAIN_APWEBSERWER_H_ */
