
#include "common.h"
#include "dbsql.h"

int db_open()
{
	int rc;
	char dbfilename[512];

	snprintf(dbfilename, 512, "%s/%s", DATABASEDIR, DATABASEFILE);

	rc = sqlite3_open(dbfilename, &db);

	if (rc) {
		if (debug)
			printf("Can't open database \"%s\": %s\n", dbfilename, sqlite3_errmsg(db));
		return 1;
	} else {
		if (debug)
			printf("Opened or created database \"%s\" successfully (%d)\n", dbfilename, rc);
	}
	return 0;
}

int db_exec(char *sql)
{
	int rc;
	sqlite3_stmt *sqlstmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &sqlstmt, NULL);
	if (rc) {
		if (debug)
			printf("  Insert prepare failed (%d): %s\n", rc, sqlite3_errmsg(db));
		return 1;
	}

	rc = sqlite3_step(sqlstmt);
	if (rc!=SQLITE_DONE) {
		if (debug)
			printf("  Insert step failed (%d): %s\n", rc, sqlite3_errmsg(db));
		return 1;
	}

	rc = sqlite3_finalize(sqlstmt);
	if (rc) {
		if (debug)
			printf("  Finalize failed (%d): %s\n", rc, sqlite3_errmsg(db));
		return 1;
	}

	return 0;
}

int db_create()
{
	int rc, i;
	char *sql;
	char *datatables[] = {"fiveminute", "hour", "day", "month", "year"};

	/* TODO: check: COMMIT or END may be missing in error cases and return gets called before COMMIT */

	rc = sqlite3_exec(db, "BEGIN", 0, 0, 0);
	if (rc) {
		return rc;
	}

	sql = "CREATE TABLE info(\n" \
		"  id       INTEGER PRIMARY KEY,\n" \
		"  name     TEXT UNIQUE NOT NULL,\n" \
		"  value    TEXT NOT NULL);";

	rc = db_exec(sql);
	if (rc) {
		return rc;
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

	rc = db_exec(sql);
	if (rc) {
		return rc;
	}

	for (i=0; i<5; i++) {
		if (debug)
			printf("%d: %s\n", i, datatables[i]);
		sql = malloc(sizeof(char)*512);

		snprintf(sql, 512, "CREATE TABLE %s(\n" \
			"  id           INTEGER PRIMARY KEY,\n" \
			"  interface    INTEGER REFERENCES interface ON DELETE CASCADE,\n" \
			"  date         DATE NOT NULL,\n" \
			"  rx           INTEGER NOT NULL,\n" \
			"  tx           INTEGER NOT NULL,\n" \
			"  CONSTRAINT u UNIQUE (interface, date));", datatables[i]);

		rc = db_exec(sql);
		free(sql);
		if (rc) {
			return rc;
		}
	}

	rc = sqlite3_exec(db, "COMMIT", 0, 0, 0);
	if (rc) {
		return rc;
	}

	return 0;
}

void db_addinterface(char *iface)
{
	char *sql;

	sql = "insert into interface (name, active, created, updated, rxcounter, txcounter, rxtotal, txtotal) values ('eth0', 1, datetime('now'), datetime('now'), 0, 0, 0, 0);";
	db_exec(sql);
}

void db_addtraffic(char *iface, int rx, int tx)
{
	int i;
	char sql[1024];
	char *datatables[] = {"fiveminute", "hour", "day", "month", "year"};
	char *datadates[] = {"datetime('now', ('-' || (strftime('%M', 'now')) || ' minutes'), ('-' || (strftime('%S', 'now')) || ' seconds'), ('+' || (round(strftime('%M', 'now')/5,0)*5) || ' minutes'))", \
			"strftime('%Y-%m-%d %H:00:00', 'now')", \
			"date('now')", "strftime('%Y-%m-01', 'now')", \
			"strftime('%Y-01-01', 'now')"};

	sqlite3_exec(db, "BEGIN", 0, 0, 0);

	/* total */
	snprintf(sql, 1024, "update interface set rxtotal=rxtotal+%d, txtotal=txtotal+%d, updated=datetime('now') where name='eth0';", rx, tx);
	db_exec(sql);

	/* time specific */
	for (i=0; i<5; i++) {
		snprintf(sql, 1024, "insert or ignore into %s (interface, date, rx, tx) values (1, %s, 0, 0);", datatables[i], datadates[i]);
		db_exec(sql);
		snprintf(sql, 1024, "update %s set rx=rx+%d, tx=tx+%d where interface=1 and date=%s;", datatables[i], rx, tx, datadates[i]);
		db_exec(sql);
	}

	sqlite3_exec(db, "COMMIT", 0, 0, 0);
}
