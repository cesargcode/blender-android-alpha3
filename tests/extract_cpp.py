"""Extract complete C++ function definitions without copying their implementations.

The scanner balances braces while ignoring ordinary C/C++ strings, characters,
and comments. It intentionally fails on raw string literals, which are not used
by the tested Android input functions, rather than silently mis-extracting them.
"""

from __future__ import annotations

import re


def _code_mask(source: str) -> str:
    chars = list(source)
    i = 0
    while i < len(source):
        if source.startswith('R"', i):
            # Raw literals need a delimiter-aware parser if introduced here.
            raise ValueError("Raw C++ strings are unsupported by this extractor")
        if source.startswith("//", i):
            end = source.find("\n", i)
            if end < 0:
                end = len(source)
        elif source.startswith("/*", i):
            closing = source.find("*/", i + 2)
            if closing < 0:
                raise ValueError("Unterminated C++ comment")
            end = closing + 2
        elif source[i] in ('"', "'"):
            quote = source[i]
            end = i + 1
            while end < len(source):
                if source[end] == "\\":
                    end += 2
                elif source[end] == quote:
                    end += 1
                    break
                else:
                    end += 1
            else:
                raise ValueError("Unterminated C++ string/character")
        else:
            i += 1
            continue
        for at in range(i, min(end, len(source))):
            if chars[at] != "\n":
                chars[at] = " "
        i = end
    return "".join(chars)


def extract_function(source: str, qualified_name: str) -> tuple[str, int]:
    """Return original definition text and its one-based source line.

    Definitions are recognized at file scope. Calls, prototypes, and comments
    containing the same name cannot accidentally replace the function under test.
    """
    mask = _code_mask(source)
    pattern = re.compile(r"(?<![\w:])" + re.escape(qualified_name) + r"\s*\(")
    matches = []
    for match in pattern.finditer(mask):
        line_start = mask.rfind("\n", 0, match.start()) + 1
        # The input methods use a return type on the same line as the name.
        prefix = mask[line_start:match.start()]
        if not prefix.strip() or any(char in prefix for char in "{};="):
            continue
        paren = 1
        pos = match.end()
        while pos < len(mask) and paren:
            paren += (mask[pos] == "(") - (mask[pos] == ")")
            pos += 1
        if paren:
            raise ValueError(f"Unbalanced parameter list: {qualified_name}")
        opening = mask.find("{", pos)
        terminator = mask.find(";", pos)
        if opening < 0 or (terminator >= 0 and terminator < opening):
            continue
        # A call may be followed by an unrelated later definition. Only accept
        # method qualifiers/attributes between ')' and the opening body brace.
        if any(char in mask[pos:opening] for char in "=()"):
            continue
        depth = 1
        ending = opening + 1
        while ending < len(mask) and depth:
            depth += (mask[ending] == "{") - (mask[ending] == "}")
            ending += 1
        if depth:
            raise ValueError(f"Unbalanced body: {qualified_name}")
        matches.append((source[line_start:ending], source.count("\n", 0, line_start) + 1))
    if len(matches) != 1:
        raise ValueError(f"Expected one definition for {qualified_name}, found {len(matches)}")
    return matches[0]


def extract_braced_declaration(source: str, prefix: str) -> tuple[str, int]:
    """Extract a struct or aggregate initializer, including its final semicolon."""
    mask = _code_mask(source)
    start = mask.find(prefix)
    if start < 0 or mask.find(prefix, start + 1) >= 0:
        raise ValueError(f"Expected one declaration starting with {prefix!r}")
    opening = mask.find("{", start + len(prefix))
    if opening < 0:
        raise ValueError(f"Missing declaration body: {prefix}")
    depth = 1
    ending = opening + 1
    while ending < len(mask) and depth:
        depth += (mask[ending] == "{") - (mask[ending] == "}")
        ending += 1
    if depth:
        raise ValueError(f"Unbalanced declaration body: {prefix}")
    while ending < len(mask) and mask[ending].isspace():
        ending += 1
    if ending == len(mask) or mask[ending] != ";":
        raise ValueError(f"Missing declaration semicolon: {prefix}")
    return source[start:ending + 1], source.count("\n", 0, start) + 1


def extraction_self_test() -> None:
    example = '''// void Demo::run() { not_a_body(); }
void Demo::run(int value)
{
  const char *s = "braces } { and \\\"";
  const char c = '}';
  /* }}} */
  if (value) { value++; }
}
void caller() { Demo::run(1); }
'''
    definition, line = extract_function(example, "Demo::run")
    assert line == 2
    assert definition.endswith("\n}")
    assert "void caller" not in definition


if __name__ == "__main__":
    extraction_self_test()
    print("PASS: C++ definition extraction ignores comments and quoted braces")
