import os
import sys


def main(proj_dir):
    c_file = open("%s/shaders.c" % proj_dir, "w+")
    c_file.write("""#include \"shaders.h\"

""")

    h_file = open("%s/shaders.h" % proj_dir, "w+")
    h_file.write("""#ifndef shaders_h
#define shaders_h


""")

    shaders_dir = os.path.join(proj_dir, 'shaders')
    for item in os.listdir(shaders_dir):
        if item.endswith('.vert'):
            shader_name_suffix = 'vert'
        elif item.endswith('.frag'):
            shader_name_suffix = 'frag'
        else:
            continue

        shader_path = os.path.join(shaders_dir, item)
        shader_name = '%s_%s' % (item[:-5], shader_name_suffix)
        shader_file = open(shader_path, "r")
        shader_text = shader_file.read()

        h_file.write('extern const char *%s;\n' % shader_name)
        c_file.write('\nconst char *%s = "%s";\n' % (shader_name, shader_text.replace("\n", "\\n \\\n")))


    h_file.write("""
#endif
""")


if __name__ == "__main__":
   main(sys.argv[1])
