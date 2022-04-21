extern double moon_phase(int year, int month, int day, double hour, int *ip);

extern void riseset(const double lat, const double lon, const int day,
                    const int month, const int year, const int TimezoneOffset,double *rise, double *set);

extern double lst(const double lon, const double jd, const double z);

extern int moon_vis(int hour,int min) ;