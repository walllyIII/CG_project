"""Render the locked courtyard demo frames. Requires Python 3 and Pillow."""
import argparse
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

from PIL import Image, ImageChops, ImageDraw


EFFECTS = ("full", "no-reflection", "no-distortion", "fixed-blend")
VIEWS = ("grazing", "top", "edge", "shore")
FRAMES = [(name, name, "default") for name in (*EFFECTS, "no-absorption", "depth")]
FRAMES += [(view, "full", view) for view in VIEWS]


def contact_sheet(folder, name, panels):
    sheet = Image.new("RGB", (1280, 776), "#e9ece5")
    draw = ImageDraw.Draw(sheet)
    for index, (frame, caption) in enumerate(panels):
        x, y = index % 2 * 640, index // 2 * 388
        with Image.open(folder / (frame + ".png")) as image:
            sheet.paste(image.resize((640, 360), Image.Resampling.LANCZOS), (x, y))
        draw.text((x + 12, y + 367), caption, fill="#263c34")
    sheet.save(folder / (name + ".png"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--verify", type=Path, help="compare RGB frames to a separate reference directory")
    args = parser.parse_args()
    if args.verify and args.verify.resolve() == args.output.resolve():
        parser.error("use a separate output directory when verifying")
    args.output.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=args.output) as scratch:
        ppm = Path(scratch) / "frame.ppm"
        for name, mode, view in FRAMES:
            subprocess.run([str(args.executable.resolve()), "--capture", str(ppm),
                            "0.35", "1", mode, view], check=True)
            with Image.open(ppm) as image:
                if image.size != (1280, 720) or image.mode != "RGB":
                    raise ValueError(f"{name}: unexpected framebuffer format")
                if args.verify:
                    with Image.open(args.verify / (name + ".png")) as ref:
                        if image.size != ref.size or ImageChops.difference(image, ref.convert("RGB")).getbbox():
                            raise ValueError(f"{name}: pixels differ from reference")
                image.save(args.output / (name + ".png"))
    contact_sheet(args.output, "effects", zip(EFFECTS, (
        "Full effect", "Reflection off", "Refraction distortion off", "Fixed reflection weight: 0.32")))
    contact_sheet(args.output, "views", zip(VIEWS, (
        "Low view: pavilion reflection", "Overhead: shallow steps", "Waterline: refraction boundary",
        "South bank: wet stone")))
    print(f"{len(FRAMES)} frames exported" + (" and pixel-verified" if args.verify else ""))


if __name__ == "__main__":
    main()
