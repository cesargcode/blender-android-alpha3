// Exercise extracted production handlers using raw Android boundary data.
// Helpers only construct input and inspect recorded output; no button mapping
// or gesture algorithm is reproduced here. Device identity is a JNI test double.
static AInputEvent motion_event(int tool, int action, int buttons = 0, float pressure = 0,
                               float x = 100, float y = 200, int device = 42)
{
  AInputEvent event{};
  event.type = AINPUT_EVENT_TYPE_MOTION;
  event.action = action;
  event.device_id = device;
  event.button_state = buttons;
  event.pointers.push_back({tool, x, y, pressure, 0, 0});
  return event;
}

static void deliver_motion(GHOST_SystemAndroid &system, AInputEvent event)
{
  require(system.handleInputEvent(&event) == 1, "motion travels through real native input dispatcher");
}

static std::vector<const GHOST_Event *> motion_buttons(const GHOST_SystemAndroid &system)
{
  std::vector<const GHOST_Event *> result;
  for (const auto &event : system.events) {
    if (event->type == GHOST_kEventButtonDown || event->type == GHOST_kEventButtonUp) {
      result.push_back(event.get());
    }
  }
  return result;
}

static void expect_motion_buttons(
    const GHOST_SystemAndroid &system,
    const std::vector<std::pair<GHOST_TEventType, GHOST_TButton>> &expected)
{
  const auto actual = motion_buttons(system);
  require(actual.size() == expected.size(), "exact button edge count, without extra tip or side clicks");
  for (size_t i = 0; i < expected.size(); i++) {
    require(actual[i]->type == expected[i].first && actual[i]->button == expected[i].second,
            "button edge matches requested independent physical action");
  }
}

static void expect_pointer_mask(const GHOST_SystemAndroid &system, unsigned expected)
{
  GHOST_Buttons queried;
  require(system.getButtons(queried) == GHOST_kSuccess, "real button query succeeds");
  require(queried.mask == expected, "queried button ownership matches emitted edges");
}

static void motion_tip_and_pressure()
{
  test_wacom_device_ids = {42};
  test_device_queries = 0;
  for (int tool : {AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_TOOL_TYPE_ERASER}) {
    GHOST_SystemAndroid system;
    deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_DOWN, 0, 0.25f, 10, 20));
    require(!system.touch_pending_, "pen tip does not wait for finger long-press classification");
    require(system.events.size() == 2 && system.events[0]->type == GHOST_kEventCursorMove,
            "tip position is delivered before immediate left press");
    require(system.events[0]->x == 10 && system.events[0]->y == 20,
            "cursor carries the contact coordinates");
    const GHOST_TTabletMode mode = tool == AMOTION_EVENT_TOOL_TYPE_STYLUS ?
                                       GHOST_kTabletModeStylus : GHOST_kTabletModeEraser;
    require(system.events[0]->tablet.Active == mode && system.events[1]->tablet.Active == mode,
            "real tablet mode reaches both cursor and button payloads");
    require(system.events[0]->tablet.Pressure == 0.25f && system.events[1]->tablet.Pressure == 0.25f,
            "initial pressure survives the production cursor and button paths");
    AInputEvent move = motion_event(tool, AMOTION_EVENT_ACTION_MOVE, 0, 0.75f, 35, 40);
    move.pointers[0].tilt = float(M_PI_2);
    deliver_motion(system, move);
    require(system.events.back()->type == GHOST_kEventCursorMove &&
            system.events.back()->tablet.Pressure == 0.75f,
            "drag updates pressure without repeating the left-button press");
    require(std::fabs(system.events.back()->tablet.Xtilt) < 0.00001f &&
            std::fabs(system.events.back()->tablet.Ytilt + 1.0f) < 0.00001f,
            "real tablet tilt conversion is retained");
    deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_UP, 0, 0, 35, 40));
    expect_motion_buttons(system, {{GHOST_kEventButtonDown, GHOST_kButtonMaskLeft},
                                   {GHOST_kEventButtonUp, GHOST_kButtonMaskLeft}});
    require(motion_buttons(system).back()->tablet.Pressure == 0,
            "tip release retains the event's release pressure");
    expect_pointer_mask(system, 0);
  }
  require(test_device_queries == 0, "tip-only motion does not query Java device identity");
  std::puts("PASS: real motion tip down/move/up, coordinates, pressure and tablet tilt");
}

