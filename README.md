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

#### ModelSettings

#####Impulse:
In German most numbers are very short, less than 500ms. Only one number ("sieben" = "seven" is longer, I only recorded samples with the 2nd half of the number to reduce length - that worked)
* Recorded samples Length: 700ms
* Impulse With: 600ms 
* Window increase: 100ms (this results in 2 features per sample)

#####Prepropessing MFCC: 
* Coefficients:17 
* Frame Length: 0.014
* Frame Stride: 0.014
* Filter: 32
* FFT Length: 256
* Window Size: 261
* Frequency: 300-8000 (0)
* Preemphasis: 0.98, with 1 Shift

##### Classifier:1D Convolutional
``` python
model.add(Reshape((int(input_length / 17), 17), input_shape=(input_length, )))
model.add(Conv1D(18, kernel_size=3, activation='relu', padding='same'))
model.add(MaxPooling1D(pool_size=2, strides=2, padding='same'))
model.add(Dropout(0.5))
model.add(Conv1D(34, kernel_size=3, activation='relu', padding='same'))
model.add(MaxPooling1D(pool_size=2, strides=2, padding='same'))
model.add(Dropout(0.6))
model.add(Flatten())
model.add(Dense(classes, activation='softmax', name='y_pred'))
```


### Status
* Full flow with platformio and Clion-Ide  supported, compiling edgeimpulse sdk and dsp, inference
* integrated INMP441 I2S MEMS microphone in framework
* allow (via define) to switch between voice acquisition (for sampling) and inference (voice recognition) using the same functions
* performance is well for my own voice

Details can be found at: https://44-2.de