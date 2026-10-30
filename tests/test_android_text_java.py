#!/usr/bin/env python3
"""Execute extracted Java input methods and compile their real JNI declarations.

Android framework objects are deliberately small test doubles. This checks our
forwarding/encoding and JNI types, not Android's keyboard maps or device runtime.
Requires JDK 17 and either a C++17 compiler or Zig for the JNI fixture.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from extract_cpp import extract_function


JAVA_TEST = r'''
import java.nio.charset.StandardCharsets;
import java.util.*;

class InputDevice {
  static final Map<Integer, InputDevice> devices = new HashMap<>();
  final int vendor, product;
  final boolean virtual;
  InputDevice(int v, int p, boolean isVirtual) { vendor=v; product=p; virtual=isVirtual; }
  static InputDevice getDevice(int id) { return devices.get(id); }
  int getVendorId() { return vendor; }
  int getProductId() { return product; }
  boolean isVirtual() { return virtual; }
}
class KeyCharacterMap {
  static class UnavailableException extends RuntimeException {}
  static int device, key, meta, raw, accent, base, combined;
  static KeyCharacterMap load(int id) {
    device = id;
    if (id == -99) throw new UnavailableException();
    return new KeyCharacterMap();
  }
  int get(int k, int m) { key = k; meta = m; return raw; }
  static int getDeadChar(int a, int b) { accent = a; base = b; return combined; }
}
class KeyEvent {
  static final int ACTION_DOWN = 0, ACTION_UP = 1, ACTION_MULTIPLE = 2;
  static final int KEYCODE_UNKNOWN = 0, KEYCODE_DEL = 67, KEYCODE_FORWARD_DEL = 112;
  final int key, action, meta, unicode, repeat;
  final String characters;
  int unicodeQueries;
  KeyEvent(int k, int a, int m, int u, int r, String s) {
    key=k; action=a; meta=m; unicode=u; repeat=r; characters=s;
  }
  int getKeyCode() { return key; }
  int getAction() { return action; }
  int getMetaState() { return meta; }
  int getUnicodeChar() { unicodeQueries++; return unicode; }
  int getRepeatCount() { return repeat; }
  String getCharacters() { return characters; }
}
public class AndroidTextJavaFixture {
  static int checks;
  final List<int[]> keys = new ArrayList<>();
  final List<byte[]> text = new ArrayList<>();
  private void nativeOnKey(int k, int a, int m, int u, int r) {
    keys.add(new int[] { k, a, m, u, r });
  }
  private void nativeOnCommitText(byte[] utf8) { text.add(utf8); }
  static void check(boolean ok, String message) {
    checks++;
    if (!ok) throw new AssertionError(message);
  }
  void keyIs(int index, int... expected) {
    check(Arrays.equals(keys.get(index), expected), "event " + index);
  }
  void reset() { keys.clear(); text.clear(); }
  // EXTRACTED_METHODS
  public static void main(String[] args) {
    AndroidTextJavaFixture f = new AndroidTextJavaFixture();
    InputDevice.devices.put(8, new InputDevice(0x056a, 0x0374, false));
    check(f.isWacomIntuosS(8), "CTL-4100 USB identity");
    InputDevice.devices.put(9, new InputDevice(0x2d1f, 0x0374, false));
    check(f.isWacomIntuosS(9), "CTL-4100 Android-mode identity");
    InputDevice.devices.put(10, new InputDevice(0x056a, 0x0376, false));
    check(!f.isWacomIntuosS(10), "other Wacom model remains unchanged");
    InputDevice.devices.put(11, new InputDevice(0x2d1f, 0x0375, false));
    check(!f.isWacomIntuosS(11), "other Android-mode Wacom remains unchanged");
    InputDevice.devices.put(12, new InputDevice(0x04e8, 0x0374, false));
    check(!f.isWacomIntuosS(12), "matching product from another vendor rejected");
    InputDevice.devices.put(13, new InputDevice(0, 0, false));
    check(!f.isWacomIntuosS(13), "unknown identity is not guessed from a name");
    InputDevice.devices.put(14, new InputDevice(0x056a, 0x0374, true));
    check(!f.isWacomIntuosS(14), "virtual device with matching IDs rejected");
    check(!f.isWacomIntuosS(99), "missing device rejected");
    InputDevice.devices.remove(8);
    check(!f.isWacomIntuosS(8), "disconnected CTL-4100 not cached");
    InputDevice.devices.put(8, new InputDevice(0, 0, false));
    check(!f.isWacomIntuosS(8), "reused device ID with unknown identity rejected");
    InputDevice.devices.put(8, new InputDevice(0x2d1f, 0x0374, false));
    check(f.isWacomIntuosS(8), "reused device ID re-queries current CTL-4100");

    KeyCharacterMap.raw = 0x800000b4;
    check(f.getKeyUnicode(8, 75, 0x20002) == 0x800000b4, "dead-key bit preserved");
    check(KeyCharacterMap.device == 8 && KeyCharacterMap.key == 75 &&
          KeyCharacterMap.meta == 0x20002, "device and layout meta forwarded");
    check(f.getKeyUnicode(-99, 29, 0) == 0, "unavailable key map does not invent layout");
    KeyCharacterMap.raw = 'q';
    check(f.getKeyUnicode(42, 29, 0) == 'q', "key label does not force US mapping");
    KeyCharacterMap.combined = 0xe9;
    check(f.composeKeyAccent(0xb4, 'e') == 0xe9, "Android accent composition result");
    check(KeyCharacterMap.accent == 0xb4 && KeyCharacterMap.base == 'e', "accent arguments");
    KeyCharacterMap.combined = 0;
    check(f.composeKeyAccent(0xb4, 'x') == 0, "uncomposable accent returned to native");

    int[][] cases = {
      {14, 0, 0, '7', 0}, {29, 0, 1, 'A', 0}, {29, 0, 0, 0xe7, 2},
      {144, 0, 0x200000, '0', 0}, {158, 0, 0x200000, ',', 0},
      {33, 0, 0x22032, 0x20ac, 0}, {75, 0, 0, 0x800000b4, 0},
      {67, 0, 0, 0, 0}, {66, 0, 0, 10, 0}, {29, 0, 0x1000, 0, 0},
      {29, 0, 0, 0x1f642, 0}
    };
    for (int[] c : cases) {
      f.reset();
      KeyEvent event = new KeyEvent(c[0], c[1], c[2], c[3], c[4], null);
      check(f.sendKeyEvent(event), "keydown accepted");
      check(f.keys.size() == 1 && f.text.isEmpty(), "one event without duplicate commit");
      f.keyIs(0, c);
    }
    f.reset();
    KeyEvent up = new KeyEvent(29, 1, 1, 'A', 3, null);
    check(f.sendKeyEvent(up), "keyup accepted");
    f.keyIs(0, 29, 1, 1, 0, 0);
    check(up.unicodeQueries == 0, "release cannot generate text or compose a dead key");

    f.reset();
    check(f.sendKeyEvent(new KeyEvent(0, 2, 0, 0, 0, "\u00e7\ud83c\udfa8")),
          "legacy string event accepted");
    check(f.keys.isEmpty() && f.text.size() == 1, "ACTION_MULTIPLE string only committed once");
    check(Arrays.equals(f.text.get(0), new byte[] {(byte)0xc3, (byte)0xa7, (byte)0xf0,
          (byte)0x9f, (byte)0x8e, (byte)0xa8}), "standard UTF8 includes supplementary code point");
    f.reset();
    check(f.sendKeyEvent(new KeyEvent(14, 2, 1, '&', 3, null)), "legacy repeat accepted");
    check(f.keys.size() == 6 && f.text.isEmpty(), "repeat count produces exact key pairs");
    for (int i=0; i<3; i++) {
      f.keyIs(i*2, 14, 0, 1, '&', i);
      f.keyIs(i*2+1, 14, 1, 1, 0, 0);
    }
    f.reset();
    check(!f.sendKeyEvent(new KeyEvent(0, 2, 0, 0, 0, null)), "missing characters unhandled");
    check(!f.sendKeyEvent(new KeyEvent(29, 9, 0, 'a', 0, null)), "unknown action unhandled");
    check(f.keys.isEmpty() && f.text.isEmpty(), "invalid actions do not type");
    check(f.sendKeyEvent(new KeyEvent(29, 2, 0, 'a', 0, null)), "zero repeats accepted");
    check(f.keys.isEmpty(), "zero repeats do not invent input");

    f.reset();
    check(f.commitText(new StringBuilder("Nome 123 \u00e9"), 1), "CharSequence commit");
    check(new String(f.text.get(0), StandardCharsets.UTF_8).equals("Nome 123 \u00e9"),
          "renaming text survives UTF8 encoding");
    f.commitText(null, 1);
    f.commitText("", 1);
    check(f.text.size() == 1, "empty commits do not duplicate previous text");
    f.reset();
    check(f.deleteSurroundingText(2, 1), "IME deletion accepted");
    check(f.keys.size() == 6, "before and after cursor deletion");
    f.keyIs(0, 67, 0, 0, 0, 0);
    f.keyIs(1, 67, 1, 0, 0, 0);
    f.keyIs(2, 67, 0, 0, 0, 0);
    f.keyIs(3, 67, 1, 0, 0, 0);
    f.keyIs(4, 112, 0, 0, 0, 0);
    f.keyIs(5, 112, 1, 0, 0, 0);
    f.reset();
    check(!f.deleteSurroundingText(-1, 1) && !f.deleteSurroundingText(1, -1),
          "invalid deletion rejected before mutation");
    check(f.keys.isEmpty(), "invalid deletion has no partial effects");
    System.out.println("PASS: " + checks + " Java input/device assertions (framework doubles)");
  }
}
'''


JNI_TEST = r'''
#include <jni.h>
#include "org_blender_blender_BlenderActivity.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
struct GHOST_SystemAndroid {
  std::vector<std::string> text;
  std::vector<std::array<int, 5>> keys;
  void handleTextInput(const char *utf8) { text.emplace_back(utf8); }
  void handleJavaKeyEvent(int k, int a, int m, int u, int r) { keys.push_back({k,a,m,u,r}); }
};
static GHOST_SystemAndroid *ready = nullptr;
static GHOST_SystemAndroid *android_system_if_ready() { return ready; }
static bool fail_copy = false;
static int checks = 0;
static void check(bool ok) { ++checks; if (!ok) throw std::runtime_error("JNI assertion"); }
static jsize JNICALL array_length(JNIEnv *, jarray array) {
  return jsize(reinterpret_cast<std::string *>(array)->size());
}
static void JNICALL copy_array(JNIEnv *, jbyteArray array, jsize start, jsize size, jbyte *out) {
  if (!fail_copy) std::memcpy(out, reinterpret_cast<std::string *>(array)->data() + start, size);
}
static jboolean JNICALL exception_check(JNIEnv *) { return fail_copy; }
// EXTRACTED_METHODS
int main() {
  JNINativeInterface_ table{};
  table.GetArrayLength = array_length;
  table.GetByteArrayRegion = copy_array;
  table.ExceptionCheck = exception_check;
  JNIEnv env{&table};
  GHOST_SystemAndroid system;
  std::string utf8 = "Nome 123 \xc3\xa9 \xf0\x9f\x99\x82";
  jbyteArray array = reinterpret_cast<jbyteArray>(&utf8);
  auto commit = Java_org_blender_blender_BlenderActivity_nativeOnCommitText;
  auto key = Java_org_blender_blender_BlenderActivity_nativeOnKey;
  commit(&env, nullptr, array);
  key(&env, nullptr, 29, 0, 1, 'A', 4);
  check(system.text.empty() && system.keys.empty());
  ready = &system;
  commit(&env, nullptr, array);
  check(system.text.size() == 1 && system.text[0] == utf8);
  commit(&env, nullptr, nullptr);
  std::string empty;
  commit(&env, nullptr, reinterpret_cast<jbyteArray>(&empty));
  check(system.text.size() == 1);
  fail_copy = true;
  commit(&env, nullptr, array);
  check(system.text.size() == 1);
  fail_copy = false;
  key(&env, nullptr, 29, 0, 1, 'A', 4);
  check(system.keys.size() == 1 && system.keys[0] == std::array<int,5>{29,0,1,'A',4});
  key(&env, nullptr, 75, 0, 0, jint(0x800000b4u), 0);
  check(system.keys[1][3] == jint(0x800000b4u));
  std::printf("PASS: %d JNI assertions; exported signatures match javac -h\n", checks);
}
'''


def run(command: list[str], environment: dict[str, str] | None = None) -> None:
    subprocess.run(command, check=True, env=environment)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--jdk", type=Path, help="JDK root; otherwise resolve javac in PATH")
    parser.add_argument("--zig", type=Path)
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--generated-dir", type=Path, default=Path(__file__).parent / "generated")
    args = parser.parse_args()
    source = args.source.resolve()
    javac = (args.jdk.resolve() / "bin" / "javac") if args.jdk else Path(shutil.which("javac") or "javac").resolve()
    jdk = args.jdk.resolve() if args.jdk else javac.parent.parent
    java = jdk / "bin" / "java"
    java_paths = [source / "build_files/android" / suffix / "org/blender/blender/BlenderActivity.java"
                  for suffix in ("apk/app/src/main/java", "java")]
    source_text = [path.read_text(encoding="utf-8") for path in java_paths]
    names = ["getKeyUnicode", "composeKeyAccent", "isWacomIntuosS", "commitInputText", "forwardImeKeyEvent",
             "commitText", "sendKeyEvent", "deleteSurroundingText"]
    methods = [extract_function(source_text[0], name)[0] for name in names]
    for name, method in zip(names, methods):
        assert method == extract_function(source_text[1], name)[0], f"Java copies diverged: {name}"
    native_names = ["nativeOnCommitText", "nativeOnKey"]
    declarations = []
    for name in native_names:
        pattern = rf"private native void {name}\s*\([^;]+;"
        matches = [re.search(pattern, text) for text in source_text]
        assert all(matches), f"Missing native declaration {name}"
        assert matches[0][0] == matches[1][0], f"Native declarations differ: {name}"
        declarations.append(matches[0][0])
    main_cpp = (source / "intern/ghost/intern/GHOST_AndroidMain.cc").read_text(encoding="utf-8")
    native_methods = [extract_function(main_cpp, "Java_org_blender_blender_BlenderActivity_" + name)[0]
                      for name in native_names]
    generated = args.generated_dir.resolve()
    generated.mkdir(parents=True, exist_ok=True)
    compile_env = os.environ.copy()
    compile_env["ZIG_GLOBAL_CACHE_DIR"] = str(source.parent / "tools/zig-global-cache")
    compile_env["ZIG_LOCAL_CACHE_DIR"] = str(source.parent / "tools/zig-local-cache")
    with tempfile.TemporaryDirectory(prefix="android-text-java-", dir=generated) as temp:
        directory = Path(temp)
        fixture = directory / "AndroidTextJavaFixture.java"
        fixture.write_text(JAVA_TEST.replace("// EXTRACTED_METHODS", "\n".join(methods)), encoding="utf-8")
        bridge = directory / "BlenderActivity.java"
        bridge.write_text("package org.blender.blender;\npublic class BlenderActivity {\n" +
                          "\n".join(declarations) + "\n}\n", encoding="utf-8")
        run([str(javac), "-encoding", "UTF-8", "-h", str(directory), "-d", str(directory),
             str(fixture), str(bridge)])
        run([str(java), "-cp", str(directory), "AndroidTextJavaFixture"])
        cpp = directory / "jni_fixture.cc"
        cpp.write_text(JNI_TEST.replace("// EXTRACTED_METHODS", "\n".join(native_methods)), encoding="utf-8")
        exe = directory / ("jni_fixture.exe" if os.name == "nt" else "jni_fixture")
        compiler = [str(args.zig.resolve()), "c++"] if args.zig else [args.cxx]
        include_os = "win32" if os.name == "nt" else ("darwin" if sys.platform == "darwin" else "linux")
        run(compiler + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I" + str(jdk / "include"),
                        "-I" + str(jdk / "include" / include_os), str(cpp), "-o", str(exe)], compile_env)
        run([str(exe)])
    print("PASS: both Java copies share tested input methods and native signatures")


if __name__ == "__main__":
    main()
