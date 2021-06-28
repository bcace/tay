import os
import sys


def main(proj_dir):
    c_file = open("%s/agent_ocl.c" % proj_dir, "w+")
    h_file = open("%s/agent_ocl.h" % proj_dir, "w+")

    c_files = [
        'agent',
        'taystd',
    ]

    h_files = [
        'agent',
        'taystd',
    ]

    for c in c_files:
        h_file.write('extern const char *%s_ocl_c;\n' % c)
        text = open(os.path.join(proj_dir, '%s.c' % c), "r").read()
        c_file.write('const char *%s_ocl_c = "%s";\n\n' % (c, text.replace("\n", "\\n \\\n")))

    for h in h_files:
        h_file.write('extern const char *%s_ocl_h;\n' % h)
        text = open(os.path.join(proj_dir, '%s.h' % h), "r").read()
        c_file.write('const char *%s_ocl_h = "%s";\n\n' % (h, text.replace("\n", "\\n \\\n")))


if __name__ == "__main__":
   main(sys.argv[1])
