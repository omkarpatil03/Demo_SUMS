#include <fstream>
#include <cstdint>

#include "trng.hpp"

#define     MIN     1
#define     MAX     100

int getTrueRandomNumber(void){

    std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);

    if(!urandom){
        throw std::runtime_error("failed to open /dev/urandom\r\n");
    }

    uint32_t random_value;
    urandom.read(reinterpret_cast<char *>(&random_value), sizeof(random_value));
    urandom.close();


    return (random_value % (MAX - MIN + 1)) + MIN;

}