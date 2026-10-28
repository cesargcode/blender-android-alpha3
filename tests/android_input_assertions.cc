static void send(GHOST_SystemAndroid &system, int key, int action, int meta, int repeat = 0) {
  AInputEvent event{key, action, meta, repeat};
  system.handleKeyEvent(&event);
}

static bool held(const GHOST_SystemAndroid &system, GHOST_TModifierKey side) {
  GHOST_ModifierKeys modifiers;
  require(system.getModifierKeys(modifiers) == GHOST_kSuccess, "modifier query succeeds");
  return modifiers.get(side);
}

static void expect_key(const GHOST_SystemAndroid &system,
                       size_t index,
                       GHOST_TKey key,
                       GHOST_TEventType type,
                       const char *description) {
  require(index < system.events.size(), description);
  require(system.events[index]->key == key && system.events[index]->type == type, description);
}

static void mapping_and_each_side() {
  struct Expected {
    int android_key;
    int side_meta;
    GHOST_TKey mapped;
    GHOST_TKey logical;
    GHOST_TModifierKey side;
    GHOST_TModifierKey other_side;
  };
  const Expected sides[] = {
      {AKEYCODE_SHIFT_LEFT, AMETA_SHIFT_LEFT_ON, GHOST_kKeyLeftShift,
       GHOST_kKeyLeftShift, GHOST_kModifierKeyLeftShift, GHOST_kModifierKeyRightShift},
      {AKEYCODE_SHIFT_RIGHT, AMETA_SHIFT_RIGHT_ON, GHOST_kKeyRightShift,
       GHOST_kKeyLeftShift, GHOST_kModifierKeyRightShift, GHOST_kModifierKeyLeftShift},
      {AKEYCODE_CTRL_LEFT, AMETA_CTRL_LEFT_ON, GHOST_kKeyLeftControl,
       GHOST_kKeyLeftControl, GHOST_kModifierKeyLeftControl, GHOST_kModifierKeyRightControl},
      {AKEYCODE_CTRL_RIGHT, AMETA_CTRL_RIGHT_ON, GHOST_kKeyRightControl,
       GHOST_kKeyLeftControl, GHOST_kModifierKeyRightControl, GHOST_kModifierKeyLeftControl},
      {AKEYCODE_ALT_LEFT, AMETA_ALT_LEFT_ON, GHOST_kKeyLeftAlt,
       GHOST_kKeyLeftAlt, GHOST_kModifierKeyLeftAlt, GHOST_kModifierKeyRightAlt},
      {AKEYCODE_ALT_RIGHT, AMETA_ALT_RIGHT_ON, GHOST_kKeyRightAlt,
       GHOST_kKeyLeftAlt, GHOST_kModifierKeyRightAlt, GHOST_kModifierKeyLeftAlt},
      {AKEYCODE_META_LEFT, AMETA_META_LEFT_ON, GHOST_kKeyLeftOS,
       GHOST_kKeyLeftOS, GHOST_kModifierKeyLeftOS, GHOST_kModifierKeyRightOS},
      {AKEYCODE_META_RIGHT, AMETA_META_RIGHT_ON, GHOST_kKeyRightOS,
       GHOST_kKeyLeftOS, GHOST_kModifierKeyRightOS, GHOST_kModifierKeyLeftOS},
  };
  for (const Expected &expected : sides) {
    require(convertAndroidKey(expected.android_key) == expected.mapped,
            "all eight Android modifier sides map to the correct GHOST key");
    GHOST_SystemAndroid system;
    // DOWN may carry the state from before the key went down.
    send(system, expected.android_key, AKEY_EVENT_ACTION_DOWN, 0);
    require(held(system, expected.side), "own DOWN corrects stale absent metadata");
    require(!held(system, expected.other_side), "one physical side never fabricates the other");
    require(system.events.size() == 1, "one logical DOWN per modifier group");
    expect_key(system, 0, expected.logical, GHOST_kEventKeyDown, "logical DOWN event");
    send(system, expected.android_key, AKEY_EVENT_ACTION_DOWN, expected.side_meta, 4);
    require(system.events.size() == 1, "modifier repeat does not duplicate DOWN");
    // UP may still contain the released key's bit.
    send(system, expected.android_key, AKEY_EVENT_ACTION_UP, expected.side_meta);
    require(!held(system, expected.side), "own UP corrects stale present metadata");
    require(system.events.size() == 2, "one logical UP per modifier group");
    expect_key(system, 1, expected.logical, GHOST_kEventKeyUp, "logical UP event");
  }
  std::puts("PASS: eight modifier sides, own-event stale metadata, repeats");
}

