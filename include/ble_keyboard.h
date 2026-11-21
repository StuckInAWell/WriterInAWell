#pragma once

#include <Arduino.h>
#include <stddef.h>

enum KeyKind : uint8_t {
  KEY_TEXT,
  KEY_BACKSPACE,
  KEY_ENTER,
  KEY_ESCAPE,
  KEY_TAB,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_UP,
  KEY_DOWN,
  KEY_DELETE,
  KEY_HOME,
  KEY_END,
  KEY_PAGE_UP,
  KEY_PAGE_DOWN,
  KEY_CTRL_S,
  KEY_CTRL_O,
};

struct KeyEvent {
  KeyKind kind;
  char text;
};

void keyboardBegin();
bool keyboardNext(KeyEvent &event);
const char *keyboardStatus();
size_t keyboardCandidateCount();
const char *keyboardCandidateName(size_t index);
bool keyboardConnect(size_t index);
void keyboardRescan();
bool keyboardIsConnected();
void keyboardDisconnect();
