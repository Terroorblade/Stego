#include <iostream>
#include <fstream>
#include <cstring>
#include <limits>
#include <cmath>
#include <vector>
#include <bitset>

using namespace std;

struct WAVEHeader
{
    uint8_t  chunkID[4] = {'R','I','F','F'};// Содержит буквы "RIFF" в форме ASCII
    uint32_t  chunkSize;  // 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
    uint8_t  format[4]  = {'W','A','V','E'};  // Содержит буквы "WAVE" в форме ASCII
    uint8_t  subchunk1ID[4] = {'f','m','t',' '}; // Содержит буквы "fmt " в форме ASCII
    uint32_t  subchunk1Size  = 16; // 16 for PCM (хранит данные)
    uint16_t audioFormat; // PCM = 1 (Значения, отличающиеся от 1, обозначают некоторый формат сжатия.)
    uint16_t numChannels;
    uint32_t  sampleRate; // частота дискретизации 8000, 44100, etc.
    uint32_t  byteRate;  // Количество байт, переданных за секунду воспроизведения. SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign; // Количество байт для одного сэмпла, включая все каналы. NumChannels * BitsPerSample/8
    uint16_t bitsPerSample; //Количество бит в сэмпле (глубина звучания) 8 бит, 16 бит и тд
    uint8_t  subchunk2ID[4] = {'d','a','t','a'};//
    uint32_t  subchunk2Size;

    bool ReadWAVEHeader(ifstream& file)
    {
        if (!file.is_open())
        {
            cerr << "Ошибка: Файл не открыт для чтения." << endl;
            return false;
        }
        file.read(reinterpret_cast<char*>(&chunkID), 4);
        if (memcmp(chunkID, "RIFF", 4) != 0)
        {
            cerr << "Ошибка: Некорректный chunkID заголовка WAV" << endl;
            return false;
        }
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        file.read(reinterpret_cast<char*>(&format), 4);
        if (memcmp(format, "WAVE", 4) != 0)
        {
            cerr << "Ошибка: Некорректный формат заголовка WAV" << endl;
            return false;
        }
        file.read(reinterpret_cast<char*>(&subchunk1ID), 4);
        if (memcmp(subchunk1ID, "fmt ", 4) != 0)
        {
            cerr << "Ошибка: Некорректный subchunk1ID заголовка WAV" << endl;
            return false;
        }
        file.read(reinterpret_cast<char*>(&subchunk1Size), sizeof(subchunk1Size));
        file.read(reinterpret_cast<char*>(&audioFormat), sizeof(audioFormat));
        file.read(reinterpret_cast<char*>(&numChannels), sizeof(numChannels));
        file.read(reinterpret_cast<char*>(&sampleRate), sizeof(sampleRate));
        file.read(reinterpret_cast<char*>(&byteRate), sizeof(byteRate));
        file.read(reinterpret_cast<char*>(&blockAlign), sizeof(blockAlign));
        file.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(bitsPerSample));
        file.seekg(176);// В стандартном представлении первые 44 байта - заголовок (пропускаем)
        file.read(reinterpret_cast<char*>(&subchunk2ID), 4);
        file.read(reinterpret_cast<char*>(&subchunk2Size), sizeof(subchunk2Size));

        return true;
    }
};
struct WAVEFile
{
    ifstream inpFile;
    ofstream outFile;
    char* filename;
    WAVEHeader header;

    bool OpenFileForRead(const char* filename)
    {
        this->filename = new char[strlen(filename) + 1];
        strcpy(this->filename, filename);
        inpFile.open(filename, ios::binary);
        if (!inpFile.is_open()) {
            cerr << "Ошибка: Невозможно прочитать файл: " << filename << endl;
            return false;
        }
        return header.ReadWAVEHeader(inpFile);
    }
    bool OpenFileForWrite(const char* filename, short numChannels, long sampleRate, short bitsPerSample)
    {
        this->filename = new char[strlen(filename)];
        strcpy(this->filename, filename);
        outFile.open(filename, ios::binary);
        if (!outFile.is_open())
        {
            cerr << "Ошибка: Не удалось открыть файл для записи: " << filename << endl;
            return false;
        }
        header.numChannels = numChannels; //закидываем в функцию сами
        header.sampleRate = sampleRate;
        header.bitsPerSample = bitsPerSample;
        header.subchunk1Size = 16; //PCM формат

        header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
        header.blockAlign = header.numChannels * header.bitsPerSample / 8;

        outFile.write((char*)&header.chunkID, 4);
        outFile.write((char*)&header.chunkSize, sizeof(header.chunkSize));

        return true;
    }
    void CloseFile()
    {
        if (inpFile.is_open()) inpFile.close();
        if (outFile.is_open()) outFile.close();
    }
};

