#include <Arduino.h>
#include "driver/twai.h"

#define CAN_TX_PIN GPIO_NUM_4
#define CAN_RX_PIN GPIO_NUM_5

unsigned long last_telemetry_time = 0;
unsigned long last_wake_time = 0;
uint8_t wake_counter = 15;
uint8_t telemetry_counter = 15;

unsigned char P_L_CC_CRC_LUT_APV[256] = { 
    0, 47, 94, 113, 188, 147, 226, 205, 87, 120, 9, 38, 235, 196, 181, 154, 174, 129, 240, 223, 18, 61, 76, 99, 249, 214, 167, 136, 69, 106,
    27, 52, 115, 92, 45, 2, 207, 224, 145, 190, 36, 11, 122, 85, 152, 183, 198, 233, 221, 242, 131, 172, 97, 78, 63, 16, 138, 165, 212, 251, 54, 25, 104, 71, 230, 201, 184, 151, 90, 117,
    4, 43, 177, 158, 239, 192, 13, 34, 83, 124, 72, 103, 22, 57, 244, 219, 170, 133, 31, 48, 65, 110, 163, 140, 253, 210, 149, 186, 203, 228, 41, 6, 119, 88, 194, 237, 156, 179, 126, 81,
    32, 15, 59, 20, 101, 74, 135, 168, 217, 246, 108, 67, 50, 29, 208, 255, 142, 161, 227, 204, 189, 146, 95, 112, 1, 46, 180, 155, 234, 197, 8, 39, 86, 121, 77, 98, 19, 60, 241, 222,
    175, 128, 26, 53, 68, 107, 166, 137, 248, 215, 144, 191, 206, 225, 44, 3, 114, 93, 199, 232, 153, 182, 123, 84, 37, 10, 62, 17, 96, 79, 130, 173, 220, 243, 105, 70, 55, 24, 213, 250,
    139, 164, 5, 42, 91, 116, 185, 150, 231, 200, 82, 125, 12, 35, 238, 193, 176, 159, 171, 132, 245, 218, 23, 56, 73, 102, 252, 211, 162, 141, 64, 111, 30, 49, 118, 89, 40, 7, 202, 229,
    148, 187, 33, 14, 127, 80, 157, 178, 195, 236, 216, 247, 134, 169, 100, 75, 58, 21, 143, 160, 209, 254, 51, 28, 109, 66 
};

unsigned char P_L_CC_KENNUNG_APV_ESP21[16] = { 0xb4, 0xef, 0xf8, 0x49, 0x1e, 0xe5, 0xc2, 0xc0, 0x97, 0x19, 0x3c, 0xc9, 0xf1, 0x98, 0xd6, 0x61 };

void init_twai() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_driver_uninstall();
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();
}

void send_frame(uint32_t id, uint8_t len, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7) {
    twai_message_t msg;
    msg.identifier = id;
    msg.extd = 0; msg.rtr = 0; msg.ss = 0;
    msg.data_length_code = len;
    msg.data[0]=b0; msg.data[1]=b1; msg.data[2]=b2; msg.data[3]=b3;
    msg.data[4]=b4; msg.data[5]=b5; msg.data[6]=b6; msg.data[7]=b7;
    twai_transmit(&msg, pdMS_TO_TICKS(5));
}

uint8_t calculate_advanced_crc(uint8_t data[], uint8_t len, uint8_t counter, unsigned char* kennung_array) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 1; i < len; i++) {
        crc = P_L_CC_CRC_LUT_APV[crc ^ data[i]];
    }
    crc = P_L_CC_CRC_LUT_APV[crc ^ kennung_array[counter]];
    return crc ^ 0xFF;
}

void handle_telemetry() {
    unsigned long now = millis();
    if (now - last_telemetry_time < 20) return;
    last_telemetry_time = now;

    float t = now / 1000.000f;
    float current_rpm = 3000.000f + 2500.000f * sin(t * 2.000f);
    float current_mph = 50.000f + 40.000f * cos(t * 1.500f);

    uint16_t rpm_raw = (uint16_t)(current_rpm / 3.000f);
    send_frame(0x107, 8, 0x00, 0x00, 0x00, (uint8_t)(rpm_raw & 0xFF), (uint8_t)((rpm_raw >> 8) & 0xFF), 0x00, 0x00, telemetry_counter);
    send_frame(0x10A, 8, 0x00, telemetry_counter, 0x07, 0x15, 0x11, 0x22, 0x33, 0x44);

    float kmh = current_mph * 1.609f;
    uint16_t raw_speed = (uint16_t)(kmh / 0.010f);
    
    uint8_t data_esp[8];
    data_esp[1] = telemetry_counter & 0x0F;
    data_esp[2] = 0x00; 
    data_esp[3] = 0x00;
    data_esp[4] = raw_speed & 0xFF;
    data_esp[5] = (raw_speed >> 8) & 0xFF;
    data_esp[6] = 0x00;
    data_esp[7] = 0x00;
    data_esp[0] = calculate_advanced_crc(data_esp, 8, telemetry_counter, P_L_CC_KENNUNG_APV_ESP21); 

    send_frame(0x1A0, 8, data_esp[0], data_esp[1], data_esp[2], data_esp[3], data_esp[4], data_esp[5], data_esp[6], data_esp[7]);

    telemetry_counter = (telemetry_counter == 0) ? 15 : telemetry_counter - 1;
}

void handle_wakeup() {
    unsigned long now = millis();
    if (now - last_wake_time < 100) return;
    last_wake_time = now;
    send_frame(0x3C0, 4, (uint8_t)(0x3C ^ wake_counter ^ 0x03), wake_counter, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00);
    send_frame(0x5C0, 4, (uint8_t)(0x5C ^ wake_counter ^ 0x03), wake_counter, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00);
    wake_counter = (wake_counter == 0) ? 15 : wake_counter - 1;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    init_twai();
}

void loop() {
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK && status.state == TWAI_STATE_BUS_OFF) {
        init_twai(); return;
    }
    handle_telemetry();
    handle_wakeup();
}