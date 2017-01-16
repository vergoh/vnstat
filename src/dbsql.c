#include "common.h"
#include "misc.h"
#include "dbsql.h"

int db_open(const int createifnotfound)
{
	int rc, createdb = 0;
	char dbfilename[512];

#ifdef CHECK_VNSTAT
	/* use ram based database when testing for shorter test execution times by reducing disk i/o */
	snprintf(dbfilename, 512, ":memory:");
	createdb = 1;
#else
	struct stat filestat;

	snprintf(dbfilename, 512, "%s/%s", cfg.dbdir, DATABASEFILE);

	/* create database if file doesn't exist */
	if (stat(dbfilename, &filestat) != 0) {
		if (errno == ENOENT && createifnotfound) {
			createdb = 1;
		} else {
			if (debug)
				printf("Error: Handling database \"%s\" failed: %s\n", dbfilename, strerror(errno));
			return 0;
		}
	} else {
		if (filestat.st_size == 0) {
			if (createifnotfound) {
				createdb = 1;
			} else {
				printf("Error: Database \"%s\" contains 0 bytes and isn't a valid database, exiting.\n", dbfilename);
				exit(EXIT_FAILURE);
			}
		}
	}
#endif
	rc = sqlite3_open(dbfilename, &db);

	if (rc) {
		if (debug)
			printf("Error: Can't open database \"%s\": %s\n", dbfilename, sqlite3_errmsg(db));
		return 0;
	} else {
		if (debug)
			printf("Database \"%s\" open\n", dbfilename);
	}

	if (createdb) {
#ifndef CHECK_VNSTAT
		if (!spacecheck(cfg.dbdir)) {
			printf("Error: Not enough free diskspace available in \"%s\", exiting.\n", cfg.dbdir);
			exit(EXIT_FAILURE);
		}
#endif
		if (!db_create()) {
			if (debug)
				printf("Error: Creating database \"%s\" structure failed\n", dbfilename);
			return 0;
		} else {
			if (debug)
				printf("Database \"%s\" structure created\n", dbfilename);
			if (!db_setinfo("dbversion", SQLDBVERSION, 1)) {
				if (debug)
					printf("Error: Writing version info to database \"%s\" failed\n", dbfilename);
				return 0;
			}
		}
	}

	if (createifnotfound) {
		if (!db_setinfo("vnstatversion", getversion(), 1)) {
			return 0;
		}
	}

	return 1;
}

int db_close(void)
{
	int rc;
	rc = sqlite3_close(db);
	if (rc == SQLITE_OK) {
		return 1;
	} else {
		if (debug)
			printf("Error: Closing database failed (%d): %s\n", rc, sqlite3_errmsg(db));
		return 0;
	}
}

int db_exec(const char *sql)
{
	int rc;
	sqlite3_stmt *sqlstmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc) {
		if (debug)
			printf("Error: Exec prepare \"%s\" failed (%d): %s\n", sql, rc, sqlite3_errmsg(db));
		return 0;
	}

	rc = sqlite3_step(sqlstmt);
	if (rc != SQLITE_DONE) {
		if (debug)
			printf("Error: Exec step \"%s\" failed (%d): %s\n", sql, rc, sqlite3_errmsg(db));
		sqlite3_finalize(sqlstmt);
		return 0;
	}

	rc = sqlite3_finalize(sqlstmt);
	if (rc) {
		if (debug)
			printf("Error: Exec finalize \"%s\" failed (%d): %s\n", sql, rc, sqlite3_errmsg(db));
		return 0;
	}

	return 1;
}