//Функция преобразования символа в двоичную последовательность (8 бит)
string toBinaryChar(char ch)
{
    string binary_message;
    if (ch < 0 || ch > 255)
    {
        cerr << "Ошибка: Символ выходит за пределы таблицы ASCII." << endl;
    }
    for(int power = 7; power >= 0; power--)
    {
        char bit = (ch >> power) & 1; //извлекаем текущий бит
        binary_message += bit + '0'; //преобразовать бит в '0' или '1' и добавить его к строке
    }
    return binary_message;
}

// Функция преобразования целого числа в двоичную строку (32 бита)
string intToBinaryString(int number)
{
    return bitset<32>(number).to_string();
}

// Функция преобразования двоичной строки обратно в целое число
int binaryStringToInt(const string& binary)
{
    return static_cast<int>(bitset<32>(binary).to_ulong());
}

//LSB
//Алгоритм встраивания текста в аудио
void LSB_HIDE(const string& audioPath, const string& messagePath, const string& outputAudioPath, int bytes_per_sample)
{
    ifstream audioFile(audioPath, ios::binary);
    ifstream messageFile(messagePath, ios::binary);
    if (!audioFile || !messageFile)
    {
        cerr << "Ошибка: Невозможно открыть исходный файл." << endl;
        return;
    }
    ofstream outputFile(outputAudioPath, ios::binary);
    if (!outputFile)
    {
        cerr << "Ошибка: Невозможно создать выходной аудиофайл." << endl;
        audioFile.close();
        return;
    }
    vector<char> waveBuffer(bytes_per_sample);
    char waveByte;//message получает следующий байт сообщения или -1
    //информация до subchunk2size (data 69-73 бит - после нее subchunk2size с 74 бита - откуда и начинаем встраивать информацию)
    for(int i = 0; i < 176; i++)
    {
        audioFile.read(&waveBuffer[0],bytes_per_sample);
        outputFile.write(&waveBuffer[0], bytes_per_sample);
    }
    string inp_message;
    getline(messageFile, inp_message);
    string binaryMessage; // строка с скрываемым сообщением
    for (char ch : inp_message)
    {
        binaryMessage += toBinaryChar(ch);
    }
    int binaryMessage_len = binaryMessage.length();
    binaryMessage = intToBinaryString(binaryMessage_len) + binaryMessage; //к скрываемому сообщению в начало добавляем размер сообщения (представлено в виде двоичной последовательности из 32 бит)
    size_t bitIndex = 0;
    while (audioFile.read(&waveBuffer[0], bytes_per_sample))     // Алгоритм замены младшего бита
    {
        if (bitIndex >= binaryMessage.size())
        {
            outputFile.write(&waveBuffer[0], bytes_per_sample);//Если встроили всё сообщение, просто перезаписываем остальные данные без изменений
        }
        else
        {
            waveByte = waveBuffer[bytes_per_sample - 1];
            waveByte = (waveByte & 0xFE) | ((binaryMessage[bitIndex++] - '0') & 1);
            waveBuffer[bytes_per_sample - 1] = waveByte;
            outputFile.write(&waveBuffer[0], bytes_per_sample);
        }
    }
    audioFile.close();
    messageFile.close();
    outputFile.close();
}

//Алгоритм извлечения внедренного сообщения
void LSB_EXTRACT(const string& audioPath, const string& outputMessagePath, int bytesPerSample)
{
    ifstream audioFile(audioPath, ios::binary);
    ofstream outputMessageFile(outputMessagePath, ios::binary);
    if (!audioFile || !outputMessageFile)
    {
        cerr << "Ошибка: Невозможно открыть файл." << endl;
        return;
    }
    vector<char> waveBuffer(bytesPerSample);
    char message = 0, waveBit;
    int bitCount = 0;// Счетчик битов
    string binaryMessageLengthRaw;
    string resultMessage; //строка со считанным скрытым сообщением
    // Пропускаем заголовок WAV файла
    for (int i = 0; i < 176; ++i)
    {
        audioFile.read(&waveBuffer[0], bytesPerSample);
    }
    int binaryMessageLength; //длина скрываемого сообщения
    //Считываем байты сообщения из аудиофайла
    //binaryMessageLength < bitCount - 32 - истинно, пока кол-во бит меньше, чем было записано в wav-файл (скрытое сообщение)
    while(audioFile.read(&waveBuffer[0], bytesPerSample) && (bitCount <= 32 || binaryMessageLength > bitCount - 32))
    {
        waveBit = waveBuffer[bytesPerSample - 1]; // берем последний бит
        message = (message << 1) | (waveBit & 1); // Извлекаем младший бит
        bitCount++;
        bool fl = waveBit & 1;
        if(bitCount <= 32) //собираем 32 бита для перевода в длину сообщения
        {
            binaryMessageLengthRaw.append( fl ? "1" : "0");
        }
        if(bitCount==32)
        {
            binaryMessageLength = binaryStringToInt(binaryMessageLengthRaw); //сохранили длину скрываемого сообщения
        }
        if (bitCount%8==0 && bitCount > 32)//после первых 32 бит () размера сообщения  - уже идет скрытый текст - тут мы его считываем по 8 бит чтобы преобразовать из двоичного представления в символы
        { // Если собрано 8 битов
            resultMessage += message; // Добавляем байт к бинарной строке
        }
        if(bitCount%8==0)
        {
            message = 0;// Обнуляем переменную для следующего байта
        }
    }
    cout << endl;
    outputMessageFile << resultMessage;
    audioFile.close();
    outputMessageFile.close();
}



