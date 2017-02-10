/*
 * Copyright (C) 2014-2016 Intel Corporation
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
 */

#include "gtest/gtest.h"
#include "test_utils.h"


const int MAX_ARGS = 200;
const char *gTestArgv[MAX_ARGS];
const char *gExecutableName;
int gTestArgc = 0;
bool gValgrindRun = false;
// If --dump command line argument is given, dump image buffer on every frame.
// Else use test default values.
bool gDumpEveryFrame = false;

int main(int argc, char* argv[])
{
    gExecutableName = argv[0];
    PRINTLN("Usage: %s [--valgrind] [--dump] ...", gExecutableName);
    // Parameterization happens during InitGoogleTest so we need to
    // extract our own cmd line arguments already before it runs.
    if (argc > 1) {
        int newArgc = 0;
        for (int i = 1; i < argc; i++) {
            // take args which don't belong to gtest
            if (strstr(argv[i], "--gtest") == NULL) {
                if (strstr(argv[i], "--valgrind") == NULL && strstr(argv[i], "--dump") == NULL) {
                    gTestArgv[newArgc] = argv[i];
                    newArgc++;
                } else {
                    if (strstr(argv[i], "--valgrind") != NULL)
                        gValgrindRun = true;
                    else if (strstr(argv[i], "--dump") != NULL)
                        gDumpEveryFrame = true;
                }
            }
        }
        if (newArgc > 0)
            gTestArgc = newArgc;
    }
    for (int i = 0; i < gTestArgc; i++) {
        PRINTLN("Sensor %d: %s", i, gTestArgv[i]);
    }

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
