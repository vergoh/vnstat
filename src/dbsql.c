#include "common.h"
#include "misc.h"
#include "dbsql.h"

/* global db */
sqlite3 *db;
int db_errcode;
int db_intransaction;

int db_open_ro(void)
{
	return db_open(0, 1);
}

int db_open_rw(const int createifnotfound)
{
	return db_open(createifnotfound, 0);
}

int db_open(const int createifnotfound, const int readonly)
{
	int rc, createdb = 0;
	char dbfilename[530];

#ifdef CHECK_VNSTAT
	/* use ram based database when testing for shorter test execution times by reducing disk i/o */
	snprintf(dbfilename, 530, ":memory:");
	createdb = 1;
#else
	struct stat filestat;

	snprintf(dbfilename, 530, "%s/%s", cfg.dbdir, DATABASEFILE);

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
	db_errcode = 0;
	db_intransaction = 0;
	if (readonly) {
		rc = sqlite3_open_v2(dbfilename, &db, SQLITE_OPEN_READONLY, NULL);
	} else {
		rc = sqlite3_open_v2(dbfilename, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	}

	if (rc) {
		db_errcode = rc;
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
			db_close();
			exit(EXIT_FAILURE);
		}
#endif
		if (!db_create()) {
			if (debug)
				printf("Error: Creating database \"%s\" structure failed\n", dbfilename);
			db_close();
			return 0;
		} else {
			if (debug)
				printf("Database \"%s\" structure created\n", dbfilename);
			if (!db_setinfo("dbversion", SQLDBVERSION, 1)) {
				if (debug)
					printf("Error: Writing version info to database \"%s\" failed\n", dbfilename);
				db_close();
				return 0;
			}
		}
	}

	/* TODO: add db version check to validate that version doesn't come from the future */
	/*       and to do possible upgrade actions is current version is older than latest */

	if (createifnotfound) {
		if (!db_setinfo("vnstatversion", getversion(), 1)) {
			db_close();
			return 0;
		}
	}

	/* set pragmas */
	if (!db_setpragmas()) {
		db_close();
		return 0;
	}

	return 1;
}

int db_setpragmas(void)
{
	int rc;
	sqlite3_stmt *sqlstmt;

	/* enable use of foreign keys */
	if (!db_exec("PRAGMA foreign_keys = ON;")) {
		return 0;
	}

	rc = sqlite3_prepare_v2(db, "PRAGMA foreign_keys;", -1, &sqlstmt, NULL);
	if (rc != SQLITE_OK) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Exec prepare \"PRAGMA foreign_keys;\" failed (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		return 0;
	}

	/* PRAGMA foreign_keys; is expected to return one row if the feature is supported */
	rc = sqlite3_step(sqlstmt);
	if (rc != SQLITE_ROW) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "PRAGMA foreign_keys returned no row (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		sqlite3_finalize(sqlstmt);
		return 0;
	}

	rc = sqlite3_finalize(sqlstmt);
	if (rc) {
		db_errcode = rc;
		snprintf(errorstring, 1024, " Exec finalize \"PRAGMA foreign_keys;\" failed (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		return 0;
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
		db_errcode = rc;
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
	if (rc != SQLITE_OK) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Exec prepare failed (%d: %s): \"%s\"", rc, sqlite3_errmsg(db), sql);
		printe(PT_Error);
		return 0;
	}

	rc = sqlite3_step(sqlstmt);
	if (rc != SQLITE_DONE) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Exec step failed (%d: %s): \"%s\"", rc, sqlite3_errmsg(db), sql);
		printe(PT_Error);
		sqlite3_finalize(sqlstmt);
		return 0;
	}

	rc = sqlite3_finalize(sqlstmt);
	if (rc) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Exec finalize failed (%d: %s): \"%s\"", rc, sqlite3_errmsg(db), sql);
		printe(PT_Error);
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
			"  interface    INTEGER REFERENCES interface(id) ON DELETE CASCADE,\n" \
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

