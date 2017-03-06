#include <stddef.h>
#include <ion.h>
#include <ion/src/device/backlight.h>
#include <ion/src/device/display.h>
#include <ion/src/device/led.h>
#include <poincare.h>

typedef void (*CommandFunction)(const char * input);

void command_ping(const char * input);
void command_mcu_serial(const char * input);

class CommandHandler {
public:
  constexpr CommandHandler(const char * name, CommandFunction function) :
    m_name(name), m_function(function) {}
  bool valid() const;
  bool handle(const char * command) const;
private:
  bool matches(const char * command) const;
  const char * m_name;
  CommandFunction m_function;
};

bool CommandHandler::valid() const {
  return (m_name != nullptr && m_function != nullptr);
}

bool CommandHandler::handle(const char * command) const {
  if (matches(command)) {
    size_t nameLength = strlen(m_name);
    if (command[nameLength] == '=') {
      m_function(command+nameLength+1); // Skip the "Equal character"
    } else {
      m_function(nullptr);
    }
    return true;
  }
  return false;
}

bool CommandHandler::matches(const char * command) const {
  const char * c = command;
  const char * n = m_name;
  while (true) {
    if (*n == NULL) {
      if (*c == NULL || *c == '=') {
        return true;
      }
    }
    if (*c != *n) {
      return false;
    }
    c++;
    n++;
  }
}

class CommandList {
public:
  constexpr CommandList(const CommandHandler * handlers) : m_handlers(handlers) {}
  void dispatch(const char * command) const;
private:
  const CommandHandler * m_handlers;
};

void CommandList::dispatch(const char * command) const {
  const CommandHandler * handler = m_handlers;
  while (handler->valid()) {
    if (handler->handle(command)) {
      return;
    }
    handler++;
  }
  Ion::Console::writeLine("NOT_FOUND");
}

static const char * sOK = "OK";
static const char * sSyntaxError = "SYNTAX_ERROR";
static const char * sON = "ON";
static const char * sOFF = "OFF";

void command_ping(const char * input) {
  if (input != nullptr) {
    Ion::Console::writeLine(sSyntaxError);
    return;
  }
  Ion::Console::writeLine("PONG");
}

void command_mcu_serial(const char * input) {
  if (input != nullptr) {
    Ion::Console::writeLine(sSyntaxError);
    return;
  }
  char response[11+24+1] = {'M', 'C', 'U', '_', 'S', 'E', 'R', 'I', 'A', 'L', '=', 0};
  strlcpy(response+11, Ion::serialNumber(), 25);
  Ion::Console::writeLine(response);
}

static inline int8_t hexChar(char c) {
  if (c >= '0' && c <= '9') {
    return (c - '0');
  }
  if (c >= 'A' && c <= 'F') {
    return (c - 'A') + 0xA;
  }
  return -1;
}
static inline bool isHex(char c) { return hexChar(c) >= 0; }
static inline uint32_t hexNumber(const char * s) {
  uint32_t result = 0;
  while (*s != NULL) {
    result = (result << 4) | hexChar(*s++);
  }
  return result;
}

void command_led(const char * input) {
  // Input must be of the form "0xAABBCC" or "ON" or "OFF"
  if (strcmp(input, sON) == 0) {
    Ion::LED::Device::init();
    Ion::Console::writeLine(sOK);
    return;
  }
  if (strcmp(input, sOFF) == 0) {
    Ion::LED::Device::shutdown();
    Ion::Console::writeLine(sOK);
    return;
  }
  if (input == nullptr || input[0] != '0' || input[1] != 'x' || !isHex(input[2]) ||!isHex(input[3]) || !isHex(input[4]) || !isHex(input[5]) || !isHex(input[6]) || !isHex(input[7]) || input[8] != NULL) {
    Ion::Console::writeLine(sSyntaxError);
    return;
  }
  uint32_t hexColor = hexNumber(input+2);
  KDColor ledColor = KDColor::RGB24(hexColor);
  Ion::LED::setColor(ledColor);
  Ion::Console::writeLine(sOK);
}

void command_display(const char * input) {
  if (strcmp(input, sON) == 0) {
    Ion::Display::Device::init();
    Ion::Console::writeLine(sOK);
    return;
  }
  if (strcmp(input, sOFF) == 0) {
    Ion::Display::Device::shutdown();
    Ion::Console::writeLine(sOK);
    return;
  }
  Ion::Console::writeLine(sSyntaxError);
}

void command_backlight(const char * input) {
  // Input must be of the form "0xAA" or "ON" or "OFF"
  if (strcmp(input, sON) == 0) {
    Ion::Backlight::Device::init();
    Ion::Console::writeLine(sOK);
    return;
  }
  if (strcmp(input, sOFF) == 0) {
    Ion::Backlight::Device::shutdown();
    Ion::Console::writeLine(sOK);
    return;
  }
  if (input == nullptr || input[0] != '0' || input[1] != 'x' || !isHex(input[2]) ||!isHex(input[3]) || input[4] != NULL) {
    Ion::Console::writeLine(sSyntaxError);
    return;
  }
  uint32_t brightness = hexNumber(input+2);
  Ion::Backlight::setBrightness(brightness);
  Ion::Console::writeLine(sOK);
}

void command_adc(const char * input) {
  if (input != nullptr) {
    Ion::Console::writeLine(sSyntaxError);
    return;
  }
  float result = Ion::Battery::voltage();
  constexpr int precision = 8;
  constexpr int bufferSize = Poincare::Complex::bufferSizeForFloatsWithPrecision(precision);
  char responseBuffer[bufferSize+4] = {'A', 'D', 'C', '='}; // ADC=
  Poincare::Complex::convertFloatToText(result, responseBuffer+4, bufferSize, precision);
  Ion::Console::writeLine(responseBuffer);
}

void command_charge(const char * input) {
  if (input != nullptr) {
    Ion::Console::writeLine(sSyntaxError);
    return;
  }
  if (Ion::Battery::isCharging()) {
    Ion::Console::writeLine("CHARGE=ON");
  } else {
    Ion::Console::writeLine("CHARGE=OFF");
  }
}

void command_keyboard(const char * input) {
  if (input != nullptr) {
    Ion::Console::writeLine(sSyntaxError);
    return;
  }
  char result[9+Ion::Keyboard::NumberOfKeys+1] = { 'K', 'E', 'Y', 'B', 'O', 'A', 'R', 'D', '=' };
  for (uint8_t i=0; i<Ion::Keyboard::NumberOfKeys; i++) {
    result[9+i] = Ion::Keyboard::keyDown((Ion::Keyboard::Key)i) ? '1' : '0';
  }
  result[9+Ion::Keyboard::NumberOfKeys] = 0;
  Ion::Console::writeLine(result);
}

constexpr CommandHandler handles[] = {
  CommandHandler("PING", command_ping),
  CommandHandler("MCU_SERIAL", command_mcu_serial),
  CommandHandler("LED", command_led),
  CommandHandler("BACKLIGHT", command_backlight),
  CommandHandler("ADC", command_adc),
  CommandHandler("CHARGE", command_charge),
  CommandHandler("KEYBOARD", command_keyboard),
  CommandHandler("DISPLAY", command_display),
  CommandHandler(nullptr, nullptr)
};
constexpr const CommandList sCommandList = CommandList(handles);

constexpr int kMaxCommandLength = 255;

void ion_app() {
  char command[kMaxCommandLength];
  while (true) {
    Ion::Console::readLine(command, kMaxCommandLength);
    sCommandList.dispatch(command);
  }
}
