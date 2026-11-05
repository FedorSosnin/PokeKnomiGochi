#!/usr/bin/env python3
"""Crop game screenshots to the round screen, with transparent corners.

The game draws a 466x466 picture for a round screen of radius 231; the
screenshots are saved larger (480x480). Everything outside that circle becomes
transparent, with a smooth edge, so the shots look the same on light and dark
pages.

Usage: round_shots.py IMAGE.png [IMAGE.png ...]   (each is rewritten in place)
"""
import sys

from PIL import Image, ImageChops, ImageDraw

SCREEN = 466   # what the game draws
RADIUS = 231   # the round screen inside it
SUPER = 4      # supersampling, for a smooth edge


def circle_mask(w, h):
    r = RADIUS * w / SCREEN
    big = Image.new("L", (w * SUPER, h * SUPER), 0)
    box = ((w / 2 - r) * SUPER, (h / 2 - r) * SUPER, (w / 2 + r) * SUPER, (h / 2 + r) * SUPER)
    ImageDraw.Draw(big).ellipse(box, fill=255)
    return big.resize((w, h), Image.LANCZOS)


def round_image(path):
    im = Image.open(path).convert("RGBA")
    mask = circle_mask(*im.size)
    im.putalpha(ImageChops.darker(im.getchannel("A"), mask))  # never more opaque than it was
    im.save(path, optimize=True)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for p in sys.argv[1:]:
        round_image(p)
        print("rounded", p)


if __name__ == "__main__":
    main()
