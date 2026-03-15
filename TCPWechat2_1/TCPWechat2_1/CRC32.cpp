// CRC32.cpp 核心代码（直接复制）
#include "CRC32.h"
uint32_t CRC32::calculate(const QByteArray& data) {
    uint32_t crc = 0xFFFFFFFF;
    for (char c : data) {
        crc ^= static_cast<uint8_t>(c);
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }
    return ~crc;
}
bool CRC32::verify(const QByteArray& data, uint32_t checksum) {
    return calculate(data) == checksum;
}