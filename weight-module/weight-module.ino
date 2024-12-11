#include <WiFi.h>
#include <PubSubClient.h>
#include "HX711.h"

// Load cell configs
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

HX711 scale;
#define STABILIZATION_PERIOD 5  // Number of seconds over which stability should be checked
#define STABILITY_THRESHOLD 10  // Max allowable fluctuation (in units)
// Sliding window size - Usually the same as stabilization period if 1 sample per second
#define NUM_READINGS 5          // Number of recent readings to keep track of for stability check

long readings[NUM_READINGS];   // Array to store recent readings
int readIndex = 0;             // Index to track current position in the readings array
unsigned long lastMeasurementTime = 0; // Time of last measurement
unsigned long currentTime = 0; // Current time


char ssid[] = "Why-Fly";
char pass[] = "Password";
int status  = WL_IDLE_STATUS;    // the Wifi radio's status

WiFiSSLClient wifiClient;
PubSubClient client(wifiClient);

#define THING_NAME "espweight"

char mqttServer[]     = "a1il69uezpqouq-ats.iot.ap-southeast-1.amazonaws.com";
char clientId[]       = "espweight";
char publishTopic[]   = THING_NAME "/weight";
char publishPayload[MQTT_MAX_PACKET_SIZE];

char* rootCABuff = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\n" \
"yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n" \
"ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n" \
"U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n" \
"ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n" \
"aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\n" \
"MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n" \
"ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\n" \
"biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\n" \
"U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\n" \
"aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\n" \
"nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\n" \
"t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\n" \
"SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\n" \
"BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\n" \
"rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\n" \
"NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\n" \
"BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\n" \
"BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\n" \
"aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\n" \
"MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\n" \
"p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\n" \
"5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\n" \
"WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\n" \
"4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\n" \
"hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\n" \
"-----END CERTIFICATE-----\n";

/* Fill your certificate.pem.crt with LINE ENDING */
char* certificateBuff = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDWTCCAkGgAwIBAgIUE1UsPqN2mfvCGh2DLX2HWs3NOIYwDQYJKoZIhvcNAQEL\n" \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n" \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTE2MDYyNzA2MzQ0\n" \
"NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n" \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANM8/fzFJeZoS2yIf0Yy\n" \
"seFqUlnwNwNsg0G4U+3mUPj47ogs3jLDFA208P85S3qnfDaUY553wYY3BcIlBzpp\n" \
"y7vstrgapTOxH2c/Nrk/QfDqg/gUBSZs24c12WvyqlfDnKcIsRxALbfO0yMWKATB\n" \
"fFfnnZRCOVO6eKcW2O3ptfYDH8tIhrzCAEAAAF6CYmlSzyw9KrWh7ypYCp3AcJ/Y\n" \
"Fp4+SsF6xxLRuLOk37NJL8HfuIappkUIN7seGU9Y6Bo2YgOj9yBXyW/dmV3IxKVW\n" \
"SUSMSdIP+pc/b8lFfnE14yFtfK7jKhdy4XOh9LpOMJYs0i74UVQKJs7NYL8MXHIu\n" \
"FlUCAwEAAaNgMF4wHwYDVR0jBBgwFoAUoTuxNa4LQJd07hfP4se/TevgQ0AwHQYD\n" \
"VR0OBBYEFAQ9fAKopGZ52+f+w0dJMAwTi/hVMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n" \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQB6YQ+w6YTCZYWOlZfH09D8WhwV\n" \
"zSAzcEUdWH5T7bNK1N5r7/5zVIedseFxPUqtuC8j+C7CWDuLykpI8A2WAnayXOXn\n" \
"UPc/SNIet0nGts50cd4zzg7xMMqseGVdSMLAjFwqI8npJp9Ij8inrsf5f1hY0nhb\n" \
"Zb6FkVghlYhgmWv9p949kBQ6ODrtiyQqAYjzzZa957HwR7ajZfMIWm+HaV6f+NYq\n" \
"O43eQbt14xrRQNjAhSQaoVT64PH1TA6XiyoF25xlnrrxDXGAo5cxVDa1MmXu249X\n" \
"9z2+uzKgCYuugQk+w+8JmC6hR0EH4q4+ydsNkYTC0LK3MndHumA1Dj3OwUfA\n" \
"-----END CERTIFICATE-----\n";