static void shift_a_order_and_inferred_modifiers() {
  GHOST_SystemAndroid system;
  send(system, AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_ON | AMETA_SHIFT_LEFT_ON);
  send(system, AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_ON | AMETA_SHIFT_LEFT_ON);
  send(system, AKEYCODE_A, AKEY_EVENT_ACTION_UP, AMETA_SHIFT_ON | AMETA_SHIFT_LEFT_ON);
  send(system, AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_UP, 0);
  require(system.events.size() == 4, "Shift+A has exactly modifier/key down/up events");
  expect_key(system, 0, GHOST_kKeyLeftShift, GHOST_kEventKeyDown, "Shift precedes A");
  expect_key(system, 1, GHOST_kKeyA, GHOST_kEventKeyDown, "A DOWN after Shift");
  expect_key(system, 2, GHOST_kKeyA, GHOST_kEventKeyUp, "A UP before Shift release");
  expect_key(system, 3, GHOST_kKeyLeftShift, GHOST_kEventKeyUp, "Shift releases last");

  GHOST_SystemAndroid inferred;
  send(inferred, AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_ON);
  require(inferred.events.size() == 2, "group-only metadata repairs a missed modifier DOWN");
  expect_key(inferred, 0, GHOST_kKeyLeftShift, GHOST_kEventKeyDown, "inferred modifier precedes key");
  expect_key(inferred, 1, GHOST_kKeyA, GHOST_kEventKeyDown, "key follows inferred modifier");
  send(inferred, AKEYCODE_A, AKEY_EVENT_ACTION_UP, 0);
  expect_key(inferred, 2, GHOST_kKeyLeftShift, GHOST_kEventKeyUp, "native zero metadata clears missed release");
  std::puts("PASS: Shift+A ordering and authoritative native snapshots");
}

static void two_sides_release() {
  const int keys[][2] = {{AKEYCODE_SHIFT_LEFT, AKEYCODE_SHIFT_RIGHT},
                         {AKEYCODE_CTRL_LEFT, AKEYCODE_CTRL_RIGHT},
                         {AKEYCODE_ALT_LEFT, AKEYCODE_ALT_RIGHT},
                         {AKEYCODE_META_LEFT, AKEYCODE_META_RIGHT}};
  const int metadata[][3] = {{AMETA_SHIFT_ON, AMETA_SHIFT_LEFT_ON, AMETA_SHIFT_RIGHT_ON},
                             {AMETA_CTRL_ON, AMETA_CTRL_LEFT_ON, AMETA_CTRL_RIGHT_ON},
                             {AMETA_ALT_ON, AMETA_ALT_LEFT_ON, AMETA_ALT_RIGHT_ON},
                             {AMETA_META_ON, AMETA_META_LEFT_ON, AMETA_META_RIGHT_ON}};
  for (int group = 0; group < 4; ++group) {
    for (int release_first = 0; release_first < 2; ++release_first) {
      GHOST_SystemAndroid system;
      const int both = metadata[group][0] | metadata[group][1] | metadata[group][2];
      send(system, keys[group][0], AKEY_EVENT_ACTION_DOWN, metadata[group][0] | metadata[group][1]);
      send(system, keys[group][1], AKEY_EVENT_ACTION_DOWN, both);
      require(system.events.size() == 1, "second side does not duplicate group DOWN");
      send(system, keys[group][release_first], AKEY_EVENT_ACTION_UP, both);
      require(system.events.size() == 1, "releasing first side does not release second side");
      const int remaining = 1 - release_first;
      send(system, keys[group][remaining], AKEY_EVENT_ACTION_UP,
           metadata[group][0] | metadata[group][remaining + 1]);
      require(system.events.size() == 2, "last side releases group exactly once");
      require(system.meta_state_ == 0, "both released sides leave no physical modifier");
    }
  }
  std::puts("PASS: left/right overlap and either release order for all groups");
}

