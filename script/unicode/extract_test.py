import io
from pathlib import Path
import sys
import zipfile

def extract(data: bytes):
    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        with zf.open("NormalizationTest.txt", 'r') as f:
            return f.read()

    return None

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: extract_test.py <UCD.zip location> <output file>")

    zipf = Path(sys.argv[1]).resolve()
    sfile = Path(sys.argv[2]).resolve()

    with open(zipf, 'rb') as file:
        data = file.read()

    with open(sfile, 'wb') as f:
        f.write(extract(data))