int db_create(void)
{
	int i;
	char *sql;
	char buffer[32];
	char *datatables[] = {"fiveminute", "hour", "day", "month", "year", "top"};

	if (!db_begintransaction()) {
		return 0;
	}

	sql = "CREATE TABLE info(\n" \
		"  id       INTEGER PRIMARY KEY,\n" \
		"  name     TEXT UNIQUE NOT NULL,\n" \
		"  value    TEXT NOT NULL);";

	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	sql = "CREATE TABLE interface(\n" \
		"  id           INTEGER PRIMARY KEY,\n" \
		"  name         TEXT UNIQUE NOT NULL,\n" \
		"  alias        TEXT,\n" \
		"  active       INTEGER NOT NULL,\n" \
		"  created      DATE NOT NULL,\n" \
		"  updated      DATE NOT NULL,\n" \
		"  rxcounter    INTEGER NOT NULL,\n" \
		"  txcounter    INTEGER NOT NULL,\n" \
		"  rxtotal      INTEGER NOT NULL,\n" \
		"  txtotal      INTEGER NOT NULL);";

	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	sql = malloc(sizeof(char)*512);
	for (i=0; i<6; i++) {
		sqlite3_snprintf(512, sql, "CREATE TABLE %s(\n" \
			"  id           INTEGER PRIMARY KEY,\n" \
			"  interface    INTEGER REFERENCES interface ON DELETE CASCADE,\n" \
			"  date         DATE NOT NULL,\n" \
			"  rx           INTEGER NOT NULL,\n" \
			"  tx           INTEGER NOT NULL,\n" \
			"  CONSTRAINT u UNIQUE (interface, date));", datatables[i]);

		if (!db_exec(sql)) {
			free(sql);
			db_rollbacktransaction();
			return 0;
		}
	}
	free(sql);

	snprintf(buffer, 32, "%"PRIu64"", (uint64_t)MAX32);
	if (!db_setinfo("btime", buffer, 1)) {
		db_rollbacktransaction();
		return 0;
	}

	return db_committransaction();
}

int db_addinterface(const char *iface)
{
	char sql[1024];

	if (!strlen(iface)) {
		return 0;
	}

	sqlite3_snprintf(1024, sql, "insert into interface (name, active, created, updated, rxcounter, txcounter, rxtotal, txtotal) values ('%q', 1, datetime('now', 'localtime'), datetime('now', 'localtime'), 0, 0, 0, 0);", iface);
	return db_exec(sql);
}

uint64_t db_getinterfacecount(void)
{
	return db_getinterfacecountbyname("");
}

uint64_t db_getinterfacecountbyname(const char *iface)
{
	int rc;
	uint64_t result = 0;
	char sql[512];
	sqlite3_stmt *sqlstmt;

	if (strlen(iface) > 0) {
		sqlite3_snprintf(512, sql, "select count(*) from interface where name='%q'", iface);
	} else {
		sqlite3_snprintf(512, sql, "select count(*) from interface");
	}
	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc) {
		return 0;
	}
	if (sqlite3_column_count(sqlstmt) != 1) {
		return 0;
	}
	if (sqlite3_step(sqlstmt) == SQLITE_ROW) {
		result = (uint64_t)sqlite3_column_int64(sqlstmt, 0);
	}
	sqlite3_finalize(sqlstmt);

	return result;
}

sqlite3_int64 db_getinterfaceid(const char *iface, const int createifnotfound)
{
	int rc;
	char sql[512];
	sqlite3_int64 ifaceid = 0;
	sqlite3_stmt *sqlstmt;

	sqlite3_snprintf(512, sql, "select id from interface where name='%q'", iface);
	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (!rc) {
		if (sqlite3_step(sqlstmt) == SQLITE_ROW) {
			ifaceid = sqlite3_column_int64(sqlstmt, 0);
		}
		sqlite3_finalize(sqlstmt);
	}

	if (ifaceid == 0 && createifnotfound) {
		if (!db_addinterface(iface)) {
			return 0;
		}
		ifaceid = sqlite3_last_insert_rowid(db);
	}

	return ifaceid;
}

