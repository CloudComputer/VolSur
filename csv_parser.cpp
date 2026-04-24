
// Copyright Contributors to the OpenVDB Project
// @author CLOUD

#include "csv_parser.h"
#include "csv.h"

#include <stdlib.h>
#include <string.h>


static io::LineReader* g_reader = NULL;


int csv_reader_init(const char* file_name)
{
    if (g_reader != NULL) {
        csv_reader_free(); // 避免重复初始化
    }
    try {
        g_reader = new io::LineReader(file_name);
        return 0; // 成功
    }
    catch (...) {
        g_reader = NULL;
        return -1; // 失败
    }
}

char* csv_reader_next_line(void)
{
    if (g_reader == NULL) return NULL;
    try {
        return g_reader->next_line();
    }
    catch (...) {
        return NULL;
    }
}

unsigned int csv_reader_get_file_line(void)
{
    return g_reader ? g_reader->get_file_line() : 0;
}

const char* csv_reader_get_file_name(void)
{
    return g_reader ? g_reader->get_truncated_file_name() : "";
}

void csv_reader_free(void)
{
    if (g_reader != NULL) {
        delete g_reader;
        g_reader = NULL;
    }
}



