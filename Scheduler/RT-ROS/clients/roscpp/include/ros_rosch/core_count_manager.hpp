#ifndef CORE_COUNT_MANAGER_HPP
#define CORE_COUNT_MANAGER_HPP

namespace rosch {
class SingletonCoreCountManager {
private:
  SingletonCoreCountManager();
  SingletonCoreCountManager &operator=(const SingletonCoreCountManager &);
  ~SingletonCoreCountManager();
  int core_;
  // int next

public:
  static SingletonCoreCountManager &getInstance();
  void set_core(int core);
  int get_core();
  int decrement();
};
}

#endif // CORE_COUNT_MANAGER_HPP