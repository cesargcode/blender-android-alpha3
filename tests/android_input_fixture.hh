// Host-only Android/GHOST interface doubles. No modifier logic lives here.
// Android values: android/keycodes.h and android/input.h, matching KeyEvent API:
// https://developer.android.com/ndk/reference/group/input
// https://developer.android.com/reference/android/view/KeyEvent
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// Android's math.h exposes this standard constant; Windows C++17 need not.
#ifndef M_PI_2
static constexpr double M_PI_2 = 1.57079632679489661923;
#endif

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

enum {
  AINPUT_EVENT_TYPE_KEY = 1, AINPUT_EVENT_TYPE_MOTION = 2,
  AMOTION_EVENT_ACTION_MASK = 0xff,
  AMOTION_EVENT_ACTION_DOWN = 0, AMOTION_EVENT_ACTION_UP = 1,
  AMOTION_EVENT_ACTION_MOVE = 2, AMOTION_EVENT_ACTION_CANCEL = 3,
  AMOTION_EVENT_ACTION_POINTER_DOWN = 5, AMOTION_EVENT_ACTION_POINTER_UP = 6,
  AMOTION_EVENT_ACTION_HOVER_MOVE = 7, AMOTION_EVENT_ACTION_SCROLL = 8,
  AMOTION_EVENT_ACTION_HOVER_ENTER = 9, AMOTION_EVENT_ACTION_HOVER_EXIT = 10,
  AMOTION_EVENT_ACTION_BUTTON_PRESS = 11, AMOTION_EVENT_ACTION_BUTTON_RELEASE = 12,
  AMOTION_EVENT_TOOL_TYPE_UNKNOWN = 0, AMOTION_EVENT_TOOL_TYPE_FINGER = 1,
  AMOTION_EVENT_TOOL_TYPE_STYLUS = 2, AMOTION_EVENT_TOOL_TYPE_MOUSE = 3,
  AMOTION_EVENT_TOOL_TYPE_ERASER = 4,
  AMOTION_EVENT_BUTTON_PRIMARY = 1, AMOTION_EVENT_BUTTON_SECONDARY = 2,
  AMOTION_EVENT_BUTTON_TERTIARY = 4, AMOTION_EVENT_BUTTON_BACK = 8,
  AMOTION_EVENT_BUTTON_FORWARD = 16, AMOTION_EVENT_BUTTON_STYLUS_PRIMARY = 32,
  AMOTION_EVENT_BUTTON_STYLUS_SECONDARY = 64,
  AMOTION_EVENT_AXIS_ORIENTATION = 8, AMOTION_EVENT_AXIS_VSCROLL = 9,
  AMOTION_EVENT_AXIS_HSCROLL = 10, AMOTION_EVENT_AXIS_TILT = 25,
};
struct TestMotionPointer {
  int32_t tool = AMOTION_EVENT_TOOL_TYPE_STYLUS;
  float x = 0, y = 0, pressure = 0, tilt = 0, orientation = 0;
};
struct AInputEvent {
  int32_t keycode;
  int32_t action;
  int32_t meta_state;
  int32_t repeat = 0;
  int32_t device_id = 4;
  int32_t type = AINPUT_EVENT_TYPE_KEY;
  int32_t button_state = 0;
  std::vector<TestMotionPointer> pointers{};
  float vscroll = 0, hscroll = 0;
};
static int32_t AKeyEvent_getKeyCode(const AInputEvent *e) { return e->keycode; }
static int32_t AKeyEvent_getAction(const AInputEvent *e) { return e->action; }
static int32_t AKeyEvent_getMetaState(const AInputEvent *e) { return e->meta_state; }
static int32_t AKeyEvent_getRepeatCount(const AInputEvent *e) { return e->repeat; }
static int32_t AInputEvent_getDeviceId(const AInputEvent *e) { return e->device_id; }
static int32_t AInputEvent_getType(const AInputEvent *e) { return e->type; }
static int32_t AMotionEvent_getAction(const AInputEvent *e) { return e->action; }
static size_t AMotionEvent_getPointerCount(const AInputEvent *e) { return e->pointers.size(); }
static int32_t AMotionEvent_getToolType(const AInputEvent *e, size_t i) { return e->pointers.at(i).tool; }
static int32_t AMotionEvent_getButtonState(const AInputEvent *e) { return e->button_state; }
static float AMotionEvent_getX(const AInputEvent *e, size_t i) { return e->pointers.at(i).x; }
static float AMotionEvent_getY(const AInputEvent *e, size_t i) { return e->pointers.at(i).y; }
static float AMotionEvent_getPressure(const AInputEvent *e, size_t i) { return e->pointers.at(i).pressure; }
static float AMotionEvent_getAxisValue(const AInputEvent *e, int32_t axis, size_t i) {
  switch (axis) {
    case AMOTION_EVENT_AXIS_TILT: return e->pointers.at(i).tilt;
    case AMOTION_EVENT_AXIS_ORIENTATION: return e->pointers.at(i).orientation;
    case AMOTION_EVENT_AXIS_VSCROLL: return e->vscroll;
    case AMOTION_EVENT_AXIS_HSCROLL: return e->hscroll;
    default: return 0;
  }
}
// Display scaling and Android device classification are external boundaries.
static float ghost_android_scale_input(float coordinate) { return coordinate; }

