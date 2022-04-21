/*
 * Copyright � 2010 Guido Trentalancia IZ6RDB
 * This program is freeware, however it is provided as is, without
 * any warranty.
 */

/* Calculate Moon rise and set time */

/* This program is the translation in C of the Javascript algorithm
 * by Stephen R. Schmitt. For the original program, please see:
 *  http://mysite.verizon.net/res148h4j/javascript/script_moon_rise_set.html
 */

/* Before compiling, please edit the longitude and latitude variables
 * in main() according to the desired location.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <Arduino.h>

static double Sky[3] = {0.0, 0.0, 0.0};
static double Dec[3] = {0.0, 0.0, 0.0};
static double VHz[3] = {0.0, 0.0, 0.0};
static double RAn[3] = {0.0, 0.0, 0.0};

const static double DR = M_PI / 180;
const static double K1 = 15 * M_PI * 1.0027379 / 180;
static double Rise_az = 0.0, Set_az = 0.0;
static double Rise_time[2] = {0.0, 0.0}, Set_time[2] = {0.0, 0.0};

static bool Moonrise = false, Moonset = false;

/* moon's position using fundamental arguments (Van Flandern & Pulkkinen, 1979) */
static void moon(double jd)
{
  double d, f, g, h, m, n, s, u, v, w;

  h = 0.606434 + 0.03660110129 * jd;
  m = 0.374897 + 0.03629164709 * jd;
  f = 0.259091 + 0.0367481952 * jd;
  d = 0.827362 + 0.03386319198 * jd;
  n = 0.347343 - 0.00014709391 * jd;
  g = 0.993126 + 0.0027377785 * jd;

  h = h - floor(h);
  m = m - floor(m);
  f = f - floor(f);
  d = d - floor(d);
  n = n - floor(n);
  g = g - floor(g);

  h = h * 2 * M_PI;
  m = m * 2 * M_PI;
  f = f * 2 * M_PI;
  d = d * 2 * M_PI;
  n = n * 2 * M_PI;
  g = g * 2 * M_PI;

  v = 0.39558 * sin(f + n);
  v = v + 0.082 * sin(f);
  v = v + 0.03257 * sin(m - f - n);
  v = v + 0.01092 * sin(m + f + n);
  v = v + 0.00666 * sin(m - f);
  v = v - 0.00644 * sin(m + f - 2 * d + n);
  v = v - 0.00331 * sin(f - 2 * d + n);
  v = v - 0.00304 * sin(f - 2 * d);
  v = v - 0.0024 * sin(m - f - 2 * d - n);
  v = v + 0.00226 * sin(m + f);
  v = v - 0.00108 * sin(m + f - 2 * d);
  v = v - 0.00079 * sin(f - n);
  v = v + 0.00078 * sin(f + 2 * d + n);

  u = 1 - 0.10828 * cos(m);
  u = u - 0.0188 * cos(m - 2 * d);
  u = u - 0.01479 * cos(2 * d);
  u = u + 0.00181 * cos(2 * m - 2 * d);
  u = u - 0.00147 * cos(2 * m);
  u = u - 0.00105 * cos(2 * d - g);
  u = u - 0.00075 * cos(m - 2 * d + g);

  w = 0.10478 * sin(m);
  w = w - 0.04105 * sin(2 * f + 2 * n);
  w = w - 0.0213 * sin(m - 2 * d);
  w = w - 0.01779 * sin(2 * f + n);
  w = w + 0.01774 * sin(n);
  w = w + 0.00987 * sin(2 * d);
  w = w - 0.00338 * sin(m - 2 * f - 2 * n);
  w = w - 0.00309 * sin(g);
  w = w - 0.0019 * sin(2 * f);
  w = w - 0.00144 * sin(m + n);
  w = w - 0.00144 * sin(m - 2 * f - n);
  w = w - 0.00113 * sin(m + 2 * f + 2 * n);
  w = w - 0.00094 * sin(m - 2 * d + g);
  w = w - 0.00092 * sin(2 * m - 2 * d);

  s = w / sqrt(u - v * v); /* calculer l'ascension droite de la lune ...  */
  Sky[0] = h + atan(s / sqrt(1 - s * s));

  s = v / sqrt(u); /* declination ... */
  Sky[1] = atan(s / sqrt(1 - s * s));

  Sky[2] = 60.40974 * sqrt(u); /* and parallax */
}
float sgn(float x)
{
  float rv;
  if (x > 0.0)
    rv = 1;
  else if (x < 0.0)
    rv = -1;
  else
    rv = 0;
  return rv;
}
/* tester une heure pour un événement */
double test_moon(int k, double t0, double lat, double plx)
{
  double ha[3] = {0.0, 0.0, 0.0};
  double a, b, c, d, e, s, z;
  double hr, min, time;
  double az, hz, nz, dz;
  if (RAn[2] < RAn[0])
    RAn[2] = RAn[2] + 2 * M_PI;
  ha[0] = t0 - RAn[0] + (k * K1);
  ha[2] = t0 - RAn[2] + (k * K1) + K1;
  ha[1] = (ha[2] + ha[0]) / 2;    /* angle de l'heure à la demi-heure */
  Dec[1] = (Dec[2] + Dec[0]) / 2; /* déclinaison à la demi-heure */
  s = sin(DR * lat);
  c = cos(DR * lat);
  /* réfraction + semidiamètre du soleil à l'horizon + correction de la parallaxe */
  z = cos(DR * (90.567 - 41.685 / plx));
  if (k <= 0) /* premier appel de la fonction */
    VHz[0] = s * sin(Dec[0]) + c * cos(Dec[0]) * cos(ha[0]) - z;
  VHz[2] = s * sin(Dec[2]) + c * cos(Dec[2]) * cos(ha[2]) - z;
  if (sgn(VHz[0]) == sgn(VHz[2]))
    return VHz[2]; /* aucun événement cette heure-ci */
  VHz[1] = s * sin(Dec[1]) + c * cos(Dec[1]) * cos(ha[1]) - z;
  a = 2 * VHz[2] - 4 * VHz[1] + 2 * VHz[0];
  b = 4 * VHz[1] - 3 * VHz[0] - VHz[2];
  d = b * b - 4 * a * VHz[0];
  if (d < 0)
    return VHz[2]; /* aucun événement cette heure-ci */
  d = sqrt(d);
  e = (-b + d) / (2 * a);
  if ((e > 1) || (e < 0))
    e = (-b - d) / (2 * a);
  time = ((double)k) + e + 1 / 120; /* heure d'un événement + arrondi vers le haut */
  hr = floor(time);
  min = floor((time - hr) * 60);
  hz = ha[0] + e * (ha[2] - ha[0]); /* azimut de la lune lors de l'événement */
  nz = -cos(Dec[1]) * sin(hz);
  dz = c * sin(Dec[1]) - s * cos(Dec[1]) * cos(hz);
  az = atan2(nz, dz) / DR;
  if (az < 0)
    az = az + 360;
  if ((VHz[0] < 0) && (VHz[2] > 0))
  {
    Rise_time[0] = (int)hr;
    Rise_time[1] = (int)min;
    Rise_az = az;
    Moonrise = true;
  }
  if ((VHz[0] > 0) && (VHz[2] < 0))
  {
    Set_time[0] = (int)hr;
    Set_time[1] = (int)min;
    Set_az = az;
    Moonset = true;
  }
  return VHz[2];
}

