#include "ble_keyboard.h"

#include <NimBLEDevice.h>
#include <new>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace {
constexpr uint16_t HID_SERVICE = 0x1812;
constexpr uint16_t HID_REPORT = 0x2A4D;
constexpr size_t REPORT_QUEUE_LENGTH = 48;
constexpr size_t MAX_KEYBOARDS = 6;

QueueHandle_t eventQueue;
volatile bool connected = false;
volatile bool scanning = false;
char connectionLabel[48] = "BLE scan: searching";
NimBLEClient *activeClient = nullptr;
struct KeyboardCandidate {
  NimBLEAdvertisedDevice *device = nullptr;
  char name[32] = {};
  char address[20] = {};
};
KeyboardCandidate candidates[MAX_KEYBOARDS];
size_t candidateCount = 0;
int pendingConnectIndex = -1;
portMUX_TYPE candidateMux = portMUX_INITIALIZER_UNLOCKED;
char selectedKeyboardName[32] = {};
uint8_t previousKeys[6] = {};
uint8_t heldModifiers = 0;
bool pressedKeys[256] = {};
uint32_t repeatAt[256] = {};
portMUX_TYPE keyStateMux = portMUX_INITIALIZER_UNLOCKED;

enum : uint8_t {
  HID_NONE = 0,
  HID_BACKSPACE = 0x2A,
  HID_ENTER = 0x28,
  HID_ESCAPE = 0x29,
  HID_TAB = 0x2B,
  HID_SPACE = 0x2C,
  HID_DELETE = 0x4C,
  HID_RIGHT = 0x4F,
  HID_LEFT = 0x50,
  HID_DOWN = 0x51,
  HID_UP = 0x52,
  HID_HOME = 0x4A,
  HID_END = 0x4D,
  HID_PAGE_UP = 0x4B,
  HID_PAGE_DOWN = 0x4E,
};

void enqueue(KeyKind kind, char ch = 0) {
  if (!eventQueue) return;
  KeyEvent event{kind, ch};
  xQueueSend(eventQueue, &event, 0);
}

char keyToAscii(uint8_t usage, bool shift) {
  if (usage >= 0x04 && usage <= 0x1D) {
    const char c = static_cast<char>('a' + usage - 0x04);
    return shift ? static_cast<char>(c - 'a' + 'A') : c;
  }
  if (usage >= 0x1E && usage <= 0x27) {
    static const char plain[] = "1234567890";
    static const char shifted[] = "!@#$%^&*()";
    const size_t i = usage - 0x1E;
    return shift ? shifted[i] : plain[i];
  }
  switch (usage) {
  case HID_SPACE: return ' ';
  case 0x2D: return shift ? '_' : '-';
  case 0x2E: return shift ? '+' : '=';
  case 0x2F: return shift ? '{' : '[';
  case 0x30: return shift ? '}' : ']';
  case 0x31: return shift ? '|' : '\\';
  case 0x32: return shift ? '~' : '`';
  case 0x33: return shift ? ':' : ';';
  case 0x34: return shift ? '"' : '\'';
  case 0x35: return shift ? '~' : '`';
  case 0x36: return shift ? '<' : ',';
  case 0x37: return shift ? '>' : '.';
  case 0x38: return shift ? '?' : '/';
  default: return 0;
  }
}

void emitUsage(uint8_t usage, uint8_t modifiers) {
  const bool ctrl = (modifiers & 0x11) != 0;
  const bool shift = (modifiers & 0x22) != 0;
  if (!usage) return;
  if (ctrl && (usage == 0x16)) {  // Ctrl-S
    enqueue(KEY_CTRL_S);
    return;
  }
  if (ctrl && usage == 0x12) {  // Ctrl-O
    enqueue(KEY_CTRL_O);
    return;
  }
  switch (usage) {
  case HID_BACKSPACE: enqueue(KEY_BACKSPACE); return;
  case HID_ENTER: enqueue(KEY_ENTER); return;
  case HID_ESCAPE: enqueue(KEY_ESCAPE); return;
  case HID_TAB: enqueue(KEY_TAB); return;
  case HID_LEFT: enqueue(KEY_LEFT); return;
  case HID_RIGHT: enqueue(KEY_RIGHT); return;
  case HID_UP: enqueue(KEY_UP); return;
  case HID_DOWN: enqueue(KEY_DOWN); return;
  case HID_DELETE: enqueue(KEY_DELETE); return;
  case HID_HOME: enqueue(KEY_HOME); return;
  case HID_END: enqueue(KEY_END); return;
  case HID_PAGE_UP: enqueue(KEY_PAGE_UP); return;
  case HID_PAGE_DOWN: enqueue(KEY_PAGE_DOWN); return;
  default: break;
  }
  const char c = keyToAscii(usage, shift);
  if (c) enqueue(KEY_TEXT, c);
}

