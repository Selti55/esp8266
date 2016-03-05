/*--------------------------------------------------
HTTP 1.1 Webserver als AccessPoint fuer ESP8266 
=== Arduino IDE ===

Juergens Jalousine

Info Selti:
--set  WiFi IP Adresse
--192.168.255.x to play in ROFA
--ask Martin why ;-)
--------------------------------------------------*/

#include <ESP8266WiFi.h>


/*
Die Pin's des ESP8266 mittels define zuordnen
*/
// #define ESP12E_VCC 8			// +.3V
// #define ESP12E_GND 15			// GND
// #define ESP12E_RST 1			// RST   (Reset mit LOW)
// #define ESP12E_ADC 2			// ADC   (Analogeingang 3,3V max)
// #define ESP12E_EN 3				// EN    (Enable mit HIGH)
// #define ESP12E_CS0 9			// CSO
// #define ESP12E_MISO 10			// MISO
// #define ESP12E_MOSI 13			// MOSI
// #define ESP12E_SCLK 14			// SCLK
// #define ESP12E_RXT0 21			// RXT0   (GBIO3)
// #define ESP12E_TXT0 22			// TXT0   (GPIO1)
// #define ESP12E_GPIO0 18			// GPIO01 (Flash mit LOW)
// #define ESP12E_GPIO2 17			// GPIO02 (interne LED)
#define M1AUF 19				// GPIO04 <-- Antrieb M1 auf
#define M1AB 20					// GPIO05 <-- Antrieb M1 ab
// #define M2AUF 11				// GPIO09 <-- Antrieb M2 auf	<---Problem
#define M2AB 12					// GPIO10 <-- Antrieb M2 ab
// #define M3AUF 6					// GPIO12 <-- Antrieb M3 auf    <---Problem
#define M2AUF 5					// GPIO14 <-- Antrieb M3 ab
// #define M2AUF 7				// GPIO13						<---Problem
// #define ESP12E_GPIO15 16		// GPIO15 (LOW <-- Bootoption)
// #define ESP12E_GPIO16 4			// GPIO16 (Benutzt für Deepsleep - mus mit RST verbunden werden)
///////////////////////////////////////////////////////////

const char* ssid = "Jalousine";		// SSID vergeben
// const char* password = "12345678";  		// Passwort festlegen
const char* password ="";				// <--- kein Passwort nötig



unsigned long ulReqcount;


// Eine Instance vom Server auf Port 80
// und setzen der IP - Adress des Servers
WiFiServer server(80);
IPAddress apIP(192,168,255,1);				// 192.168.255.x to play in ROFA

void setup() 
{
	// Setup Globals
	ulReqcount=0; 
  
	// prepare GPIO
	// M1
	pinMode(M1AUF, OUTPUT);
	digitalWrite(M1AUF, 0);
  
    pinMode(M1AB, OUTPUT);
	digitalWrite(M1AB, 0);
	
	// M2
    pinMode(M2AUF, OUTPUT);
	digitalWrite(M2AUF, 0);

    pinMode(M2AB, OUTPUT);
	digitalWrite(M2AB, 0);

	// M3 <----------------- problem
    // pinMode(M3AUF, OUTPUT);
	// digitalWrite(M3AUF, 0);

    // pinMode(M3AB, OUTPUT);
	// digitalWrite(M3AB, 0);	
 
	// Start Serial
	Serial.begin(115200);
	delay(100);
  
	// AP Mode
	WiFi.mode(WIFI_AP);						// als Access Point
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
	WiFi.softAP(ssid, password);			// SSID & Passwort
	server.begin();							// starten

	delay(1000);
	Serial.println ("AP wird gestartet!!!");// nur zur Info --> Meldung
	Serial.println ("--------------------");
	delay(5000); // zum Test
}

