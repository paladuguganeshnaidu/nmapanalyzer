#ifndef DB_H
#define DB_H

int db_init(const char *db_path);
void db_close(void);
int db_is_ready(void);
int db_save_scan(const char *target, const char *ip, const char *timestamp,
                 int level, const char *result_json);
char *db_get_history_json(void);

#endif