void onReport(NimBLERemoteCharacteristic *, uint8_t *data, size_t length,
              bool) {
  // Standard boot keyboard report: modifier, reserved, six usages. Some
  // report-protocol keyboards prefix the same payload with report ID 1.
  size_t offset = 0;
  if (length == 9 && data[0] == 1) offset = 1;
  if (length < offset + 8) return;
  const uint8_t modifiers = data[offset];
  for (size_t i = offset + 2; i < offset + 8; ++i) {
    if (data[i] == 1) return;  // HID rollover/error usage
  }
  uint8_t current[6] = {};
  for (size_t i = 0; i < 6; ++i) current[i] = data[offset + 2 + i];
  const uint32_t now = millis();
  uint8_t justPressed[6] = {};
  size_t justPressedCount = 0;
  portENTER_CRITICAL(&keyStateMux);
  for (size_t i = 0; i < 6; ++i) {
    const uint8_t usage = current[i];
    if (!usage) continue;
    bool wasDown = false;
    for (uint8_t old : previousKeys) wasDown |= old == usage;
    if (!wasDown) {
      justPressed[justPressedCount++] = usage;
      pressedKeys[usage] = true;
      repeatAt[usage] = now + 450;
    }
  }
  for (uint8_t old : previousKeys) {
    if (!old) continue;
    bool stillDown = false;
    for (uint8_t usage : current) stillDown |= usage == old;
    if (!stillDown) pressedKeys[old] = false;
  }
  heldModifiers = modifiers;
  memcpy(previousKeys, current, sizeof(previousKeys));
  portEXIT_CRITICAL(&keyStateMux);
  for (size_t i = 0; i < justPressedCount; ++i) {
    emitUsage(justPressed[i], modifiers);
  }
}

void serviceKeyRepeats() {
  const uint32_t now = millis();
  uint8_t repeat[6] = {};
  size_t repeatCount = 0;
  uint8_t modifiers = 0;
  portENTER_CRITICAL(&keyStateMux);
  modifiers = heldModifiers;
  if ((modifiers & 0x55) == 0) {  // avoid repeating control/alt shortcuts
    for (size_t i = 0; i < 6; ++i) {
      const uint8_t usage = previousKeys[i];
      if (!usage || !pressedKeys[usage]) continue;
      if (static_cast<int32_t>(now - repeatAt[usage]) >= 0) {
        repeat[repeatCount++] = usage;
        repeatAt[usage] = now + 55;
      }
    }
  }
  portEXIT_CRITICAL(&keyStateMux);
  for (size_t i = 0; i < repeatCount; ++i) emitUsage(repeat[i], modifiers);
}

class ScanCallbacks final : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice *device) override {
    if (connected || !device->isAdvertisingService(NimBLEUUID(HID_SERVICE))) return;
    NimBLEAdvertisedDevice *copy =
        new (std::nothrow) NimBLEAdvertisedDevice(*device);
    if (!copy) return;
    const std::string address = device->getAddress().toString();
    std::string name = device->getName();
    if (name.empty()) name = address;

    portENTER_CRITICAL(&candidateMux);
    size_t slot = candidateCount;
    for (size_t i = 0; i < candidateCount; ++i) {
      if (strcmp(candidates[i].address, address.c_str()) == 0) {
        slot = i;
        break;
      }
    }
    if (slot == candidateCount && candidateCount < MAX_KEYBOARDS) {
      slot = candidateCount++;
    }
    if (slot < candidateCount && !candidates[slot].device) {
      if (copy) {
        candidates[slot].device = copy;
        copy = nullptr;
        strlcpy(candidates[slot].address, address.c_str(),
                sizeof(candidates[slot].address));
        strlcpy(candidates[slot].name, name.c_str(),
                sizeof(candidates[slot].name));
      }
    }
    const size_t found = candidateCount;
    portEXIT_CRITICAL(&candidateMux);
    delete copy;

    snprintf(connectionLabel, sizeof(connectionLabel), "BLE scan: %u found",
             unsigned(found));
  }
};

class ClientCallbacks final : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *) override {
    connected = true;
    scanning = false;
    snprintf(connectionLabel, sizeof(connectionLabel), "Connected: %.30s",
             selectedKeyboardName);
  }

  void onDisconnect(NimBLEClient *, int) override {
    connected = false;
    activeClient = nullptr;
    portENTER_CRITICAL(&keyStateMux);
    memset(previousKeys, 0, sizeof(previousKeys));
    memset(pressedKeys, 0, sizeof(pressedKeys));
    heldModifiers = 0;
    portEXIT_CRITICAL(&keyStateMux);
    strlcpy(connectionLabel, "BLE scan: searching", sizeof(connectionLabel));
    NimBLEDevice::getScan()->start(0, false);
    scanning = true;
  }
};

ScanCallbacks scanCallbacks;
ClientCallbacks clientCallbacks;

