/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ARCH_SHARED_CACHE_SIZE
#define ARCH_SHARED_CACHE_SIZE  (1024*1024)                     /* Silvermont L2 Cache */
#endif
#ifndef ARCH_DATA_CACHE_SIZE
#define ARCH_DATA_CACHE_SIZE    (24*1024)                       /* Silvermont L1 Data Cache */
#endif

/* Values are optimized for Silvermont */
#define SHARED_CACHE_SIZE       (ARCH_SHARED_CACHE_SIZE)        /* Silvermont L2 Cache */
#define DATA_CACHE_SIZE         (ARCH_DATA_CACHE_SIZE)          /* Silvermont L1 Data Cache */

#define SHARED_CACHE_SIZE_HALF	(SHARED_CACHE_SIZE / 2)
#define DATA_CACHE_SIZE_HALF	(DATA_CACHE_SIZE / 2)
