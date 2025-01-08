#include "common.h"
#include "dbsql.h"
#include "fs.h"
#include "iflist.h"
#include "dbmerge.h"

// TODO: tests

int dbmerge(const char *srcdbfile, const char *srciface, const char *dstdbfile, const char *dstiface, const int execute)
{
	int newdb = 0, i = 0, result = 0;
	size_t maxlen = 11; // strlen("Destination") by default as it's the longest header
	iflist *srcdbifl = NULL, *dstdbifl = NULL, *dbifl_i = NULL;
	sqlite3 *srcdb = NULL, *dstdb = NULL;

	if (!db_open_filename(srcdbfile, 0, 1)) {
		return 0;
	}

	printf("     Source: %s\n", srcdbfile);

	if (strlen(srciface) > 0) {
		if (!db_getinterfacecountbyname(srciface)) {
			printf("Error: Source interface \"%s\" not found in source database.\n", srciface);
			db_close();
			return 0;
		} else {
			if (strlen(srciface) > maxlen) {
				maxlen = strlen(srciface);
			}
		}
	} else {
		if (db_getiflist(&srcdbifl) <= 0) {
			printf("Error: Source database \"%s\" has no interfaces.\n", srcdbfile);
			db_close();
			return 0;
		} else {
			dbifl_i = srcdbifl;
			while (dbifl_i != NULL) {
				if (strlen(dbifl_i->interface) > maxlen) {
					maxlen = strlen(dbifl_i->interface);
				}
				dbifl_i = dbifl_i->next;
			}
		}
	}
	if (!execute) {
		db_close();
	} else {
		srcdb = db;
		db = NULL;
	}

	if (!fileexists(dstdbfile)) {
		newdb = 1;
	}

	if (!newdb) {
		if (!db_open_filename(dstdbfile, 0, !execute)) {
			printf("Error: Failed to open destination database \"%s\": %s\n", dstdbfile, strerror(errno));
			iflistfree(&srcdbifl);
			return 0;
		}
		dstdb = db;
		if (db_getiflist(&dstdbifl) < 0) {
			db_close();
			return 0;
		}
	} else if (execute) {
		if (!db_open_filename(dstdbfile, 1, 0)) {
			printf("Error: Failed to open destination database \"%s\": %s\n", dstdbfile, strerror(errno));
			iflistfree(&srcdbifl);
			db = srcdb;
			db_close();
			return 0;
		}
		dstdb = db;
		if (db_getiflist(&dstdbifl) < 0) {
			db_close();
			db = srcdb;
			db_close();
			return 0;
		}
	}

	if (strlen(dstiface) > maxlen) {
		maxlen = strlen(dstiface);
	}

	maxlen += 4; // column padding

	printf("Destination: %s\n\n", dstdbfile);

	printf("%-*s %-*s Action\n", (int)maxlen, "Source", (int)maxlen, "Destination");
	for (i = 0; i <= (int)maxlen*2+7; i++) { // 7 = strlen(" Action")
		printf("-");
	}
	printf("\n");

	if (strlen(srciface) > 0) {
		if (strlen(dstiface) > 0) {
			printf("%-*s %-*s ", (int)maxlen, srciface, (int)maxlen, dstiface);
			if (iflistsearch(&dstdbifl, dstiface)) {
				printf("%6s\n", "merge");
			} else {
				printf("%6s\n", "copy");
			}
			if (execute) {
				result = mergeinterface(srcdb, srciface, dstdb, dstiface);
			}
		} else {
			printf("%-*s %-*s ", (int)maxlen, srciface, (int)maxlen, srciface);
			if (iflistsearch(&dstdbifl, srciface)) {
				printf("%6s\n", "merge");
			} else {
				printf("%6s\n", "copy");
			}
			if (execute) {
				result = mergeinterface(srcdb, srciface, dstdb, srciface);
			}
		}
	} else {
		dbifl_i = srcdbifl;
		while (dbifl_i != NULL) {
			printf("%-*s %-*s ", (int)maxlen, dbifl_i->interface, (int)maxlen, dbifl_i->interface);
			if (!newdb && iflistsearch(&dstdbifl, dbifl_i->interface)) {
				printf("%6s\n", "merge");
			} else {
				printf("%6s\n", "copy");
			}
			if (execute) {
				result = mergeinterface(srcdb, dbifl_i->interface, dstdb, dbifl_i->interface);
				if (!result) {
					break;
				}
			}
			dbifl_i = dbifl_i->next;
		}
	}

	iflistfree(&srcdbifl);
	iflistfree(&dstdbifl);
	db = dstdb;
	db_close();

	if (!execute) {
		printf("\nSource database will not be modified.\n");
		if (newdb) {
			printf("New database for destination will be created.\n");
		} else {
			printf("It's recommended to take a backup of the destination database as the merge operation cannot be reversed.\n");
		}
		printf("Add --force once ready to execute the merge.\n");
	} else {
		db = srcdb;
		db_close();
		if (result) {
			printf("\nMerge complete successfully.\n");
		} else {
			printf("\nMerge failed.\n");
			return 0;
		}
	}

	return 1;
}

