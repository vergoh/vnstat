
# New configuration settings

 * 2.4 - 2.6: (none)

 * 2.3: DatabaseWriteAheadLogging, DatabaseSynchronous

 * 2.2: 64bitInterfaceCounters

 * 2.1: (none)

 * 2.0: List5Mins, ListHours, ListDays, ListMonths, ListYears, ListTop,
        5MinuteHours, HourlyDays, DailyDays, MonthlyMonths, YearlyYears,
        TopDayEntries, MonthRotateAffectsYears, TrafficlessEntries

 * 1.18: TimeSyncWait, DefaultDecimals, HourlyDecimals, HourlySectionStyle

 * 1.17: (none)

 * 1.16: RateUnitMode

 * 1.14 - 1.15: (none)

 * 1.13: BandwidthDetection, BandwidthDetectionInterval

 * 1.12: DaemonUser, DaemonGroup, CreateDirs, UpdateFileOwner

 * 1.10 - 1.11: (none)

 * 1.9: OutputStyle, SummaryLayout, SummaryRate, SaveOnStatusChange,
        OfflineSaveInterval

 * 1.8: ShowRate, RateUnit, TrafficlessDays, HourlyRate, TransparentBg

 * 1.7: UnitMode + all settings under vnstatd and vnstati


# Upgrading from versions 1.3 and later

All past versions starting from 1.3 can be upgraded directly to the current
version without having to use any intermediate versions.

 1. Follow the normal install procedure, `make install` step will upgrade the
    configuration file if it exists in the usual location. All user settings
    will be kept. However, unidentified content, such as unknown keywords
    and additional comments, will get cleaned away.

 2. Review the updated configuration file. Refer to comments in the file and
    the `vnstat.conf` man page for documentation regarding each setting.

 3. After the configuration file has been updated, restart the daemon
    (`vnstatd`) or configure it to use if not previously done already. Note
    that using a cron entry for updates (something like `vnstat -u`) is no
    longer supported and such commands should be removed if still existing.

 4. Upgrades from 1.x to 2.x will result in data being imported from the
    legacy databases of each interface during the first start of the new
    daemon (`vnstatd`) binary. The `vnstat` and `vnstati` commands will not
    be able to access any data before this part of the upgrade has been
    completed. The legacy databases will be left untouched by the import.
