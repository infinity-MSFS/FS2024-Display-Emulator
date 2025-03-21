#pragma once

struct sGaugeInstallData {
  int iSizeX;
  int iSizeY;
  char *strParameters;
};

struct sGaugeDrawData {
  double mx;
  double my;
  double t;
  double dt;
  int winWidth;
  int winHeight;
  int fbWidth;
  int fbHeight;
};