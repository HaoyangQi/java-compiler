#include <stdio.h>

#include "file.h"
#include "debug.h"

int main(int argc, char* argv[])
{
    char* file_name = "./test/1.java";
    file_buffer reader;

    file_buffer_init(&reader);

    debug_file_buffer(&reader);

    if (!file_buffer_load_file(&reader, file_name))
    {
        printf("File failed to load.");
        return 0;
    }

    debug_file_buffer(&reader);

    file_buffer_release(&reader);
    return 0;
}
