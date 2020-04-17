#include "common.h"
#include "misc.h"
#include "iflist.h"
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

	if (db != NULL) {
		return 1;
	}

	snprintf(dbfilename, 530, "%s/%s", cfg.dbdir, DATABASEFILE);

	/* create database if file doesn't exist */
	if (stat(dbfilename, &filestat) != 0) {
		if (errno == ENOENT && createifnotfound && !readonly) {
			createdb = 1;
		} else {
			if (debug)
				printf("Error (debug): Handling database \"%s\" failed: %s\n", dbfilename, strerror(errno));
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
			printf("Error (debug): Can't open database \"%s\": %s\n", dbfilename, sqlite3_errmsg(db));
		return 0;
	} else {
		if (debug)
			printf("Database \"%s\" open (ro: %d)\n", dbfilename, readonly);
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
				printf("Error (debug): Creating database \"%s\" structure failed\n", dbfilename);
			db_close();
			return 0;
		} else {
			if (debug)
				printf("Database \"%s\" structure created\n", dbfilename);
			if (!db_setinfo("dbversion", SQLDBVERSION, 1)) {
				if (debug)
					printf("Error (debug): Writing version info to database \"%s\" failed\n", dbfilename);
				db_close();
				return 0;
			}
		}
	}

	/* set pragmas */
	if (!readonly) {
		sqlite3_busy_timeout(db, cfg.updateinterval * 1000);
		if (!db_setpragmas()) {
			db_close();
			return 0;
		}
	} else {
		/* set busy timeout when database is open in read-only mode */
		sqlite3_busy_timeout(db, DBREADTIMEOUTSECS * 1000);
	}

	if (!createdb) {
		if (!db_validate(readonly)) {
			db_close();
			return 0;
		}
	}

	if (createifnotfound && !readonly) {
		if (!db_setinfo("vnstatversion", getversion(), 1)) {
			db_close();
			return 0;
		}
	}

	return 1;
}

int db_validate(const int readonly)
{
	int dbversion, currentversion;

	db_errcode = 0;
	dbversion = atoi(db_getinfo("dbversion"));
	if (db_errcode) {
		return 0;
	}

	currentversion = atoi(SQLDBVERSION);

	if (debug) {
		printf("Database version \"%d\", current version \"%d\"\n", dbversion, currentversion);
	}

	if (dbversion == currentversion) {
		return 1;

	} else if (dbversion == 0) {
		printf("Error: Database version \"%d\" suggests error situation in database, exiting.\n", dbversion);
		return 0;

	} else if (dbversion > currentversion) {
		printf("Error: Database version \"%d\" is not supported. Support is available up to version \"%d\", exiting.\n", dbversion, currentversion);
		return 0;

	} else if (dbversion < currentversion) {
		if (readonly) {
			/* database upgrade actions should be performed here once needed */
			printf("Error: Unable to upgrade read-only database from version \"%d\" to \"%d\", exiting.\n", dbversion, currentversion);
			return 0;
		}
		/* database upgrade actions should be performed here once needed, then return 1 */
		/* however, since this is the first database version, always return 0 */
	}

	return 0;
}

int db_setpragmas(void)
{
	int rc;
	char sql[25];
	sqlite3_stmt *sqlstmt;

	/* enable use of foreign keys */
	if (!db_exec("PRAGMA foreign_keys = ON")) {
		return 0;
	}

	rc = sqlite3_prepare_v2(db, "PRAGMA foreign_keys", -1, &sqlstmt, NULL);
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
		snprintf(errorstring, 1024, "Exec finalize \"PRAGMA foreign_keys;\" failed (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		return 0;
	}

#if HAVE_DECL_SQLITE_CHECKPOINT_RESTART
	/* set journal_mode */
	if (cfg.waldb) {
		if (!db_exec("PRAGMA journal_mode = WAL")) {
			return 0;
		}
	} else {
		if (!db_exec("PRAGMA journal_mode = DELETE")) {
			return 0;
		}
	}
#endif

	/* set synchronous */
	if (cfg.dbsynchronous == -1) {
#if HAVE_DECL_SQLITE_CHECKPOINT_RESTART
		if (cfg.waldb) {
			if (!db_exec("PRAGMA synchronous = 1")) {
				return 0;
			}
		} else {
			if (!db_exec("PRAGMA synchronous = 2")) {
				return 0;
			}
		}
#else
		if (!db_exec("PRAGMA synchronous = 2")) {
			return 0;
		}
#endif
	} else {
		snprintf(sql, 25, "PRAGMA synchronous = %d", cfg.dbsynchronous);
		if (!db_exec(sql)) {
			return 0;
		}
	}

	return 1;
}