static void motion_wacom_side_buttons()
{
  test_wacom_device_ids = {42};
  for (int tool : {AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_TOOL_TYPE_ERASER,
                   AMOTION_EVENT_TOOL_TYPE_MOUSE}) {
    for (const auto &pair : std::vector<std::pair<int, int>>{
             {AMOTION_EVENT_BUTTON_STYLUS_PRIMARY, AMOTION_EVENT_BUTTON_STYLUS_SECONDARY},
             {AMOTION_EVENT_BUTTON_SECONDARY, AMOTION_EVENT_BUTTON_TERTIARY}}) {
      GHOST_SystemAndroid system;
      deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_PRESS, pair.first));
      expect_pointer_mask(system, GHOST_kButtonMaskMiddle);
      deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_HOVER_MOVE, pair.first, 0, 150, 220));
      expect_motion_buttons(system, {{GHOST_kEventButtonDown, GHOST_kButtonMaskMiddle}});
      require(system.events.back()->type == GHOST_kEventCursorMove && system.events.back()->x == 150,
              "hover drag moves while lower pen button stays held");
      deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_RELEASE));
      deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_PRESS, pair.second));
      expect_pointer_mask(system, GHOST_kButtonMaskRight);
      deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_RELEASE));
      expect_motion_buttons(system, {{GHOST_kEventButtonDown, GHOST_kButtonMaskMiddle},
                                     {GHOST_kEventButtonUp, GHOST_kButtonMaskMiddle},
                                     {GHOST_kEventButtonDown, GHOST_kButtonMaskRight},
                                     {GHOST_kEventButtonUp, GHOST_kButtonMaskRight}});
      expect_pointer_mask(system, 0);

      for (bool release_lower_first : {false, true}) {
        GHOST_SystemAndroid both;
        deliver_motion(both, motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_PRESS, pair.first));
        deliver_motion(both, motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_PRESS, pair.first | pair.second));
        expect_pointer_mask(both, GHOST_kButtonMaskMiddle | GHOST_kButtonMaskRight);
        const int remaining = release_lower_first ? pair.second : pair.first;
        deliver_motion(both, motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_RELEASE, remaining));
        expect_pointer_mask(both, release_lower_first ? GHOST_kButtonMaskRight : GHOST_kButtonMaskMiddle);
        deliver_motion(both, motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_RELEASE));
        expect_motion_buttons(both, {{GHOST_kEventButtonDown, GHOST_kButtonMaskMiddle},
                                    {GHOST_kEventButtonDown, GHOST_kButtonMaskRight},
                                    {GHOST_kEventButtonUp, release_lower_first ? GHOST_kButtonMaskMiddle : GHOST_kButtonMaskRight},
                                    {GHOST_kEventButtonUp, release_lower_first ? GHOST_kButtonMaskRight : GHOST_kButtonMaskMiddle}});
        expect_pointer_mask(both, 0);
      }
    }
  }
  std::puts("PASS: Wacom primary/lower MMB, secondary/upper RMB, stylus/legacy masks and mouse classification");
}

static void motion_tip_with_sides_and_keyboard()
{
  test_wacom_device_ids = {42};
  GHOST_SystemAndroid system;
  AInputEvent shift{AKEYCODE_SHIFT_LEFT, AKEY_EVENT_ACTION_DOWN, AMETA_SHIFT_LEFT_ON};
  require(system.handleInputEvent(&shift) == 1, "physical Shift enters through the native dispatcher");
  const int lower = AMOTION_EVENT_BUTTON_STYLUS_PRIMARY;
  const int upper = AMOTION_EVENT_BUTTON_STYLUS_SECONDARY;
  deliver_motion(system, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_DOWN,
                                      0, 0.25f));
  deliver_motion(system, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_PRESS,
                                      lower, 0.25f));
  deliver_motion(system, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_PRESS,
                                      lower | upper, 0.25f));
  expect_pointer_mask(system, GHOST_kButtonMaskLeft | GHOST_kButtonMaskMiddle | GHOST_kButtonMaskRight);
  deliver_motion(system, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_MOVE,
                                      lower | upper, 0.75f, 120, 240));
  require(system.events.back()->tablet.Pressure == 0.75f, "three held pen buttons preserve drag pressure");
  deliver_motion(system, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_UP, lower | upper));
  expect_pointer_mask(system, GHOST_kButtonMaskMiddle | GHOST_kButtonMaskRight);
  deliver_motion(system, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_RELEASE, upper));
  expect_pointer_mask(system, GHOST_kButtonMaskRight);
  deliver_motion(system, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_RELEASE));
  expect_motion_buttons(system, {{GHOST_kEventButtonDown, GHOST_kButtonMaskLeft},
                                 {GHOST_kEventButtonDown, GHOST_kButtonMaskMiddle},
                                 {GHOST_kEventButtonDown, GHOST_kButtonMaskRight},
                                 {GHOST_kEventButtonUp, GHOST_kButtonMaskLeft},
                                 {GHOST_kEventButtonUp, GHOST_kButtonMaskMiddle},
                                 {GHOST_kEventButtonUp, GHOST_kButtonMaskRight}});
  GHOST_ModifierKeys keys;
  require(system.getModifierKeys(keys) == GHOST_kSuccess && keys.get(GHOST_kModifierKeyLeftShift),
          "pointer events with zero metadata do not erase physical Shift");
  expect_pointer_mask(system, 0);
  std::puts("PASS: independent tip and simultaneous pen sides retain physical keyboard modifier");
}

