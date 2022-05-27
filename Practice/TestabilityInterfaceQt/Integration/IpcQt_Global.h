#pragma once

#include "MAssIpcCall.h"

void SutInit(const QString host_name, uint16_t target_port);
void SutRegister(QObject* sut_object);

MAssIpcCall& IpcClient();