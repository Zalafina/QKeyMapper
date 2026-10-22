"""Development entry point; the packaged bridge will embed its own runtime."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "src"))

from qkeymapper_mcp.__main__ import main

if __name__ == "__main__":
    main()