static void motion_cancel_focus_and_hover()
{
  test_wacom_device_ids = {42};
  const int both = AMOTION_EVENT_BUTTON_STYLUS_PRIMARY | AMOTION_EVENT_BUTTON_STYLUS_SECONDARY;
  for (int tool : {AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_TOOL_TYPE_MOUSE}) {
    for (bool lose_focus : {false, true}) {
      GHOST_SystemAndroid system;
      deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_DOWN,
                                          both | AMOTION_EVENT_BUTTON_PRIMARY, 0.5f));
      expect_pointer_mask(system, GHOST_kButtonMaskLeft | GHOST_kButtonMaskMiddle | GHOST_kButtonMaskRight);
      if (lose_focus) system.handleWindowFocus(false);
      else deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_CANCEL,
                                                 both | AMOTION_EVENT_BUTTON_PRIMARY, 0.5f));
      expect_pointer_mask(system, 0);
      require(!system.stylus_button_down_ && !system.stylus_middle_button_down_ && !system.touch_button_down_,
              "cancel/focus clears both pen side trackers and tip contact");
      const auto edges = motion_buttons(system);
      require(edges.size() == 6, "cancel/focus releases three held buttons exactly once");
      for (auto button : {GHOST_kButtonMaskLeft, GHOST_kButtonMaskMiddle, GHOST_kButtonMaskRight}) {
        size_t releases = 0;
        for (const auto *event : edges) if (event->type == GHOST_kEventButtonUp && event->button == button) releases++;
        require(releases == 1, "each held pen button receives one release");
      }
      if (lose_focus) {
        require(system.events.back()->type == GHOST_kEventWindowDeactivate, "pointer releases precede deactivate");
        const size_t old_size = system.events.size();
        AInputEvent ignored = motion_event(tool, AMOTION_EVENT_ACTION_BUTTON_PRESS, both);
        require(system.handleInputEvent(&ignored) == 0 && system.events.size() == old_size,
                "unfocused native motion cannot resurrect pen buttons");
        system.handleWindowFocus(true);
        deliver_motion(system, motion_event(tool, AMOTION_EVENT_ACTION_HOVER_MOVE));
        expect_pointer_mask(system, 0);
      }
    }
  }
  GHOST_SystemAndroid hover;
  deliver_motion(hover, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_PRESS,
                                     AMOTION_EVENT_BUTTON_STYLUS_PRIMARY));
  deliver_motion(hover, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_HOVER_EXIT,
                                     AMOTION_EVENT_BUTTON_STYLUS_PRIMARY));
  hover.test_time_ms += 101;
  hover.hoverExitCheck();
  require(hover.cursor_x_ == 100 && hover.cursor_y_ == 200, "held lower pen button prevents cursor parking");
  expect_pointer_mask(hover, GHOST_kButtonMaskMiddle);
  deliver_motion(hover, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_RELEASE));
  deliver_motion(hover, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_HOVER_EXIT));
  hover.test_time_ms += 99;
  hover.hoverExitCheck();
  require(hover.cursor_x_ == 100, "true proximity exit uses its existing delay");
  hover.test_time_ms++;
  hover.hoverExitCheck();
  require(hover.cursor_x_ == -1 && hover.cursor_y_ == -1 &&
          hover.events.back()->tablet.Active == GHOST_kTabletModeNone, "unheld proximity exit parks with no tablet");
  GHOST_SystemAndroid contact;
  deliver_motion(contact, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_HOVER_EXIT));
  deliver_motion(contact, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_DOWN, 0, 0.5f));
  contact.test_time_ms += 101;
  contact.hoverExitCheck();
  require(contact.cursor_x_ == 100 && !contact.hover_exit_pending_, "contact after hover-exit cancels delayed park");
  AInputEvent empty = motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_MOVE);
  empty.pointers.clear();
  const size_t count = contact.events.size();
  require(contact.handleInputEvent(&empty) == 0 && contact.events.size() == count,
          "zero-pointer motion is rejected before reading pointer zero");
  std::puts("PASS: real motion stale-mask cancel, focus release/ignore/regain, hover delay and empty pointer guard");
}

