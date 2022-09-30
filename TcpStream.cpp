/*
    This file is part of printserver-esp8266.

    printserver-esp8266 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    printserver-esp8266 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with printserver-esp8266.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TcpStream.h"

TcpStream::TcpStream(WiFiClient s): tcpConnection(s) {
  timedOut = false;
}

void TcpStream::waitAvailable(int numBytes) {
  if (timedOut) {
    return;
  }
  unsigned long start = millis();
  while (tcpConnection.available() < numBytes) {
    delay(10);
    if (millis() - start > NETWORK_READ_TIMEOUT_MS) {
      timedOut = true;
      handleTimeout();
      return;
    }
  }
}

byte TcpStream::read() {
  waitAvailable(1);
  if (timedOut) {
    return 0;
  }
  return tcpConnection.read();
}

uint16_t TcpStream::read2Bytes() {
  waitAvailable(2);
  if (timedOut) {
    return 0;
  }
  uint16_t hi = read();
  uint16_t lo = read();
  return (hi << 8) | lo;
}

uint32_t TcpStream::read4Bytes() {
  waitAvailable(4);
  if (timedOut) {
    return 0;
  }
  uint32_t b0 = read();
  uint32_t b1 = read();
  uint32_t b2 = read();
  uint32_t b3 = read();
  return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

String TcpStream::readStringUntil(char delim) {
  if (timedOut) {
    return "";
  }
  String result = "";
  result.reserve(16);
  char c;
  while (!timedOut && (c = read()) != delim) {
    result += c;
  }
  return timedOut ? "" : result;
}

String TcpStream::readString(int len) {
  waitAvailable(len);
  if (timedOut) {
    return "";
  }
  String result = "";
  result.reserve(len);
  for (int i = 0; i < len; i++) {
    result += (char) read();
  }
  return result;
}

bool TcpStream::hasMoreData() {
  return !timedOut && tcpConnection.connected();
}

bool TcpStream::dataAvailable() {
  return !timedOut && tcpConnection.available() > 0;
}

void TcpStream::write(byte b) {
  if (timedOut) {
    return;
  }
  sendBuffer[sendBufferIndex] = b;
  sendBufferIndex++;
  if (sendBufferIndex == SEND_BUFFER_SIZE) {
    flushSendBuffer();
  }
}

void TcpStream::write2Bytes(uint16_t data) {
  write((byte) ((data & 0xFF00) >> 8));
  write((byte) (data & 0x00FF));
}

void TcpStream::write4Bytes(uint32_t data) {
  write((byte) ((data & 0xFF000000) >> 24));
  write((byte) ((data & 0x00FF0000) >> 16));
  write((byte) ((data & 0x0000FF00) >> 8));
  write((byte) (data & 0x000000FF));
}

void TcpStream::print(String s) {
  for (int i = 0; i < s.length(); i++) {
    write((byte) s[i]);
  }
}

void TcpStream::flushSendBuffer() {
  if (!timedOut) {
    tcpConnection.write((byte*)sendBuffer, sendBufferIndex);
    sendBufferIndex = 0;
  }
}

void TcpStream::handleTimeout() {
  Serial.println("Connection timed out!");
  tcpConnection.stop();
}

TcpStream::~TcpStream() {
  flushSendBuffer();
  Serial.println("Closing connection!");
  tcpConnection.stop();
}

