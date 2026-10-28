// Host-only Android/GHOST interface doubles. No modifier logic lives here.
// Android values: android/keycodes.h and android/input.h, matching KeyEvent API:
// https://developer.android.com/ndk/reference/group/input
// https://developer.android.com/reference/android/view/KeyEvent
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

enum {
  AKEY_EVENT_ACTION_DOWN = 0,
  AKEY_EVENT_ACTION_UP = 1,
  AKEY_EVENT_ACTION_MULTIPLE = 2,
  AKEYCODE_UNKNOWN = 0,
  AKEYCODE_BACK = 4,
  AKEYCODE_0 = 7,
  AKEYCODE_9 = 16,
  AKEYCODE_DPAD_UP = 19,
  AKEYCODE_DPAD_DOWN = 20,
  AKEYCODE_DPAD_LEFT = 21,
  AKEYCODE_DPAD_RIGHT = 22,
  AKEYCODE_A = 29,
  AKEYCODE_Z = 54,
  AKEYCODE_COMMA = 55,
  AKEYCODE_PERIOD = 56,
  AKEYCODE_ALT_LEFT = 57,
  AKEYCODE_ALT_RIGHT = 58,
  AKEYCODE_SHIFT_LEFT = 59,
  AKEYCODE_SHIFT_RIGHT = 60,
  AKEYCODE_TAB = 61,
  AKEYCODE_SPACE = 62,
  AKEYCODE_ENTER = 66,
  AKEYCODE_DEL = 67,
  AKEYCODE_GRAVE = 68,
  AKEYCODE_MINUS = 69,
  AKEYCODE_EQUALS = 70,
  AKEYCODE_LEFT_BRACKET = 71,
  AKEYCODE_RIGHT_BRACKET = 72,
  AKEYCODE_BACKSLASH = 73,
  AKEYCODE_SEMICOLON = 74,
  AKEYCODE_APOSTROPHE = 75,
  AKEYCODE_SLASH = 76,
  AKEYCODE_PLUS = 81,
  AKEYCODE_ESCAPE = 111,
  AKEYCODE_FORWARD_DEL = 112,
  AKEYCODE_CTRL_LEFT = 113,
  AKEYCODE_CTRL_RIGHT = 114,
  AKEYCODE_META_LEFT = 117,
  AKEYCODE_META_RIGHT = 118,
  AKEYCODE_MOVE_HOME = 122,
  AKEYCODE_MOVE_END = 123,
  AKEYCODE_F1 = 131,
  AKEYCODE_F12 = 142,
  AKEYCODE_NUMPAD_0 = 144,
  AKEYCODE_NUMPAD_9 = 153,
  AKEYCODE_NUMPAD_DIVIDE = 154,
  AKEYCODE_NUMPAD_MULTIPLY = 155,
  AKEYCODE_NUMPAD_SUBTRACT = 156,
  AKEYCODE_NUMPAD_ADD = 157,
  AKEYCODE_NUMPAD_DOT = 158,
  AKEYCODE_NUMPAD_COMMA = 159,
  AKEYCODE_NUMPAD_ENTER = 160,
  AKEYCODE_NUMPAD_EQUALS = 161,
};

enum {
  AMETA_NONE = 0,
  AMETA_SHIFT_ON = 0x1,
  AMETA_ALT_ON = 0x2,
  AMETA_ALT_LEFT_ON = 0x10,
  AMETA_ALT_RIGHT_ON = 0x20,
  AMETA_SHIFT_LEFT_ON = 0x40,
  AMETA_SHIFT_RIGHT_ON = 0x80,
  AMETA_CTRL_ON = 0x1000,
  AMETA_CTRL_LEFT_ON = 0x2000,
  AMETA_CTRL_RIGHT_ON = 0x4000,
  AMETA_META_ON = 0x10000,
  AMETA_META_LEFT_ON = 0x20000,
  AMETA_META_RIGHT_ON = 0x40000,
  AMETA_CAPS_LOCK_ON = 0x100000,
  AMETA_NUM_LOCK_ON = 0x200000,
};

struct AInputEvent {
  int32_t keycode;
  int32_t action;
  int32_t meta_state;
  int32_t repeat = 0;
  int32_t device_id = 4;
};
static int32_t AKeyEvent_getKeyCode(const AInputEvent *e) { return e->keycode; }
static int32_t AKeyEvent_getAction(const AInputEvent *e) { return e->action; }
static int32_t AKeyEvent_getMetaState(const AInputEvent *e) { return e->meta_state; }
static int32_t AKeyEvent_getRepeatCount(const AInputEvent *e) { return e->repeat; }
static int32_t AInputEvent_getDeviceId(const AInputEvent *e) { return e->device_id; }

// JNI/Android character-map boundary only: tests provide explicit layout responses.
// Character selection is never synthesized from a US keyboard table here or in production.
struct android_app {};
struct TestKeyCharacter {
  int32_t device_id, keycode, meta_state, unicode;
};
static std::vector<TestKeyCharacter> test_key_characters;
static int unicode_lookup_count = 0;
static int32_t android_key_unicode(android_app *, int32_t device, int32_t key, int32_t meta) {
  ++unicode_lookup_count;
  for (const auto &entry : test_key_characters) {
    if (entry.device_id == device && entry.keycode == key && entry.meta_state == meta) {
      return entry.unicode;
    }
  }
  return 0;
}
struct TestAccentComposition { int32_t accent, character, composed; };
static std::vector<TestAccentComposition> test_accent_compositions;
static int32_t android_compose_key_accent(android_app *, int32_t accent, int32_t character) {
  for (const auto &entry : test_accent_compositions) {
    if (entry.accent == accent && entry.character == character) return entry.composed;
  }
  return 0;
}

