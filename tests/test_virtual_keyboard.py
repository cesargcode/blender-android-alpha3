"""Host regression tests using the real virtual-keyboard C++ function bodies.

UI, window queues and GHOST queries are small in-memory fixtures. This checks modifier
ownership and event ordering; it is not an Android or Blender integration build.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import tempfile

from extract_cpp import extract_function, extraction_self_test


FIXTURE = r'''
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

enum { KM_SHIFT = 1, KM_CTRL = 2, KM_ALT = 4 };
enum { VK_MOD_CTRL = 0, VK_MOD_SHIFT = 1, VK_MOD_ALT = 2, VK_MOD_NUM = 3 };
enum GHOST_TKey {
  GHOST_kKeyUnknown, GHOST_kKeyLeftControl, GHOST_kKeyRightControl,
  GHOST_kKeyLeftShift, GHOST_kKeyRightShift, GHOST_kKeyLeftAlt, GHOST_kKeyRightAlt,
  GHOST_kKeyA
};
enum GHOST_TModifierKey {
  GHOST_kModifierKeyLeftControl, GHOST_kModifierKeyRightControl,
  GHOST_kModifierKeyLeftShift, GHOST_kModifierKeyRightShift,
  GHOST_kModifierKeyLeftAlt, GHOST_kModifierKeyRightAlt
};
enum {
  GHOST_kEventKeyDown, GHOST_kEventKeyUp, GHOST_kEventWindowDeactivate,
  GHOST_kEventCursorMove, GHOST_kEventButtonDown, GHOST_kEventButtonUp,
  GHOST_kEventTrackpad, GHOST_kButtonMaskLeft = 1
};
struct GHOST_TEventKeyData { GHOST_TKey key; };
struct GHOST_TEventCursorData { int x, y; };
struct GHOST_TEventButtonData { int button; };
struct wmEvent { uint8_t modifier = 0; int xy[2] = {}; };
struct wmWindowRuntime { wmEvent *eventstate; };
struct wmWindow { wmWindowRuntime *runtime; };
struct wmWindowManager {};
struct rcti { int xmin = 0, xmax = 20, ymin = 0, ymax = 20; };
enum class VKKind { Key, Mod, Layer, Close, Move };
struct VKKeySpec {
  const char *label;
  VKKind kind;
  int code;
  const char *utf8;
  const char *utf8_shift;
  float units;
};
struct VKPlacedKey { const VKKeySpec *spec; rcti rect; };
struct VirtualKeyboard {
  bool open = false;
  wmWindow *win = nullptr;
  wmWindowManager *wm = nullptr;
  void *draw_handle = nullptr;
  rcti rect;
  float offset = 0;
  int layer = 0;
  uint8_t mods = 0, locked = 0;
  uint8_t external_modifiers = 0;
  std::vector<VKPlacedKey> keys;
  const char *const *text_edit = nullptr;
  char last_key[64] = "";
  double last_key_time = 0;
  int hover = -1, pressed = -1;
  double press_time = 0;
  bool moving = false;
  int cursor[2] = {}, pinned[2] = {}, drag_prev[2] = {};
};
static VirtualKeyboard g_vk;
static const double VK_LONG_PRESS_SECONDS = 0.45;
static double test_time = 1.0;
struct GHOST_ISystem {
  bool held[6] = {};
  static GHOST_ISystem *getSystem();
  void getModifierKeyState(GHOST_TModifierKey key, bool &down) const { down = held[key]; }
};
static GHOST_ISystem ghost;
GHOST_ISystem *GHOST_ISystem::getSystem() { return &ghost; }
static double BLI_time_now_seconds() { return test_time; }
static void copy_v2_v2_int(int dst[2], const int src[2]) { dst[0] = src[0]; dst[1] = src[1]; }
static bool BLI_rcti_isect_pt_v(const rcti *r, const int xy[2])
{
  return xy[0] >= r->xmin && xy[0] <= r->xmax && xy[1] >= r->ymin && xy[1] <= r->ymax;
}
static void BLI_strncpy(char *dst, const char *src, size_t size)
{
  std::snprintf(dst, size, "%s", src);
}
#define BLI_snprintf std::snprintf
static void vk_tag_redraw(wmWindow *) {}
static void WM_draw_cb_exit(wmWindow *, void *) {}
static void vk_target_position(const VirtualKeyboard &vk, const wmWindow *, int xy[2])
{
  copy_v2_v2_int(xy, vk.pinned);
}
static size_t vk_modifier_text(const VirtualKeyboard &, char *buf, size_t) { buf[0] = 0; return 0; }
static void vk_ensure_layout(VirtualKeyboard &, wmWindow *) {}
static bool vk_point_is_owned(const wmWindow *, const int[2]) { return false; }
static bool vk_position_in_area(const wmWindow *, const int[2]) { return true; }
static void wm_cursor_position_from_ghost_screen_coords(wmWindow *, int *, int *) {}
static int vk_key_at(const VirtualKeyboard &, const int[2]) { return 0; }
static uint8_t key_flag(GHOST_TKey key)
{
  if (key == GHOST_kKeyLeftControl || key == GHOST_kKeyRightControl) return KM_CTRL;
  if (key == GHOST_kKeyLeftShift || key == GHOST_kKeyRightShift) return KM_SHIFT;
  if (key == GHOST_kKeyLeftAlt || key == GHOST_kKeyRightAlt) return KM_ALT;
  return 0;
}
struct RecordedEvent { GHOST_TKey key; bool down; uint8_t modifier; char text[16]; };
static std::vector<RecordedEvent> events;
uint8_t wm_virtual_keyboard_modifiers_get(const wmWindow *win);
static void vk_send_ghost_key(wmWindowManager *, wmWindow *win,
                              GHOST_TKey key, const char *text, bool down)
{
  wmEvent event = *win->runtime->eventstate;
  const uint8_t flag = key_flag(key);
  if (flag) event.modifier = down ? (event.modifier | flag) : (event.modifier & ~flag);
  /* The production merge line is inserted here by the Python runner. */
  @WM_MODIFIER_MERGE@
  win->runtime->eventstate->modifier = event.modifier;
  RecordedEvent recorded{key, down, event.modifier, ""};
  if (text) BLI_strncpy(recorded.text, text, sizeof(recorded.text));
  events.push_back(recorded);
}
'''


CASES = r'''
static wmEvent event_state;
static wmWindowRuntime runtime{&event_state};
static wmWindow window{&runtime}, other_window{&runtime};
static wmWindowManager manager;
static int assertions = 0;
static const VKKeySpec shift_key{"Shift", VKKind::Mod, VK_MOD_SHIFT, nullptr, nullptr, 1};
static const VKKeySpec ctrl_key{"Ctrl", VKKind::Mod, VK_MOD_CTRL, nullptr, nullptr, 1};
static const VKKeySpec letter_key{"A", VKKind::Key, GHOST_kKeyA, "a", "A", 1};
static void check(bool condition, const char *message)
{
  assertions++;
  if (!condition) throw std::runtime_error(message);
}
static void reset()
{
  g_vk = {};
  ghost = {};
  event_state = {};
  events.clear();
  g_vk.open = true;
  g_vk.win = &window;
  g_vk.wm = &manager;
  g_vk.pinned[0] = g_vk.pinned[1] = 100;
  g_vk.cursor[0] = g_vk.cursor[1] = 10;
  test_time = 1.0;
}
static void tap(const VKKeySpec &key, double duration = 0.1)
{
  g_vk.keys = {{&key, {}}};
  vk_press(g_vk, &window, 0);
  test_time += duration;
  vk_release(g_vk, &manager, &window);
}
static bool physical(GHOST_TKey key, bool down)
{
  if (key != GHOST_kKeyA) ghost.held[int(key) - 1] = down;
  GHOST_TEventKeyData data{key};
  const bool consumed = wm_virtual_keyboard_ghost_event(
      &manager, &window, down ? GHOST_kEventKeyDown : GHOST_kEventKeyUp, &data);
  if (!consumed) vk_send_ghost_key(&manager, &window, key, nullptr, down);
  return consumed;
}
int main()
{
  try {
    reset();
    tap(shift_key);
    check(event_state.modifier == KM_SHIFT, "tapping Shift immediately changes pointer state");
    check(events.size() == 1 && events[0].down, "virtual latch emits one press");
    check(wm_virtual_keyboard_modifiers_get(&other_window) == 0, "latch belongs to its own window");
    check(physical(GHOST_kKeyLeftShift, true), "redundant physical modifier down is consumed");
    check(physical(GHOST_kKeyLeftShift, false), "physical release cannot end virtual precision");
    check(events.size() == 1 && event_state.modifier == KM_SHIFT, "no false modal release is queued");
    check(!physical(GHOST_kKeyA, true), "physical non-modifier remains available");
    check(events.back().modifier == KM_SHIFT, "physical key inherits virtual Shift");
    physical(GHOST_kKeyA, false);
    check(g_vk.mods != 0, "physical key does not consume the documented virtual latch");
    GHOST_TEventCursorData cursor{100, 100};
    check(!wm_virtual_keyboard_ghost_event(&manager, &window, GHOST_kEventCursorMove, &cursor),
          "pointer outside the panel reaches the editor");
    GHOST_TEventButtonData button{GHOST_kButtonMaskLeft};
    check(!wm_virtual_keyboard_ghost_event(&manager, &window, GHOST_kEventButtonDown, &button),
          "outside pointer press remains available");
    check(event_state.modifier == KM_SHIFT, "outside mouse/tablet event retains latch");

    for (GHOST_TKey side : {GHOST_kKeyLeftShift, GHOST_kKeyRightShift}) {
      reset();
      physical(side, true);
      events.clear();
      tap(shift_key);
      check(events.empty(), "already-held physical Shift does not get another press");
      tap(letter_key);
      check(events.size() == 2, "virtual key does not release a held physical modifier");
      check(std::strcmp(events[0].text, "A") == 0, "physical Shift selects shifted virtual text");
      check(g_vk.mods == 0 && event_state.modifier == KM_SHIFT, "sticky expiry preserves physical owner");
      physical(side, false);
      check(event_state.modifier == 0, "physical owner can subsequently release normally");
    }

    reset();
    physical(GHOST_kKeyLeftControl, true);
    tap(shift_key);
    events.clear();
    tap(letter_key);
    check(events[0].modifier == (KM_CTRL | KM_SHIFT), "physical Ctrl and virtual Shift combine");
    check(event_state.modifier == KM_CTRL, "virtual sticky expiry preserves other physical modifiers");

    reset();
    tap(ctrl_key);
    physical(GHOST_kKeyLeftControl, true);
    ghost.held[GHOST_kModifierKeyLeftControl] = false; /* A later release is already queued in GHOST. */
    events.clear();
    tap(ctrl_key);
    check(events.empty(), "unlatch uses dispatched external state, not GHOST's future snapshot");
    physical(GHOST_kKeyA, true);
    check(events.back().modifier == KM_CTRL, "queued Ctrl+A keeps Ctrl until its release is dispatched");
    physical(GHOST_kKeyLeftControl, false);
    check(event_state.modifier == 0, "queued external release ends Ctrl at the correct boundary");

    reset();
    wm_virtual_keyboard_external_modifiers_set(&window, KM_SHIFT);
    event_state.modifier = KM_SHIFT; /* WM reconciliation after focus activation. */
    tap(shift_key);
    tap(shift_key);
    check(events.empty() && event_state.modifier == KM_SHIFT, "WM resync preserves physical ownership");
    wm_virtual_keyboard_external_modifiers_set(&other_window, KM_CTRL);
    check(g_vk.external_modifiers == KM_SHIFT, "another window cannot replace external ownership");

    reset();
    tap(shift_key, 0.6);
    check(g_vk.locked != 0, "long press locks the modifier");
    events.clear();
    tap(letter_key);
    check(events.size() == 2 && event_state.modifier == KM_SHIFT, "locked Shift survives virtual key");
    tap(shift_key);
    check(g_vk.locked == 0 && event_state.modifier == 0, "tap unlocks and releases virtual-only Shift");

    reset();
    tap(shift_key);
    vk_close(&window);
    check(!g_vk.open && event_state.modifier == 0, "close releases virtual-only modifier");
    check(!events.back().down, "close queues release for modal consumers");
    check(g_vk.win == nullptr && g_vk.wm == nullptr, "close drops queue ownership");

    reset();
    tap(shift_key);
    physical(GHOST_kKeyRightShift, true);
    events.clear();
    vk_close(&window);
    check(events.empty() && event_state.modifier == KM_SHIFT, "close preserves held physical Shift");
    check(!physical(GHOST_kKeyRightShift, false), "closed keyboard does not filter desktop keys");
    check(event_state.modifier == 0, "physical Shift can release after close");

    reset();
    tap(shift_key, 0.6);
    g_vk.pressed = 0;
    g_vk.moving = true;
    check(!wm_virtual_keyboard_ghost_event(&manager, &window, GHOST_kEventWindowDeactivate, nullptr),
          "window deactivation continues to normal modifier cleanup");
    check(g_vk.mods == 0 && g_vk.locked == 0, "focus loss clears both latch and lock");
    check(g_vk.pressed == -1 && !g_vk.moving, "focus loss cancels pending key/drag");
    vk_send_ghost_key(&manager, &window, GHOST_kKeyLeftShift, nullptr, false);
    check(event_state.modifier == 0, "focus-loss release is not masked by virtual ownership");
    check(!wm_virtual_keyboard_ghost_event(&manager, &other_window, GHOST_kEventKeyUp, nullptr),
          "keyboard does not touch another window's events");

    reset();
    tap(shift_key, 0.6);
    wm_virtual_keyboard_window_close(&window);
    check(!g_vk.open && g_vk.mods == 0 && g_vk.locked == 0, "destroy clears virtual ownership");
    check(g_vk.win == nullptr && g_vk.wm == nullptr, "destroy clears retained owner pointers");
    std::printf("PASS: virtual-keyboard production functions, %d assertions\n", assertions);
    return 0;
  }
  catch (const std::exception &error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
  }
}
'''


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    workspace = Path(__file__).resolve().parents[2]
    parser.add_argument("--source", type=Path, default=workspace / "blender-alpha3-source")
    parser.add_argument("--zig", type=Path, default=workspace / "tools/ziglang/ziglang/zig.exe")
    args = parser.parse_args()
    extraction_self_test()
    source = args.source / "source/blender/windowmanager/intern/wm_virtual_keyboard.cc"
    source_text = source.read_text(encoding="utf-8")
    event_source = source.with_name("wm_event_system.cc").read_text(encoding="utf-8")
    merge_line = "event.modifier |= wm_virtual_keyboard_modifiers_get(win);"
    if event_source.count(merge_line) != 1:
        raise RuntimeError("Expected exactly one production virtual-modifier merge")
    pieces = [FIXTURE.replace("@WM_MODIFIER_MERGE@", merge_line)]
    functions = [
        "vk_modifier_flag", "wm_virtual_keyboard_modifiers_get",
        "wm_virtual_keyboard_external_modifiers_set", "vk_mod_is_on",
        "vk_modifier_ghost_key", "vk_sync_modifiers", "vk_send_key", "vk_close",
        "wm_virtual_keyboard_window_close", "vk_press", "vk_release",
        "wm_virtual_keyboard_ghost_event",
    ]
    for name in functions:
        definition, line = extract_function(source_text, name)
        pieces.append(f'\n#line {line} "{source.as_posix()}"\n{definition}\n')
    pieces.append('\n#line 1 "virtual_keyboard_cases.cc"\n' + CASES)
    generated = Path(__file__).resolve().parent / "generated"
    generated.mkdir(exist_ok=True)
    fixture = generated / "virtual_keyboard_test.cc"
    fixture.write_text("\n".join(pieces), encoding="utf-8")
    compile_env = os.environ.copy()
    compile_env["ZIG_GLOBAL_CACHE_DIR"] = str(workspace / "tools/zig-global-cache")
    compile_env["ZIG_LOCAL_CACHE_DIR"] = str(workspace / "tools/zig-local-cache")
    with tempfile.TemporaryDirectory(prefix="blender-vk-tests-", dir=generated) as temporary:
        executable = Path(temporary) / "virtual_keyboard_test.exe"
        subprocess.run([str(args.zig), "c++", "-std=c++17", "-O0", "-Wall", "-Wextra",
                        str(fixture), "-o", str(executable)], check=True, env=compile_env)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
