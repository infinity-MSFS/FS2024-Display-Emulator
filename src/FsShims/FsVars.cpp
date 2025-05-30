#include "FsVars.hpp"

#include <iostream>
#include <map>

#include "GaugeLoader/GaugeLoader.hpp"


static std::map<std::string, double> s_Vars;


extern "C" {
FsUnitId fsVarsGetUnitId(const char* unitName) {
  std::cout << "Registered unit: " << unitName << std::endl;
  return 1;
}
FsSimVarId fsVarsGetAircraftVarId(const char* simVarName) {
  auto sim_var_id = GaugeLoader::GetInstance()->AddVariable(simVarName, 0);
  return sim_var_id;
}

FsVarError fsVarsAircraftVarGet(FsSimVarId simvar, FsUnitId unit, FsVarParamArray param, double* result) {
  if (result == nullptr) {
    return FS_VAR_ERROR_INVALID_ARGS;
  }
  auto gauge_loader = GaugeLoader::GetInstance();
  auto value = gauge_loader->GetVariable(simvar);
  result[0] = value;
  return 0;
}
FsVarError fsVarsAircraftVarSet(FsSimVarId simvar, FsUnitId unit, FsVarParamArray param, double value) {
  auto gauge_loader = GaugeLoader::GetInstance();
  gauge_loader->UpdateVariable(simvar, value);
  return 0;
}
}
