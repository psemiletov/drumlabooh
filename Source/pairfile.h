/***********************************************************
 *   this code by Peter Semiletov is Public Domain         *
 **********************************************************/

#ifndef PAIRFILE_H
#define PAIRFILE_H

#include <string>
#include <vector>
#include <map>

//using namespace std;

typedef std::map <std::string, std::string> TSPair;

class CPairFile
{
public:
  
  std::string file_name;
  
  TSPair values;

  bool get_bool (const std::string &key, bool def_value = false);
  int get_int (const std::string &key, int def_value = 0);
  size_t get_num (const std::string &key, unsigned long long def_value = 0);

  float get_float (const std::string &key, float def_value = 0.0f);
  std::string get_string (const std::string &key, const std::string &def_value = "");
  //size_t get_file_size (const std::string &key, const std::string &def_value = "64k"); //return value in bytes

  void set_string (const std::string &key, const std::string &value);
  void set_int (const std::string &key, int value);
  void set_float (const std::string &key, float value);
  
  void save();
  void load (const std::string &fname);
  
  CPairFile(){};

};

#endif