/* Temps sidéral local pour la zone */
extern double lst(const double lon, const double jd, const double z)
{
  double s =
      24110.5 + 8640184.812999999 * jd / 36525 + 86636.6 * z +
      86400 * lon;
  s = s / 86400;
  s = s - floor(s);
  return s * 360 * DR;
}

/* 3-point interpolation */
double interpolate(const double f0, const double f1, const double f2, const double p)
{
  double a = f1 - f0;
  double b = f2 - f1 - a;
  double f = f0 + p * (2 * a + b * (2 * p - 1));
  return f;
}
// déterminer le jour julien à partir de la date du calendrier : Jean Meeus, "Astronomical Algorithms", Willmann-Bell, 1991
float iauJuliandate(const int day_const, const int month_const, const int year_const)
{
  float a, b, jd;
  bool gregorian;

  int month = month_const;
  int day = day_const;
  float year = (float)year_const;

  gregorian = (year < 1583) ? 0 : 1;

  if ((month == 1) || (month == 2))
  {
    year = year - 1;
    month = month + 12;
  }

  a = floor(year / 100);
  if (gregorian)
    b = 2.0 - a + floor(a / 4.0);
  else
    b = 0.0;

  jd = floor(365.25 * (float)(year + 4716.0)) + floor(30.6001 * (float)(month + 1)) + day + b - 1524.5;

  return jd;
}
/* calculer les heures de lever et de coucher de la lune */
extern void riseset(const double lat, const double lon, const int day,
                    const int month, const int year, const int TimezoneOffset, double *rise, double *set)
{
  int i, j, k;
  int zone = round(TimezoneOffset / 60);
  double ph, jd, tz, t0, mp[3][3], lon_local;
  /* guide : le jour julien a été converti de double en int */
  jd = (iauJuliandate(day, month, year)) - 2451546; /* Jour julien relatif à Jan 1.5, 2000 */
  lon_local = lon;
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
      mp[i][j] = 0.0;
  }
  lon_local = lon / 360;
  tz = ((double)zone) / 24;
  t0 = lst(lon_local, jd, tz); /* temps sidéral local */
  jd = jd + tz;                /* obtenir la position de la lune au début du jour */
  for (k = 0; k < 3; k++)
  {
    moon(jd);
    mp[k][0] = Sky[0];
    mp[k][1] = Sky[1];
    mp[k][2] = Sky[2];
    jd = jd + 0.5;
  }
  if (mp[1][0] <= mp[0][0])
    mp[1][0] = mp[1][0] + 2 * M_PI;
  if (mp[2][0] <= mp[1][0])
    mp[2][0] = mp[2][0] + 2 * M_PI;
  RAn[0] = mp[0][0];
  Dec[0] = mp[0][1];
  Moonrise = false; /* initialiser */
  Moonset = false;
  for (k = 0; k < 24; k++)
  { /* vérifier chaque heure de ce jour */
    ph = ((double)(k + 1)) / 24;
    RAn[2] = interpolate(mp[0][0], mp[1][0], mp[2][0], ph);
    Dec[2] = interpolate(mp[0][1], mp[1][1], mp[2][1], ph);
    VHz[2] = test_moon(k, t0, lat, mp[1][2]);
    RAn[0] = RAn[2]; /* avancer à l'heure suivante */
    Dec[0] = Dec[2];
    VHz[0] = VHz[2];
  }
  *rise = (double)Rise_time[0] + (double)Rise_time[1] / 60;
  *set = (double)Set_time[0] + (double)Set_time[1] / 60;
}

