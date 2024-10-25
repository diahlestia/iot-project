#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h>

#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22
#define LED_Flash 4  // GPIO4 untuk flash

const char* ssid = "Incidal";    // Your Wifi SSID
const char* password = "casio1314";   // Wifi Password

String dataIn;
String server_addr = "192.168.1.5";  // your server address or computer IP
String url = "http://" + server_addr + "/absensi/webapi/api/upload.php?";

void setup() {
    Serial.begin(9600);
    pinMode(LED_Flash, OUTPUT); 
    WiFi.begin(ssid, password);

    // Camera configuration
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // Frame parameters
    if (psramFound()) {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // Camera initialization
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        delay(1000);
        return;
    }

    Serial.println("ESP32-CAM Ready");
}

void loop() {
    // Cek apakah ada trigger dari ESP8266
    if (Serial.available()) {
        dataIn = Serial.readStringUntil('#');  // Baca data sampai newline
        Serial.println("Trigger diterima. Mengambil gambar...");
        digitalWrite(LED_Flash, HIGH);
        delay(2000);

        // Ambil gambar
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Gagal mengambil gambar");
            return;
        }

        digitalWrite(LED_Flash, LOW);

        // Encode gambar ke Base64
        String imageFile = "data:image/jpeg;base64,";
        String encodedImage = base64::encode((uint8_t*)fb->buf, fb->len);  // Encode buffer gambar langsung ke Base64
        
        imageFile += encodedImage;  // Gabungkan hasil encode ke dalam variabel imageFile
        
        esp_camera_fb_return(fb);  // Kembalikan frame buffer ke sistem
        Serial.println("Gambar diambil dan di-encode");

        // Kirim data melalui HTTP
        String url = "http://" + server_addr + "/absensi/webapi/api/upload.php";  // Base URL

        HTTPClient http;
        http.begin(url);  // Begin HTTP connection

        // Add header for URL-encoded form data
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Prepare the POST data
        String postData = "id=" + dataIn + "&foto=" + urlencode(imageFile);  // Kombinasi parameter id dan foto

        // Send the POST request
        int httpResponseCode = http.POST(postData);

        Serial.println(dataIn);  // Print the ID for debugging
        if (httpResponseCode > 0) {
            // Jika berhasil
            Serial.println(httpResponseCode);  // Cetak kode respons
            Serial.println(http.getString());  // Cetak respons dari server
        } else {
            // Jika terjadi error
            Serial.print("Error on sending request: ");
            Serial.println(httpResponseCode);
        }
        http.end();  // Akhiri koneksi HTTP
        dataIn = "";
    }
}

// Fungsi untuk urlencode jika diperlukan
String urlencode(String str) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == ' ') {
            encodedString += '+';
        } else if (isalnum(c)) {
            encodedString += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) {
                code0 = c - 10 + 'A';
            }
            code2 = '\0';
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }
    }
    return encodedString;
}
