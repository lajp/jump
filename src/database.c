//
// Created by toke019 on 8.10.2021.
//

#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>
#include <sqlite3.h>
#include <errno.h>

#include "../include/extern.h"
#include "../include/utils.h"
#include "../include/database.h"

int         validate_DB_existence(char *, char *);
int         get_db_path(char *, char *);
int         populate_with_defaults(sqlite3 *);
int         execute_query(const char *query, sqlite3 *controller);
sqlite3*    get_connection(char *, char *);

sqlite3*
init_db() {
    sqlite3 *db = NULL;
    char db_path[PATH_MAX_LENGTH];
    char cfg_full_path[PATH_MAX_LENGTH];

    if (get_db_path(db_path, cfg_full_path) < 0)
        return NULL;

    if ((db = get_connection(db_path, cfg_full_path)) == NULL)
        return NULL;

    if (populate_with_defaults(db) < 0)
        return NULL;

    return db;
}

int
show_all(sqlite3 *controller, sqlite3_callback callback)
{
    if (controller == NULL) {
        print_err("Function passed null pointer, exit");
        return -1;
    }

    int rc;
    const char *sql = "SELECT * FROM config;";

    rc = sqlite3_exec(controller,sql, callback, 0, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "(%s) Unable to execute query: %s\n",
                "ERROR", sqlite3_errmsg(controller));

        return -1;
    }

    return 0;
}

int
new_path(const char *alias, const char *path, sqlite3 *controller)
{
    int rc;
    sqlite3_stmt *statement;
    const char *query = "INSERT INTO config(alias, path) VALUES (?, ?)";
    rc =
        sqlite3_prepare_v2(controller, query, -1, &statement, 0);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "(%s) Unable to prepare statement: %s\n",
                "ERROR", sqlite3_errmsg(controller));
        return -1;
    }

    sqlite3_bind_text(statement, 1, alias, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, path, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(statement);
    if (rc != SQLITE_DONE) {
        printf("(%s) Execution failed: %s\n",
               "ERROR", sqlite3_errmsg(controller));
        sqlite3_finalize(statement);
        return -1;
    }

    sqlite3_finalize(statement);

    return 0;
}

// TODO: verify that the alias even exists before running delete query
int
delete_record_by_alias(const char *keyword, sqlite3 *controller)
{
    if (alias_exists(keyword, controller) == 0) {
        print_err("Alias not found, can not delete");
        return -1;
    }

    int rc;
    sqlite3_stmt *statement;
    const char *query = "DELETE FROM config WHERE alias = ?";

    rc = sqlite3_prepare_v2(controller, query, -1, &statement,0 );

    if (rc != SQLITE_OK) {
        fprintf(stderr, "(%s) Unable to prepare statement: %s\n",
                "ERROR", sqlite3_errmsg(controller));
        return -1;
    }

    sqlite3_bind_text(statement, 1, keyword, -1, NULL);

    rc = sqlite3_step(statement);
    if (rc != SQLITE_DONE) {
        printf("(%s) Execution failed: %s\n",
               "ERROR", sqlite3_errmsg(controller));

        sqlite3_finalize(statement);
        return -1;
    }

    sqlite3_finalize(statement);

    return 0;
}

// returns connection to database
sqlite3*
get_connection(char *dir_path, char *file_path)
{

    sqlite3 *db = NULL;
    if (validate_DB_existence(dir_path, file_path) < 0)
        return NULL;

    if (sqlite3_open(file_path, &db) != SQLITE_OK) {
        print_err("Database connection couldn't be opened.");
        return NULL;
    }

    return db;
}

// reset database; delete all records
// it does not ask confirmation, so please do that yourself.
int
reset_table(sqlite3 *controller)
{
    const char *sql_query = "DELETE FROM config;";
    if (execute_query(sql_query, controller) < 0) {
        return -1;
    }

    return 0;
}

// Executes single query
int
execute_query(const char *query, sqlite3 *controller)
{
    if (controller == NULL) {
        print_err("No database controller received");
        return -1;
    }

    char *errmsg;
    int err;
    err = sqlite3_exec(controller, query, NULL, 0, &errmsg);

    if (err != SQLITE_OK) {
        fprintf(stderr, "(%s) Unable to create table in database: %s\n",
                "ERROR", errmsg);

        sqlite3_free(errmsg);
        return -1;
    }
    sqlite3_free(errmsg);
    return 0;
}

int
alias_exists(const char *keyword, sqlite3 *controller)
{
    int rc;
    sqlite3_stmt *statement;
    const char *query = "SELECT path FROM config WHERE alias = ?";

    if (strlen(keyword) > 64) {
        print_err("Too long keyword");
        return -1;
    }
    rc = sqlite3_prepare_v2(controller, query, -1, &statement, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "(%s) Unable to prepare statement: %s\n",
                "ERROR", sqlite3_errmsg(controller));

        return -1;
    }

    sqlite3_bind_text(statement, 1, keyword, -1, NULL);

    rc = sqlite3_step(statement);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        printf("(%s) Execution failed: %s\n", "ERROR",
               sqlite3_errmsg(controller));
        sqlite3_finalize(statement);
        return -1;
    } else if (rc == SQLITE_DONE) {
        return 0;
    } else {
        return 1;
    }
}