void loop() 
{ 
	// Check: ist der Client "conected"
	WiFiClient client = server.available();
	if (!client) 							// wenn nicht zurück
	{
		Serial.println ("Warten auf Client");	// zum Test
		delay(1000);							// zum Test
		return;
	}
  
	// Warten bis der Client Daten sendetWait
	Serial.println("new client");
	unsigned long ultimeout = millis()+250;
	while(!client.available() && (millis()<ultimeout) ) // kein Client und 250 ms vergangen
	{
		delay(1);
	}
	if(millis()>ultimeout) 					// Timeout
	{ 
		Serial.println("client connection time-out!");
		return; 
	}
  
	// Lese die erste Zeile der Nachricht (Anfrage / Request)
	String sRequest = client.readStringUntil('\r');	// Return - bzw. Zeilenrücklauf
	Serial.println(sRequest);				// Zeile ausgeben
	client.flush();							// Wartet, bis alle ausgehenden Zeichen in Puffer gesendet wurden.
  
	// Stop Client, wenn request leer ist
	if(sRequest=="")
	{
		Serial.println("empty request! - stopping client");
		client.stop();
		return;
	}
  
	// Pfad holen (get path); Pfadende ist "space" oder ?
	// Syntax z.B.: GET /?pin=MOTOR1STOP HTTP/1.1
	String sPath="",sParam="", sCmd="";
	String sGetstart="GET ";
	int iStart,iEndSpace,iEndQuest;
	iStart = sRequest.indexOf(sGetstart);
	if (iStart>=0)
	{
		iStart+=+sGetstart.length();
		iEndSpace = sRequest.indexOf(" ",iStart);
		iEndQuest = sRequest.indexOf("?",iStart);
    
		// sind Parameter vorhanden?
		if(iEndSpace>0)
		{
			if(iEndQuest>0)
			{
				// there are parameters
				sPath  = sRequest.substring(iStart,iEndQuest);
				sParam = sRequest.substring(iEndQuest,iEndSpace);
			}
			else
			{
				// NO Parameters
				sPath  = sRequest.substring(iStart,iEndSpace);
			}
		}
	}
  
	/////////////////////////////////
	// Ausgabe der Parameter (Serial)	
	/////////////////////////////////
	
	// Bei dem Beispiel von oben: GET /?pin=MOTOR1STOP HTTP/1.1
	// sCmd == MOTOR1STOP
	if(sParam.length()>0)
	{
		int iEqu=sParam.indexOf("=");
		if(iEqu>=0)
		{
			sCmd = sParam.substring(iEqu+1,sParam.length());
			Serial.println(sCmd);
		}
	}
  
  
	////////////////////////////////////////////////////////////
	// Formatierung der HTML Antwort // format the html response
	////////////////////////////////////////////////////////////
	String sResponse,sHeader;
  
	////////////////////////////////////////////////////////////
	// 404 wenn nicht gefunden      // 404 for non-matching path
	////////////////////////////////////////////////////////////
	if(sPath!="/")
	{
		sResponse="<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>";
    
		sHeader  = "HTTP/1.1 404 Not found\r\n";
		sHeader += "Content-Length: ";
		sHeader += sResponse.length();
		sHeader += "\r\n";
		sHeader += "Content-Type: text/html\r\n";
		sHeader += "Connection: close\r\n";
		sHeader += "\r\n";
	}
	
	////////////////////////////////////////////////////////////
	// Formatiere die HTML Seite         // format the html page
	////////////////////////////////////////////////////////////
	else
	{
		ulReqcount++;
		sResponse = "<html><head><title>Jalousine Juergen</title></head><body>";
		sResponse += "<font color=\"#000000\"><body bgcolor=\"#d0d0f0\">";
		sResponse += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">";
		sResponse += "<h1>Jalousine</h1>";
		sResponse += "M1 = Antrieb zum -Schwenken-<BR>";
		sResponse += "M2 = Antrieb zum -Heben-<BR>";
		sResponse += "M3 = Antrieb zum -Drehen-<BR>";
		sResponse += "<FONT SIZE=+1>";
		sResponse += "<p>M1<a href=\"?pin=M1AUF\"><button>M1_AUF</button></a>&nbsp;<a href=\"?pin=M1AB\"><button>M1_AB</button></a></p>";
		sResponse += "<p>M2<a href=\"?pin=M2AUF\"><button>M2_AUF</button></a>&nbsp;<a href=\"?pin=M2AB\"><button>M2_AB</button></a></p>";
		sResponse += "<p>M3<a href=\"?pin=M3AUF\"><button>M3_AUF</button></a>&nbsp;<a href=\"?pin=M3AB\"><button>M3_AB</button></a></p>";
	
		////////////////////////////////////////////////////////
		// Reaktion auf die Parameter     // react on parameters
		////////////////////////////////////////////////////////
		if (sCmd.length()>0)
		{
			// schreibe das empfangene Kommando auf die HTML Seite // write received command to html page
			sResponse += "Kommando:" + sCmd + "<BR>";
      
			// switch GPIO // Ansteuerung der 3 Antriebe
			if(sCmd.indexOf("M1AUF")>=0)
			{
				digitalWrite(M1AUF, 1);
				Serial.println ("M1AUF");		//// zum Test
			}
			else if (sCmd.indexOf("M1AB")>=0)
			{
				digitalWrite(M1AB, 1);
				Serial.println ("M1AB");		//// zum Test
			}
			else if (sCmd.indexOf("M2AUF")>=0)
			{
				digitalWrite(M2AUF, 1),
				Serial.println ("M2AUF");		//// zum Test
			}
			else if (sCmd.indexOf("M2AB")>=0)
			{
				digitalWrite(M2AB, 1);
				Serial.println ("M2AB");		//// zum Test
			}
			// else if (sCmd.indexOf("M3AUF")>=0)
			// {
				// digitalWrite(M3AUF, 1),
				// Serial.println ("M3AUF");		//// zum Test
			// }
			// else if (sCmd.indexOf("M3AB")>=0)
			// {
				// digitalWrite(M3AB, 1);
				// Serial.println ("M3AB");		//// zum Test
			// }
			else if (sCmd.indexOf("M3AUF")>=0)  ////// zum test mit alle Antriebe aus belegt
			{
				digitalWrite(M1AUF, 0);
				digitalWrite(M1AB, 0);
				digitalWrite(M2AUF, 0);
				digitalWrite(M2AB, 0);
			}
		}
		else // alle Antriebe aus
		{
			// digitalWrite(M1AUF, 0);
			// digitalWrite(M1AB, 0);
			// digitalWrite(M2AUF, 0);
			// digitalWrite(M2AB, 0);
			// digitalWrite(M3AUF, 0);
			// digitalWrite(M3AB, 0);
			
			Serial.println ("Alle Antriebe aus");	//// zum Test
		}
	
    
		sResponse += "<FONT SIZE=-2>";
		sResponse += "<BR>Aufrufz&auml;hler="; 
		sResponse += ulReqcount;
		sResponse += "<BR>";
		sResponse += "Werner Seltmann 03/2016<BR>";
		sResponse += "</body></html>";
    
		sHeader  = "HTTP/1.1 200 OK\r\n";
		sHeader += "Content-Length: ";
		sHeader += sResponse.length();
		sHeader += "\r\n";
		sHeader += "Content-Type: text/html\r\n";
		sHeader += "Connection: close\r\n";
		sHeader += "\r\n";
	}
  
	// Sende die Antwort zun Client // Send the response to the client
	client.print(sHeader);
	client.print(sResponse);
  
	// und halte den Client an // and stop the client
	client.stop();
	Serial.println("Client disonnected");
}

