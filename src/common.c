#include "common.h"

int printe(PrintType type)
{
	int result = 1;

	/* daemon running but log not enabled */
	if (noexit==2 && cfg.uselogging==0) {
		return 1;

	/* daemon running, log enabled */
	} else if (noexit==2) {

		switch (type) {
			case PT_Multiline:
				break;
			default:
				result = logprint(type);
				break;
		}

	/* daemon isn't running */
	} else {

		switch (type) {
			case PT_Info:
				printf("Info: %s\n", errorstring);
				break;
			case PT_Error:
				printf("Error: %s\n", errorstring);
				break;
			case PT_Config:
				printf("Config: %s\n", errorstring);
				break;
			case PT_Multiline:
				printf("%s\n", errorstring);
				break;
			case PT_ShortMultiline:
				break;
			default:
				printf("%d: %s\n", type, errorstring);
				break;
		}	

	}

	return result;
}

int logprint(PrintType type)
{
	char timestamp[22], buffer[512];
	time_t current;
	FILE *logfile;

	/* logfile */
	if (cfg.uselogging==1) {

		if ((logfile = fopen(cfg.logfile, "a")) == NULL) {
			return 0;
		}

		current = time(NULL);
		strftime(timestamp, 22, "%Y.%m.%d %H:%M:%S", localtime(&current));

		switch (type) {
			case PT_Info:
				snprintf(buffer, 512, "[%s] %s\n", timestamp, errorstring);
				break;
			case PT_Error:
				snprintf(buffer, 512, "[%s] Error: %s\n", timestamp, errorstring);
				break;
			case PT_Config:
				snprintf(buffer, 512, "[%s] Config: %s\n", timestamp, errorstring);
				break;
			case PT_Multiline:
				break;
			case PT_ShortMultiline:
				snprintf(buffer, 512, "[%s] %s\n", timestamp, errorstring);
				break;
			default:
				snprintf(buffer, 512, "[%s] (%d): %s\n", timestamp, type, errorstring);
				break;
		}
	
		if (fwrite(buffer, strlen(buffer), 1, logfile)!=1) {
			fclose(logfile);
			return 0;
		}
	
		fclose(logfile);
		return 1;
	
	/* syslog */
	} else if (cfg.uselogging==2) {

		openlog("vnstatd", LOG_PID, LOG_DAEMON);

		switch (type) {
			case PT_Multiline:
				break;
			case PT_Error:
				snprintf(buffer, 512, "Error: %s", errorstring);
				syslog(LOG_ERR, "%s", buffer);
				break;
			case PT_Config:
				snprintf(buffer, 512, "Config: %s", errorstring);
				syslog(LOG_ERR, "%s", buffer);
				break;
			case PT_Info:
			case PT_ShortMultiline:
				snprintf(buffer, 512, "%s", errorstring);
				syslog(LOG_NOTICE, "%s", buffer);
				break;
			default:
				snprintf(buffer, 512, "(%d): %s", type, errorstring);
				syslog(LOG_NOTICE, "%s", buffer);
				break;
		}

		closelog();
		return 1;
	}
	
	return 0;
}
