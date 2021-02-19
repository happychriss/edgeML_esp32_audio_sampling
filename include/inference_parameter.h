//
// Created by development on 22.01.21.
//

#ifndef EDGE_ESP32_INFERENCE_PARAMETER_H
#define EDGE_ESP32_INFERENCE_PARAMETER_H
// If your target is limited in memory remove this macro to save 10K RAM
#undef EIDSP_QUANTIZE_FILTERBANK
#define EIDSP_QUANTIZE_FILTERBANK   0

/**
 * Define the number of slices per model window. E.g. a model window of 1000 ms
 * with slices per model window set to 4. Results in a slice size of 250 ms.
 * For more info: https://docs.edgeimpulse.com/docs/continuous-audio-sampling
 */
#undef EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 5
#endif //EDGE_ESP32_INFERENCE_PARAMETER_H