static void motion_other_devices_and_finger()
{
  test_wacom_device_ids = {42};
  GHOST_SystemAndroid spen;
  deliver_motion(spen, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_PRESS,
                                    AMOTION_EVENT_BUTTON_STYLUS_PRIMARY, 0, 100, 200, 7));
  deliver_motion(spen, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_RELEASE,
                                    0, 0, 100, 200, 7));
  deliver_motion(spen, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_PRESS,
                                    AMOTION_EVENT_BUTTON_STYLUS_SECONDARY, 0, 100, 200, 7));
  expect_motion_buttons(spen, {{GHOST_kEventButtonDown, GHOST_kButtonMaskRight},
                              {GHOST_kEventButtonUp, GHOST_kButtonMaskRight}});
  GHOST_SystemAndroid mouse;
  for (const auto &button : std::vector<std::pair<int, GHOST_TButton>>{
           {AMOTION_EVENT_BUTTON_PRIMARY, GHOST_kButtonMaskLeft},
           {AMOTION_EVENT_BUTTON_SECONDARY, GHOST_kButtonMaskRight},
           {AMOTION_EVENT_BUTTON_TERTIARY, GHOST_kButtonMaskMiddle},
           {AMOTION_EVENT_BUTTON_BACK, GHOST_kButtonMaskButton4},
           {AMOTION_EVENT_BUTTON_FORWARD, GHOST_kButtonMaskButton5}}) {
    mouse.events.clear();
    deliver_motion(mouse, motion_event(AMOTION_EVENT_TOOL_TYPE_MOUSE, AMOTION_EVENT_ACTION_BUTTON_PRESS,
                                       button.first, 0, 100, 200, 7));
    deliver_motion(mouse, motion_event(AMOTION_EVENT_TOOL_TYPE_MOUSE, AMOTION_EVENT_ACTION_BUTTON_RELEASE,
                                       0, 0, 100, 200, 7));
    expect_motion_buttons(mouse, {{GHOST_kEventButtonDown, button.second}, {GHOST_kEventButtonUp, button.second}});
  }
  // Reused device IDs must not keep a former Wacom assignment.
  test_wacom_device_ids.clear();
  GHOST_SystemAndroid reused;
  deliver_motion(reused, motion_event(AMOTION_EVENT_TOOL_TYPE_STYLUS, AMOTION_EVENT_ACTION_BUTTON_PRESS,
                                      AMOTION_EVENT_BUTTON_STYLUS_PRIMARY));
  expect_pointer_mask(reused, GHOST_kButtonMaskRight);
  GHOST_SystemAndroid finger;
  test_device_queries = 0;
  deliver_motion(finger, motion_event(AMOTION_EVENT_TOOL_TYPE_FINGER, AMOTION_EVENT_ACTION_DOWN));
  require(motion_buttons(finger).empty(), "finger contact remains delayed");
  finger.test_time_ms += TOUCH_LONG_PRESS_MS - 1;
  finger.touchLongPressCheck();
  require(motion_buttons(finger).empty(), "finger long-press waits for production threshold");
  finger.test_time_ms++;
  finger.touchLongPressCheck();
  deliver_motion(finger, motion_event(AMOTION_EVENT_TOOL_TYPE_FINGER, AMOTION_EVENT_ACTION_UP));
  expect_motion_buttons(finger, {{GHOST_kEventButtonDown, GHOST_kButtonMaskRight},
                                {GHOST_kEventButtonUp, GHOST_kButtonMaskRight}});
  require(test_device_queries == 0, "finger events do not perform Wacom JNI classification");
  std::puts("PASS: S Pen and ordinary mouse mappings, reused device ID, and existing finger long-press");
}

static void motion_regressions()
{
  motion_tip_and_pressure();
  motion_wacom_side_buttons();
  motion_tip_with_sides_and_keyboard();
  motion_cancel_focus_and_hover();
  motion_other_devices_and_finger();
  test_wacom_device_ids.clear();
}
