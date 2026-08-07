#define NOB_IMPLEMENTATION
#include "nob.h"
#include "src/main.h"

#ifndef PROJ_NAME
#define PROJ_NAME "init"
#endif

typedef struct {
    char *project;
    char *description;
    char *author;
    char *contact;
    char *website;
    char *c_standard;
} EnvVars;

global_var EnvVars env_variables =
(EnvVars){.project = PROJ_NAME,
    .description = "This is a new C project!",
    .author = "Simon Danielsson",
    .contact = "contact@simondanielsson.se",
    .website = "https://simondanielsson.se/",
    .c_standard = "c99"};

#define COMP_FLAGS_COUNT 9
global_var char *compilation_flags[COMP_FLAGS_COUNT] = {
    "-O0",
    "-DDEBUG",
    "-fsanitize=address",
    "-fsanitize=undefined",
    "-fno-omit-frame-pointer",
    "-Wall",
    "-Wpedantic",
    "-Wshadow",
    "-Werror=format-security",
};

#define COMP_FLAGS_REL_COUNT 3
global_var char *compilation_flags_release[COMP_FLAGS_REL_COUNT] = {
    "-flto", "-O2", "-DNDEBUG"};

// definitions ----------------------------------------------------------------

#define DIR_BUILD "./build/"
#define DIR_SRC "./src"
#define DIR_STATIC "./src/static"
#define DIR_TESTS "./tests"

#define MAX_SRC_FILES 256
global_var char *src_files[MAX_SRC_FILES];
global_var size_t src_files_count = 0;
global_var char *static_files[MAX_SRC_FILES];
global_var size_t static_files_count = 0;

// helper: embed_static(const char *path)
char *format_path_to_name(const char *filename);
void embed_static(const char *path);
bool collect_src_files(Nob_Walk_Entry entry);
bool collect_static_files(Nob_Walk_Entry entry);
intern_fn void print_help();
void append_env_variables(Nob_Cmd *cmd);
#define MAX_ENV_VALUE_LEN 256
void append_env_var(Nob_Cmd *cmd, const char *prefix, const char *value,
        bool c_standard);

typedef enum {
    ARG_BUILD,
    ARG_PRG,
} ArgParserState;

typedef enum { A_HELP, A_HELP_LONG, A_TEST } Arg;

intern_fn const char *arg_to_str(Arg a);

// program --------------------------------------------------------------------

int main(int argc, char **argv) {

    char *prg_args[128] = {0};
    size_t prg_args_count = 0;

    char *build_args[128] = {0};
    size_t build_args_count = 0;

    {
        uint i = 0;
        ArgParserState aps = ARG_BUILD;
        while (argv[i]) {

            if (strcmp(argv[i], arg_to_str(A_TEST)) == 0) {
                build_args[build_args_count++] = "-DTEST";
            }

            if (strcmp(argv[i], arg_to_str(A_HELP)) == 0 ||
                    strcmp(argv[i], arg_to_str(A_HELP_LONG)) == 0) {
                print_help();
                return 0;
            }

            if (strcmp(argv[i], "--") == 0) {
                aps = ARG_PRG;
                i++;
                continue;
            }

            if (aps == ARG_PRG) {
                prg_args[prg_args_count++] = strdup(argv[i]);
            }
            i++;
        }
    }

    GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(DIR_BUILD))
        return 1;

    // collect '.c' files
    {
        if (!nob_walk_dir(DIR_SRC, collect_src_files))
            return 1;
        if (!nob_walk_dir(DIR_TESTS, collect_src_files))
            return 1;
    }

    // collect and embed static files
    {
        if (!nob_mkdir_if_not_exists(DIR_STATIC))
            return 1;
        if (!nob_walk_dir(DIR_STATIC, collect_static_files))
            return 1;
        if (static_files_count > 0) {
            for (size_t i = 0; i < static_files_count; i++)
                embed_static(static_files[i]);
        }
    }

    // build
    {
        Nob_Cmd cmd = {0};

        nob_cc(&cmd);

        for (size_t i = 0; i < build_args_count; i++) {
            nob_cmd_append(&cmd, build_args[i]);
        }

        {
            append_env_var(&cmd, "-DENV_PROJECT", env_variables.project, false);
            append_env_var(&cmd, "-DENV_DESCR", env_variables.description, false);
            append_env_var(&cmd, "-DENV_AUTHOR", env_variables.author, false);
            append_env_var(&cmd, "-DENV_CONTACT", env_variables.contact, false);
            append_env_var(&cmd, "-DENV_WEBSITE", env_variables.website, false);
            append_env_var(&cmd, "-std", env_variables.c_standard, true);
        }

        for (size_t i = 0; i < COMP_FLAGS_COUNT; i++) {
            nob_cmd_append(&cmd, compilation_flags[i]);
        }

        nob_cc_output(&cmd, DIR_BUILD PROJ_NAME);
        for (size_t i = 0; i < src_files_count; i++) {
            nob_cmd_append(&cmd, src_files[i]);
        }

        if (!nob_cmd_run(&cmd))
            return 1;
    }

    // run
    {
        Nob_Cmd cmd_run = {0};
        nob_cmd_append(&cmd_run, DIR_BUILD PROJ_NAME);
        for (size_t i = 0; i < prg_args_count; i++) {
            nob_cmd_append(&cmd_run, prg_args[i]);
        }
        if (!nob_cmd_run(&cmd_run))
            return 1;
    }

    return 0;
}