int mergeinterface(sqlite3 *srcdb, const char *srciface, sqlite3 *dstdb, const char *dstiface)
{
	int i;
	interfaceinfo srcifinfo, dstifinfo;
	sqlite3_int64 dstifaceid = 0;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	const char *datatables[] = {"fiveminute", "hour", "day", "month", "year", "top"};

	timeused_debug(__func__, 1);

	db = srcdb;
	if (!db_getinterfaceinfo(srciface, &srcifinfo)) {
		return 0;
	}

	db = dstdb;
	dstifaceid = db_getinterfaceid(dstiface, 1);
	if (dstifaceid == 0) {
		return 0;
	}

	if (!db_getinterfaceinfo(dstiface, &dstifinfo)) {
		return 0;
	}

	if (!db_begintransaction()) {
		return 0;
	}

	if (srcifinfo.created < dstifinfo.created) {
		if (!db_setcreation(dstiface, srcifinfo.created)) {
			db_rollbacktransaction();
			return 0;
		}
		if (!db_setalias(dstiface, srcifinfo.alias)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (srcifinfo.updated > dstifinfo.updated || (dstifinfo.rxcounter == 0 && dstifinfo.txcounter == 0)) {
		if (!db_setupdated(dstiface, srcifinfo.updated)) {
			db_rollbacktransaction();
			return 0;
		}
		if (!db_setcounters(dstiface, srcifinfo.rxcounter, srcifinfo.txcounter)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (!db_settotal(dstiface, srcifinfo.rxtotal+dstifinfo.rxtotal, srcifinfo.txtotal+dstifinfo.txtotal)) {
		db_rollbacktransaction();
		return 0;
	}

	for (i = 0; i < 6; i++) {
		db = srcdb;
		if (!db_getdata(&datalist, &datainfo, srciface, datatables[i], 0)) {
			printf("Error: Failed to fetch %s data for interface \"%s\".\n", datatables[i], srciface);
			db_rollbacktransaction();
			return 0;
		}

		if (debug)
			printf("%u x %s\n", datainfo.count, datatables[i]);

		db = dstdb;
		datalist_i = datalist;
		while (datalist_i != NULL) {
			if (!mergedata(datatables[i], dstifaceid, datalist_i->rx, datalist_i->tx, datalist_i->timestamp)) {
				dbdatalistfree(&datalist);
				db_rollbacktransaction();
				return 0;
			}
			datalist_i = datalist_i->next;
		}
		dbdatalistfree(&datalist);
	}

	if (!db_committransaction()) {
		return 0;
	}

	timeused_debug(__func__, 0);
	return 1;
}

int mergedata(const char *table, const sqlite3_int64 ifaceid, const uint64_t rx, const uint64_t tx, const time_t timestamp)
{
	int i, tableindex = -1;
	char sql[1024], entrydate[64], *dgen;
	const char *datatables[] = {"fiveminute", "hour", "day", "month", "year", "top"};

	for (i = 0; i < 6; i++) {
		if (strcmp(table, datatables[i]) == 0) {
			tableindex = i;
			break;
		}
	}

	if (tableindex == -1 || timestamp == 0) {
		return 0;
	}

	snprintf(entrydate, 64, "datetime(%" PRId64 ", 'unixepoch')", (int64_t)timestamp);
	dgen = db_get_date_generator(tableindex, 1, entrydate);

	sqlite3_snprintf(1024, sql, "insert or ignore into %s (interface, date, rx, tx) values (%" PRId64 ", %s, 0, 0)", table, (int64_t)ifaceid, dgen);
	if (!db_exec(sql)) {
		return 0;
	}

	sqlite3_snprintf(1024, sql, "update %s set rx=rx+%" PRIu64 ", tx=tx+%" PRIu64 " where interface=%" PRId64 " and date=%s", table, rx, tx, (int64_t)ifaceid, dgen);
	if (!db_exec(sql)) {
		return 0;
	}

	return 1;
}