int main()
{
    WAVEFile inp_file, out_file;
    string audioPath = "/Users/terroor/Stego/nohearing(сжатый).wav";
//    string audioPath = "/Users/terroor/Stego/bogus.wav";
//    string messagePath = "/Users/terroor/Stego/message(LSB2).txt";
    string messagePath = "/Users/terroor/Stego/BIGMES.txt";
    string outputAudioPath = "/Users/terroor/Stego/modifiednohearing.wav";
//    string outputAudioPath = "/Users/terroor/Stego/modifiedbogus.wav";
    string outputMessagePath = "/Users/terroor/Stego/output_message(LSB2).txt";

    WAVEFile testWAV; //исходный файл
    int bytesPerSample = 1;  // Размер буфера аудиофайла в байтах

    bool menu = true;
    while(menu)
    {
        system("clear");
        int choise;
        cout << "1. Внедрить текст в WAV файл.\n"
             << "2. Извлечь скрытый текст из WAV файла.\n"
             << "3. Выход.\n"
             << endl << "Введите число: ";
        cin >> choise;
        cout << endl;
        switch (choise)
        {
            case 1:
            {
                LSB_HIDE(audioPath, messagePath, outputAudioPath, bytesPerSample);
                if(out_file.OpenFileForRead(outputAudioPath.c_str()))
                {
                    cout << "Сообщение успешно встроено в аудиофайл." << endl;
                }
                cout << endl;
                inp_file.CloseFile();
                out_file.CloseFile();
                break;
            }
            case 2:
            {
                LSB_EXTRACT(outputAudioPath, outputMessagePath, bytesPerSample);
                ifstream outputMessageFile(outputMessagePath);
                if (outputMessageFile.is_open())
                {
                    string extractedMessage;
                    getline(outputMessageFile, extractedMessage);
                    cout << "Извлеченное сообщение: " << extractedMessage << endl;
                    outputMessageFile.close();
                }
                else
                {
                    cerr << "Ошибка: Невозможно открыть файл выходного сообщения." << endl;
                }
                cout << endl;
                inp_file.CloseFile();
                out_file.CloseFile();
                break;
            }
            case 3:
            {
                menu = false;
                break;
            }
            case 4:
            {
                if (!testWAV.OpenFileForRead(audioPath.c_str()))
                    cerr << "Ошибка: Невозможно открыть файл." << endl;

                cout << "Размер Subchunk1: "<< testWAV.header.subchunk1Size << endl;
                cout << "Аудио формат (PCM): " << testWAV.header.audioFormat << endl;
                cout << "Количество каналов: " << testWAV.header.numChannels << endl;
                cout << "Частота дискретизации: " << testWAV.header.sampleRate << " Гц" << endl;
                cout << "Количество байт, переданных за секунду воспроизведения (битрейт): " << testWAV.header.byteRate << endl;
                cout << "Количество байт для одного сэмпла: " << testWAV.header.blockAlign << endl;
                cout << "Количество бит в сэмпле: " << testWAV.header.bitsPerSample << endl;
                cout << "Subchunk2ID: " << testWAV.header.subchunk2ID << endl;
                cout << "Размер Subchunk2 в битах: " << testWAV.header.subchunk2Size << endl;

                float fDurationSeconds = static_cast<float>(testWAV.header.subchunk2Size)/(testWAV.header.bitsPerSample/8)/testWAV.header.numChannels/testWAV.header.sampleRate / 1000;
                cout << "Длительность файла в секундах: " << fDurationSeconds << endl;
                int iDurationMinutes = static_cast<float>(fDurationSeconds) / 60;
                cout << "Длительность файла в минутах: " << iDurationMinutes << endl;

                cout << endl;
                inp_file.CloseFile();
                out_file.CloseFile();
                break;
            }
            default:
            {
                cout << "Некорректный выбор. Повторите снова.\n" << endl;
                break;
            }
        }

    }
}