/*
 * Histogram.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2018 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FLOW_HISTOGRAM_H
#define FLOW_HISTOGRAM_H
#pragma once

#include <flow/Arena.h>

#include <string>
#include <map>

#ifdef _WIN32
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#endif

class Histogram;

class HistogramRegistry {
    public:
        void registerHistogram(Histogram * h);
        void unregisterHistogram(Histogram * h);
        void logReport();
    private:
        std::map<std::string, Histogram*> histograms;
};

// TODO: This should be scoped properly for simulation (instead of just having all the "machines" share one histogram namespace)
HistogramRegistry & GetHistogramRegistry();

class Histogram {
public:
    enum class Unit {
        microseconds,
        bytes
    };

    Histogram(StringRef group, StringRef op, Unit unit) : group(group.toString()), op(op.toString()), unit(unit), registry(GetHistogramRegistry()) {
        clear();
        registry.registerHistogram(this);
    }

    ~Histogram() {
        registry.unregisterHistogram(this);
    }

    inline void sample(uint32_t sample) {
#ifdef _WIN32
        unsigned long index;
        buckets[_BitScanReverse(&index, sample) ? index : 0]++;
#else
        buckets[sample ? (31 - __builtin_clz(sample)) : 0]++;
#endif
    }

    inline void sampleSeconds(double delta) {
        sample((uint32_t)(delta * 1000000)); // convert to microseconds and truncate to integer
    }

    void clear() {
        for (uint32_t & i : buckets) {
            i = 0;
        }
    }
    void writeToLog();

    std::string name() {
        return group + ":" + op;
    }

    std::string const group;
    std::string const op;
    Unit const unit;
    HistogramRegistry & registry;
    
private:
    uint32_t buckets[32];
};

#endif // FLOW_HISTOGRAM_H