// Only identity and contiguous A-Z/0-9 arithmetic matter to the functions tested.
// These host enum values deliberately make no ABI or Android build claim.
enum GHOST_TKey {
  GHOST_kKeyUnknown = 0,
  GHOST_kKeyA = 100,
  GHOST_kKeyZ = GHOST_kKeyA + 25,
  GHOST_kKey0 = 200,
  GHOST_kKey9 = GHOST_kKey0 + 9,
  GHOST_kKeySpace = 300,
  GHOST_kKeyEnter,
  GHOST_kKeyBackSpace,
  GHOST_kKeyDelete,
  GHOST_kKeyTab,
  GHOST_kKeyEsc,
  GHOST_kKeyLeftArrow,
  GHOST_kKeyRightArrow,
  GHOST_kKeyUpArrow,
  GHOST_kKeyDownArrow,
  GHOST_kKeyLeftShift,
  GHOST_kKeyRightShift,
  GHOST_kKeyLeftControl,
  GHOST_kKeyRightControl,
  GHOST_kKeyLeftAlt,
  GHOST_kKeyRightAlt,
  GHOST_kKeyLeftOS,
  GHOST_kKeyRightOS,
  GHOST_kKeyComma,
  GHOST_kKeyPeriod,
  GHOST_kKeyAccentGrave,
  GHOST_kKeyMinus,
  GHOST_kKeyEqual,
  GHOST_kKeyLeftBracket,
  GHOST_kKeyRightBracket,
  GHOST_kKeyBackslash,
  GHOST_kKeySemicolon,
  GHOST_kKeyQuote,
  GHOST_kKeySlash,
  GHOST_kKeyPlus,
  GHOST_kKeyHome,
  GHOST_kKeyEnd,
  GHOST_kKeyNumpad0 = 400,
  GHOST_kKeyNumpad9 = GHOST_kKeyNumpad0 + 9,
  GHOST_kKeyNumpadPeriod,
  GHOST_kKeyNumpadEnter,
  GHOST_kKeyNumpadPlus,
  GHOST_kKeyNumpadMinus,
  GHOST_kKeyNumpadAsterisk,
  GHOST_kKeyNumpadSlash,
  GHOST_kKeyF1 = 500,
  GHOST_kKeyF12 = GHOST_kKeyF1 + 11,
};

enum GHOST_TModifierKey {
  GHOST_kModifierKeyLeftShift,
  GHOST_kModifierKeyRightShift,
  GHOST_kModifierKeyLeftControl,
  GHOST_kModifierKeyRightControl,
  GHOST_kModifierKeyLeftAlt,
  GHOST_kModifierKeyRightAlt,
  GHOST_kModifierKeyLeftOS,
  GHOST_kModifierKeyRightOS,
};

struct GHOST_ModifierKeys {
  std::array<bool, 8> state{};
  void set(GHOST_TModifierKey key, bool down) { state.at(key) = down; }
  bool get(GHOST_TModifierKey key) const { return state.at(key); }
  void clear() { state.fill(false); }
};

enum GHOST_TSuccess { GHOST_kFailure, GHOST_kSuccess };
enum GHOST_TEventType {
  GHOST_kEventKeyDown,
  GHOST_kEventKeyUp,
  GHOST_kEventWindowActivate,
  GHOST_kEventWindowDeactivate,
  GHOST_kEventButtonDown,
  GHOST_kEventButtonUp,
};
enum GHOST_TButton {
  GHOST_kButtonMaskLeft = 1,
  GHOST_kButtonMaskMiddle = 2,
  GHOST_kButtonMaskRight = 4,
  GHOST_kButtonMaskButton4 = 8,
  GHOST_kButtonMaskButton5 = 16,
};
struct GHOST_TabletData {};
static constexpr GHOST_TabletData GHOST_TABLET_DATA_NONE{};
struct GHOST_WindowAndroid {};
struct GHOST_Buttons {
  unsigned mask = 0;
  void set(GHOST_TButton button, bool down) {
    mask = down ? (mask | button) : (mask & ~unsigned(button));
  }
  void clear() { mask = 0; }
  bool get(GHOST_TButton button) const { return (mask & button) != 0; }
};

struct GHOST_Event {
  uint64_t time;
  GHOST_TEventType type;
  GHOST_WindowAndroid *window;
  GHOST_TKey key = GHOST_kKeyUnknown;
  bool repeat = false;
  GHOST_TButton button = GHOST_kButtonMaskLeft;
  std::string text;
  GHOST_Event(uint64_t t, GHOST_TEventType kind, GHOST_WindowAndroid *w)
      : time(t), type(kind), window(w) {}
  virtual ~GHOST_Event() = default;
};
struct GHOST_EventKey : GHOST_Event {
  GHOST_EventKey(uint64_t t,
                 GHOST_TEventType kind,
                 GHOST_WindowAndroid *w,
                 GHOST_TKey code,
                 bool repeated,
                 const char *utf8 = nullptr)
      : GHOST_Event(t, kind, w) {
    key = code;
    repeat = repeated;
    if (utf8) text = utf8;
  }
};
struct GHOST_EventButton : GHOST_Event {
  GHOST_EventButton(uint64_t t,
                   GHOST_TEventType kind,
                   GHOST_WindowAndroid *w,
                   GHOST_TButton mask,
                   GHOST_TabletData)
      : GHOST_Event(t, kind, w) {
    button = mask;
  }
};

static int checks = 0;
static void require(bool condition, const char *description) {
  ++checks;
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", description);
    std::exit(1);
  }
}
