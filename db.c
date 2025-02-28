#include <stdio.h>
#include <sqlite3.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

// Callback function to print results of the SELECT query
static int callback(void *data, int argc, char **argv, char **azColName)
{
    FILE *output_file = (FILE *)data;

    for (int i = 0; i < argc; i++)
    {
        fprintf(output_file, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    // fprintf(output_file, "\n");
    return 0;
}

int main()
{
    srand(time(NULL));
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    FILE *public_output_file, *private_output_file;
    int min = 1, max = 100;

    while (1)
    {
        // Open the SQLite database
        rc = sqlite3_open("test.db", &db);
        if (rc)
        {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            return 0;
        }
        else
        {
            fprintf(stderr, "Opened database successfully\n");
        }

        if (((rand() % (max - min + 1)) + min)% 2 == 0)
        {
            // Open private output file for writing
            private_output_file = fopen("query_results_private.txt", "a");
            if (private_output_file == NULL)
            {
                fprintf(stderr, "Can't open output file\n");
                sqlite3_close(db);
                return 0;
            }

            // Execute a SELECT query
            const char *sql = "SELECT * FROM secret_users;";
            rc = sqlite3_exec(db, sql, callback, (void *)private_output_file, &zErrMsg);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }
            else
            {
                fprintf(stderr, "Query executed successfully, results written to file\n");
            }
            fclose(private_output_file);
        }
        else
        {
            // Open output file for writing
            public_output_file = fopen("query_results_public.txt", "a");
            if (public_output_file == NULL)
            {
                fprintf(stderr, "Can't open output file\n");
                sqlite3_close(db);
                return 0;
            }

            // Execute a SELECT query
            const char *sql = "SELECT * FROM users;";
            rc = sqlite3_exec(db, sql, callback, (void *)public_output_file, &zErrMsg);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }
            else
            {
                fprintf(stderr, "Query executed successfully, results written to file\n");
            }
            fclose(public_output_file);
        }

        // Close database and output file
        sqlite3_close(db);

        sleep(5);
    }

    return 0;
}
