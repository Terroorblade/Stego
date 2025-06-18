#include "Algorithm.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstdlib>

using namespace std;


//встраивание ЦВЗ (скрытого сообщения) в аудио
int WriteMessage(vector<char>& buffer, string message)
{
    if((buffer.size()/4) < message.size())
    {
        cout << "Сообщение слишком большое для встраивания в аудио.";
        return 1;
    }

}
