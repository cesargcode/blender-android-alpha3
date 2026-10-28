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
  system.dispatchJavaKeyEvent(AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_MULTIPLE, 0, 0, 0);
  require(system.events.size() == count, "ACTION_MULTIPLE produces no native/Java fake keyup");
  require(held(system, GHOST_kModifierKeyLeftShift), "ACTION_MULTIPLE preserves held state");
  send(system, AKEYCODE_UNKNOWN, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_LEFT_ON);
  require(system.events.size() == count, "unknown key does not fabricate keyboard event");
  std::puts("PASS: ACTION_MULTIPLE and unknown keys");
}

static void fifo_disconnect_and_ime() {
  GHOST_SystemAndroid system;
  send(system, AKEYCODE_CTRL_LEFT, AKEY_EVENT_ACTION_DOWN, AMETA_CTRL_LEFT_ON);
  system.handleJavaKeyEvent(AKEYCODE_DEL, AKEY_EVENT_ACTION_DOWN, 0, 0, 0);
  system.handleJavaKeyEvent(AKEYCODE_DEL, AKEY_EVENT_ACTION_UP, 0, 0, 0);
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
  ime.dispatchJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, 0, 0, 0);
  require(held(ime, GHOST_kModifierKeyRightControl), "IME meta zero preserves physical Ctrl");
  require(ime.events.size() == 2, "IME ordinary key does not emit modifier release");
  ime.dispatchJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_UP, AMETA_CAPS_LOCK_ON, 0, 0);
  require(held(ime, GHOST_kModifierKeyRightControl), "IME Caps Lock metadata preserves physical Ctrl");
  require(ime.events.size() == 3, "IME lock metadata does not fabricate modifier releases");

  GHOST_SystemAndroid explicit_ime;
  send(explicit_ime, AKEYCODE_SHIFT_RIGHT, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_RIGHT_ON);
  explicit_ime.dispatchJavaKeyEvent(AKEYCODE_CTRL_LEFT, AKEY_EVENT_ACTION_DOWN, 0, 0, 0);
  require(held(explicit_ime, GHOST_kModifierKeyRightShift), "explicit JNI Ctrl DOWN preserves physical Shift");
  require(held(explicit_ime, GHOST_kModifierKeyLeftControl), "explicit JNI Ctrl DOWN sets own side");
  explicit_ime.dispatchJavaKeyEvent(AKEYCODE_CTRL_LEFT, AKEY_EVENT_ACTION_UP, AMETA_CTRL_LEFT_ON, 0, 0);
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
  system.handleJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, 0, 0, 0);
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
  system.handleJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, 0, 0, 0);
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

static void send_character(GHOST_SystemAndroid &system, int key, int unicode,
                           int meta = 0, int repeat = 0, int device = 4) {
  test_key_characters = {{device, key, meta, unicode}};
  AInputEvent event{key, AKEY_EVENT_ACTION_DOWN, meta, repeat, device};
  require(system.handleKeyEvent(&event) == 1, "character key-down is handled");
}

