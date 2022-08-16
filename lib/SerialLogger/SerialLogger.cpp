// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "SerialLogger.h"
#include <time.h>

#define UNIX_EPOCH_START_YEAR 1900

static String getTime()
{
  struct tm* ptm;
  time_t now = time(NULL);
  String time = "";

  ptm = gmtime(&now);
  time += ptm->tm_year + UNIX_EPOCH_START_YEAR;
  time += "/";
  time += ptm->tm_mon + 1;
  time += "/";
  time += ptm->tm_mday;
  time += " ";

  if (ptm->tm_hour < 10)
  {
    time += "0";
  }

  time += ptm->tm_hour;
  time += ":";

  if (ptm->tm_min < 10)
  {
    time += "0";
  }

  time += ptm->tm_min;
  time += ":";

  if (ptm->tm_sec < 10)
  {
    time += "0";
  }

  time += ptm->tm_sec;
  return time;
}

SerialLogger::SerialLogger() { Serial.begin(SERIAL_LOGGER_BAUD_RATE); }

void SerialLogger::log(Channel chan, String message) {
  String channel = "";
  if (chan == INFO)
    channel = " [INFO] ";
  else if (chan == ERROR)
    channel = " [ERROR] ";
  else if (chan == WARNING)
    channel = " [WARNING] ";
  else if (chan == DEBUG)
    channel = " [DEBUG] ";
  
  String time = getTime();

  // Saving last logs
  String line = time + channel + message;
  Serial.println(line);
  if(this->logs.size() > MAX_LINES) {
    this->logs.pop_front();
  }
  this->logs.push_back(line);
}

void SerialLogger::Debug(String message)
{
  log(DEBUG, message);
}

void SerialLogger::Error(String message)
{
  log(ERROR, message);
}

void SerialLogger::Info(String message)
{
  log(INFO, message);
}

void SerialLogger::Warning(String message)
{
  log(WARNING, message);
}

SerialLogger Logger;