static void touch_and_physical_ownership() {
  GHOST_SystemAndroid system;
  send(system, AKEYCODE_SHIFT_RIGHT, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_ON | AMETA_SHIFT_RIGHT_ON);
  system.touchSendShift(true);
  system.touchSendShift(false);
  require(system.events.size() == 1, "touch Shift release preserves physical Shift");
  require(held(system, GHOST_kModifierKeyRightShift), "right physical Shift remains held after gesture");
  send(system, AKEYCODE_SHIFT_RIGHT, AKEY_EVENT_ACTION_UP, 0);
  require(system.events.size() == 2, "physical release ends remaining Shift");

  GHOST_SystemAndroid reverse;
  reverse.touchSendShift(true);
  send(reverse, AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_LEFT_ON);
  send(reverse, AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_UP, 0);
  require(reverse.events.size() == 1, "physical release preserves touch-owned Shift");
  reverse.resetPhysicalModifiers();
  require(reverse.events.size() == 1, "physical reset preserves touch-owned Shift");
  require(held(reverse, GHOST_kModifierKeyLeftShift), "query includes synthetic Shift");
  reverse.touchSendShift(false);
  require(reverse.events.size() == 2, "last touch source emits Shift UP");
  std::puts("PASS: independent touch/physical ownership in both orders");
}

static void action_multiple_and_unknown() {
  GHOST_SystemAndroid system;
  send(system, AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_LEFT_ON);
  const size_t count = system.events.size();
  send(system, AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_MULTIPLE, 0);
  system.dispatchJavaKeyEvent(AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_MULTIPLE, 0);
  require(system.events.size() == count, "ACTION_MULTIPLE produces no native/Java fake keyup");
  require(held(system, GHOST_kModifierKeyLeftShift), "ACTION_MULTIPLE preserves held state");
  send(system, AKEYCODE_UNKNOWN, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_LEFT_ON);
  require(system.events.size() == count, "unknown key does not fabricate keyboard event");
  std::puts("PASS: ACTION_MULTIPLE and unknown keys");
}

static void fifo_disconnect_and_ime() {
  GHOST_SystemAndroid system;
  send(system, AKEYCODE_CTRL_LEFT, AKEY_EVENT_ACTION_DOWN, AMETA_CTRL_LEFT_ON);
  system.handleJavaKeyEvent(AKEYCODE_DEL, AKEY_EVENT_ACTION_DOWN, 0);
  system.handleJavaKeyEvent(AKEYCODE_DEL, AKEY_EVENT_ACTION_UP, 0);
  system.handleTextInput("c");
  system.handleKeyboardDisconnected();
  require(system.events.size() == 1, "Java callbacks enqueue without touching GHOST event queue");
  system.drainJavaInput();
  require(system.events.size() == 6, "FIFO contains delete pair, insertion pair, then disconnect release");
  expect_key(system, 1, GHOST_kKeyBackSpace, GHOST_kEventKeyDown, "IME Delete comes before replacement");
  expect_key(system, 2, GHOST_kKeyBackSpace, GHOST_kEventKeyUp, "IME Delete release precedes replacement");
  require(system.events[3]->text == "c", "replacement text retains original FIFO position");
  expect_key(system, 5, GHOST_kKeyLeftControl, GHOST_kEventKeyUp, "disconnect reset follows earlier Java keys");
  require(system.meta_state_ == 0, "disconnect clears physical state");
  system.drainJavaInput();
  require(system.events.size() == 6, "drain is idempotent once queue is empty");

  GHOST_SystemAndroid ime;
  send(ime, AKEYCODE_CTRL_RIGHT, AKEY_EVENT_ACTION_DOWN, AMETA_CTRL_RIGHT_ON);
  ime.dispatchJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, 0);
  require(held(ime, GHOST_kModifierKeyRightControl), "IME meta zero preserves physical Ctrl");
  require(ime.events.size() == 2, "IME ordinary key does not emit modifier release");
  ime.dispatchJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_UP, AMETA_CAPS_LOCK_ON);
  require(held(ime, GHOST_kModifierKeyRightControl), "IME Caps Lock metadata preserves physical Ctrl");
  require(ime.events.size() == 3, "IME lock metadata does not fabricate modifier releases");

  GHOST_SystemAndroid explicit_ime;
  send(explicit_ime, AKEYCODE_SHIFT_RIGHT, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_RIGHT_ON);
  explicit_ime.dispatchJavaKeyEvent(AKEYCODE_CTRL_LEFT, AKEY_EVENT_ACTION_DOWN, 0);
  require(held(explicit_ime, GHOST_kModifierKeyRightShift), "explicit JNI Ctrl DOWN preserves physical Shift");
  require(held(explicit_ime, GHOST_kModifierKeyLeftControl), "explicit JNI Ctrl DOWN sets own side");
  explicit_ime.dispatchJavaKeyEvent(AKEYCODE_CTRL_LEFT, AKEY_EVENT_ACTION_UP, AMETA_CTRL_LEFT_ON);
  require(held(explicit_ime, GHOST_kModifierKeyRightShift), "explicit JNI Ctrl UP preserves physical Shift");
  require(!held(explicit_ime, GHOST_kModifierKeyLeftControl), "explicit JNI Ctrl UP clears own stale bit");
  require(explicit_ime.events.size() == 3, "JNI modifier edge does not release unrelated group");
  std::puts("PASS: Java FIFO, physical disconnect reset, IME meta zero");
}