static void physical_text_and_numeric_keys() {
  for (int digit = 0; digit <= 9; ++digit) {
    for (bool numpad : {false, true}) {
      GHOST_SystemAndroid system;
      const int android_key = (numpad ? AKEYCODE_NUMPAD_0 : AKEYCODE_0) + digit;
      const GHOST_TKey ghost_key = GHOST_TKey((numpad ? GHOST_kKeyNumpad0 : GHOST_kKey0) + digit);
      const int meta = numpad ? AMETA_NUM_LOCK_ON : 0;
      send_character(system, android_key, '0' + digit, meta);
      expect_key(system, 0, ghost_key, GHOST_kEventKeyDown, "digits keep row/numpad key identity");
      require(system.events[0]->text == std::string(1, char('0' + digit)),
              "all digits carry text on their original key-down");
      const int lookups = unicode_lookup_count;
      send(system, android_key, AKEY_EVENT_ACTION_UP, meta, 7);
      require(system.events.size() == 2, "digit produces one press/release pair without duplicate text");
      expect_key(system, 1, ghost_key, GHOST_kEventKeyUp, "digit key-up keeps original identity");
      require(system.events[1]->text.empty() && !system.events[1]->repeat,
              "key-up has neither text nor repeat flag");
      require(unicode_lookup_count == lookups, "key-up never queries the Android layout");
    }
  }
  struct Symbol { int android_key; GHOST_TKey key; char text; };
  const Symbol symbols[] = {
      {AKEYCODE_MINUS, GHOST_kKeyMinus, '-'},
      {AKEYCODE_PLUS, GHOST_kKeyPlus, '+'},
      {AKEYCODE_PERIOD, GHOST_kKeyPeriod, '.'},
      {AKEYCODE_COMMA, GHOST_kKeyComma, ','},
      {AKEYCODE_EQUALS, GHOST_kKeyEqual, '='},
      {AKEYCODE_LEFT_BRACKET, GHOST_kKeyLeftBracket, '['},
      {AKEYCODE_RIGHT_BRACKET, GHOST_kKeyRightBracket, ']'},
      {AKEYCODE_BACKSLASH, GHOST_kKeyBackslash, '\\'},
      {AKEYCODE_SLASH, GHOST_kKeySlash, '/'},
      {AKEYCODE_SEMICOLON, GHOST_kKeySemicolon, ';'},
      {AKEYCODE_APOSTROPHE, GHOST_kKeyQuote, '\''},
      {AKEYCODE_GRAVE, GHOST_kKeyAccentGrave, '`'},
      {AKEYCODE_SPACE, GHOST_kKeySpace, ' '},
      {AKEYCODE_NUMPAD_DOT, GHOST_kKeyNumpadPeriod, '.'},
      {AKEYCODE_NUMPAD_COMMA, GHOST_kKeyNumpadPeriod, ','},
      {AKEYCODE_NUMPAD_ADD, GHOST_kKeyNumpadPlus, '+'},
      {AKEYCODE_NUMPAD_SUBTRACT, GHOST_kKeyNumpadMinus, '-'},
      {AKEYCODE_NUMPAD_MULTIPLY, GHOST_kKeyNumpadAsterisk, '*'},
      {AKEYCODE_NUMPAD_DIVIDE, GHOST_kKeyNumpadSlash, '/'},
      {AKEYCODE_NUMPAD_EQUALS, GHOST_kKeyEqual, '='},
  };
  for (const auto &symbol : symbols) {
    GHOST_SystemAndroid system;
    send_character(system, symbol.android_key, symbol.text, 0, 3);
    require(system.events.size() == 1, "symbol has one event carrying both key and text");
    expect_key(system, 0, symbol.key, GHOST_kEventKeyDown, "symbol key mapping");
    require(system.events[0]->text == std::string(1, symbol.text), "symbol text follows layout including decimal comma");
    require(system.events[0]->repeat, "held symbol preserves repeat flag");
  }
  for (int i = 0; i < 12; ++i) {
    require(convertAndroidKey(AKEYCODE_F1 + i) == GHOST_TKey(GHOST_kKeyF1 + i),
            "F1-F12 including F2 rename map to actual GHOST function keys");
  }
  require(convertAndroidKey(AKEYCODE_MOVE_HOME) == GHOST_kKeyHome, "Home supports editing");
  require(convertAndroidKey(AKEYCODE_MOVE_END) == GHOST_kKeyEnd, "End supports editing");
  const Symbol controls[] = {
      {AKEYCODE_ENTER, GHOST_kKeyEnter, '\n'},
      {AKEYCODE_NUMPAD_ENTER, GHOST_kKeyNumpadEnter, '\n'},
      {AKEYCODE_DEL, GHOST_kKeyBackSpace, '\b'},
  };
  for (const auto &control : controls) {
    GHOST_SystemAndroid system;
    send_character(system, control.android_key, control.text, 0, 2);
    expect_key(system, 0, control.key, GHOST_kEventKeyDown, "editing commands retain their keycode");
    require(system.events[0]->text.empty() && system.events[0]->repeat,
            "Enter/Backspace remain repeated commands without control-character insertion");
  }
  std::puts("PASS: row/numpad digits, operators, decimal comma, F2, navigation, commands, repeats");
}