int db_close(void)
{
	int rc;

	if (db == NULL) {
		return 1;
	}

	rc = sqlite3_close(db);
	if (rc == SQLITE_OK) {
		db = NULL;
		if (debug)
			printf("Database closed\n");
		return 1;
	} else {
		db_errcode = rc;
		if (debug)
			printf("Error (debug): Closing database failed (%d): %s\n", rc, sqlite3_errmsg(db));
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
	if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
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
	const char *constsql;
	char *sql;
	char buffer[32];
	const char *datatables[] = {"fiveminute", "hour", "day", "month", "year", "top"};

	if (!db_begintransaction()) {
		return 0;
	}

	constsql = "CREATE TABLE info(\n"
		  "  id       INTEGER PRIMARY KEY,\n"
		  "  name     TEXT UNIQUE NOT NULL,\n"
		  "  value    TEXT NOT NULL)";

	if (!db_exec(constsql)) {
		db_rollbacktransaction();
		return 0;
	}

	constsql = "CREATE TABLE interface(\n"
		  "  id           INTEGER PRIMARY KEY,\n"
		  "  name         TEXT UNIQUE NOT NULL,\n"
		  "  alias        TEXT,\n"
		  "  active       INTEGER NOT NULL,\n"
		  "  created      DATE NOT NULL,\n"
		  "  updated      DATE NOT NULL,\n"
		  "  rxcounter    INTEGER NOT NULL,\n"
		  "  txcounter    INTEGER NOT NULL,\n"
		  "  rxtotal      INTEGER NOT NULL,\n"
		  "  txtotal      INTEGER NOT NULL)";

	if (!db_exec(constsql)) {
		db_rollbacktransaction();
		return 0;
	}

	sql = malloc(sizeof(char) * 512);
	for (i = 0; i < 6; i++) {
		sqlite3_snprintf(512, sql, "CREATE TABLE %s(\n"
								   "  id           INTEGER PRIMARY KEY,\n"
								   "  interface    INTEGER REFERENCES interface(id) ON DELETE CASCADE,\n"
								   "  date         DATE NOT NULL,\n"
								   "  rx           INTEGER NOT NULL,\n"
								   "  tx           INTEGER NOT NULL,\n"
								   "  CONSTRAINT u UNIQUE (interface, date))",
						 datatables[i]);

		if (!db_exec(sql)) {
			free(sql);
			db_rollbacktransaction();
			return 0;
		}
	}
	free(sql);

	snprintf(buffer, 32, "%" PRIu64 "", (uint64_t)MAX32);
	if (!db_setinfo("btime", buffer, 1)) {
		db_rollbacktransaction();
		return 0;
	}

	return db_committransaction();
}

int db_addinterface(const char *iface)
{
	char sql[256];

	if (!strlen(iface)) {
		return 0;
	}

	sqlite3_snprintf(256, sql, "insert into interface (name, active, created, updated, rxcounter, txcounter, rxtotal, txtotal) values ('%q', 1, datetime('now', 'localtime'), datetime('now', 'localtime'), 0, 0, 0, 0)", iface);
	return db_exec(sql);
}

int db_removeinterface(const char *iface)
{
	char sql[64];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(64, sql, "delete from interface where id=%" PRId64 "", (int64_t)ifaceid);
	return db_exec(sql);
}

int db_renameinterface(const char *iface, const char *newifname)
{
	char sql[128];
	sqlite3_int64 ifaceid = 0;

	if (!strlen(newifname)) {
		return 0;
	}

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(128, sql, "update interface set name='%q' where id=%" PRId64 "", newifname, (int64_t)ifaceid);
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
	char sql[128], *inquery = NULL;
	sqlite3_stmt *sqlstmt;

	if (strchr(iface, '+') == NULL) {
		if (strlen(iface) > 0) {
			sqlite3_snprintf(128, sql, "select count(*) from interface where name='%q'", iface);
		} else {
			sqlite3_snprintf(128, sql, "select count(*) from interface");
		}
	} else {
		inquery = getifaceinquery(iface);
		if (inquery == NULL) {
			return 0;
		}
		sqlite3_snprintf(128, sql, "select count(*) from interface where name in (%q)", inquery);
		free(inquery);
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

	/* consider merge query as invalid if not all requested interfaces are found or are not unique */
	if (strchr(iface, '+') != NULL) {
		if (result != getqueryinterfacecount(iface)) {
			result = 0;
		}
	}

	return result;
}

sqlite3_int64 db_getinterfaceid(const char *iface, const int createifnotfound)
{
	int rc;
	char sql[128];
	sqlite3_int64 ifaceid = 0;
	sqlite3_stmt *sqlstmt;

	sqlite3_snprintf(128, sql, "select id from interface where name='%q'", iface);
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

char *db_getinterfaceidin(const char *iface)
{
	int rc;
	char sql[256], *result, *inquery;
	sqlite3_stmt *sqlstmt;

	result = NULL;
	inquery = getifaceinquery(iface);
	if (inquery == NULL) {
		return NULL;
	}

	sqlite3_snprintf(256, sql, "select group_concat(id) from interface where name in (%q)", inquery);
	free(inquery);
	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc == SQLITE_OK) {
		if (sqlite3_step(sqlstmt) == SQLITE_ROW) {
			if (sqlite3_column_text(sqlstmt, 0) != NULL) {
				result = strdup((const char *)sqlite3_column_text(sqlstmt, 0));
			}
		}
		sqlite3_finalize(sqlstmt);
	} else {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Failed to get interface id from database (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
	}

	return result;
}

int db_setactive(const char *iface, const int active)
{
	char sql[64];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(64, sql, "update interface set active=%d where id=%" PRId64 "", active, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_setupdated(const char *iface, const time_t timestamp)
{
	char sql[256];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(256, sql, "update interface set updated=datetime(%" PRIu64 ", 'unixepoch', 'localtime') where id=%" PRId64 "", (uint64_t)timestamp, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_setcounters(const char *iface, const uint64_t rxcounter, const uint64_t txcounter)
{
	char sql[256];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(256, sql, "update interface set rxcounter=%" PRIu64 ", txcounter=%" PRIu64 " where id=%" PRId64 "", rxcounter, txcounter, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_getcounters(const char *iface, uint64_t *rxcounter, uint64_t *txcounter)
{
	int rc;
	char sql[128];
	sqlite3_int64 ifaceid = 0;
	sqlite3_stmt *sqlstmt;

	*rxcounter = *txcounter = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(128, sql, "select rxcounter, txcounter from interface where id=%" PRId64 "", (int64_t)ifaceid);
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
	char sql[512], *ifaceidin = NULL;
	sqlite3_int64 ifaceid;
	sqlite3_stmt *sqlstmt;

	if (strchr(iface, '+') == NULL) {
		ifaceid = db_getinterfaceid(iface, 0);
		if (ifaceid == 0) {
			return 0;
		}
		sqlite3_snprintf(512, sql, "select name, alias, active, strftime('%%s', created, 'utc'), strftime('%%s', updated, 'utc'), rxcounter, txcounter, rxtotal, txtotal from interface where id=%" PRId64 "", (int64_t)ifaceid);
	} else {
		ifaceidin = db_getinterfaceidin(iface);
		if (ifaceidin == NULL || strlen(ifaceidin) < 1) {
			free(ifaceidin);
			return 0;
		}
		sqlite3_snprintf(512, sql, "select \"%q\", NULL, max(active), max(strftime('%%s', created, 'utc')), min(strftime('%%s', updated, 'utc')), 0, 0, sum(rxtotal), sum(txtotal) from interface where id in (%q)", iface, ifaceidin);
		free(ifaceidin);
	}

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
	char sql[128];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(128, sql, "update interface set alias='%q' where id=%" PRId64 "", alias, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_setinfo(const char *name, const char *value, const int createifnotfound)
{
	int rc;
	char sql[128];

	sqlite3_snprintf(128, sql, "update info set value='%q' where name='%q'", value, name);
	rc = db_exec(sql);
	if (!rc || (!sqlite3_changes(db) && !createifnotfound)) {
		return 0;
	}
	if (!sqlite3_changes(db) && createifnotfound) {
		sqlite3_snprintf(512, sql, "insert into info (name, value) values ('%q', '%q')", name, value);
		rc = db_exec(sql);
	}
	return rc;
}

char *db_getinfo(const char *name)
{
	int rc;
	char sql[128];
	static char buffer[64];
	sqlite3_stmt *sqlstmt;

	buffer[0] = '\0';

	sqlite3_snprintf(128, sql, "select value from info where name='%q'", name);
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

int db_getiflist(iflist **ifl)
{
	return db_getiflist_sorted(ifl, 0);
}

int db_getiflist_sorted(iflist **ifl, const int orderbytraffic)
{
	int rc;
	const char *constsql;
	sqlite3_stmt *sqlstmt;

	if (!orderbytraffic) {
		constsql = "select name from interface order by name asc";
	} else {
		constsql = "select name from interface order by rxtotal+txtotal desc";
	}

	rc = sqlite3_prepare_v2(db, constsql, -1, &sqlstmt, NULL);
	if (rc != SQLITE_OK) {
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
		if (!iflistadd(ifl, (const char *)sqlite3_column_text(sqlstmt, 0), 0)) {
			break;
		}
		rc++;
	}

	sqlite3_finalize(sqlstmt);

	return rc;
}

char *db_get_date_generator(const int range, const short direct, const char *nowdate)
{
	static char dgen[512];
	dgen[0] = '\0';

	switch (range) {
		case 0: /* 5min */
			snprintf(dgen, 512, "datetime(%s, ('-' || (strftime('%%M', %s)) || ' minutes'), ('-' || (strftime('%%S', %s)) || ' seconds'), ('+' || (round(strftime('%%M', %s)/5,0)*5) || ' minutes'), 'localtime')", nowdate, nowdate, nowdate, nowdate);
			break;
		case 1: /* hour */
			snprintf(dgen, 512, "strftime('%%Y-%%m-%%d %%H:00:00', %s, 'localtime')", nowdate);
			break;
		case 2: /* day */
		case 5: /* top */
			snprintf(dgen, 512, "date(%s, 'localtime')", nowdate);
			break;
		case 3: /* month */
			if (direct || cfg.monthrotate == 1) {
				snprintf(dgen, 512, "strftime('%%Y-%%m-01', %s, 'localtime')", nowdate);
			} else {
				snprintf(dgen, 512, "strftime('%%Y-%%m-01', datetime(%s, '-%d days'), 'localtime')", nowdate, cfg.monthrotate - 1);
			}
			break;
		case 4: /* year */
			if (direct || cfg.monthrotate == 1 || cfg.monthrotateyears == 0) {
				snprintf(dgen, 512, "strftime('%%Y-01-01', %s, 'localtime')", nowdate);
			} else {
				snprintf(dgen, 512, "strftime('%%Y-01-01', datetime(%s, '-%d days'), 'localtime')", nowdate, cfg.monthrotate - 1);
			}
			break;
		default:
			break;
	}
	return dgen;
}

int db_addtraffic(const char *iface, const uint64_t rx, const uint64_t tx)
{
	return db_addtraffic_dated(iface, rx, tx, 0);
}

int db_addtraffic_dated(const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp)
{
	int i, intransaction = db_intransaction;
	char sql[1024], nowdate[64];
	sqlite3_int64 ifaceid = 0;

	const char *datatables[] = {"fiveminute", "hour", "day", "month", "year", "top"};
	int32_t *featurecfg[] = {&cfg.fiveminutehours, &cfg.hourlydays, &cfg.dailydays, &cfg.monthlymonths, &cfg.yearlyyears, &cfg.topdayentries};

	ifaceid = db_getinterfaceid(iface, 1);
	if (ifaceid == 0) {
		return 0;
	}

	if (timestamp > 0) {
		snprintf(nowdate, 64, "datetime(%" PRIu64 ", 'unixepoch')", timestamp);
	} else {
		snprintf(nowdate, 64, "'now'");
	}

	if (debug)
		printf("db add %s (%" PRId64 ") %" PRIu64 ": rx %" PRIu64 " - tx %" PRIu64 "\n", iface, (int64_t)ifaceid, timestamp, rx, tx);

	if (!intransaction) {
		if (!db_begintransaction()) {
			return 0;
		}
	}

	/* total */
	if (rx > 0 || tx > 0) {
		sqlite3_snprintf(1024, sql, "update interface set rxtotal=rxtotal+%" PRIu64 ", txtotal=txtotal+%" PRIu64 " where id=%" PRId64 "", rx, tx, (int64_t)ifaceid);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	/* time specific */
	for (i = 0; i < 6; i++) {
		if (featurecfg[i] == 0) {
			continue;
		}
		sqlite3_snprintf(1024, sql, "insert or ignore into %s (interface, date, rx, tx) values (%" PRId64 ", %s, 0, 0)", datatables[i], (int64_t)ifaceid, db_get_date_generator(i, 0, nowdate));
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
		sqlite3_snprintf(1024, sql, "update %s set rx=rx+%" PRIu64 ", tx=tx+%" PRIu64 " where interface=%" PRId64 " and date=%s", datatables[i], rx, tx, (int64_t)ifaceid, db_get_date_generator(i, 0, nowdate));
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
	char sql[256];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(256, sql, "update interface set created=datetime(%" PRIu64 ", 'unixepoch', 'localtime') where id=%" PRId64 "", (uint64_t)timestamp, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_settotal(const char *iface, const uint64_t rx, const uint64_t tx)
{
	char sql[256];
	sqlite3_int64 ifaceid = 0;

	ifaceid = db_getinterfaceid(iface, 0);
	if (ifaceid == 0) {
		return 0;
	}

	sqlite3_snprintf(256, sql, "update interface set rxtotal=%" PRIu64 ", txtotal=%" PRIu64 " where id=%" PRId64 "", rx, tx, (int64_t)ifaceid);
	return db_exec(sql);
}

int db_insertdata(const char *table, const char *iface, const uint64_t rx, const uint64_t tx, const uint64_t timestamp)
{
	int i, index = -1;
	char sql[1024], nowdate[64];
	sqlite3_int64 ifaceid = 0;

	const char *datatables[] = {"hour", "day", "month", "year", "top"};

	for (i = 0; i < 5; i++) {
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

	snprintf(nowdate, 64, "datetime(%" PRIu64 ", 'unixepoch')", timestamp);

	sqlite3_snprintf(1024, sql, "insert or ignore into %s (interface, date, rx, tx) values (%" PRId64 ", %s, %" PRIu64 ", %" PRIu64 ")", table, (int64_t)ifaceid, db_get_date_generator(index + 1, 1, nowdate), rx, tx);
	return db_exec(sql);
}

int db_removeoldentries(void)
{
	char sql[256];

	if (debug) {
		printf("db: removing old entries\n");
	}

	if (!db_begintransaction()) {
		return 0;
	}

	if (!db_removeoldentries_top()) {
		db_rollbacktransaction();
		return 0;
	}

	if (cfg.fiveminutehours > 0) {
		if (debug) {
			printf("db: fiveminute cleanup (%dh)\n", cfg.fiveminutehours);
		}
		sqlite3_snprintf(256, sql, "delete from fiveminute where date < datetime('now', '-%d hours', 'localtime')", cfg.fiveminutehours);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (cfg.hourlydays > 0) {
		if (debug) {
			printf("db: hour cleanup (%dd)\n", cfg.hourlydays);
		}
		sqlite3_snprintf(256, sql, "delete from hour where date < datetime('now', '-%d days', 'localtime')", cfg.hourlydays);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (cfg.dailydays > 0) {
		if (debug) {
			printf("db: day cleanup (%dd)\n", cfg.dailydays);
		}
		sqlite3_snprintf(256, sql, "delete from day where date < date('now', '-%d days', 'localtime')", cfg.dailydays);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (cfg.monthlymonths > 0) {
		if (debug) {
			printf("db: month cleanup (%dm)\n", cfg.monthlymonths);
		}
		sqlite3_snprintf(256, sql, "delete from month where date < date('now', '-%d months', 'localtime')", cfg.monthlymonths);
		if (!db_exec(sql)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (cfg.yearlyyears > 0) {
		if (debug) {
			printf("db: year cleanup (%dy)\n", cfg.yearlyyears);
		}
		sqlite3_snprintf(256, sql, "delete from year where date < date('now', '-%d years', 'localtime')", cfg.yearlyyears);
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
	iflist *dbifl = NULL, *dbifl_iterator = NULL;
	sqlite3_int64 ifaceid;

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

		sqlite3_snprintf(512, sql, "delete from top where id in ( select id from top where interface=%" PRId64 " and date!=date('now', 'localtime') order by rx+tx desc limit -1 offset %d )", (int64_t)ifaceid, cfg.topdayentries);

		if (!db_exec(sql)) {
			errorcount++;
			dbifl_iterator = dbifl_iterator->next;
			continue;
		}

		dbifl_iterator = dbifl_iterator->next;
	}

	iflistfree(&dbifl);

	if (errorcount) {
		return 0;
	}

	return 1;
}

int db_vacuum(void)
{
	if (debug) {
		printf("db: vacuum\n");
	}
	return db_exec("VACUUM");
}

int db_begintransaction(void)
{
	int rc;

	if (debug) {
		printf("db: begin transaction\n");
	}

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

	if (debug) {
		printf("db: commit transaction\n");
	}

	db_intransaction = 0;

	rc = sqlite3_exec(db, "COMMIT", 0, 0, 0);
	if (rc) {
		snprintf(errorstring, 1024, "Commit transaction to database failed (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		/* execute rollback if commit failure left the transaction open */
		if (!sqlite3_get_autocommit(db)) {
			db_rollbacktransaction();
		}
		db_errcode = rc;
		return 0;
	}
	return 1;
}

int db_rollbacktransaction(void)
{
	int rc;

	if (debug) {
		printf("db: rollback transaction\n");
	}

	db_intransaction = 0;

	rc = sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
	if (rc) {
		db_errcode = rc;
		snprintf(errorstring, 1024, "Transaction rollback failed (%d): %s", rc, sqlite3_errmsg(db));
		printe(PT_Error);
		return 0;
	}
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

int db_isdiskfull(int errcode)
{
	if (errcode == SQLITE_FULL) {
		return 1;
	} else {
		return 0;
	}
}

#if HAVE_DECL_SQLITE_CHECKPOINT_RESTART
void db_walcheckpoint(void)
{
	double used_secs = 0.0;

	timeused(__func__, 1);
#if HAVE_DECL_SQLITE_CHECKPOINT_TRUNCATE
	sqlite3_wal_checkpoint_v2(db, NULL, SQLITE_CHECKPOINT_TRUNCATE, NULL, NULL);
#else
	sqlite3_wal_checkpoint_v2(db, NULL, SQLITE_CHECKPOINT_RESTART, NULL, NULL);
#endif
	timeused(__func__, 0);

	used_secs = timeused(__func__, 0);
	if (used_secs > SLOWDBWARNLIMIT) {
		snprintf(errorstring, 1024, "Write-Ahead Logging checkpoint took %.1f seconds.", used_secs);
		printe(PT_Warning);
	}
}
#endif

int db_getdata(dbdatalist **dbdata, dbdatalistinfo *listinfo, const char *iface, const char *table, const uint32_t resultlimit)
{
	return db_getdata_range(dbdata, listinfo, iface, table, resultlimit, "", "");
}

int db_getdata_range(dbdatalist **dbdata, dbdatalistinfo *listinfo, const char *iface, const char *table, const uint32_t resultlimit, const char *databegin, const char *dataend)
{
	int ret = 1, i, rc;
	const char *datatables[] = {"fiveminute", "hour", "day", "month", "year", "top"};
	char sql[512], limit[64], dbegin[32], dend[44], *ifaceidin = NULL;
	sqlite3_stmt *sqlstmt;
	time_t timestamp;
	int64_t rowid;
	uint64_t rx, tx;

	listinfo->count = 0;

	ret = 0;
	for (i = 0; i < 6; i++) {
		if (strcmp(table, datatables[i]) == 0) {
			ret = 1;
			break;
		}
	}
	if (!ret) {
		return 0;
	}

	ifaceidin = db_getinterfaceidin(iface);
	if (ifaceidin == NULL) {
		return 0;
	}

	dbegin[0] = '\0';
	if (strlen(databegin)) {
		snprintf(dbegin, 32, "and date >= '%s'", databegin);
	}

	dend[0] = '\0';
	if (strlen(dataend)) {
		if (strchr(dataend, ':')) {
			snprintf(dend, 44, "and date <= datetime('%s')", dataend);
		} else {
			snprintf(dend, 44, "and date <= datetime('%s 23:59:59')", dataend);
		}
	}

	limit[0] = '\0';
	if (resultlimit > 0 && (!strlen(dbegin) || !strlen(dend))) {
		snprintf(limit, 64, "limit %" PRIu32 "", resultlimit);
	}

	/* note that using the linked list reverses the order */
	/* most recent last in the linked list is considered the normal order */
	if (strcmp(table, "top") == 0) {
		/* 'top' entries, requires different query due to rx+tx ordering */
		if (strlen(dbegin)) {
			if (resultlimit > 0) {
				snprintf(limit, 64, "limit %" PRIu32 "", resultlimit);
			}
			sqlite3_snprintf(512, sql, "select * from (select id, strftime('%%s', date, 'utc'), sum(rx) as rx, sum(tx) as tx from day where interface in (%q) %s %s group by date order by rx+tx desc %s) order by rx+tx asc", ifaceidin, dbegin, dend, limit);
		} else {
			sqlite3_snprintf(512, sql, "select * from (select id, strftime('%%s', date, 'utc'), sum(rx) as rx, sum(tx) as tx from top where interface in (%q) group by date order by rx+tx desc %s) order by rx+tx asc", ifaceidin, limit);
		}
	} else {
		if (strlen(dbegin) && strlen(limit)) {
			sqlite3_snprintf(512, sql, "select * from (select id, strftime('%%s', date, 'utc') as unixdate, sum(rx), sum(tx) from %s where interface in (%q) %s %s group by date order by date asc %s) order by unixdate desc", table, ifaceidin, dbegin, dend, limit);
		} else {
			sqlite3_snprintf(512, sql, "select id, strftime('%%s', date, 'utc'), sum(rx), sum(tx) from %s where interface in (%q) %s %s group by date order by date desc %s", table, ifaceidin, dbegin, dend, limit);
		}
	}
	free(ifaceidin);

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
		listinfo->sumrx = rx;
		listinfo->sumtx = tx;
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
		listinfo->sumrx += rx;
		listinfo->sumtx += tx;
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

unsigned int getqueryinterfacecount(const char *input)
{
	unsigned int i, ifacecount = 1;

	if (!strlen(input) || input[0] == '+' || input[strlen(input) - 1] == '+') {
		return 0;
	}

	for (i = 0; i < (unsigned int)strlen(input); i++) {
		if (input[i] == '+') {
			if (i > 0 && input[i - 1] == '+') {
				return 0;
			} else {
				ifacecount++;
			}
		}
	}

	return ifacecount;
}

char *getifaceinquery(const char *input)
{
	unsigned int i, j, ifacecount = 1;
	char *result;

	ifacecount = getqueryinterfacecount(input);

	if (ifacecount == 0) {
		return NULL;
	}

	/* each interface requires two quotes and comma or \0 so 3 extra chars */
	j = (unsigned int)strlen(input) + ifacecount * 3;
	result = malloc(sizeof(char) * j);
	if (result == NULL) {
		return NULL;
	}
	memset(result, '\0', j);

	result[0] = '"';
	j = 1;
	for (i = 0; i < (unsigned int)strlen(input); i++) {
		if (input[i] == '+') {
			strcat(result, "\",\"");
			j += 3;
		} else {
			result[j] = input[i];
			j++;
		}
	}
	result[j] = '"';

	return result;
}