void connectSelected(NimBLEAdvertisedDevice *device) {
  if (!device || connected) {
    delete device;
    return;
  }
  NimBLEDevice::getScan()->stop();
  scanning = false;
  if (!activeClient) activeClient = NimBLEDevice::createClient();
  if (!activeClient) {
    delete device;
    strlcpy(connectionLabel, "BLE client unavailable", sizeof(connectionLabel));
    return;
  }
  activeClient->setClientCallbacks(&clientCallbacks, false);
  if (!activeClient->connect(device)) {
    delete device;
    strlcpy(connectionLabel, "BLE connection failed; rescan", sizeof(connectionLabel));
    NimBLEDevice::getScan()->start(0, false);
    scanning = true;
    return;
  }
  delete device;
  NimBLERemoteService *service = activeClient->getService(NimBLEUUID(HID_SERVICE));
  if (!service) {
    strlcpy(connectionLabel, "BLE device has no HID service", sizeof(connectionLabel));
    activeClient->disconnect();
    return;
  }
  const auto characteristics = service->getCharacteristics(true);
  bool subscribed = false;
  for (auto *characteristic : characteristics) {
    if (characteristic->getUUID() == NimBLEUUID(HID_REPORT) &&
        characteristic->canNotify()) {
      subscribed |= characteristic->subscribe(true, onReport);
    }
  }
  if (!subscribed) {
    strlcpy(connectionLabel, "Unsupported keyboard report", sizeof(connectionLabel));
    activeClient->disconnect();
    return;
  }
  connected = true;
  scanning = false;
  snprintf(connectionLabel, sizeof(connectionLabel), "Connected: %.30s",
           selectedKeyboardName);
}

void processPendingConnection() {
  NimBLEAdvertisedDevice *device = nullptr;
  portENTER_CRITICAL(&candidateMux);
  const int index = pendingConnectIndex;
  pendingConnectIndex = -1;
  if (index >= 0 && static_cast<size_t>(index) < candidateCount) {
    device = candidates[index].device;
    candidates[index].device = nullptr;
    strlcpy(selectedKeyboardName, candidates[index].name,
            sizeof(selectedKeyboardName));
  }
  portEXIT_CRITICAL(&candidateMux);
  if (device) connectSelected(device);
}
} // namespace

void keyboardBegin() {
  eventQueue = xQueueCreate(REPORT_QUEUE_LENGTH, sizeof(KeyEvent));
  NimBLEDevice::init("WriterInAWell");
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(&scanCallbacks, false);
  scan->setMaxResults(0);
  scan->setActiveScan(true);
  scan->setInterval(1000);
  scan->setWindow(80);
  scan->start(0, false);
  scanning = true;
  strlcpy(connectionLabel, "BLE scan: searching", sizeof(connectionLabel));
}

bool keyboardNext(KeyEvent &event) {
  processPendingConnection();
  serviceKeyRepeats();
  return eventQueue && xQueueReceive(eventQueue, &event, 0) == pdTRUE;
}

const char *keyboardStatus() {
  return connectionLabel;
}

size_t keyboardCandidateCount() {
  portENTER_CRITICAL(&candidateMux);
  const size_t count = candidateCount;
  portEXIT_CRITICAL(&candidateMux);
  return count;
}

const char *keyboardCandidateName(size_t index) {
  if (index >= candidateCount) return "";
  return candidates[index].name;
}

bool keyboardConnect(size_t index) {
  bool queued = false;
  portENTER_CRITICAL(&candidateMux);
  if (!connected && pendingConnectIndex < 0 &&
      index < candidateCount && candidates[index].device) {
    pendingConnectIndex = static_cast<int>(index);
    snprintf(connectionLabel, sizeof(connectionLabel), "Connecting: %.28s",
             candidates[index].name);
    queued = true;
  }
  portEXIT_CRITICAL(&candidateMux);
  return queued;
}

void keyboardRescan() {
  if (connected) {
    strlcpy(connectionLabel, "Disconnect keyboard before rescanning",
            sizeof(connectionLabel));
    return;
  }
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->stop();
  NimBLEAdvertisedDevice *garbage[MAX_KEYBOARDS] = {};
  portENTER_CRITICAL(&candidateMux);
  for (size_t i = 0; i < candidateCount; ++i) {
    garbage[i] = candidates[i].device;
    candidates[i] = {};
  }
  candidateCount = 0;
  pendingConnectIndex = -1;
  portEXIT_CRITICAL(&candidateMux);
  for (auto *device : garbage) delete device;
  scan->clearResults();
  strlcpy(connectionLabel, "BLE scan: searching", sizeof(connectionLabel));
  scanning = scan->start(0, false);
}

bool keyboardIsConnected() {
  return connected;
}

void keyboardDisconnect() {
  if (activeClient && connected) activeClient->disconnect();
}
