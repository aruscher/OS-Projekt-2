/**
 * @file parser.h
 *
 * Parser interface. This header file describes the interface of a simple
 * recursive-descent parser that understands a very limited shell language
 * sufficient to mainly execute external commands and programs.
 *
 * The parser supports:
 * - lists of commands with their arguments separated by semicolons (;)
 * - commands to be executed as foreground or background (&) jobs
 * - input (<) and output (>) redirections from/to a file
 * - commands assembled to pipes (|)
 * - the builtin commands exit, cd [path], jobs, fg [id], bg [id], and
 *   [un]setenv variable [value]
 * - comments (#) that are ignored until end of line
 * - variable substitutions with $variable or ${variable}
 * - quotations with single quotation marks (') protecting enclosed content
 * - the backslash (\) that only protects the following character
 *
 * @author Helge Parzyjegla <helge.parzyjegla(at)uni-rostock.de>
 * @version 0.1
 * @date created: 07.05.2011,
 *       modified: 07.06.2011
 */

/* parser status and global variables *************************************
 * ************************************************************************/

/**
 * Parser error codes.  Enumeration of parser errors to determine whether
 * parsing an input line succeeded and if not what the cause might be.
 */
enum parser_errors
{
	PARSER_OK,                 /**< Parsing succeeded.                    */
	PARSER_OVERFLOW,           /**< Internal buffer overflow.             */
	PARSER_MALLOC,             /**< Memory allocation issue.              */
	PARSER_INVALID_STATE,      /**< A bug. Please file a bug report.      */
	PARSER_BAD_SUBSTITUTION,   /**< Illegal variable identifier.          */
	PARSER_MISSING_COMMAND,    /**< A command or program was expected.    */
	PARSER_UNEXPECTED_EOF,     /**< EOF while parsing a quotation.        */
	PARSER_ILLEGAL_COMBINATION,/**< Use of builtin commands in pipes.     */
	PARSER_ILLEGAL_REDIRECTION,/**< Illegal redirection in pipes.         */
	PARSER_MISSING_ARGUMENT,   /**< Missing argument for builtin command. */
	PARSER_ILLEGAL_ARGUMENT,   /**< Illegal argument for builtin command. */
	PARSER_MISSING_FILE        /**< Missing file for redirection.         */
};

/**
 * Parser status. Contains  @c PARSER_OK if parsing succeeded,
 * otherwise an error code is provided.
 *
 * @see parser_errors
 * @see parser_message
 * @see error_line
 * @see error_column
 */
extern enum  parser_errors parser_status;

/**
 * Human readable error message. In case of a parsing errors a human
 * readable error description is provided.
 *
 * @see parser_status
 */
extern char* parser_message;

/**
 * Line number of error. In case of a parsing error, the error's line number
 * in the input sting is given.
 *
 * @see parser_status
 * @see error_column
 */
extern int   error_line;

/**
 * Column number of error. In case of a parsing error, the error's column
 * number in the input sting is given.
 *
 * @see parser_status
 * @see error_line
 */
extern int   error_column;


/* data structures for representing a parsed command line *****************
 * ************************************************************************/

/**
 * Arguments of builtin @c cd command. Struct represents the path argument
 * of the builtin @c cd command.
 */
typedef struct cd_args
{
	char* path; /**< Directory path for cd. @c path is @c NULL if no
	                 directory is given for cd.                           */
} cd_args;

/**
 * Arguments of builtin @c [un]setenv command. Struct represents an
 * environment variable/value pair to be set or unset.
 */
typedef struct env_args
{
	char* name;  /**< Name of environment variable to [un]set guaranteed
	                  to be not @c NULL.                                  */
	char* value; /**< Value to set environment variable. @c value is
	                  @c NULL if variable needs to be unset.              */
} env_args;

/**
 * Job control commands.  Enumeration represents the different types of
 * builtin job control commands.
 */
enum job_kind
{
	INFO, /**< The @c jobs command to show list of jobs.                  */
	BG,   /**< The @c bg command to continue a job in the background.     */
	FG    /**< The @c fg command to continue a job in the foreground.     */
};

/**
 * Arguments of builtin commands for job control. Struct represents the
 * type of job control command and the job id on which the command needs
 * to be applied.
 */
