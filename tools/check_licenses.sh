#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

declare -a scan_files=()

if [[ -d "${ROOT_DIR}/3rdparty" ]]; then
  while IFS= read -r file; do
    scan_files+=("${file}")
  done < <(find "${ROOT_DIR}/3rdparty" -maxdepth 3 -type f \
    \( -iname "license*" -o -iname "copying*" -o -iname "copyright*" \) \
    -not -path "*/3rdparty/vcpkg/*")
fi

if [[ -f "${ROOT_DIR}/vcpkg.json" ]]; then
  scan_files+=("${ROOT_DIR}/vcpkg.json")
fi

if [[ -f "${ROOT_DIR}/vcpkg-configuration.json" ]]; then
  scan_files+=("${ROOT_DIR}/vcpkg-configuration.json")
fi

if [[ ${#scan_files[@]} -eq 0 ]]; then
  echo "No dependency license metadata files were found to scan."
  exit 0
fi

disallowed_regex='(GNU[[:space:]]+GENERAL[[:space:]]+PUBLIC[[:space:]]+LICENSE|GNU[[:space:]]+AFFERO[[:space:]]+GENERAL[[:space:]]+PUBLIC[[:space:]]+LICENSE|AGPL|LGPL|non-commercial|academic[[:space:]]+use[[:space:]]+only|research[[:space:]]+use[[:space:]]+only)'

violations=0
for file in "${scan_files[@]}"; do
  if grep -Eiq "${disallowed_regex}" "${file}"; then
    echo "ERROR: Disallowed license terms detected in ${file}"
    grep -Ein "${disallowed_regex}" "${file}" | head -n 5 || true
    violations=1
  fi
done

if [[ "${violations}" -ne 0 ]]; then
  echo "License policy check failed."
  exit 1
fi

echo "License policy check passed."