int db_removeinterface(const char *iface)
{
	char sql[256];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(256, sql, "delete from interface where id=%"PRId64";", (int64_t)ifaceid);
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
	if (rc != SQLITE_OK) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Failed to get interface count from database (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
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
	if (rc == SQLITE_OK) {
		if (sqlite3_step(sqlstmt) == SQLITE_ROW) {
			ifaceid = sqlite3_column_int64(sqlstmt, 0);
		}
		sqlite3_finalize(sqlstmt);
	} else {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Failed to get interface id from database (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
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

int db_setupdated(const char *iface, const time_t timestamp)
{
	char sql[512];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "update interface set updated=datetime(%"PRIu64", 'unixepoch', 'localtime') where id=%"PRId64";", (uint64_t)timestamp, (int64_t)ifaceid);
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
	if (rc != SQLITE_OK) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Failed to get interface counters from database (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		return 0;
	}
	if (sqlite3_column_count(sqlstmt) != 2) {
		sqlite3_finalize(sqlstmt);
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
	if (rc != SQLITE_OK) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Failed to get interface information from database (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
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
	if (rc != SQLITE_OK) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Failed to get info value for \"%s\" from database (%d): %s", name, rc, sqlite3_errmsg(db));
		printe(PT_Error);
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
	sqlite3_stmt *sqlstmt;

	sqlite3_snprintf(512, sql, "select name from interface order by name desc;");

	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc != SQLITE_OK ) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Failed to get interface list from database (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
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
	int i, intransaction = db_intransaction;
	char sql[1024], datebuffer[512], nowdate[64];
	sqlite3_int64 ifaceid = 0;

	const char *datatables[] = {"fiveminute", "hour", "day", "month", "year", "top"};
	int32_t *featurecfg[] = {&cfg.fiveminutehours, &cfg.hourlydays, &cfg.dailydays, &cfg.monthlymonths, &cfg.yearlyyears, &cfg.topdayentries};
	const char *datadates[] = {"datetime(%1$s, ('-' || (strftime('%%M', %1$s)) || ' minutes'), ('-' || (strftime('%%S', %1$s)) || ' seconds'), ('+' || (round(strftime('%%M', %1$s)/5,0)*5) || ' minutes'), 'localtime')", \
			"strftime('%%Y-%%m-%%d %%H:00:00', %s, 'localtime')", \
			"date(%s, 'localtime')", \
			"strftime('%%Y-%%m-01', %s, 'localtime')", \
			"strftime('%%Y-01-01', %s, 'localtime')", \
			"date(%s, 'localtime')"};

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

	if (!intransaction) {
		if (!db_begintransaction()) {
			return 0;
		}
	}

	/* change updated only if more recent than previous when timestamp provided */
	if (timestamp > 0) {
		sqlite3_snprintf(1024, sql, "update interface set active=1, updated=datetime(%s, 'localtime') where id=%"PRId64" and updated < datetime(%s, 'localtime');", nowdate, (int64_t)ifaceid, nowdate);
	} else {
		sqlite3_snprintf(1024, sql, "update interface set active=1, updated=datetime(%s, 'localtime') where id=%"PRId64";", nowdate, (int64_t)ifaceid);
	}
	if (!db_exec(sql)) {
		/* no transaction rollback needed here as failure of the first step results in no transaction being active */
		return 0;
	}

	/* total */
	if (rx > 0 || tx > 0) {
		sqlite3_snprintf(1024, sql, "update interface set rxtotal=rxtotal+%"PRIu64", txtotal=txtotal+%"PRIu64" where id=%"PRId64";", rx, tx, (int64_t)ifaceid);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	/* time specific */
	for (i=0; i<6; i++) {
		if (featurecfg[i] == 0) {
			continue;
		}
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

	if (!intransaction) {
		return db_committransaction();
	}
	return 1;
}

int db_setcreation(const char *iface, const time_t timestamp)
{
	char sql[512];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(512, sql, "update interface set created=datetime(%"PRIu64", 'unixepoch', 'localtime') where id=%"PRId64";", (uint64_t)timestamp, (int64_t)ifaceid);
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

	const char *datatables[] = {"hour", "day", "month", "year", "top"};
	const char *datadates[] = {"strftime('%%Y-%%m-%%d %%H:00:00', %s, 'localtime')", \
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

	if (debug) {
		printf("db: removing old entries\n");
	}

	if (!db_removeoldentries_top()) {
		return 0;
	}

	if (!db_begintransaction()) {
		return 0;
	}

	if (cfg.fiveminutehours > 0) {
		if (debug) {
			printf("db: fiveminute cleanup (%dh)\n", cfg.fiveminutehours);
		}
		sqlite3_snprintf(512, sql, "delete from fiveminute where date < datetime('now', '-%d hours', 'localtime');", cfg.fiveminutehours);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (cfg.hourlydays > 0) {
		if (debug) {
			printf("db: hour cleanup (%dd)\n", cfg.hourlydays);
		}
		sqlite3_snprintf(512, sql, "delete from hour where date < datetime('now', '-%d days', 'localtime');", cfg.hourlydays);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (cfg.dailydays > 0) {
		if (debug) {
			printf("db: day cleanup (%dd)\n", cfg.dailydays);
		}
		sqlite3_snprintf(512, sql, "delete from day where date < date('now', '-%d days', 'localtime');", cfg.dailydays);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (cfg.monthlymonths > 0) {
		if (debug) {
			printf("db: month cleanup (%dm)\n", cfg.monthlymonths);
		}
		sqlite3_snprintf(512, sql, "delete from month where date < date('now', '-%d months', 'localtime');", cfg.monthlymonths);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (cfg.yearlyyears > 0) {
		if (debug) {
			printf("db: year cleanup (%dy)\n", cfg.yearlyyears);
		}
		sqlite3_snprintf(512, sql, "delete from year where date < date('now', '-%d years', 'localtime');", cfg.yearlyyears);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	return db_committransaction();
}

int db_removeoldentries_top(void)
{
	int errorcount = 0;
	char sql[512];
	dbiflist *dbifl = NULL, *dbifl_iterator = NULL;
	sqlite3_int64 ifaceid = 0;

	if (cfg.topdayentries <= 0) {
		return 1;
	}

	if (db_getiflist(&dbifl) < 0) {
		return 0;
	}

	dbifl_iterator = dbifl;

	while (dbifl_iterator != NULL) {
		if (debug) {
			printf("db: top cleanup: %s (%d)\n", dbifl_iterator->interface, cfg.topdayentries);
		}

		ifaceid = db_getinterfaceid(dbifl_iterator->interface, 0);
		if (ifaceid == 0) {
			errorcount++;
			dbifl_iterator = dbifl_iterator->next;
			continue;
		}

		sqlite3_snprintf(512, sql, "delete from top where id in ( select id from top where interface=%"PRId64" and date!=date('now', 'localtime') order by rx+tx desc limit -1 offset %d );", (int64_t)ifaceid, cfg.topdayentries);

		if (!db_exec(sql)) {
			errorcount++;
			dbifl_iterator = dbifl_iterator->next;
			continue;
		}

		dbifl_iterator = dbifl_iterator->next;
	}

	dbiflistfree(&dbifl);

	if (errorcount) {
		return 0;
	}

	return 1;
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
		db_errcode = rc;
		snprintf(errorstring, 1024, "Begin transaction to database failed (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		return 0;
	}
	db_intransaction = 1;
	return 1;
}

int db_committransaction(void)
{
	int rc;

	rc = sqlite3_exec(db, "COMMIT", 0, 0, 0);
	if (rc) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Commit transaction to database failed (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		db_intransaction = 0;
		return 0;
	}
	db_intransaction = 0;
	return 1;
}

int db_rollbacktransaction(void)
{
	int rc;

	rc = sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
	if (rc) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Transaction rollback failed (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		db_intransaction = 0;
		return 0;
	}
	db_intransaction = 0;
	return 1;
}

int db_iserrcodefatal(int errcode)
{
	switch (errcode) {
		case SQLITE_OK:
		case SQLITE_FULL:
		case SQLITE_IOERR:
		case SQLITE_LOCKED:
		case SQLITE_BUSY:
			return 0;
		default:
			return 1;
	}
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

int db_getdata(dbdatalist **dbdata, dbdatalistinfo *listinfo, const char *iface, const char *table, const uint32_t resultlimit)
{
	int ret = 1, i, rc;
	char *datatables[] = {"fiveminute", "hour", "day", "month", "year", "top"};
	char sql[512], limit[64];
	sqlite3_int64 ifaceid = 0;
	sqlite3_stmt *sqlstmt;
	time_t timestamp;
	int64_t rowid;
	uint64_t rx, tx;

	listinfo->count = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	ret = 0;
	for (i=0; i<6; i++) {
		if (strcmp(table, datatables[i]) == 0) {
			ret = 1;
			break;
		}
	}
	if (!ret) {
		return 0;
	}

	if (resultlimit > 0) {
		snprintf(limit, 64, "limit %"PRIu32"", resultlimit);
	} else {
		limit[0] = '\0';
	}

	/* note that using the linked list reverses the order */
	/* most recent last in the linked list is considered the normal order */
	if (strcmp(table, "top") == 0) {
		sqlite3_snprintf(512, sql, "select * from (select id, strftime('%%s', date, 'utc'), rx, tx from %s where interface=%"PRId64" order by rx+tx desc %s) order by rx+tx asc;", table, (int64_t)ifaceid, limit);
	} else {
		sqlite3_snprintf(512, sql, "select id, strftime('%%s', date, 'utc'), rx, tx from %s where interface=%"PRId64" order by date desc %s;", table, (int64_t)ifaceid, limit);
	}

	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc != SQLITE_OK) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Get data prepare failed (%d: %s): \"%s\"", rc, sqlite3_errmsg(db), sql);
		printe(PT_Error);
		return 0;
	}

	rc = sqlite3_column_count(sqlstmt);
	if (rc != 4) {
		snprintf(errorstring, 1024, "Get data returned unexpected column count %d instead of 4: \"%s\"", rc, sql);
		printe(PT_Error);
		sqlite3_finalize(sqlstmt);
		return 0;
	}

	while (sqlite3_step(sqlstmt) == SQLITE_ROW) {
		rowid = (int64_t)sqlite3_column_int64(sqlstmt, 0);
		timestamp = (time_t)sqlite3_column_int64(sqlstmt, 1);
		rx = (uint64_t)sqlite3_column_int64(sqlstmt, 2);
		tx = (uint64_t)sqlite3_column_int64(sqlstmt, 3);
		if (!dbdatalistadd(dbdata, rx, tx, timestamp, rowid)) {
			snprintf(errorstring, 1024, "Storing data for processing failed: %s", strerror(errno));
			printe(PT_Error);
			ret = 0;
			break;
		}
		updatelistinfo(listinfo, rx, tx, timestamp);
	}
	sqlite3_finalize(sqlstmt);

	/* clean list on failure */
	if (!ret) {
		dbdatalistfree(dbdata);
		listinfo->count = 0;
	}

	return ret;
}

void updatelistinfo(dbdatalistinfo *listinfo, const uint64_t rx, const uint64_t tx, const time_t timestamp)
{
	if (listinfo->count == 0) {
		listinfo->maxtime = timestamp;
		listinfo->mintime = timestamp;
		listinfo->maxrx = rx;
		listinfo->minrx = rx;
		listinfo->maxtx = tx;
		listinfo->mintx = tx;
		listinfo->min = rx + tx;
		listinfo->max = rx + tx;
	} else {
		if (timestamp > listinfo->maxtime) {
			listinfo->maxtime = timestamp;
		}
		if (timestamp < listinfo->mintime) {
			listinfo->mintime = timestamp;
		}
		if (rx < listinfo->minrx) {
			listinfo->minrx = rx;
		}
		if (tx < listinfo->mintx) {
			listinfo->mintx = tx;
		}
		if (rx > listinfo->maxrx) {
			listinfo->maxrx = rx;
		}
		if (tx > listinfo->maxtx) {
			listinfo->maxtx = tx;
		}
		if (rx + tx > listinfo->max) {
			listinfo->max = rx + tx;
		}
		if (rx + tx < listinfo->min) {
			listinfo->min = rx + tx;
		}
	}
	listinfo->count++;
}

int dbdatalistadd(dbdatalist **dbdata, const uint64_t rx, const uint64_t tx, const time_t timestamp, const int64_t rowid)
{
	dbdatalist *newdata;

	newdata = malloc(sizeof(dbdatalist));
	if (newdata == NULL) {
		return 0;
	}

	newdata->next = *dbdata;
	*dbdata = newdata;

	newdata->rowid = rowid;
	newdata->timestamp = timestamp;
	newdata->rx = rx;
	newdata->tx = tx;

	return 1;
}

void dbdatalistfree(dbdatalist **dbdata)
{
	dbdatalist *dbdata_prev;

	while (*dbdata != NULL) {
		dbdata_prev = *dbdata;
		*dbdata = (*dbdata)->next;
		free(dbdata_prev);
	}

}