static void focus_reset() {
  GHOST_SystemAndroid system;
  send(system, AKEYCODE_CTRL_RIGHT, AKEY_EVENT_ACTION_DOWN, AMETA_CTRL_RIGHT_ON);
  system.touchSendShift(true);
  system.touch_pan_active_ = true;
  system.touchSendButton(GHOST_kButtonMaskMiddle, GHOST_kEventButtonDown);
  system.buttons_.set(GHOST_kButtonMaskRight, true);
  system.stylus_button_down_ = true;
  system.gesture_active_ = true;
  system.hover_exit_pending_ = true;
  system.handleJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, 0);
  system.handleWindowFocus(false);
  require(!system.input_focused_ && !system.java_input_enabled_, "focus loss disables native/Java input");
  require(system.java_input_.empty(), "focus loss discards pre-focus Java queue");
  require(system.meta_state_ == 0 && !system.touch_shift_active_, "focus loss clears physical and gesture modifiers");
  require(system.emitted_modifiers_ == 0, "focus loss emitted modifier releases");
  require(system.buttons_.mask == 0, "focus loss releases all held pointer buttons");
  require(!system.stylus_button_down_ && !system.gesture_active_ && !system.hover_exit_pending_,
          "focus loss clears transient touch/stylus state");
  require(system.events.back()->type == GHOST_kEventWindowDeactivate, "releases precede deactivate event");
  const size_t count = system.events.size();
  system.handleTextInput("ignored");
  system.handleJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, 0);
  system.handleKeyboardDisconnected();
  system.drainJavaInput();
  require(system.events.size() == count, "callbacks while unfocused cannot resurrect input");
  system.handleWindowFocus(true);
  system.handleTextInput("b");
  system.drainJavaInput();
  require(system.events.size() == count + 3, "focus regain accepts fresh input only");
  require(system.events[count]->type == GHOST_kEventWindowActivate, "activation precedes fresh key");
  require(system.events[count + 1]->text == "b", "fresh text appears after refocus");
  std::puts("PASS: focus loss releases state, drops stale queue, suppresses inactive callbacks");
}

int main() {
  mapping_and_each_side();
  shift_a_order_and_inferred_modifiers();
  two_sides_release();
  touch_and_physical_ownership();
  action_multiple_and_unknown();
  fifo_disconnect_and_ime();
  focus_reset();
  std::printf("PASS: %d assertions against extracted production C++ functions\n", checks);
  return 0;
}
