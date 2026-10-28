#!/usr/bin/env python3
"""Adapt the pinned upstream APK packager without editing its source checkout.

The complete resource/Java/native assembly is retained. Only path discovery is
made explicit for the temporary script, and the signing tail is replaced with
zipalign. No signing key is loaded or generated in this path.
"""
import argparse
import hashlib
from pathlib import Path

EXPECTED_SHA256 = "9263c75354dd9fc2c1bb8cd6a3bbd4522cf7953ff55a83716efa1d3b720ac950"
LOCATION = 'SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"'
SIGNING_MARKER = 'echo "[apk] signing"\n'


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    contents = args.source.read_bytes().replace(b"\r\n", b"\n")
    if hashlib.sha256(contents).hexdigest() != EXPECTED_SHA256:
        raise SystemExit("Upstream package.sh differs from the pinned Alpha 3 version.")
    text = contents.decode("utf-8")
    if text.count(LOCATION) != 1 or text.count(SIGNING_MARKER) != 1:
        raise SystemExit("Pinned package.sh layout differs; review unsigned adaptation.")
    text = text.split(SIGNING_MARKER, 1)[0]
    text = text.replace(LOCATION, 'SCRIPT_DIR="${BLENDER_PACKAGE_SCRIPT_DIR:?}"')
    text += '''echo "[apk] aligning unsigned package (no signing key)"
"$BT/zipalign" -f -p 4 "$OUT" "$STAGE/blender-aligned.apk"
mv "$STAGE/blender-aligned.apk" "$OUT"
echo "[apk] unsigned package ready -> $OUT"
ls -lh "$OUT"
'''
    destination = args.output.resolve()
    if destination == args.source.resolve():
        raise SystemExit("The unsigned adapter must not overwrite the upstream source.")
    destination.write_text(text, encoding="utf-8", newline="\n")
    print(f"Prepared unsigned packager from verified source SHA-256 {EXPECTED_SHA256}")


if __name__ == "__main__":
    main()
