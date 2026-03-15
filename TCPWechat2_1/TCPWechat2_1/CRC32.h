// CRC32.h 核心代码（直接复制）
#ifndef CRC32_H
#define CRC32_H
#include <QByteArray>
class CRC32 {
public:
    static uint32_t calculate(const QByteArray& data);
    static bool verify(const QByteArray& data, uint32_t checksum);
};
#endif

