
// @author CLOUD

#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	int csv_reader_init(const char* file_name);

	char* csv_reader_next_line(void);

	unsigned int csv_reader_get_file_line(void);

	const char* csv_reader_get_file_name(void);

	void csv_reader_free(void);

#ifdef __cplusplus
}
#endif

#endif // CSV_PARSER_H