int db_setactive(const char *iface, const int active)
{
	char sql[512];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "update interface set active=%d where id=%"PRId64";", active, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_setcounters(const char *iface, const uint64_t rxcounter, const uint64_t txcounter)
{
	char sql[512];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "update interface set rxcounter=%"PRIu64", txcounter=%"PRIu64" where id=%"PRId64";", rxcounter, txcounter, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_getcounters(const char *iface, uint64_t *rxcounter, uint64_t *txcounter)
{
	int rc;
	char sql[512];
	sqlite3_int64 ifaceid = 0;
	sqlite3_stmt *sqlstmt;

	*rxcounter = *txcounter = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "select rxcounter, txcounter from interface where id=%"PRId64";", (int64_t)ifaceid);
	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc) {
		return 0;
	}
	if (sqlite3_column_count(sqlstmt) != 2) {
		return 0;
	}
	if (sqlite3_step(sqlstmt) == SQLITE_ROW) {
		*rxcounter = (uint64_t)sqlite3_column_int64(sqlstmt, 0);
		*txcounter = (uint64_t)sqlite3_column_int64(sqlstmt, 1);
	}
	sqlite3_finalize(sqlstmt);

	return 1;
}

int db_getinterfaceinfo(const char *iface, interfaceinfo *info)
{
	int rc;
	char sql[512];
	sqlite3_int64 ifaceid = 0;
	sqlite3_stmt *sqlstmt;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "select name, alias, active, strftime('%%s', created, 'utc'), strftime('%%s', updated, 'utc'), rxcounter, txcounter, rxtotal, txtotal from interface where id=%"PRId64";", (int64_t)ifaceid);
	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc) {
		return 0;
	}
	if (sqlite3_column_count(sqlstmt) != 9) {
		return 0;
	}
	if (sqlite3_step(sqlstmt) == SQLITE_ROW) {
		if (sqlite3_column_text(sqlstmt, 0) != NULL) {
			strncpy_nt(info->name, (const char *)sqlite3_column_text(sqlstmt, 0), 32);
		} else {
			info->name[0] = '\0';
		}
		if (sqlite3_column_text(sqlstmt, 1) != NULL) {
			strncpy_nt(info->alias, (const char *)sqlite3_column_text(sqlstmt, 1), 32);
		} else {
			info->alias[0] = '\0';
		}
		info->active = sqlite3_column_int(sqlstmt, 2);
		info->created = (time_t)sqlite3_column_int64(sqlstmt, 3);
		info->updated = (time_t)sqlite3_column_int64(sqlstmt, 4);
		info->rxcounter = (uint64_t)sqlite3_column_int64(sqlstmt, 5);
		info->txcounter = (uint64_t)sqlite3_column_int64(sqlstmt, 6);
		info->rxtotal = (uint64_t)sqlite3_column_int64(sqlstmt, 7);
		info->txtotal = (uint64_t)sqlite3_column_int64(sqlstmt, 8);
	}
	sqlite3_finalize(sqlstmt);

	return 1;
}

int db_setalias(const char *iface, const char *alias)
{
	char sql[512];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "update interface set alias='%q' where id=%"PRId64";", alias, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_setinfo(const char *name, const char *value, const int createifnotfound)
{
	int rc;
	char sql[512];

	sqlite3_snprintf(512, sql, "update info set value='%q' where name='%q';", value, name);
	rc = db_exec(sql);
	if (!rc || (!sqlite3_changes(db) && !createifnotfound)) {
		return 0;
	}
	if (!sqlite3_changes(db) && createifnotfound) {
		sqlite3_snprintf(512, sql, "insert into info (name, value) values ('%q', '%q');", name, value);
		rc = db_exec(sql);
	}
	return rc;
}

char *db_getinfo(const char *name)
{
	int rc;
	char sql[512];
	static char buffer[64];
	sqlite3_stmt *sqlstmt;

	buffer[0] = '\0';

	sqlite3_snprintf(512, sql, "select value from info where name='%q';", name);
	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc) {
		return buffer;
	}
	if (sqlite3_step(sqlstmt) == SQLITE_ROW) {
		if (sqlite3_column_text(sqlstmt, 0) != NULL) {
			strncpy_nt(buffer, (const char *)sqlite3_column_text(sqlstmt, 0), 64);
		}
	}
	sqlite3_finalize(sqlstmt);

	return buffer;
}

