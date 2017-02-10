/*
 * Copyright (C) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <cutils/properties.h>
#include <utils/Mutex.h>
#include <map>
#include <string>
#include <fstream>
#include <sstream>

using std::string;

// to protect against simultaneous accesses
static android::Mutex sPropertyMutex;
static bool sPropertiesInitialized = false;

// storage for properties
static std::map<std::string, std::string> sProperties;

/* Example /etc/camerahal.cfg:
 * # syntax for lines is "key=value" and no spaces, newline at end of line
 * camera.hal.debug=1
 * camera.hal.perf=1
 *
 */

void propertyInitializeValues(std::string filename)
{
    if (filename.empty())
        return;

    std::ifstream infile(filename);
    string line;
    while (std::getline(infile, line)) {
        std::istringstream isLine(line);
        string key;
        if (std::getline(isLine, key, '=')) {
            string value;
            if (key[0] == '#')
                continue;

            if (std::getline(isLine, value)) {
                sProperties[key] = value;
            }
        }
    }
    sPropertiesInitialized = true;
}

int property_get(const char *key, char *value, const char *defaultValue)
{
    android::Mutex::Autolock lock(sPropertyMutex);

    if (!sPropertiesInitialized) {
        propertyInitializeValues("/etc/camerahal.cfg");
    }

    string sKey(key);
    string cfgValue;
    if (sProperties.find(sKey) != sProperties.end()) {
        cfgValue = sProperties[sKey];
    } else {
        if (defaultValue != NULL)
            cfgValue += defaultValue;
    }

    int len = cfgValue.length();
    if (len >= PROPERTY_VALUE_MAX) {
        len = PROPERTY_VALUE_MAX - 1;
    }

    std::size_t copyLength = cfgValue.copy(value, len, 0);
    value[copyLength] = '\0';

    return len;
}
