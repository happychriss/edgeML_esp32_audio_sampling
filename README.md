## Continuous speech recognition with ESP32 for numbers (0--9) using a 1D CNN / MFCC 

With this project I want to try out if its possible to detect the spoken numbers "zero" to "nine" by using an ESP32 processor and MEMS microphone and ML 1D CNN network 
While there are a lot examples to distinguish 3 or 4 different keywords, I have not seen a working example to distinguish 10 different labels.
Understanding, that this could be to complex for such a small device, I will try to train it at least listening to one specific voice.

##### Hardware and Framework
* Lolin D32 Pro (ESP32) and an INMP441 I2S MEMS Microphone for sample generation and inference.
* CLION and Platformio as development platform
* EdgeImpulse as framework for data acquisition, feature generation (MFCC), DSP and model build.
* Python Scripts to generate single, labeled test sample from continuous recording and uploading data to EdgeImpulse (https://github.com/happychriss/edgeML_esp32_training)

####Sampling
EdgeImpulse provides direct integration (data forwarding) from a connected device (e.g. ESP32) into the cloud for data collection, but with small devices the amount of sampling data per run is limited.
To streamline this process I am using WIFI connection to download the samples in realtime from ESP32 and to the split and preparation locally via Python.
Ready made testdata is uploaded to EdgeImpulse.

####Inference
After model-build on EdgeImpulse I download the new model via script download_model.sh.
The inference is running continuously on the ESP32.


### Status
* Full flow with platformio and clion supported, compiling edgeimpulse sdk and dsp, inference
* integrated INMP441 in framework
* allow (via define) to switch between voice acquisition (for sampling) and inference (voice recognition) using the same functions
* current final performance is 60%, not enough :-(

Details can be found at: https://44-2.de