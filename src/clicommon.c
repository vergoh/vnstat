#include "common.h"
#include "iflist.h"
#include "dbsql.h"
#include "clicommon.h"

void showdbiflist(const int mode)
{
	int dbifcount;
	iflist *dbifl = NULL, *dbifl_i = NULL;

	if (db == NULL && !db_open_ro()) {
		printf("Error: Failed to open database \"%s\" in read-only mode: %s\n", cfg.dbfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (mode == 2) {
		dbifcount = (int)db_getinterfacecount();
		printf("%d\n", dbifcount);
		db_close();
		return;
	}

	dbifcount = db_getiflist(&dbifl);
	if (dbifcount < 0) {
		printf("Error: Failed to get interface list from database \"%s\".\n", cfg.dbfile);
		exit(EXIT_FAILURE);
	}

	if (dbifcount == 0 && mode == 0) {
		printf("Database is empty.\n");
	} else {
		dbifl_i = dbifl;

		if (mode == 0) {
			printf("Interfaces in database:");
			while (dbifl_i != NULL) {
				printf(" %s", dbifl_i->interface);
				dbifl_i = dbifl_i->next;
			}
			printf("\n");
		} else {
			while (dbifl_i != NULL) {
				printf("%s\n", dbifl_i->interface);
				dbifl_i = dbifl_i->next;
			}
		}
	}

	iflistfree(&dbifl);
	db_close();
}
