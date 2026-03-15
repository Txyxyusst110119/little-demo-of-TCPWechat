#include "Packet.h"

// 打包：序列化（包含type字段）
QByteArray Packet::pack() const {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    uint32_t dataLen = data.size();
    uint32_t checksum = CRC32::calculate(data);

    stream << MAGIC_NUM;
    stream << dataLen;
    stream << static_cast<uint32_t>(type);
    stream << checksum;
    stream.writeRawData(data.constData(), dataLen);

    return bytes;
}


// 解包：反序列化（包含type字段）
bool Packet::unpack(QByteArray& buffer, Packet& out) {
    const int HEADER_SIZE = 16; // magic + len + type + crc

    if (buffer.size() < HEADER_SIZE)
        return false;

    QDataStream stream(buffer);
    stream.setByteOrder(QDataStream::BigEndian);

    uint32_t magic, len, type, crc;
    stream >> magic >> len >> type >> crc;

    if (magic != MAGIC_NUM)
        return false;

    if (buffer.size() < HEADER_SIZE + (int)len)
        return false; // 数据还没收完整

    QByteArray data(len, Qt::Uninitialized);
    stream.readRawData(data.data(), len);

    if (CRC32::calculate(data) != crc)
        return false;

    out.type = static_cast<PacketType>(type);
    out.data = data;

    buffer.remove(0, HEADER_SIZE + len);
    return true;
}
