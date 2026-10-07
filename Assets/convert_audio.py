import librosa

SAMPLE_RATE = 16000
SOUND_LIST = [("Perry the Platypus Theme (Midi by RedCubeObjectThingy51).mp3", "AUDIO_PERRY", 1)]
OUTPUT_HEADER_FILE = "../Core/Inc/imported_audio.h"
OUTPUT_C_FILE = "../Core/Src/imported_audio.c"

def gen_audio_definition(filename, sound_name, proportion):
    data, sr = librosa.load(filename, sr=SAMPLE_RATE)
    data = data[0:int(len(data) * proportion)]
    bounded_data = map(lambda a: float(a) if abs(a) <= 1 else (1 if a > 0 else -1), data)
    scaled_data = map(lambda a: int(127 * a), bounded_data)
    string_samples = list(map(str, scaled_data))

    header_info = "#define " + sound_name.upper() + "_SAMPLES " + str(len(data)) + "\n"
    header_info += "const extern int8_t " + sound_name.upper() + "[" + sound_name.upper() + "_SAMPLES];\n\n"

    c_file_info = "const int8_t " + sound_name.upper() + "[" + sound_name.upper() + "_SAMPLES] = {"
    c_file_info += ", ".join(string_samples)
    c_file_info += "};\n\n"

    return header_info, c_file_info

with open(OUTPUT_HEADER_FILE, "wt+") as out_header:
    out_header.write("#ifndef IMPORTED_AUDIO_H\n")
    out_header.write("#define IMPORTED_AUDIO_H\n\n")
    out_header.write("#include \"stdint.h\"\n\n")
    with open(OUTPUT_C_FILE, "wt+") as out_c_file:
        out_c_file.write("#include \"imported_audio.h\"\n\n")
        for (filename, sound_name, proportion) in SOUND_LIST:
            header_info, c_file_info = gen_audio_definition(filename, sound_name, proportion)
            out_header.write(header_info)
            out_c_file.write(c_file_info)
    out_header.write("\n\n#endif // IMPORTED_AUDIO_H")