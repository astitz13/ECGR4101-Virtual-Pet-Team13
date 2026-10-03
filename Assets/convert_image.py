from PIL import Image

IMAGE_LIST = [("demo_rickroll.webp", "IMG_RICKROLL", 150, 100)]
OUTPUT_HEADER_FILE = "../Core/Inc/imported_images.h"

def gen_img_definition(filename, image_name, width, height):
    with Image.open(filename) as img:
        resized = img.convert("RGBA").resize((width, height), Image.Resampling.LANCZOS)

        pixels = resized.load()
        img_data = []
        header_info = "#define " + image_name.upper() + "_WIDTH " + str(width) + "\n"
        header_info += "#define " + image_name.upper() + "_HEIGHT " + str(height) + "\n"
        header_info += "static image_pixel_t " + image_name + "[] = {\n"
        for row in range(resized.height):
            header_info += "    "
            for col in range(resized.width):
                r, g, b, a = pixels[col, row]
                scaled_r = int(r / 255 * 0x1F) & 0x1F
                scaled_g = int(g / 255 * 0x3F) & 0x3F
                scaled_b = int(b / 255 * 0x1F) & 0x1F
                r5g6b5 = scaled_b | (scaled_g << 5) | (scaled_r << 11)
                header_info += "{" + hex(r5g6b5) + ", " + str(a) + "}"
                img_data.append(r5g6b5)
                if row < resized.height - 1 or col < resized.width - 1:
                    header_info += ", "
            header_info += "\n"
        header_info += "};\n\n"
        return header_info

with open(OUTPUT_HEADER_FILE, "wt+") as out_header:
    out_header.write("#ifndef IMPORTED_IMAGES_H\n")
    out_header.write("#define IMPORTED_IMAGES_H\n")
    out_header.write("#include \"ili9341.h\"\n\n")
    for (filename, image_name, width, height) in IMAGE_LIST:
        out_header.write(gen_img_definition(filename, image_name, width, height))
    out_header.write("\n\n#endif // IMPORTED_IMAGES_H")