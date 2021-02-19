/* Edge Impulse Arduino examples
 * Copyright (c) 2020 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */



/* Includes ---------------------------------------------------------------- */
#include <Arduino.h>
#include "esp32_numbers_v3_v3_inference.h"
#include "inference.h"
#include "inference_parameter.h"
#include "driver/i2s.h"
#include "support.h"
#include <WiFi.h>
#include "WiFiCredentials.h"
#include "data_acquisition.h"

// Inference Global Data
inference_t inference;
bool record_ready = false;
signed short *sampleBuffer;
bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
QueueHandle_t m_i2sQueue;

// Wifi and Connectivity
WiFiClient *wifiClientI2S = nullptr;
HTTPClient *httpClientI2S = nullptr;

float values[EI_CLASSIFIER_LABEL_COUNT]={0.0};

#define STATE_NOTHING 0
#define STATE_FOUND 1
#define STATE_FOUND_MISSED_ONE 2
#define STATE_FOUND_MISSED_TWO 3
#define STATE_RESOLVED 4
short   result_state=STATE_NOTHING;


#undef DATA_ACQUISITION

/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */
/*


static void pdm_data_ready_inference_callback(void)
{
    int bytesAvailable = PDM.available();

    // read into the sample buffer
    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if (record_ready == true) {
        for (int i = 0; i<bytesRead>> 1; i++) {
            inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

            if (inference.buf_count >= inference.n_samples) {
                inference.buf_select ^= 1;
                inference.buf_count = 0;
                inference.buf_ready = 1;
            }
        }
    }
}
*/



/**
 * Get raw audio signal data
 */


int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

    return 0;
}


// https://github.com/leonyuhanov/ESP32_MEMSMicrophone/blob/master/ESP32_MEMSMic.ino
void CaptureSamples(void *arg) {

    i2s_init();
    DP("CaptureSamples - Running on Core:");
    DPL(xPortGetCoreID());

    while (true) {
        // wait for some data to arrive on the queue
        i2s_event_t evt;

        //this queue gives the other tasks time to run, RTOS is using simple scheduling
        if (xQueueReceive(m_i2sQueue, &evt, portMAX_DELAY) == pdPASS) {

            if (evt.type == I2S_EVENT_RX_DONE) {
                int bytesRead;
                esp_err_t res_i2s = i2s_read((i2s_port_t) 1, (uint8_t *) &sampleBuffer[0], (EI_CLASSIFIER_SLICE_SIZE >> 1) * sizeof(signed short), (size_t *) &bytesRead, 50);
                if (res_i2s != ESP_OK) {
                    ESP_LOGE(TAG, "Error in I2S read - code: %d", res_i2s);
                }

                if (record_ready == true) {

                    // todo: multiply with 16 - to fix a bug during recording
                    auto *i2s_read_16 = (int16_t *) sampleBuffer;
                    int16_t noise = 0;
                    for (int i = 0; i < bytesRead / 2; i++) {
                        // TODO: configure this value on a central place
                        i2s_read_16[i] = (int16_t) i2s_read_16[i] * 16;
                        noise = noise + abs(i2s_read_16[i]);
                    }
                    noise = noise / (bytesRead / 2);
                    if (noise > 80) {
                        DP("*** NOISE:");
                        DPL(noise);
                    }

                    for (int i = 0; i < bytesRead >> 1; i++) {
                        inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

                        if (inference.buf_count >= inference.n_samples) {
                            inference.buf_select ^= 1;
                            inference.buf_count = 0;
                            inference.buf_ready = 1;
                        }
                    }
                }

            }
        }


    }
}

/**
 * @brief      Arduino setup function
 */
void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);

    DP("Edge Impulse Inferencing Demo - Version: ");
    DPL(EI_CLASSIFIER_PROJECT_DEPLOY_VERSION);
    pinMode(LED_BUILTIN, OUTPUT); //signal for model

    pinMode(19, INPUT);
    pinMode(21, OUTPUT);     //Set up pin 21 and 25 as the BCK and LRCK pins
    pinMode(25, OUTPUT);

    DP("Main Loop - Running on Core:");
    DPL(xPortGetCoreID());

#ifdef DATA_ACQUISITION
    // launch WiFi
    DPL("Starting Data Acquisition Mode");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    WiFi.setSleep(false);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    DPL("Connected to WiFi");
    // indicator LED
    pinMode(LED_BUILTIN, OUTPUT);
    // setup the HTTP Client
    wifiClientI2S = new WiFiClient();
    httpClientI2S = new HTTPClient();


#else
    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float) EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSlices per model window %d\n", EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    DPL("Starting Inversion Mode");
    run_classifier_init();
#endif

    // setup buffers and start CaptureSamples, ends in inference.buffers
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: Failed to setup audio sampling\r\n");
        return;
    }
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */




void loop() {


    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }


#ifdef DATA_ACQUISITION
    sendData(wifiClientI2S, httpClientI2S, I2S_SERVER_URL, (uint8_t *) &inference.buffers[inference.buf_select ^ 1][0], EI_CLASSIFIER_SLICE_SIZE*2);
#else
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;

    ei_impulse_result_t result = {0};

    digitalWrite(LED_BUILTIN, HIGH);
    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }
    digitalWrite(LED_BUILTIN, LOW);
    if (++print_results >= (0)) {
//print the predictions
//       ei_printf("Predictions "); ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)", result.timing.dsp, result.timing.classification, result.timing.anomaly);
//       ei_printf(": \n");
        bool b_found_result = false;

        int max_value_idx = -1;
        float max_value = 0;

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            if ((result.classification[ix].value > 0.8) && (result.classification[ix].label[0] != 'N')) {
//                ei_printf("\nResults:\n");

                b_found_result = true;
                if (result.classification[ix].value > max_value) {
                    max_value = result.classification[ix].value;
                    max_value_idx = ix;
                }
            }
        }

        if (b_found_result) {
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
//              ei_printf("    %s: %.5f", result.classification[ix].label, result.classification[ix].value);
//                if (max_value_idx == ix) ei_printf("<-----\n");  else ei_printf("\n");
            }

        } else { ei_printf("."); }

        if (b_found_result) {
            result_state = STATE_FOUND;
            values[max_value_idx]=values[max_value_idx]+max_value;
        } else {
            switch (result_state) {
                case STATE_NOTHING: result_state = STATE_NOTHING;break;
                case  STATE_FOUND: result_state = STATE_FOUND_MISSED_ONE;break;
                case STATE_FOUND_MISSED_ONE:  result_state = STATE_FOUND_MISSED_TWO;break;
                case STATE_FOUND_MISSED_TWO:  result_state = STATE_RESOLVED;break;
                default:DPL("aaaaaaaaaaaaaaaaargh");DPL(result_state);
            }
        }

        if (result_state == STATE_RESOLVED) {

            int final_value_idx = -1;
            float final_value = 0.0;

            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {

                if (values[ix] > final_value) {
                    final_value=values[ix];
                    final_value_idx=ix;
                }
            }
            DP("\n------------> ");
            DP(ei_classifier_inferencing_categories[final_value_idx]);DP(" - "); DPL(final_value);

//            ei_printf("***********************    %s: %.5f", ei_classifier_inferencing_categories[final_value_idx], my_final_value);
            memset(values, 0, sizeof(values));
            result_state = STATE_NOTHING;
        }



#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif


        print_results = 0;

    }

#endif //DATA_ACQUISITION
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif // Invalid model for current sensor