int db_getiflist(dbiflist **dbifl)
{
	int rc;
	char sql[512];
	static sqlite3_stmt *sqlstmt;

	sqlite3_snprintf(512, sql, "select name from interface order by name desc;");

	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc) {
		return -1;
	}

	rc = 0;
	while (sqlite3_step(sqlstmt) == SQLITE_ROW) {
		if (sqlite3_column_text(sqlstmt, 0) == NULL) {
			continue;
		}
		if (!dbiflistadd(dbifl, (const char *)sqlite3_column_text(sqlstmt, 0))) {
			break;
		}
		rc++;
	}

	sqlite3_finalize(sqlstmt);

	return rc;
}

int db_addtraffic(const char *iface, const uint64_t rx, const uint64_t tx)
{
	return db_addtraffic_dated(iface, rx, tx, 0);
}

int db_addtraffic_dated(const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp)
{
	int i;
	char sql[1024], datebuffer[512], nowdate[64];
	sqlite3_int64 ifaceid = 0;

	char *datatables[] = {"fiveminute", "hour", "day", "month", "year"};
	char *datadates[] = {"datetime(%1$s, ('-' || (strftime('%%M', %1$s)) || ' minutes'), ('-' || (strftime('%%S', %1$s)) || ' seconds'), ('+' || (round(strftime('%%M', %1$s)/5,0)*5) || ' minutes'), 'localtime')", \
			"strftime('%%Y-%%m-%%d %%H:00:00', %s, 'localtime')", \
			"date(%s, 'localtime')", \
			"strftime('%%Y-%%m-01', %s, 'localtime')", \
			"strftime('%%Y-01-01', %s, 'localtime')"};

	if (rx == 0 && tx == 0) {
		return 1;
	}

	ifaceid = db_getinterfaceid(iface, 1);
	if (ifaceid == 0) {
		return 0;
	}

	if (timestamp > 0) {
		snprintf(nowdate, 64, "datetime(%"PRIu64", 'unixepoch')", timestamp);
	} else {
		snprintf(nowdate, 64, "'now'");
	}

	if (debug)
		printf("db add %s (%"PRId64") %"PRIu64": rx %"PRIu64" - tx %"PRIu64"\n", iface, (int64_t)ifaceid, timestamp, rx, tx);

	if (!db_begintransaction()) {
		return 0;
	}

	/* change updated only if more recent than previous when timestamp provided */
	if (timestamp > 0) {
		sqlite3_snprintf(1024, sql, "update interface set updated=datetime(%s, 'localtime') where id=%"PRId64" and updated < datetime(%s, 'localtime');", nowdate, (int64_t)ifaceid, nowdate);
	} else {
		sqlite3_snprintf(1024, sql, "update interface set updated=datetime(%s, 'localtime') where id=%"PRId64";", nowdate, (int64_t)ifaceid);
	}
	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	/* total */
	sqlite3_snprintf(1024, sql, "update interface set rxtotal=rxtotal+%"PRIu64", txtotal=txtotal+%"PRIu64", active=1 where id=%"PRId64";", rx, tx, (int64_t)ifaceid);
	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	/* time specific */
	for (i=0; i<5; i++) {
		snprintf(datebuffer, 512, datadates[i], nowdate);
		sqlite3_snprintf(1024, sql, "insert or ignore into %s (interface, date, rx, tx) values (%"PRId64", %s, 0, 0);", datatables[i], (int64_t)ifaceid, datebuffer);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
		sqlite3_snprintf(1024, sql, "update %s set rx=rx+%"PRIu64", tx=tx+%"PRIu64" where interface=%"PRId64" and date=%s;", datatables[i], rx, tx, (int64_t)ifaceid, datebuffer);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	return db_committransaction();
}

int db_setcreation(const char *iface, const uint64_t timestamp)
{
	char sql[512];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "update interface set created=datetime(%"PRIu64", 'unixepoch', 'localtime') where id=%"PRId64";", timestamp, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_settotal(const char *iface, const uint64_t rx, const uint64_t tx)
{
	char sql[512];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "update interface set rxtotal=%"PRIu64", txtotal=%"PRIu64" where id=%"PRId64";", rx, tx, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_insertdata(const char *table, const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp)
{
	int i, index = -1;
	char sql[1024], datebuffer[512], nowdate[64];
	sqlite3_int64 ifaceid = 0;

	char *datatables[] = {"hour", "day", "month", "year", "top"};
	char *datadates[] = {"strftime('%%Y-%%m-%%d %%H:00:00', %s, 'localtime')", \
			"date(%s, 'localtime')", \
			"strftime('%%Y-%%m-01', %s, 'localtime')", \
			"strftime('%%Y-01-01', %s, 'localtime')", \
			"date(%s, 'localtime')"};

	for (i=0; i<5; i++) {
		if (strcmp(table, datatables[i]) == 0) {
			index = i;
			break;
		}
	}

	if (index == -1) {
		return 0;
	}

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	snprintf(nowdate, 64, "datetime(%"PRIu64", 'unixepoch')", timestamp);
	snprintf(datebuffer, 512, datadates[index], nowdate);

	sqlite3_snprintf(1024, sql, "insert or ignore into %s (interface, date, rx, tx) values (%"PRId64", %s, %"PRIu64", %"PRIu64");", table, (int64_t)ifaceid, datebuffer, rx, tx);
	return db_exec(sql);
}

int db_removeoldentries(void)
{
	char sql[512];

	if (!db_begintransaction()) {
		return 0;
	}

	/* TODO: read cleanup limits from configuration and actually use this function somewhere */
	/* running this about once every hour during cache flush would keep the fiveminute table from accumulating too much excess data */

	sqlite3_snprintf(512, sql, "delete from fiveminute where date < datetime('now', '-48 hours', 'localtime');");
	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	sqlite3_snprintf(512, sql, "delete from hour where date < datetime('now', '-7 days', 'localtime');");
	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	sqlite3_snprintf(512, sql, "delete from day where date < date('now', '-30 days', 'localtime');");
	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	sqlite3_snprintf(512, sql, "delete from month where date < date('now', '-12 months', 'localtime');");
	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	sqlite3_snprintf(512, sql, "delete from year where date < date('now', '-10 years', 'localtime');");
	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}

	/* TODO: rewrite to handle entries per interface and use select for getting entry list */
	/* as the syntax below works only when sqlite is compiled with SQLITE_ENABLE_UPDATE_DELETE_LIMIT */
	/* causing failure in at least in Ubuntu <= 12.04, RHEL, Fedora and CentOS */
	/*sqlite3_snprintf(512, sql, "delete from top order by rx+tx desc limit -1 offset 10;");
	if (!db_exec(sql)) {
		db_rollbacktransaction();
		return 0;
	}*/

	return db_committransaction();
}

int db_vacuum(void)
{
	return db_exec("VACUUM;");
}

int db_begintransaction(void)
{
	int rc;

	rc = sqlite3_exec(db, "BEGIN IMMEDIATE", 0, 0, 0);
	if (rc) {
		if (debug)
			printf("Error: BEGIN failed (%d): %s\n", rc, sqlite3_errmsg(db));
		return 0;
	}
	return 1;
}

int db_committransaction(void)
{
	int rc;

	rc = sqlite3_exec(db, "COMMIT", 0, 0, 0);
	if (rc) {
		if (debug)
			printf("Error: COMMIT failed (%d): %s\n", rc, sqlite3_errmsg(db));
		return 0;
	}
	return 1;
}

int db_rollbacktransaction(void)
{
	int rc;

	rc = sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
	if (rc) {
		if (debug)
			printf("Error: ROLLBACK failed (%d): %s\n", rc, sqlite3_errmsg(db));
		return 0;
	}
	return 1;
}

int dbiflistadd(dbiflist **dbifl, const char *iface)
{
	dbiflist *newif;

	newif = malloc(sizeof(dbiflist));
	if (newif == NULL) {
		return 0;
	}

	newif->next = *dbifl;
	*dbifl = newif;
	strncpy_nt(newif->interface, iface, 32);

	return 1;
}

void dbiflistfree(dbiflist **dbifl)
{
	dbiflist *dbifl_prev;

	while (*dbifl != NULL) {
		dbifl_prev = *dbifl;
		*dbifl = (*dbifl)->next;
		free(dbifl_prev);
	}
}