typedef struct job_args
{
	enum job_kind kind; /**< Type of job control command (@c jobs/bg/fg). */
	int id;             /**< Job id on which the command needs to be
	                         applied. Guaranteed to be non-negative
	                         (@c >=0) when specified, otherwise @c -1 .   */
} job_args;

/**
 * Arguments of an external command. Struct represents arguments for
 * executing external programs including redirection settings,
 * background/foreground settings, and the programs argument vector.
 */
typedef struct prog_args
{
    char* input;            /**< Filename for input redirection. @c input
                                 is @c NULL if no redirection is given.   */
	char* output;           /**< Filename for output redirection. @c output
                                 is @c NULL if no redirection is given.   */
	int background;         /**< Flag indicating a background job.
	                             @c background!=0 for a background job,
	                             otherwise the job needs to be executed in
	                             the foreground.                          */
	int argc;               /**< Number of elements of the argument
	                             vector.  Guaranteed to be @c >=1 .       */
	char** argv;            /**< Program's null-terminated argument
	                             vector. @c argv[0] is the command to be
	                             executed.                                */
	struct prog_args* next; /**< Pointer to the next program in a pipe. Is
	                             @c NULL for the last command in pipe.    */
} prog_args;

/**
 * Type of parsed command.  Enumeration contains all types of builtin
 * and external commands the parser understands.
 */
enum cmd_kind
{
	EXIT, /**< The builtin @c exit command.                               */
	CD,   /**< The builtin @c cd command.                                 */
	ENV,  /**< The builtin @c setenv and @c unsetenv commands.            */
	JOB,  /**< The builtin @c jobs, @c bg, and @c fg commands.            */
	PROG, /**< A single external program to execute.                      */
	PIPE  /**< A pipe of external commands to execute.                    */
};

/**
 * List of parsed commands. Structure represents an item in the list of
 * parsed commands.
 */
typedef struct cmds
{
	enum cmd_kind kind; /**< Type of command.                             */
	union
	{
		cd_args  cd;    /**< Path for @c cd builtin.                      */
		env_args env;   /**< Variable name and value for @c [un]setenv.   */
		job_args job;   /**< Type of job control command and job id.      */
		prog_args prog; /**< External program and its arguments provided
		                     for @c PROG and @c PIPE commands.            */
	};                  /**< Command arguments are provided alternatively
	                         by a union that corresponds to the kind of
	                         command parsed. Please note, that the @c exit
	                         builtin command needs no further arguments.  */
    struct cmds *next;  /**< Next command in list. Is @c NULL for the
	                         last command.                                */
} cmds;


/* parser functions *******************************************************
 * ************************************************************************/

/**
 * Parses a line of input. If parsing succeeds a pointer to the list of
 * parsed commands is returned and the parser status is @c PARSER_OK.
 * The pointer may be @c NULL if @c input contains no commands. However,
 * if parsing fails, a @c NULL pointer is returned and the parser status
 * contains an appropriate error code. Furthermore, a human readable error
 * message as well as the line and column number of the error are set.
 * Memory is dynamically allocated to store the list of commands and needs
 * to be freed by @c parser_free and supplying the returned handle.
 *
 * @param  input String containing the input line(s) to parse. If @c input
 *         is @c NULL or @c '\\0' parsing succeeds and @c NULL is returned.
 * @return Pointer to the list of parsed commands. May be @c NULL if the
 *         input line contained no commands or an error occurred. In the
 *         latter case, @c parser_status contains an error code.
 *
 * @see parser_free
 * @see parser_status
 * @see parser_message
 * @see error_line
 * @see error_column
 */
extern cmds* parser_parse(char* input);

/**
 * Frees a parsed command list if it is not longer needed. If @c handle
 * is @c NULL nothing happens.
 *
 * @param handle a pointer to a command list as returned by @c parser_parse.
 *
 * @see parser_parse
 *
 */
extern void parser_free(cmds* handle);

/**
 * Prints a parsed command list supplied by @c handle. Function is provided
 * to ease development and debugging.
 *
 * @param handle a pointer to a command list as returned by @c parser_parse.
 *
 * @see parser_parse
 */
extern void parser_print(cmds* handle);

/**
 * Tests the parser using @c input and prints the results including the
 * parser state as well as the parsed command list. Please use the output
 * of this function to file a bug report.
 *
 * @param input String containing the input line(s) to test the parser with.
 */
extern void parser_test(char* input);