// implementations ------------------------------------------------------------

// helper: embed_static(const char *path)
char *format_path_to_name(const char *filename) {
    size_t len = strlen(filename);
    char *result = calloc(len + 1, sizeof(char));

    size_t i = 0;
    if (isdigit(filename[0])) {
        result[0] = 'n';
        i = 1;
    }
    for (; i < len; i++) {
        if (isspace(filename[i]) || filename[i] == '.') {
            result[i] = '_';
        } else {
            result[i] = tolower(filename[i]);
        }
    }
    return result;
}

void embed_static(const char *path) {
    char dir[256] = {0};
    const char *filename = nob_path_name(path);

    char *formatted_name = format_path_to_name(filename);

    char output_path[512] = {0};
    snprintf(output_path, sizeof(output_path), "%s%s.h", dir, formatted_name);
    FILE *dest_h = fopen(output_path, "w");

    String_Builder h_content = {0};

    String_Builder sb = {0};
    nob_read_entire_file(path, &sb);

    char tmp[256] = {0};
    snprintf(tmp, 256, "unsigned char static_%s[] = {\n", formatted_name);
    sb_append_cstr(&h_content, tmp);

#define COLUMNS 7

    size_t i = 0;
    while (i < sb.count) {
        for (int j = COLUMNS; j > 0 && i < sb.count; j--) {
            char tmp[256] = {0};
            snprintf(tmp, 256, "0x%02X, ", (unsigned char)sb.items[i++]);
            sb_append_cstr(&h_content, tmp);
        }
        sb_append_cstr(&h_content, "\n");
    }

    sb_append_cstr(&h_content, "\n};\n");

    {
        char tmp[256] = {0};
        snprintf(tmp, 256, "\nunsigned int static_%s_len = %zu;\n", formatted_name,
                sb.count);
        sb_append_cstr(&h_content, tmp);
    }

    fwrite(h_content.items, sizeof(char), h_content.count, dest_h);
    fclose(dest_h);

    free(formatted_name);
}

bool collect_src_files(Nob_Walk_Entry entry) {
    if (entry.type == FILE_REGULAR && strstr(entry.path, ".c")) {
        if (src_files_count + 1 >= MAX_SRC_FILES) {
            return false;
        }
        src_files[src_files_count++] = strdup(entry.path);
    }
    return true;
}

bool collect_static_files(Nob_Walk_Entry entry) {
    if (entry.type == FILE_REGULAR && !strstr(entry.path, ".h")) {
        if (static_files_count + 1 >= MAX_SRC_FILES) {
            return false;
        }
        static_files[static_files_count++] = strdup(entry.path);
    }
    return true;
}

intern_fn const char *arg_to_str(Arg a) {
    switch (a) {
        case A_HELP:
            return "-h";
        case A_HELP_LONG:
            return "--help";
        case A_TEST:
            return "test";
        default:
            return "--help";
    }
}

intern_fn void print_help() {
    printf("USAGE\n");
    printf("    ./nob [options|command]\n");
    printf("    Program arguments are passed after a divider '--'.\n\n");
    printf("META OPTIONS\n");
    printf("    %-22s %s\n", "-h, --help", "Display help.");
    printf("\nCOMMANDS\n");
    printf("    %-22s %s\n", "test", "Run test(s).");
}

void append_env_var(Nob_Cmd *cmd, const char *prefix, const char *value,
        bool c_standard) {
    char *tmp = malloc(MAX_ENV_VALUE_LEN);
    if (c_standard) {
        snprintf(tmp, MAX_ENV_VALUE_LEN, "%s=%s", prefix, value);
    } else {
        snprintf(tmp, MAX_ENV_VALUE_LEN, "%s=\"%s\"", prefix, value);
    }
    nob_cmd_append(cmd, tmp);
}
