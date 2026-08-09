from __future__ import annotations

import io
import sys
import zipfile
import urllib.request
from pathlib import Path


UNICODE_VERSION = "17.0.0"
UCD_URL = f"https://www.unicode.org/Public/{UNICODE_VERSION}/ucd/UCD.zip"


def download_ucd(output: Path) -> None:
    if output.exists():
        print(f"Using cached {output}")
        return

    print(f"Downloading {UCD_URL}...")

    with urllib.request.urlopen(UCD_URL) as response:
        data = response.read()

    output.write_bytes(data)
    print(f"Saved {len(data):,} bytes to {output}")

def main() -> None:
    if len(sys.argv) != 2:
        print("Usage: fetch.py <Output location>")
        quit(1)

    path = Path(sys.argv[1]).resolve()

    download_ucd(path)


if __name__ == "__main__":
    main()