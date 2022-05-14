#include "IpcDataStreamSerializersQt.h"

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const QString& v)
{
	QByteArray data = v.toUtf8();
	uint32_t data_size = data.size();
	stream<<data_size;
	stream.WriteRawData( data.data(), data.size());
	return stream;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, QString& v)
{
	uint32_t data_size = 0;
	stream>>data_size;
	QByteArray data;
	data.resize(data_size);
	stream.ReadRawData(data.data(), data_size);
	v = QString::fromUtf8(data);
	return stream;
}

//--------------------------------------------------

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const QByteArray& v)
{
	uint32_t data_size = v.size();
	stream<<data_size;
	stream.WriteRawData(v.data(), data_size);
	return stream;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, QByteArray& v)
{
	uint32_t data_size = 0;
	stream>>data_size;
	v.resize(data_size);
	stream.ReadRawData(v.data(), data_size);
	return stream;
}