/* Les fonctions suivantes des phases de la lune ont été tirées de: http://www.voidware.com/moon_phase.htm */

#define RAD (PI / 180.0)
#define SMALL_FLOAT (1e-12)

static double sun_position(double j)
{
  double n, x, e, l, dl, v;
  int i;
  n = 360 / 365.2422 * j;
  i = n / 360;
  n = n - i * 360.0;
  x = n - 3.762863;
  if (x < 0)
    x += 360;
  x *= RAD;
  e = x;
  do
  {
    dl = e - 0.016718 * sin(e) - x;
    e = e - dl / (1 - 0.016718 * cos(e));
  } while (fabs(dl) >= SMALL_FLOAT);
  v = 360 / PI * atan(1.01686011182 * tan(e / 2));
  l = v + 282.596403;
  i = l / 360;
  l = l - i * 360.0;
  return l;
}

static double moon_position(double j, double ls)
{
  double ms, l, mm, n, ev, sms, ae, ec;
  int i;
  /* ls = sun_position(j) */
  ms = 0.985647332099 * j - 3.762863;
  if (ms < 0)
    ms += 360.0;
  l = 13.176396 * j + 64.975464;
  i = l / 360;
  l = l - i * 360.0;
  if (l < 0)
    l += 360.0;
  mm = l - 0.1114041 * j - 349.383063;
  i = mm / 360;
  mm -= i * 360.0;
  n = 151.950429 - 0.0529539 * j;
  i = n / 360;
  n -= i * 360.0;
  ev = 1.2739 * sin((2 * (l - ls) - mm) * RAD);
  sms = sin(ms * RAD);
  ae = 0.1858 * sms;
  mm += ev - ae - 0.37 * sms;
  ec = 6.2886 * sin(mm * RAD);
  l += ev + ec - ae + 0.214 * sin(2 * mm * RAD);
  l = 0.6583 * sin(2 * (l - ls) * RAD) + l;
  return l;
}

extern double moon_phase(int year, int month, int day, double hour, int *ip)
{
  /*
    Calculates more accurately than Moon_phase , the phase of the moon at
    the given epoch.
  */
  double j, ls, lm, t;
  j = iauJuliandate(day, month, year) - 2444238.5;
  ls = sun_position(j);
  lm = moon_position(j, ls);
  t = lm - ls;
  if (t < 0)
    t += 360;
  *ip = (int)((t + 22.5) / 45) & 0x7;
  return (1.0 - cos((lm - ls) * RAD)) / 2;
}
// check for no moonrise and/or no moonset
extern int moon_vis(int hr,int mn) {
  int riseMin=(Rise_time[0]*60)+Rise_time[1];
  int setMin=(Set_time[0]*60)+Set_time[1];
  int nowMin=(hr*60)+mn;
  if ((!Moonrise) && (!Moonset)) { // ni lever ni coucher de lune
    if (VHz[2] < 0) return(0); // en bas toute la journée
    else return(1); // debout toute la journée
    }

  if (Moonrise && Moonset) {
    if ((setMin > riseMin) && (riseMin < nowMin) && (nowMin < setMin)) return(1); // up
    if ((setMin < riseMin) && ((nowMin < setMin) || (nowMin > riseMin))) return(1); // up
  }

  if (Moonrise && (!Moonset)) { // Lever de lune seulement
    if (nowMin > riseMin) return(1);
  }

  if (Moonset && (!Moonrise)) { // Coucher de lune seulement
    if (nowMin < setMin) return(1);
  }
  return(0); // en cas de doute, tout dessous
}