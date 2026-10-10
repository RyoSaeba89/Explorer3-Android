"""PROVISIONAL icons cut from the game's published title screen
(docs/images/titre-fr.png, 960 x 540 = 240 x 135 scaled 4 times): the rocket
for the launcher, the title screen for the OUYA.

python -I tools/icons.py <titre-fr.png> <res directory>
"""
import os
import sys

from PIL import Image

title = Image.open(sys.argv[1]).convert("RGB").resize((240, 135), Image.NEAREST)
res = sys.argv[2]


def save(img, folder, name):
    os.makedirs(os.path.join(res, folder), exist_ok=True)
    img.save(os.path.join(res, folder, name))


# Launcher: 48 game pixels square around the rocket
rocket = title.crop((134, 68, 182, 116))
for folder, size in (("drawable-mdpi", 48), ("drawable-hdpi", 72), ("drawable-xhdpi", 96)):
    save(rocket.resize((size, size), Image.NEAREST), folder, "icon.png")

# OUYA: 732 x 412, the title screen scaled 3 times, centred
ouya = Image.new("RGB", (732, 412), (0, 0, 0))
ouya.paste(title.resize((720, 405), Image.NEAREST), (6, 3))
save(ouya, "drawable-xhdpi", "ouya_icon.png")
