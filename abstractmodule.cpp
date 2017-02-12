#include "rpi-radio.h"

using namespace std;

map<string, ModuleFactory> AbstractModule::moduleMap;
int AbstractModule::currentIndex = -1;
ModuleFactory AbstractModule::currentModule = NULL;

void AbstractModule::addModule(string name, ModuleFactory factory) {
  moduleMap[name] = factory;
}

int AbstractModule::initalise() {  
  for (map<string, ModuleFactory>::iterator it=moduleMap.begin(); it!=moduleMap.end(); ++it) {
    AbstractModule* module = (it->second)(); // Factory
    if ( !module ) return -1;
    if ( module->intialiseModule() ) return -1;
  }
  if ( moduleMap.size() > 0) {
    if (setCurrentIndex(0)) return -1;
  }
  return 0;
}

void AbstractModule::shutdown() {
  for (map<string, ModuleFactory>::iterator it=moduleMap.begin(); it!=moduleMap.end(); ++it) {
    AbstractModule* module = (it->second)(); // Factory
    if ( module ) delete module;
  }
  moduleMap.clear();
  currentIndex = -1;
  currentModule = NULL;
}

int AbstractModule::getSize() {
  return moduleMap.size();
}

int AbstractModule::getCurrentIndex() {
  return currentIndex;
}

AbstractModule* AbstractModule::getCurrent() {
  if (currentModule) {
    return (*currentModule)();
  }
  return NULL;
}

int AbstractModule::setCurrentIndex(int index) {
  ModuleFactory factory = NULL;
  int i = 0;
  for (map<string, ModuleFactory>::iterator it=moduleMap.begin(); it!=moduleMap.end(); ++it) {
    if (index == i) {
      factory = it->second;
    }
    i++;    
  }
  if ( !factory ) return -1;

  currentIndex = index;
  currentModule = factory;
  
  return 0;
}

const string* AbstractModule::getDescriptions() {
  string* result = new string[getSize()];
  int i = 0;
  for (map<string, ModuleFactory>::iterator it=moduleMap.begin(); it!=moduleMap.end(); ++it) {
    AbstractModule* module = (it->second)(); // Factory
    result[i] = module->moduleDesc;
    i++;    
  }
return result;
}

AbstractModule::~AbstractModule() {
}
  
AbstractModule::AbstractModule(std::string name, std::string desc) 
  : moduleName(name), moduleDesc(desc) {
}

