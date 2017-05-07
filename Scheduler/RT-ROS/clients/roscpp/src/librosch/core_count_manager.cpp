#include "ros_rosch/core_count_manager.hpp"

using namespace rosch;
SingletonCoreCountManager::SingletonCoreCountManager():
    core_(0){}

SingletonCoreCountManager::~SingletonCoreCountManager(){}

SingletonCoreCountManager &SingletonCoreCountManager::getInstance() {
  static SingletonCoreCountManager inst;
  return inst;
}

void SingletonCoreCountManager::set_core(int core) {
  core_ = core;
}

int SingletonCoreCountManager::get_core() {
  return core_;
}

int SingletonCoreCountManager::decrement() {
   --core_;
   return core_;
}
