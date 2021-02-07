//
// Created by development on 22.01.21.
//

#ifndef EDGE_ESP32_INFERENCE_H
#define EDGE_ESP32_INFERENCE_H


#include <Arduino.h>
#include "support.h"


/** Audio buffers, pointers and selectors */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

extern  inference_t inference;
extern bool record_ready ;
extern signed short *sampleBuffer;
extern bool debug_nn ; // Set this to true to see e.g. features generated from the raw signal
extern int print_results ;
extern QueueHandle_t m_i2sQueue;

 void CaptureSamples(void *arg);
void i2s_init(void);
bool microphone_inference_start(uint32_t n_samples);
 bool microphone_inference_record(void);
 int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
 void microphone_inference_end(void);
#endif //EDGE_ESP32_INFERENCE_H
