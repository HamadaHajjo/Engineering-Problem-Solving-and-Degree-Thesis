#ifndef CAMERA_PINS_H
#define CAMERA_PINS_H

// Custom ESP32-CAM pin definitions from your datasheet
#define PWDN_GPIO_NUM     -1  // Not connected
#define RESET_GPIO_NUM    -1  // Not connected
#define XCLK_GPIO_NUM     21
#define SIOD_GPIO_NUM     26  // I2C_SDA
#define SIOC_GPIO_NUM     27  // I2C_SCL
#define Y9_GPIO_NUM       35  // CSI_Y9
#define Y8_GPIO_NUM       34  // CSI_Y8
#define Y7_GPIO_NUM       39  // CSI_Y7
#define Y6_GPIO_NUM       36  // CSI_Y6
#define Y5_GPIO_NUM       19  // CSI_Y5
#define Y4_GPIO_NUM       18  // CSI_Y4
#define Y3_GPIO_NUM       5   // CSI_Y3
#define Y2_GPIO_NUM       4   // CSI_Y2 (Note different from standard ESP32-CAM)
#define VSYNC_GPIO_NUM    25  // CSI_VSYNC
#define HREF_GPIO_NUM     23  // CSI_HREF
#define PCLK_GPIO_NUM     22  // CSI_PCLK

#endif