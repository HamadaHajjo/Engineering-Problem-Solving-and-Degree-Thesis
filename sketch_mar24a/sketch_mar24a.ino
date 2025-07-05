#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "camera_pins.h"
#include "emnist_model.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

// WiFi config
const char* ssid = "Tele2_83430b";
const char* password = "ijnwadyu";
WebServer server(80);

// TFLite config
constexpr int kTensorArenaSize = 50 * 1024;
static uint8_t tensor_arena[kTensorArenaSize];
static tflite::MicroInterpreter* interpreter = nullptr;
static tflite::ErrorReporter* error_reporter = nullptr;
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;

String lastPrediction = "?";
bool debugMode = true;
bool cameraInitialized = false;

char class_to_char(int class_id) {
  return 'A' + class_id; // Handles A-Z (0-25)
}

void setup_camera() {
  Serial.println("Initializing camera...");
  
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Try lower resolution first for stability
  config.frame_size = FRAMESIZE_QVGA; // 320x240
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    delay(1000);
    ESP.restart();
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  
  // Print camera info for debugging
Serial.printf("Camera PID: 0x%02X, VER: 0x%02X, MID: 0x%02X%02X\n", 
  s->id.PID, s->id.VER, s->id.MIDH, s->id.MIDL);


  // Simple settings for reliability
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_quality(s, 10);
  s->set_brightness(s, 1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 0);
  s->set_awb_gain(s, 1);
  s->set_whitebal(s, 1);
  
  cameraInitialized = true;
  Serial.println("Camera initialized successfully");
}

String analyze_image() {
  if (!cameraInitialized) {
    Serial.println("Camera not initialized!");
    return "Error";
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb || fb->format != PIXFORMAT_JPEG) {
    Serial.println("Camera capture failed or wrong format");
    if (fb) {
      esp_camera_fb_return(fb);
    }
    return "Error";
  }

  // Debug: Save frame to serial (first 20 bytes)
  if (debugMode) {
    Serial.println("JPEG frame captured:");
  for (int i = 0; i < min(20, static_cast<int>(fb->len)); i++) { // Fix min() error
      Serial.printf("%02X ", fb->buf[i]);
    }
    Serial.println();
  }

  // In a real application, you would process the JPEG here
  // For now, we'll just return a dummy value
  esp_camera_fb_return(fb);
  
  // Simulate prediction
  char randomLetter = 'A' + random(0, 26);
  lastPrediction = String(randomLetter);
  return lastPrediction;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n=== ESP32 Letter Detector ===");

  // Initialize camera
  setup_camera();

  // Initialize TensorFlow Lite
  Serial.println("Initializing TensorFlow Lite...");
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  
  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
    tflite::GetModel(emnist_model),
    resolver,
    tensor_arena,
    kTensorArenaSize,
    error_reporter
  );
  
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("Tensor allocation failed!");
    while(1);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);
  
  Serial.println("Model loaded successfully");
  Serial.printf("Input dimensions: %dx%d\n", input->dims->data[1], input->dims->data[2]);

  // Connect to WiFi
  Serial.printf("Connecting to %s", ssid);
  WiFi.begin(ssid, password);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi!");
    ESP.restart();
  }
  
  Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());

  // Setup web server
  server.on("/", HTTP_GET, [](){
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>ESP32 Letter Detector</title>";
    html += "<style>";
    html += "body { font-family: Arial; text-align: center; margin: 20px; }";
    html += "#stream { width: 320px; height: 240px; object-fit: cover; border: 2px solid #333; }";
    html += "#prediction { font-size: 72px; color: #FF5722; margin: 20px; }";
    html += ".container { max-width: 350px; margin: 0 auto; }";
    html += "</style></head>";
    html += "<body><div class='container'>";
    html += "<h1>Letter Detector</h1>";
    html += "<img id='stream' src='/stream'>";
    html += "<div id='prediction'>" + lastPrediction + "</div>";
    html += "<script>";
    html += "const predictionEl = document.getElementById('prediction');";
    html += "setInterval(async () => {";
    html += "  try {";
    html += "    const res = await fetch('/predict');";
    html += "    if (res.ok) {";
    html += "      const text = await res.text();";
    html += "      predictionEl.textContent = text;";
    html += "    }";
    html += "  } catch(e) { console.error('Error:', e); }";
    html += "}, 1000);";
    html += "</script>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
  });

  server.on("/stream", HTTP_GET, [](){
    WiFiClient client = server.client();
    
    if (!cameraInitialized) {
      client.println("HTTP/1.1 500 Internal Server Error");
      client.println("Content-Type: text/plain");
      client.println();
      client.println("Camera not initialized");
      return;
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Connection: close");
    client.println();
    
    while (client.connected()) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Camera capture failed");
        delay(100);
        continue;
      }

      client.println("--frame");
      client.println("Content-Type: image/jpeg");
      client.println("Content-Length: " + String(fb->len));
      client.println();
      client.write(fb->buf, fb->len);
      esp_camera_fb_return(fb);
      
      delay(33); // ~30fps
    }
  });

  server.on("/predict", HTTP_GET, [](){
    String prediction = analyze_image();
    server.send(200, "text/plain", prediction);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  delay(1);
}