#include <iostream>
#include <fstream>
#include "Algorithms.h"
//#include "audio.cpp"
using namespace std;



//void LSB(ofstream& file, double data_bit, double& audio_sample)
//{
//    for(int i = 0; i < sample_rate*duration; i++)
//    {
//// Генерация значения амплитуды
//        double amplitude = (double)i / sample_rate * max_amplitude;
//        // Генерация значения сигнала
//        double signal_value = sin((2 * M_PI * i * frequency) / sample_rate);
//        int sample_value = static_cast<int>(audio_sample); //текущее значение семпла
//        sample_value = (sample_value & 0xFFFE) | static_cast<int>(data_bit); //замена младшего бита информации
//        write_as_bytes(file,sample_value,2);
//
//    }
//
//
//}