// Class/interface fixture only. All methods declaring the behavior under test
// are defined by extracted production code in the generated translation unit.
class GHOST_SystemAndroid {
 public:
  GHOST_WindowAndroid test_window_;
  GHOST_WindowAndroid *window_ = &test_window_;
  android_app test_app_;
  android_app *app_ = &test_app_;
  std::vector<std::unique_ptr<GHOST_Event>> events;
  uint64_t test_time_ms = 1234;
  uint64_t getMilliSeconds() const { return test_time_ms; }
  void pushEvent(std::unique_ptr<GHOST_Event> event) { events.push_back(std::move(event)); }

  int32_t meta_state_ = 0;
  int32_t emitted_modifiers_ = 0;
  int32_t pending_accent_ = 0;
  int32_t pending_accent_device_ = 0;
  bool input_focused_ = true;
  bool touch_shift_active_ = false;
  bool touch_pan_active_ = false;
  bool touch_pending_ = false;
  bool touch_button_down_ = false;
  bool stylus_button_down_ = false;
  bool stylus_middle_button_down_ = false;
  bool gesture_active_ = false;
  bool hover_exit_pending_ = false;
  GHOST_TButton touch_button_ = GHOST_kButtonMaskLeft;
  GHOST_TabletData touch_tablet_ = GHOST_TABLET_DATA_NONE;
  GHOST_Buttons buttons_;
  int32_t cursor_x_ = 0, cursor_y_ = 0, touch_down_x_ = 0, touch_down_y_ = 0;
  uint64_t hover_exit_time_ = 0, touch_down_time_ = 0;
  float gesture_prev_x_ = 0, gesture_prev_y_ = 0, gesture_prev_dist_ = 0;
  float gesture_start_x_ = 0, gesture_start_y_ = 0, gesture_start_dist_ = 0;
  enum class GestureKind { Undecided, Pan, Zoom };
  GestureKind gesture_kind_ = GestureKind::Undecided;

  struct JavaInputEvent {
    enum class Type { Text, Key, KeyboardDisconnected };
    Type type;
    std::string text;
    int32_t keycode = 0, action = 0, meta_state = 0;
    int32_t unicode = 0, repeat_count = 0;
  };
  std::mutex java_input_mutex_;
  std::vector<JavaInputEvent> java_input_;
  bool java_input_enabled_ = true;

  void updatePhysicalModifiers(int32_t keycode, int32_t action, int32_t meta_state);
  void syncModifierEvents();
  void resetPhysicalModifiers();
  void touchSendShift(bool down);
  void touchSendButton(GHOST_TButton mask, GHOST_TEventType type);
  void touchEndPan();
  void touchCancelPending();
  void handleWindowFocus(bool gained);
  int32_t handleInputEvent(AInputEvent *event);
  int32_t handleMotionEvent(AInputEvent *event);
  void hoverExitCheck();
  void touchLongPressCheck();
  int32_t handleKeyEvent(AInputEvent *event);
  int32_t dispatchKeyEvent(int32_t keycode, int32_t action, int32_t meta_state,
                           int32_t unicode, int32_t repeat_count, int32_t device_id);
  GHOST_TSuccess getModifierKeys(GHOST_ModifierKeys &keys) const;
  GHOST_TSuccess getButtons(GHOST_Buttons &buttons) const;
  void handleTextInput(const char *utf8_string);
  void handleJavaKeyEvent(int32_t keycode, int32_t action, int32_t meta_state,
                          int32_t unicode, int32_t repeat_count);
  void handleKeyboardDisconnected();
  void drainJavaInput();
  void dispatchTextInput(const char *utf8_string);
  void dispatchJavaKeyEvent(int32_t keycode, int32_t action, int32_t meta_state,
                            int32_t unicode, int32_t repeat_count);
};