// search records by alias
int
find_record_by_alias(const char *keyword, sqlite3 *controller, char *dest)
{
    int rc;
    sqlite3_stmt *statement;
    const char *query = "SELECT path FROM config WHERE alias = ?";

    if (strlen(keyword) > 64) {
        print_err("Too long keyword");

        return -1;
    }

    rc = sqlite3_prepare_v2(controller, query, -1, &statement, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "(%s) Unable to prepare statement: %s\n",
                "ERROR", sqlite3_errmsg(controller));

        return -1;
    }

    sqlite3_bind_text(statement, 1, keyword, -1, NULL);

    rc = sqlite3_step(statement);

    const char *result;
    switch (rc) {
        case SQLITE_DONE:
            print_err("No results found.");
            sqlite3_finalize(statement);
            return 0;
        case SQLITE_ROW:
            result = (const char *) sqlite3_column_text(statement, 0);
            strcpy(dest, result);
            sqlite3_finalize(statement);
            break;
        default:
            printf("(%s) Execution failed: %s\n", "ERROR",
                   sqlite3_errmsg(controller));
            sqlite3_finalize(statement);
            return -1;
    }

    return 0;
}

// Creates table if it does not exist already; e.g. setup database
int
populate_with_defaults(sqlite3 *controller)
{
    char *sql_statement = "CREATE TABLE IF NOT EXISTS config("\
                                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                "alias VARCHAR(255) NOT NULL,"\
                                "path VARCHAR(255) NOT NULL,"\
                                "UNIQUE(alias)"\
                            ")";

    if (execute_query(sql_statement, controller) < 0) {
        fprintf(stderr, "Unable to execute query (@%s): %s\n",
                __func__, sqlite3_errmsg(controller));
        return -1;
    }

    return 0;
}

// Create configuration files and folders, if they don't exist.
int
validate_DB_existence(char *config_dir, char *config_full_path)
{
    struct stat cfg_dir_stat = {0};
    if (stat(config_dir, &cfg_dir_stat) == -1) {
        errno = 0;
        if (mkdir(config_dir, 0755) > 0) {
            switch (errno) {
                case EACCES:
                    print_err("Unable to create the config folder: Permission denied");
                    return -1;
                case EEXIST:
                    print_err("Unable to create the config folder: Already exists");
                    return -1;
                default:
                    print_err("Unable to create a config folder, sorry!");
                    return -1;
            }
        }
    }

    struct stat cfg_full_stat = {0};
    if (stat(config_full_path, &cfg_full_stat) == -1) {
        if (fopen(config_full_path, "w") == NULL) {
            print_err("Unable to create a new database, sorry!");
            return -1;
        }
    }

    return 0;
}

int
get_db_path(char *dest_dir, char *dest_full)
{
    int     standard_exists;
    char    xdg_config_home[PATH_MAX_LENGTH];
    char    config_home[PATH_MAX_LENGTH];
    char    config_full[PATH_MAX_LENGTH];

    char *env_get = getenv("XDG_CONFIG_HOME");

    standard_exists = env_get != NULL && strlen(env_get) > 0;

    if (standard_exists == 1) {
        strcpy(xdg_config_home, env_get);

        struct stat xdg_standard_exists = {0};
        if (stat(xdg_config_home, &xdg_standard_exists) == -1) {
            printf("Custom directory: %s\n", xdg_config_home);
            if (confirm("Preferred config directory was given, but not found. Do you want to create it now?") == 0) {
                return -1;
            }
            else {
                if (mkdir(xdg_config_home, 0755) < 0) {
                    print_err("Unable to create config directory. Do you have correct rights, and does parent directories for it exist?");
                    return -1;
                }
            }
        }

        if (strncmp(&xdg_config_home[strlen(xdg_config_home)-1], "/", PATH_MAX_LENGTH - 1)) {
            strcat(xdg_config_home, "/");
        }
        strcat(xdg_config_home, "jump/");
        strcpy(config_home, xdg_config_home);
    }

    else {
        wordexp_t p;
        wordexp( DB_DIR , &p, 0 );
        strcpy(config_home, *p.we_wordv);
        wordfree(&p);
    }

    if (strncmp(&config_home[strlen(config_home)-1], "/", 1) != 0) {
        strcat(config_home, "/");
    }

    strcpy(config_full, config_home);
    strcat(config_full, DB_FILENAME);

    strcpy(dest_dir,  config_home);
    strcpy(dest_full, config_full);

    return 0;
}
