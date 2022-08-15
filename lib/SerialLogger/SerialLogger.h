// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SERIALLOGGER_H
#define SERIALLOGGER_H

#include <Arduino.h>// #include <Vector.h>
#include <Vector.h>

#ifndef SERIAL_LOGGER_BAUD_RATE
#define SERIAL_LOGGER_BAUD_RATE 115200
#endif

enum Channel {INFO, ERROR, WARNING, DEBUG};

class SerialLogger
{
private:
  Vector<String> logs;
  void log(Channel chan, String message);

public:
  SerialLogger();
  void Debug(String message);
  void Error(String message);
  void Info(String message);
  void Warning(String message);

};

extern SerialLogger Logger;

#endif // SERIALLOGGER_H
