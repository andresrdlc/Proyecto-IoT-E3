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

void reconectar() {
  while (!cliente.connected()) {
    Serial.print("Intentando conectar a MQTT...");
    String idCliente = "ESP8266Client-";
    idCliente += String(random(0xffff), HEX);
    if (cliente.connect(idCliente.c_str())) {
      Serial.println("Conectado");
      cliente.subscribe("esp32/topico");
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(cliente.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_SENSOR_LLUVIA, INPUT);
  Serial.begin(115200);
  configurar_wifi();
  cliente.setServer(servidor_mqtt, 1883);
  cliente.setCallback(callback);
  dht.begin();
}

void loop() {
  if (!cliente.connected()) {
    reconectar();
  }
  cliente.loop();

  unsigned long ahora = millis();
  if (ahora - ultimoMensaje >30000) {
    ultimoMensaje = ahora;

    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();
    int valorHumedadSuelo = analogRead(PIN_HUMEDAD_SUELO);
    int porcentajeHumedadSuelo = map(valorHumedadSuelo, 0, 4095, 120, 0);
    int valorSensorLluvia = !digitalRead(PIN_SENSOR_LLUVIA);
    

    // Condiciones para el estado del motor
    /*if (temperatura > 30 || humedad < 30 || porcentajeHumedadSuelo < 20 || valorSensorLluvia == 1) {
      estadoMotor = HIGH; // Enciende el motor
    } else {
      estadoMotor = LOW; // Apaga el motor
    }*/

    if(porcentajeHumedadSuelo <= 60){
      estadoMotor = HIGH;
    }

    if(porcentajeHumedadSuelo >= 80){
      estadoMotor = LOW;
    }

    if (temperatura > 30){
      sendMessage("Exceso de Temperatura ");
      delay(5000);
    }

    if (temperatura < 10){
      sendMessage("Temperatura Muy Baja ");
      delay(5000);
    }

    digitalWrite(PIN_MOTOR, estadoMotor);

    snprintf(mensaje, TAMANO_BUFFER_MENSAJE, "%.2f,%.2f,%d,%d,%d", 
             temperatura, humedad, porcentajeHumedadSuelo, valorSensorLluvia, estadoMotor);
    Serial.print("Publicar mensaje: ");
    Serial.println(mensaje);
    cliente.publish(topico_salida, mensaje);

     if(valorSensorLluvia == 1){
      sendMessage("LLUVIA, RIESGO DE HUMEDAD");
      delay(5000);
    }
  }
}
