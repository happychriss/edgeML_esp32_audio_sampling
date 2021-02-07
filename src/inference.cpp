//
// Created by development on 22.01.21.
//

// clang-format on

#include "inference.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/task.h"


/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */


bool microphone_inference_start(uint32_t n_samples) {
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        free(inference.buffers[0]);
        return false;
    }

    // Sample buffer has half of the size of inference buffer (cn)
    sampleBuffer = (signed short *)malloc((n_samples >> 1) * sizeof(signed short));

    if (sampleBuffer == NULL) {
        free(inference.buffers[0]);
        free(inference.buffers[1]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    record_ready = true;

    xTaskCreatePinnedToCore(CaptureSamples, "CaptureSamples", 1024 * 32, NULL, 1, NULL, 0);
    DPL("microphone_inference_start: DONE");
    return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */


bool microphone_inference_record(void) {
    bool ret = true;

    if (inference.buf_ready == 1) {
        DP("Error sample buffer overrun. Decrease the number of slices per model window, (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
        ret = false;
    }

    while (inference.buf_ready == 0) {
        delay(1);
    }

    inference.buf_ready = 0;

    return ret;
}

/**
 * @brief      Stop PDM and release buffers
 */


void microphone_inference_end(void) {
//    PDM.end();
    free(inference.buffers[0]);
    free(inference.buffers[1]);
    free(sampleBuffer);
}



void i2s_init(void) {
    // Start listening for audio: MONO @ 16KHz

    i2s_pin_config_t pin_config = {
            .bck_io_num = 21, //this is BCK pin
            .ws_io_num = 25, // this is LRCK pin
            .data_out_num = I2S_PIN_NO_CHANGE, // this is DATA output pin
            .data_in_num = 19   //DATA IN
    };

    // https://blog.cmgresearch.com/2020/09/12/esp32-audio-input.html
    i2s_config_t i2s_config = {
            .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = 16000,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // was I2S_CHANNEL_FMT_RIGHT_LEFT
            .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 4,
            .dma_buf_len = 1024,
            .use_apll = false,
            .tx_desc_auto_clear = false,
            .fixed_mclk = 0
    };


    esp_err_t ret = 0;
    ret = i2s_driver_install((i2s_port_t) 1, &i2s_config, sizeof(i2s_event_t), &m_i2sQueue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in i2s_driver_install");
    }
    ret = i2s_set_pin((i2s_port_t) 1, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in i2s_set_pin");
    }

    ret = i2s_zero_dma_buffer((i2s_port_t) 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in initializing dma buffer with 0");
    }

}

/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */


