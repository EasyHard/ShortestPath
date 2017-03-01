#ifndef _UTILS_CONFIG_
#define _UTILS_CONFIG_
#include <map>
#include <string>
#include <cstdio>

typedef std::map<std::string, std::string> ConfigKV;
ConfigKV conf;
class Config {
 public:
  static ConfigKV readFromFile(const char* filename) {
    ConfigKV kv;
    FILE* file = fopen(filename, "r");
    assert(file != NULL);
    while (!feof(file)) {
      char s[2048];
      fgets(s, 2048, file);
      if (strlen(s) == 0) continue;
      s[strlen(s)-1] = '\0';
      char* p;
      if ((p = strchr(s, '=')) != NULL) {
        char key[2048], value[2048];
        strncpy(key, s, p - s);
        key[p-s] = '\0';
        strcpy(value, p+1);
        printf("[Config] adding <%s, %s>\n", key, value);
        kv[key] = value;
      } else {
        printf("[Config] skipping %s\n", s);
      }
    }
    conf = kv;
    return kv;
  }

};


#endif
