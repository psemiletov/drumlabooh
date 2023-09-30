#include "dsp.h"

#include <vector>

#include <cmath>

using namespace std;


#define PI 3.14159265358979323846

float db_scale;

void init_db()
{
  db_scale = log (10.0) * 0.05;
}





// Функция для гитарного искажения в стиле gabber/digital hardcore
float gabberDistortion2(float inputSample, float distortionLevel) {
    // Шаг 1: Усиление с высоким коэффициентом
    float amplifiedSample = inputSample * 48.0f * distortionLevel;

    // Шаг 2: Операционный усилитель с нелинейностью
    //float opAmpOutput = tanh(2.0f * amplifiedSample);

    // Шаг 3: Преследование насыщенности (saturation)
    //float saturatedSample = atan(opAmpOutput);

    // Шаг 4: Динамическое смягчение (dynamic waveshaping)
    //float waveshapedSample = atan(distortionLevel * saturatedSample);

    // Ограничиваем значения в диапазоне от 0 до 1
    amplifiedSample = std::min(1.0f, std::max(0.0f, amplifiedSample));

    return amplifiedSample;
}


// Функция для гитарного искажения в стиле gabber/digital hardcore
float gabberDistortion(float inputSample, float distortionLevel) {
    // Шаг 1: Усиление с высоким коэффициентом
    float amplifiedSample = inputSample * (1.5f + 4.0f * distortionLevel);

    // Шаг 2: Операционный усилитель с нелинейностью
    float opAmpOutput = tanh(2.0f * amplifiedSample);

    // Шаг 3: Преследование насыщенности (saturation)
    float saturatedSample = atan(opAmpOutput);

    // Шаг 4: Динамическое смягчение (dynamic waveshaping)
    float waveshapedSample = atan(distortionLevel * saturatedSample);

    // Ограничиваем значения в диапазоне от 0 до 1
    waveshapedSample = std::min(1.0f, std::max(0.0f, waveshapedSample));

    return waveshapedSample;
}


// Функция для моделирования гитарного искажения
float guitarDistortion(float inputSample, float distortionLevel) {
    // Шаг 1: Усиление
    float amplifiedSample = inputSample * (1.0f + distortionLevel);

    // Шаг 2: Операционный усилитель (Op-Amp)
    float opAmpOutput = tanh(amplifiedSample);

    // Шаг 3: Клиппер/ограничитель
    float clippedSample = tanh(opAmpOutput);

    // Ограничиваем значения в диапазоне от 0 до 1
    clippedSample = std::min(1.0f, std::max(0.0f, clippedSample));

    return clippedSample;
}




// Функция для более "хриплого" гитарного искажения
float grittyGuitarDistortion(float inputSample, float distortionLevel) {
    // Шаг 1: Усиление с более высоким коэффициентом
    float amplifiedSample = inputSample * (1.5f + 4.0f * distortionLevel);

    // Шаг 2: Операционный усилитель с более высокой нелинейностью
    float opAmpOutput = tanh(2.0f * amplifiedSample);

    // Шаг 3: Клиппер/ограничитель с более сильным искажением
    float clippedSample = tanh(2.0f * opAmpOutput);

    // Ограничиваем значения в диапазоне от 0 до 1
    clippedSample = std::min(1.0f, std::max(0.0f, clippedSample));

    return clippedSample;
}

void bjtfilt(float* x, int samples_count, float fs)
{
    // Calculate the coefficient
    float coeff = M_PI / fs;

    // Define the filter coefficients
    float B0 = 1;
    float B1 = -2;
    float B2 = 1;

    float A0 = 1800 * pow(coeff, 2) + 603 * coeff + 1;
    float A1 = 3600 * pow(coeff, 2) - 2;
    float A2 = 1800 * pow(coeff, 2) - 603 * coeff + 1;

    // Calculate the gain
    float amp = pow(10, (36.0 / 20.0));
    B0 *= amp;
    B1 *= amp;
    B2 *= amp;

    // Apply the filter to the input array
    for (int n = 0; n < samples_count; ++n) {
        float y = B0 * x[n];
        if (n >= 1) y += B1 * x[n - 1];
        if (n >= 2) y += B2 * x[n - 2];

        if (n >= 1) y -= A1 * x[n - 1];
        if (n >= 2) y -= A2 * x[n - 2];

        x[n] = static_cast<float>(y / A0);
    }
}




void opampfilt(float* x, int samples_count, float fs, float DIST)
{
    // Resistors and capacitors from the model
    float Rt = 100000 * DIST;
    float Rb = 100000 * (1 - DIST) + 4700;
    float Cz = 0.000001;
    float Cc = 0.000000000250;

    // Constant for the bilinear transform
    float c = 2 * fs;

    // Continuous-time coefficients (reduced)
    float ab0 = 1 / (Rt * Cc * Rb * Cz);
    float a1 = 1 / (Rb * Cz) + 1 / (Rt * Cc);
    float b1 = a1 + 1 / (Rb * Cc);

    // Discrete-time coefficients
    float B0 = ab0 + b1 * c + pow(c, 2);
    float B1 = 2 * ab0 - 2 * pow(c, 2);
    float B2 = ab0 - b1 * c + pow(c, 2);

    float A0 = ab0 + a1 * c + pow(c, 2);
    float A1 = B1;
    float A2 = ab0 - a1 * c + pow(c, 2);

    // Apply the filter to the input array
    for (int n = 0; n < samples_count; ++n) {
        float y = B0 * x[n];
        if (n >= 1) y += B1 * x[n - 1];
        if (n >= 2) y += B2 * x[n - 2];

        if (n >= 1) y -= A1 * x[n - 1];
        if (n >= 2) y -= A2 * x[n - 2];

        x[n] = static_cast<float>(y / A0);
    }
}



void diodeclip(float* x, int samples_count)
{
    float n = 2.5;

    for (int i = 0; i < samples_count; ++i) {
        x[i] = static_cast<float>(x[i] / pow(1 + pow(fabs(x[i]), n), 1.0 / n));
    }
}

/*
 DS-1, main script
% Implementation by Nestor Napoles Lopez, December 2018
% based on the paper by Yeh et al. (2007)

% Sample audio
[x, fs] = audioread('guitar_clean.wav');

% Bipolar Junction Transistor Stage
y = bjtfilt(x, fs);

bjtfilt (samples, fs);


% Op-amp Gain Stage
D = 1; % D lies between [0, 1]
y = opampfilt(y, fs, D);

% Diode clipper
y = diodeclip(y);

s = audioplayer(y, fs);
play(s);
*/

//analog
float warmify(float x, float warmth)
{
    // Проверяем, что "warmth" находится в пределах от 0 до 1
    warmth = std::min(1.0f, std::max(0.0f, warmth));

    // Применяем теплое, аналоговое воздействие на сигнал
    float warm_x = x * (1.0f - warmth) + std::sin(x * M_PI) * warmth;

    // Ограничиваем значения в диапазоне от -1 до 1
    warm_x = std::min(1.0f, std::max(-1.0f, warm_x));

    return warm_x;
}



