import librosa

# Sample rate is 16 kHz
SAMPLE_RATE = 16000

# SOUND_LIST contains list of tuples with file name, sound name, and proportion
# File name should be the relative file name from this "Assets" directory
# Sound name should be what is used to reference the sound in code
# Proportion is the proportion of the audio to include (1 is full audio, 0.5 is half, etc.)
SOUND_LIST = [("Perry the Platypus Theme (Midi by RedCubeObjectThingy51).mp3", "AUDIO_PERRY", 1)]

# Relative paths to header and C files for audio import
OUTPUT_HEADER_FILE = "../Core/Inc/imported_audio.h"
OUTPUT_C_FILE = "../Core/Src/imported_audio.c"

# This function generates the header and C file info for a file
def gen_audio_definition(filename, sound_name, proportion):
    # Use librosa to resample and extract data from audio file
    data, sr = librosa.load(filename, sr=SAMPLE_RATE)

    # Extract only necessary proportion of file (full file if proportion == 1)
    data = data[0:int(len(data) * proportion)]

    # Bound data between -1 and 1 in case floating point causes it to exceed those bounds
    bounded_data = map(lambda a: float(a) if abs(a) <= 1 else (1 if a > 0 else -1), data)

    # Scale to range of -127 to 127 for int8_t type
    scaled_data = map(lambda a: int(127 * a), bounded_data)

    # Convert samples to strings for inserting in C file
    string_samples = list(map(str, scaled_data))

    # Define macro for number of samples of file
    # Example: For audio AUDIO_PERRY, the number of samples will be stored in AUDIO_PERRY_SAMPLES
    header_info = "#define " + sound_name.upper() + "_SAMPLES " + str(len(data)) + "\n"
    # Add definition for audio (with provided name) based on number of samples
    header_info += "const extern int8_t " + sound_name.upper() + "[" + sound_name.upper() + "_SAMPLES];\n\n"

    # Generate C array initializer for audio data
    c_file_info = "const int8_t " + sound_name.upper() + "[" + sound_name.upper() + "_SAMPLES] = {"
    c_file_info += ", ".join(string_samples)
    c_file_info += "};\n\n"

    # Return both header and C file info
    return header_info, c_file_info

# Open header output and write beginning of header
# stdint.h is needed for int8_t data type
with open(OUTPUT_HEADER_FILE, "wt+") as out_header:
    out_header.write("#ifndef IMPORTED_AUDIO_H\n")
    out_header.write("#define IMPORTED_AUDIO_H\n\n")
    out_header.write("#include \"stdint.h\"\n\n")

    # Open C file output and write import for header
    # Note that the "imported_audio.h" should be changed if OUTPUT_HEADER_FILE changes
    with open(OUTPUT_C_FILE, "wt+") as out_c_file:
        out_c_file.write("#include \"imported_audio.h\"\n\n")

        # Iterate through SOUND_LIST and add to header and C files
        for (filename, sound_name, proportion) in SOUND_LIST:
            header_info, c_file_info = gen_audio_definition(filename, sound_name, proportion)
            out_header.write(header_info)
            out_c_file.write(c_file_info)
    
    # End ifndef preprocessor condition in header file (avoids repeating definitions)
    out_header.write("\n\n#endif // IMPORTED_AUDIO_H")