static void unicode_layout_and_shortcuts() {
  struct Character { int unicode; const char *utf8; };
  const Character characters[] = {{'A', "A"}, {0xe7, "\xc3\xa7"},
                                  {0x20ac, "\xe2\x82\xac"},
                                  {0x1f642, "\xf0\x9f\x99\x82"}};
  for (const auto &character : characters) {
    GHOST_SystemAndroid system;
    // Deliberately use the same Android key for different layout responses.
    send_character(system, AKEYCODE_A, character.unicode, AMETA_CAPS_LOCK_ON);
    require(system.events.size() == 1 && system.events[0]->key == GHOST_kKeyA,
            "layout Unicode does not replace the physical key identity");
    require(system.events[0]->text == character.utf8, "one-to-four-byte Unicode follows Android layout and lock metadata");
  }
  const int invalid[] = {0, 1, 9, 10, 13, 0x7f, 0x85, 0x9f, 0xd800, 0xdfff, 0x110000, -1};
  for (int unicode : invalid) {
    char utf8[6] = {'x', 'x', 'x', 'x', 'x', 'x'};
    require(!android_unicode_to_utf8(unicode, utf8), "controls, surrogates and non-scalars are rejected");
    require(std::string(utf8).empty() && utf8[5] == 0, "rejected Unicode clears entire GHOST buffer");
  }
  char upper[6];
  require(android_unicode_to_utf8(0x10ffff, upper) && std::string(upper) == "\xf4\x8f\xbf\xbf",
          "highest Unicode scalar uses four bytes and a terminator");
  for (int meta : {AMETA_CTRL_ON, AMETA_CTRL_LEFT_ON, AMETA_CTRL_RIGHT_ON,
                   AMETA_META_ON, AMETA_META_LEFT_ON, AMETA_META_RIGHT_ON}) {
    GHOST_SystemAndroid system;
    send_character(system, AKEYCODE_A, 'a', meta);
    require(system.events.back()->text.empty(), "Ctrl/Meta shortcuts suppress printable layout results");
    require(system.events.back()->key == GHOST_kKeyA, "shortcut key itself remains available");
  }
  GHOST_SystemAndroid shifted;
  send_character(shifted, AKEYCODE_A, 'A', AMETA_SHIFT_ON | AMETA_SHIFT_LEFT_ON);
  require(shifted.events.size() == 2 && shifted.events[1]->text == "A", "Shift precedes uppercase text without duplication");
  GHOST_SystemAndroid altgr;
  send_character(altgr, AKEYCODE_A, 0x20ac, AMETA_CTRL_ON | AMETA_ALT_ON | AMETA_ALT_RIGHT_ON);
  require(altgr.events.back()->text == "\xe2\x82\xac", "right AltGr can provide layout text with Ctrl metadata");
  std::puts("PASS: Unicode 1-4 bytes, layout/lock metadata, invalid scalars, Ctrl/Meta, Shift and AltGr");
}

