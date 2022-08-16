// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SERIALLOGGER_H
#define SERIALLOGGER_H

#include <Arduino.h>// #include <Vector.h>
#include <deque>

#ifndef SERIAL_LOGGER_BAUD_RATE
#define SERIAL_LOGGER_BAUD_RATE 115200
#define MAX_LINES 50
#endif

enum Channel {INFO, ERROR, WARNING, DEBUG};

class SerialLogger
{
private:
  void log(Channel chan, String message);

public:
  SerialLogger();
  std::deque<String> logs;
  void Debug(String message);
  void Error(String message);
  void Info(String message);
  void Warning(String message);
};

extern SerialLogger Logger;

#endif // SERIALLOGGER_H