// JNI/Android character-map boundary only: tests provide explicit layout responses.
// Character selection is never synthesized from a US keyboard table here or in production.
struct android_app {};
static std::vector<int32_t> test_wacom_device_ids;
static int test_device_queries = 0;
static bool android_is_wacom_intuos_s(android_app *, int32_t device_id) {
  ++test_device_queries;
  for (int32_t expected : test_wacom_device_ids) {
    if (expected == device_id) return true;
  }
  return false;
}
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
  GHOST_kEventCursorMove,
  GHOST_kEventWheel,
  GHOST_kEventTrackpad,
};
enum GHOST_TButton {
  GHOST_kButtonMaskLeft = 1,
  GHOST_kButtonMaskMiddle = 2,
  GHOST_kButtonMaskRight = 4,
  GHOST_kButtonMaskButton4 = 8,
  GHOST_kButtonMaskButton5 = 16,
};
// Tablet mode, payload and NONE constant are extracted from the real GHOST header.
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
  int32_t x = 0, y = 0, delta_x = 0, delta_y = 0;
  GHOST_TabletData tablet = GHOST_TABLET_DATA_NONE;
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
                   GHOST_TabletData tablet_data)
      : GHOST_Event(t, kind, w) {
    button = mask;
    tablet = tablet_data;
  }
};
struct GHOST_EventCursor : GHOST_Event {
  GHOST_EventCursor(uint64_t t, GHOST_TEventType kind, GHOST_WindowAndroid *w,
                   int32_t px, int32_t py, const GHOST_TabletData &data)
      : GHOST_Event(t, kind, w) { x = px; y = py; tablet = data; }
};
enum GHOST_TEventWheelAxis { GHOST_kEventWheelAxisVertical, GHOST_kEventWheelAxisHorizontal };
struct GHOST_EventWheel : GHOST_Event {
  GHOST_EventWheel(uint64_t t, GHOST_WindowAndroid *w, GHOST_TEventWheelAxis, int32_t value)
      : GHOST_Event(t, GHOST_kEventWheel, w) { delta_y = value; }
};
enum GHOST_TTrackpadEventSubTypes { GHOST_kTrackpadEventScroll, GHOST_kTrackpadEventMagnify };
struct GHOST_EventTrackpad : GHOST_Event {
  GHOST_EventTrackpad(uint64_t t, GHOST_WindowAndroid *w, GHOST_TTrackpadEventSubTypes,
                     int32_t px, int32_t py, int32_t dx, int32_t dy, bool)
      : GHOST_Event(t, GHOST_kEventTrackpad, w) {
    x = px; y = py; delta_x = dx; delta_y = dy;
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
