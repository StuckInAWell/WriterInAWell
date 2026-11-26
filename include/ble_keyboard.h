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
  KEY_CTRL_R,
  KEY_CTRL_Z,
  KEY_CTRL_ENTER,
  KEY_SHIFT_TAB,
  // Appended so the existing numeric values never move; the value is not
  // persisted, but keeping them stable avoids surprising any log decoder.
  KEY_CTRL_L,
};

struct KeyEvent {
  KeyKind kind;
  char text;
};

// preferredAddress: a previously connected peer address to auto-connect to
// once during this boot scan (nullptr or "" disables it).
void keyboardBegin(const char *preferredAddress = nullptr);
const char *keyboardConnectedAddress();
bool keyboardNext(KeyEvent &event);
const char *keyboardStatus();
size_t keyboardCandidateCount();
const char *keyboardCandidateName(size_t index);
bool keyboardConnect(size_t index);
void keyboardRescan();
bool keyboardIsConnected();
void keyboardDisconnect();