/* Fill your private.pem.key with LINE ENDING */
char* privateKeyBuff = \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIIEpAIBAAKCAQEA0zz9/MUl5mhLbIh/RjKx4WpSWfA3A2yDQbhT7eZQ+PjuiCze\n" \
"MsMUDbTw/zlLeqd8NpRjnnfBhjcFwiUHOmnLu+y2uBqlM7EfZz82uT9B8OqD+BQF\n" \
"JmzbhzXZa/KqV8OcpwixHEAtt87TIxYoBMF8V+edlEI5U7p4pxbY7em19gMfy0iG\n" \
"vMIAQAAAXoJiaVLPLD0qtaHvKlgKncBwn9gWnj5KwXrHEtG4s6Tfs0kvwd+4hqmm\n" \
"RQg3ux4ZT1joGjZiA6P3IFfJb92ZXcjEpVZJRIxJ0g/6lz9vyUV+cTXjIW18ruMq\n" \
"F3Lhc6H0uk4wlizSLvhRVAomzs1gvwxcci4WVQIDAQABAoIBAQCObSVjdRokzFVu\n" \
"jGokTrIZJrL36Ttul4+4lCwiz5PxCwbp0jbPSzEOPN3xeBQoUx0xP8QbaOuXLyo2\n" \
"yPiirgqsXuKkJ3MT820VFE41gS1Y3wa0EeuXCPbLp8c9PZUVL9ND3Fxui+dHc7Bw\n" \
"i9PXwQ2xx308JZq5lZUcNA93+oixohfoN4kgPfT3je7yPvn0j1rMHfjQPrSvpIvk\n" \
"P2XA66BZf8YS9k6lpJLzslntusHO4ZoqdQxaqa3a79qawUjkJ4G4qx3aDvzqdjXU\n" \
"DTFgTLI8sicWl4Icx3vqCgZfGWYuzRX5Ka5eUZYkwv4PgUOSynVvc6n3fjX/2F1X\n" \
"aPEqLuIBAoGBAO+Akr7F5LnRyLxLEeywjruGxvG5vHS41YvRcHSukrNLoK4mRMRA\n" \
"fTqJe8oZCRuOGsDBkRujgnSEUJhqt2+kGf9dvv5pCS35PFvTzawbpe8HX/JjitDb\n" \
"kKxAd3edsbOo3ZCOwYLFwMPMIMzemqFX67cXyQwOUhcGf6N7YBKhA6JhAoGBAOHK\n" \
"AarQJXo3C69SpWMvei3zmV8pUdVLPiiiNb1q/pustt+hykRhXNrngV3ncB7mJBET\n" \
"Q+36c5F04A6EBSp5Nr8EdiPnsFwk+ILKx7wQ6y0E29xIRLxryyu27rMAGpk8D1Oy\n" \
"8F2mX23qGWQAsMi2sxKk7o+2EGoUF1f/HKeDcOB1AoGBAM5johG8P2rSGYYJuxyY\n" \
"2adIcdCFGp4LWhrvFVW3yruvhHwOhlwIpuH28DIseOjCANPy+rUypoz6KOnvrLwM\n" \
"Ukr54kkjAsIXcahAUZDrEod1d31NwqZRT87gjxMJVcVY0/Zqzt9+wqr4EZv6iI5Z\n" \
"UcuqN5qoDJ3C/+NFwnjLQHKBAoGAQI5zX5VXwdPPQXeN1ggTFORba7vyq9txkEig\n" \
"uOHInlYJi3NE07xKwkQC1wh/JDaFBWTOvVIojOQv07ani3dQ0djCto1d/VqMu0ij\n" \
"RwBHXX3QJvF6xazEUGFjakaTVFC5ySKWWxBgpJqUW+VepmSmWqRRmUFi/BF2gzBr\n" \
"zvFj6qkCgYANcCc6wXqWNwmAnH/bfTRXunUgt3AAxmMXtkakhXAjElgFymgmXhTN\n" \
"Huu00WjTeZqOSl8bE0Ki/bcRojlx2QMMaEHGSVRL28uPJCJ4HeBAH3m22dtceFgF\n" \
"+ZHhF5b6NFe6EeLBcd/TGLtuKvc/jeru8tciNrPj9MTSJR9T0TYxhw==\n" \
"-----END RSA PRIVATE KEY-----\n";



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  // Set up HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Set calibration factor
  scale.set_scale(-370.41);    

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    if (status == WL_CONNECTED) break;
    // retry after 1 second
    delay(1000);
  }

  wifiClient.setRootCA((unsigned char*)rootCABuff);
  wifiClient.setClientCertificate((unsigned char*)certificateBuff, (unsigned char*)privateKeyBuff);

  client.setServer(mqttServer, 8883);
  client.setCallback(callback);

  // Allow the hardware to sort itself out
  delay(1500);
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }

  // Remember to calibrate before hand
  if (scale.is_ready()) {
    currentTime = millis(); // Get the current time

    // Get a new reading from the scale
    long currentReading = scale.get_units(10);

    // Store the current reading in the array (circular buffer logic)
    readings[readIndex] = currentReading;
    readIndex = (readIndex + 1) % NUM_READINGS;  // Update the index, wrap around if necessary

    // Check if enough time has passed (5 seconds)
    if (currentTime - lastMeasurementTime >= 1000) {  // Every second, check stability
      lastMeasurementTime = currentTime;

      // Check if all the readings in the array are stable
      bool isStable = true;
      long firstReading = readings[readIndex];  // The first reading to compare against

      for (int i = 0; i < NUM_READINGS; i++) {
        // Check the difference between the current reading and the first reading
        if (abs(readings[i] - firstReading) > STABILITY_THRESHOLD) {
          isStable = false;
          break;
        }
      }

      // If stable, publish data
      if (isStable) {
        // Prepare payload and publish
        client.publish(publishTopic, publishPayload);
        printf("Publish [%s] %s\r\n", publishTopic, publishPayload);

        // Tare after sending the stable data
        scale.tare();
      } else {
        Serial.println("Reading unstable, not publishing.");
      }
    }
  } else {
    Serial.println("HX711 not found.");
  }

  client.loop(); // Keep the client alive
}