#pragma once

// constructs a DATED_VERSION char[] formatted 1.2-yymmdd
#define DATED_VERSION(major, minor) const char VERSION[] = { major+'0', '.', minor+'0', '.', __DATE__[9], __DATE__[10],\
  (__DATE__[0] == 'O' || __DATE__[0] == 'N' || __DATE__[0] == 'D') ? '1' : '0',\
  (__DATE__[0] == 'J') ? ( (__DATE__[1] == 'a') ? '1' : ((__DATE__[2] == 'n') ? '6' : '7') ) :\
  (__DATE__[0] == 'F') ? '2' :\
  (__DATE__[0] == 'M') ? (__DATE__[2] == 'r') ? '3' : '5' :\
  (__DATE__[0] == 'A') ? (__DATE__[1] == 'p') ? '4' : '8' :\
  (__DATE__[0] == 'S') ? '9' :\
  (__DATE__[0] == 'O') ? '0' :\
  (__DATE__[0] == 'N') ? '1' :\
  (__DATE__[0] == 'D') ? '2' : 0,\
  __DATE__[4]==' ' ? '0' : __DATE__[4],\
  __DATE__[5], '\0' };
  