#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
String phoneNumber = "+51973436008";
String apiKey = "4174768";

#define LED_BUILTIN 2
#define PIN_DHT 14
#define TIPO_DHT DHT11
#define PIN_HUMEDAD_SUELO 32
#define PIN_SENSOR_LLUVIA 34
#define PIN_MOTOR 17

const char* ssid = "Wifi_iot2024";
const char* password = "00000000IOT";
const char* servidor_mqtt = "broker.emqx.io";
int estadoMotor = LOW;
WiFiClient espCliente;
PubSubClient cliente(espCliente);
unsigned long ultimoMensaje = 0;
#define TAMANO_BUFFER_MENSAJE (100)
char mensaje[TAMANO_BUFFER_MENSAJE];
const char* topico_salida = "esp32/topico2024"; 

DHT dht(PIN_DHT, TIPO_DHT);
void sendMessage(String message){

  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);    
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Message sent successfully");
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}

void configurar_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW); // Apaga el LED mientras se conecta
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, HIGH); // Enciende el LED cuando está conectado
}

void callback(char* topico, byte* carga, unsigned int longitud) {
  Serial.print("Mensaje recibido [");
  Serial.print(topico);
  Serial.print("] ");
  for (int i = 0; i < longitud; i++) {
    Serial.print((char)carga[i]);
  }
  Serial.println();

  // Enciende el LED si se recibe '1' como primer carácter
  if ((char)carga[0] == '1') {
    digitalWrite(LED_BUILTIN, LOW);   
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  
  }
}

