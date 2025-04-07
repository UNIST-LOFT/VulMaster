/*
 * 	readextension -
 *		Read a GIF extension block (and do nothing with it).
 *
 */
int
readextension(void)
{
    int count;
    char buf[255];
    int status = 1;

    (void) getc(infile);
    <vul-start>while ((count = getc(infile)) && count <= 255 && count >= 0)<vul-end>
        if (fread(buf, 1, count, infile) != (size_t) count) {
            fprintf(stderr, "short read from file %s (%s)\n",
                    filename, strerror(errno));
            status = 0;
            break;
        }
    return status;
}