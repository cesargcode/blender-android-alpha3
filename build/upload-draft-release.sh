#!/usr/bin/env bash
# Upload one priority file before independent evidence files. Never publish the draft.
set -euo pipefail
[[ $# == 4 ]] || { echo 'Usage: upload-draft-release.sh DIRECTORY PRIORITY_FILENAME TAG TITLE' >&2; exit 2; }
review="$(realpath -- "$1")"
priority="$2"
tag="$3"
title="$4"
: "${GITHUB_REPOSITORY:?}" "${GITHUB_SHA:?}" "${GITHUB_STEP_SUMMARY:?}" "${GH_TOKEN:?}"
[[ "$priority" != */* && "$priority" != .* && "$tag" =~ ^[A-Za-z0-9._-]+$ ]] || exit 2
[[ -d "$review" && -s "$review/$priority" ]] || { echo 'Priority file is missing or empty.' >&2; exit 2; }
scratch="$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/release-upload.XXXXXX")"
gh --version

# GitHub rejects zero-byte release assets. Make the absence of report output
# explicit, then hash exactly the bytes that will be uploaded.
python3 - "$review" <<'PY'
from pathlib import Path
import hashlib, sys
root = Path(sys.argv[1])
notes = root / 'release-notes.md'
if not notes.exists():
    notes.write_text('Draft for review only; no device/runtime validation is implied.\n')
files = sorted(root.iterdir())
for file in files:
    if not file.is_file() or file.is_symlink():
        raise SystemExit(f'Expected regular flat evidence files: {file.name}')
    if file.name != 'SHA256SUMS.txt' and file.stat().st_size == 0:
        file.write_text('EMPTY_ORIGINAL_FILE: this report originally contained zero bytes.\n'
                        'No diagnostic text was emitted; consult report-status.txt when present.\n')
lines = []
for file in files:
    if file.name != 'SHA256SUMS.txt':
        with file.open('rb') as source:
            lines.append(hashlib.file_digest(source, 'sha256').hexdigest() + '  ' + file.name)
(root / 'SHA256SUMS.txt').write_text('\n'.join(lines) + '\n')
PY

cat > "$scratch/check-release.py" <<'PY'
import hashlib, json, pathlib, re, sys
release = json.loads(pathlib.Path(sys.argv[1]).read_text())
if release.get('isDraft') is not True or release.get('targetCommitish') != sys.argv[2]:
    raise SystemExit('Refusing a release that is not this commit\'s draft.')
if len(sys.argv) == 4:
    print(release['url'])
else:
    file = pathlib.Path(sys.argv[4])
    assets = [asset for asset in release['assets'] if asset['name'] == file.name]
    if not assets:
        print('ABSENT')
    elif len(assets) != 1:
        print('MISMATCH')
    else:
        asset = assets[0]
        if asset.get('state') == 'starter' and asset.get('size') == 0:
            print('INCOMPLETE')
        elif asset.get('state') != 'uploaded':
            print('UNKNOWN')
        elif asset['size'] != file.stat().st_size:
            print('MISMATCH')
        else:
            with file.open('rb') as source:
                digest = 'sha256:' + hashlib.file_digest(source, 'sha256').hexdigest()
            remote_digest = asset.get('digest')
            if remote_digest:
                print('COMPLETE' if remote_digest == digest else 'MISMATCH')
            else:
                endpoint = re.fullmatch(r'https://api\.github\.com/repos/' +
                                        re.escape(sys.argv[3]) + r'/releases/assets/(\d+)',
                                        asset.get('apiUrl', ''))
                print('VERIFY_DOWNLOAD:' + endpoint[1] if endpoint else 'UNKNOWN')
PY

refresh_release() {
  timeout --kill-after=5s 30s gh release view "$tag" --repo "$GITHUB_REPOSITORY" \
    --json url,isDraft,targetCommitish,assets \
    > "$scratch/release.json" 2> "$scratch/api-error.txt" &&
    python3 "$scratch/check-release.py" "$scratch/release.json" "$GITHUB_SHA" "$GITHUB_REPOSITORY" > "$scratch/release-url.txt"
}

# Creation can finish remotely even if the local response times out. Recover
# that exact draft before retrying; create carries no assets and cannot cancel uploads.
release_url=''
for attempt in 1 2 3; do
  if refresh_release; then
    release_url="$(cat "$scratch/release-url.txt")"
    break
  fi
  if timeout --kill-after=5s 60s gh release create "$tag" --repo "$GITHUB_REPOSITORY" \
      --draft --target "$GITHUB_SHA" --title "$title" --notes-file "$review/release-notes.md"; then
    :
  else
    printf 'Draft creation attempt %s did not return success; checking remote state.\n' "$attempt" >&2
  fi
  if refresh_release; then
    release_url="$(cat "$scratch/release-url.txt")"
    break
  fi
  [[ "$attempt" == 3 ]] || sleep 2
done
[[ -n "$release_url" ]] || { echo 'Could not confirm the expected draft; no asset upload attempted.' >&2; exit 1; }

asset_state() {
  local file="$1" seconds="$2" state asset_id
  refresh_release || return 1
  state="$(python3 "$scratch/check-release.py" "$scratch/release.json" "$GITHUB_SHA" "$GITHUB_REPOSITORY" "$file")" || return 1
  if [[ "$state" == VERIFY_DOWNLOAD:* ]]; then
    asset_id="${state#VERIFY_DOWNLOAD:}"
    # Older API responses may omit digest. Confirm the uploaded bytes, not only size.
    timeout --kill-after=5s "${seconds}s" gh api \
      "repos/$GITHUB_REPOSITORY/releases/assets/$asset_id" \
      --header 'Accept: application/octet-stream' > "$scratch/downloaded-asset.bin" || return 1
    python3 - "$file" "$scratch/downloaded-asset.bin" <<'PY'
from pathlib import Path
import hashlib, sys
def fingerprint(name):
    path = Path(name)
    with path.open('rb') as source:
        return path.stat().st_size, hashlib.file_digest(source, 'sha256').hexdigest()
print('COMPLETE' if fingerprint(sys.argv[1]) == fingerprint(sys.argv[2]) else 'MISMATCH')
PY
  else
    [[ "$state" != UNKNOWN ]] || return 1
    printf '%s\n' "$state"
  fi
}

upload_one() {
  local file="$1" seconds="$2" attempt state
  local -a options
  for attempt in 1 2 3; do
    options=()
    if state="$(asset_state "$file" "$seconds")"; then
      case "$state" in
        COMPLETE) return 0 ;;
        ABSENT) ;;
        INCOMPLETE) options=(--clobber) ;;
        *) printf 'Remote asset mismatch: %s; refusing replacement.\n' "${file##*/}" >&2; return 1 ;;
      esac
      if timeout --kill-after=5s "${seconds}s" gh release upload "$tag" "$file" \
          --repo "$GITHUB_REPOSITORY" "${options[@]}"; then
        :
      else
        printf 'Upload attempt %s failed or timed out: %s; checking remote state.\n' "$attempt" "${file##*/}" >&2
      fi
      if state="$(asset_state "$file" "$seconds")" && [[ "$state" == COMPLETE ]]; then
        return 0
      fi
    else
      # Unknown state is not permission to delete a potentially completed upload.
      printf 'Remote state unavailable for %s; no upload/replacement in attempt %s.\n' "${file##*/}" "$attempt" >&2
    fi
    [[ "$attempt" == 3 ]] || sleep 2
  done
  return 1
}

printf '\n### Transferencia para draft\n\nDraft: %s\n\n' "$release_url" >> "$GITHUB_STEP_SUMMARY"
status_file="$scratch/upload-status.txt"
if upload_one "$review/$priority" 300; then
  printf 'UPLOADED_AND_CONFIRMED %s\n' "$priority" | tee "$status_file"
  printf 'Arquivo prioritario enviado e confirmado: **%s**.\n\n' "$priority" >> "$GITHUB_STEP_SUMMARY"
else
  printf 'FAILED priority %s\n' "$priority" | tee "$status_file"
  printf '**FALHA:** envio do arquivo prioritario nao confirmado.\n' >> "$GITHUB_STEP_SUMMARY"
  exit 1
fi

# Each evidence upload has its own process and retries. Its failure cannot cancel
# the already confirmed priority asset or prevent attempts for the other reports.
failed=0
for file in "$review"/*; do
  [[ "${file##*/}" == "$priority" ]] && continue
  if upload_one "$file" 60; then
    printf 'UPLOADED_AND_CONFIRMED %s\n' "${file##*/}" | tee -a "$status_file"
  else
    printf 'FAILED evidence %s\n' "${file##*/}" | tee -a "$status_file"
    failed=1
  fi
done
{
  printf '```text\n'
  cat "$status_file"
  printf '```\n\nFalhas de evidencias nao removem o arquivo prioritario. O draft nao foi publicado.\n'
} >> "$GITHUB_STEP_SUMMARY"
{
  cat "$review/release-notes.md"
  printf '\n## Resultado da transferencia\n\n```text\n'
  cat "$status_file"
  printf '```\n'
} > "$scratch/final-release-notes.md"
notes_updated=0
for attempt in 1 2 3; do
  if timeout --kill-after=5s 30s gh release edit "$tag" --repo "$GITHUB_REPOSITORY" \
      --notes-file "$scratch/final-release-notes.md"; then
    notes_updated=1
    break
  fi
  [[ "$attempt" == 3 ]] || sleep 2
done
if [[ "$notes_updated" == 0 ]]; then
  printf 'FAILED updating draft notes; consult the Actions summary for upload status.\n' >&2
  printf '**Falha ao atualizar notas do draft.** O estado de cada arquivo esta no resumo acima.\n' >> "$GITHUB_STEP_SUMMARY"
  failed=1
fi
exit "$failed"
