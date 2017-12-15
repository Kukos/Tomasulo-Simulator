#include <asm.h>
#include <tokens.h>
#include <parser.h>
#include <compiler.h>
#include <log.h>
#include <common.h>
#include <stdlib.h>
#include <tomasulo.h>

___before_main___(0) void init(void);
___after_main___(0) void deinit(void);

___before_main___(0) void init(void)
{
	(void)log_init(stdout, NO_LOG_TO_FILE);
}

___after_main___(0) void deinit(void)
{
	log_deinit();
}

int main(int argc, char **argv)
{
	size_t size;
	Token **program;
	size_t i;

	if (argc < 2)
	{
		fprintf(stderr, "Need path to file\n");
		return 1;
	}

	program = parse(argv[1], &size);
	(void)tomasulo(program, size);

	for (i = 0; i < size; ++i)
		token_destroy(program[i]);

	FREE(program);
	return 0;
}