static void dead_keys_and_java_unicode_fifo() {
  const int dead_acute = int32_t(0x80000000u | 0xb4u);
  test_accent_compositions = {{0xb4, 'a', 0xe1}, {0xb4, ' ', 0xb4}, {0xb4, 0xb4, 0xb4}};
  GHOST_SystemAndroid system;
  send_character(system, AKEYCODE_APOSTROPHE, dead_acute);
  send(system, AKEYCODE_APOSTROPHE, AKEY_EVENT_ACTION_UP, 0);
  require(system.events[0]->text.empty() && system.events[1]->text.empty(), "dead key emits no premature accent text");
  send_character(system, AKEYCODE_A, 'a');
  require(system.events.size() == 3 && system.events.back()->text == "\xc3\xa1",
          "accent composes once on following original key-down");
  require(system.events.back()->key == GHOST_kKeyA && system.pending_accent_ == 0,
          "composed character preserves key identity and consumes accent");

  GHOST_SystemAndroid fallback;
  send_character(fallback, AKEYCODE_APOSTROPHE, dead_acute);
  send_character(fallback, AKEYCODE_A + 16, 'q');
  require(fallback.events.size() == 4, "uncomposable accent adds its own text pair before current key");
  require(fallback.events[1]->text == "\xc2\xb4" && fallback.events[2]->text.empty(),
          "uncomposable accent is preserved with no key-up text");
  require(fallback.events[1]->key == GHOST_kKeyUnknown && fallback.events[2]->key == GHOST_kKeyUnknown,
          "delayed accent does not invent an unrelated shortcut key");
  require(fallback.events[3]->text == "q" && fallback.events[3]->key == GHOST_TKey(GHOST_kKeyA + 16),
          "fallback character keeps text and keycode together");

  for (int character : {int(' '), 0xb4}) {
    GHOST_SystemAndroid spacing;
    send_character(spacing, AKEYCODE_APOSTROPHE, dead_acute);
    send_character(spacing, character == ' ' ? AKEYCODE_SPACE : AKEYCODE_APOSTROPHE,
                   character == ' ' ? character : dead_acute);
    require(spacing.events.back()->text == "\xc2\xb4" && spacing.pending_accent_ == 0,
            "space or repeated dead key can produce its spacing accent");
  }

  GHOST_SystemAndroid devices;
  send_character(devices, AKEYCODE_APOSTROPHE, dead_acute, 0, 0, 4);
  send_character(devices, AKEYCODE_A, 'a', 0, 0, 9);
  require(devices.events.back()->text == "a", "dead accent never crosses physical keyboard devices");
  send_character(devices, AKEYCODE_APOSTROPHE, dead_acute);
  devices.handleKeyboardDisconnected();
  devices.drainJavaInput();
  require(devices.pending_accent_ == 0, "keyboard disconnect clears pending dead key");
  send_character(devices, AKEYCODE_APOSTROPHE, dead_acute);
  devices.handleWindowFocus(false);
  require(devices.pending_accent_ == 0, "focus loss clears pending dead key");

  GHOST_SystemAndroid java;
  java.handleJavaKeyEvent(AKEYCODE_DEL, AKEY_EVENT_ACTION_DOWN, 0, 8, 1);
  java.handleJavaKeyEvent(AKEYCODE_DEL, AKEY_EVENT_ACTION_UP, 0, 8, 1);
  java.handleJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, 0, 0xe7, 2);
  java.handleJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_UP, 0, 0xe7, 2);
  java.handleTextInput("b");
  require(java.events.empty(), "Java Unicode/repeat callbacks remain queued on UI thread");
  java.drainJavaInput();
  require(java.events.size() == 6, "Java Unicode retains FIFO with Backspace and committed text");
  expect_key(java, 0, GHOST_kKeyBackSpace, GHOST_kEventKeyDown, "Java delete precedes Unicode replacement");
  require(java.events[2]->text == "\xc3\xa7" && java.events[2]->repeat, "Java Unicode and repeat reach same key-down");
  require(java.events[3]->text.empty() && !java.events[3]->repeat, "Java key-up drops supplied Unicode/repeat");
  require(java.events[4]->text == "b", "subsequent committed text retains FIFO order");
  java.handleJavaKeyEvent(AKEYCODE_APOSTROPHE, AKEY_EVENT_ACTION_DOWN, 0, dead_acute, 0);
  java.handleJavaKeyEvent(AKEYCODE_A, AKEY_EVENT_ACTION_DOWN, 0, 'a', 0);
  java.drainJavaInput();
  require(java.events.back()->text == "\xc3\xa1", "InputConnection dead keys use same composition path");
  java.handleJavaKeyEvent(AKEYCODE_APOSTROPHE, AKEY_EVENT_ACTION_DOWN, 0, dead_acute, 0);
  java.handleTextInput("c");
  java.drainJavaInput();
  require(java.pending_accent_ == 0, "IME committed text ends previous key composition");
  std::puts("PASS: dead-key composition/fallback/device/reset and Java Unicode/repeat FIFO");
}

int main() {
  require(wm_text_mapping_test::unknown_key_accepts_text(),
          "real GHOST Unknown mapping is a WM keyboard event, not discarded EVENT_NONE");
  std::puts("PASS: actual GHOST/WM enums, mapping and ISKEYBOARD preserve Unknown text events");
  mapping_and_each_side();
  shift_a_order_and_inferred_modifiers();
  two_sides_release();
  touch_and_physical_ownership();
  action_multiple_and_unknown();
  fifo_disconnect_and_ime();
  focus_reset();
  physical_text_and_numeric_keys();
  unicode_layout_and_shortcuts();
  dead_keys_and_java_unicode_fifo();
  std::printf("PASS: %d assertions against extracted production C++ functions\n", checks);
  return 0;
}
