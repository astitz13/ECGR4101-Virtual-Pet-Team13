from PIL import Image

# IMAGE_LIST contains a list of tuples of the file name, image name, width, and height
# File name should be the relative file name from this "Assets" directory
# Image name should be what is used to reference the image in code
# Width and height are the dimensions of the image in pixels
IMAGE_LIST = [
    ("demo_rickroll.webp", "IMG_RICKROLL", 150, 100),
    ("demo_rickroll.webp", "IMG_SMALL_RICKROLL", 30, 20),
    # Perry sprites: every 31x30 frame shares one box and has a solid 0x6DBC background,
    # so any frame can be drawn over the previous one at the same x, y.
    ("sprites/pet_idle_0.png", "IMG_PET_IDLE_0", 31, 30),
    ("sprites/pet_blink.png", "IMG_PET_BLINK", 31, 30),
    ("sprites/feed_open.png", "IMG_FEED_OPEN", 31, 30),
    ("sprites/feed_chew.png", "IMG_FEED_CHEW", 31, 30),
    ("sprites/happy_0.png", "IMG_HAPPY_0", 31, 30),
    ("sprites/happy_1.png", "IMG_HAPPY_1", 31, 30),
    ("sprites/agent_idle_0.png", "IMG_AGENT_IDLE_0", 31, 30),
    ("sprites/agent_blink.png", "IMG_AGENT_BLINK", 31, 30),
    ("sprites/hat_2.png", "IMG_HAT_2", 31, 30),
    ("sprites/hat_4.png", "IMG_HAT_4", 31, 30),
    ("sprites/game_run_0.png", "IMG_GAME_RUN_0", 31, 30),
    ("sprites/game_run_1.png", "IMG_GAME_RUN_1", 31, 30),
    ("sprites/game_catch.png", "IMG_GAME_CATCH", 31, 30),
    ("sprites/game_hit.png", "IMG_GAME_HIT", 31, 30),
    ("sprites/ghost_0.png", "IMG_GHOST_0", 31, 30),
    ("sprites/ghost_1.png", "IMG_GHOST_1", 31, 30),
    ("sprites/poof_0.png", "IMG_POOF_0", 31, 30),
    ("sprites/poof_1.png", "IMG_POOF_1", 31, 30),
    # Small 16x16 items with transparent backgrounds
    ("sprites/fish_0.png", "IMG_FISH_0", 16, 16),
    ("sprites/fish_1.png", "IMG_FISH_1", 16, 16),
    ("sprites/mini_hat.png", "IMG_MINI_HAT", 16, 16),
    ("sprites/bomb_0.png", "IMG_BOMB_0", 16, 16),
    ("sprites/heart.png", "IMG_HEART", 16, 16),
    ("sprites/miss.png", "IMG_MISS", 16, 16),
    ("sprites/icon_game.png", "IMG_ICON_GAME", 16, 16),
]

# Relative paths to header and C files for image import
OUTPUT_HEADER_FILE = "../Core/Inc/imported_images.h"
OUTPUT_C_FILE = "../Core/Src/imported_images.c"

# This function generates the header and C file info for a file
def gen_img_definition(filename, image_name, width, height):
    # Open image using PIL
    with Image.open(filename) as img:
        # Convert to RGBA and resize
        # RGBA enables transparency in images which is supported by renderer
        resized = img.convert("RGBA").resize((width, height), Image.Resampling.LANCZOS)

        # Load pixel data for processing
        pixels = resized.load()

        # Define macros for width and height of image
        # Example: For IMG_RICKROLL, these will be IMG_RICKROLL_WIDTH and IMG_RICKROLL_HEIGHT
        header_info = "#define " + image_name.upper() + "_WIDTH " + str(width) + "\n"
        header_info += "#define " + image_name.upper() + "_HEIGHT " + str(height) + "\n"

        # Add definition for image (with provided name) based on dimensions
        header_info += "const extern image_pixel_t " + image_name + "[" + str(width*height) + "];\n"

        # Generate C array initializer for image data
        c_file_info = "const image_pixel_t " + image_name + "[" + str(width*height) + "] = {\n"

        # Iterate through height of image
        for row in range(resized.height):
            # Add indentation
            c_file_info += "    "

            # Iterate through width of image
            for col in range(resized.width):
                # Get RGBA data at current pixel
                r, g, b, a = pixels[col, row]
                # Scale R, G, and B based on R5G6B5 16-bit encoding
                scaled_r = int(r / 255 * 0x1F) & 0x1F
                scaled_g = int(g / 255 * 0x3F) & 0x3F
                scaled_b = int(b / 255 * 0x1F) & 0x1F
                # Combine R, G, and B to 16-bit encoding
                r5g6b5 = scaled_b | (scaled_g << 5) | (scaled_r << 11)
                # Add R5G6B5 and A as image_pixel_t struct in array
                c_file_info += "{" + hex(r5g6b5) + ", " + str(a) + "}"
                # If not the last item, add a comma
                if row < resized.height - 1 or col < resized.width - 1:
                    c_file_info += ", "
            # New line at the end of each row
            c_file_info += "\n"
        # Close out array initialization at end of image
        c_file_info += "};\n\n"

        # Return info for header file and C file
        return header_info, c_file_info

# Open header output and write beginning of header
# ili9341.h is needed for image_pixel_t type
with open(OUTPUT_HEADER_FILE, "wt+") as out_header:
    out_header.write("#ifndef IMPORTED_IMAGES_H\n")
    out_header.write("#define IMPORTED_IMAGES_H\n\n")
    out_header.write("#include \"ili9341.h\"\n\n")

    # Open C file output and write import for header
    # Note that the "imported_images.h" should be changed if OUTPUT_HEADER_FILE changes
    with open(OUTPUT_C_FILE, "wt+") as out_c_file:
        out_c_file.write("#include \"imported_images.h\"\n\n")

        # Iterate through SOUND_LIST and add to header and C files
        for (filename, image_name, width, height) in IMAGE_LIST:
            header_info, c_file_info = gen_img_definition(filename, image_name, width, height)
            out_header.write(header_info)
            out_c_file.write(c_file_info)
    
    # End ifndef preprocessor condition in header file (avoids repeating definitions)
    out_header.write("\n\n#endif // IMPORTED_IMAGES_H")