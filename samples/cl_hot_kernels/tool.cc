//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>

#include "cl_kernel_collector.h"

static ClKernelCollector* cpu_collector = nullptr;
static ClKernelCollector* gpu_collector = nullptr;
static std::chrono::steady_clock::time_point start;

// External Tool Interface ////////////////////////////////////////////////////

extern "C" PTI_EXPORT
void Usage() {
  std::cout <<
    "Usage: ./cl_hot_functions[.exe] <application> <args>" <<
    std::endl;
}

extern "C" PTI_EXPORT
int ParseArgs(int argc, char* argv[]) {
  return 1;
}

extern "C" PTI_EXPORT
void SetToolEnv() {}

// Internal Tool Functionality ////////////////////////////////////////////////

static uint64_t CalculateTotalTime(ClKernelCollector* collector) {
  PTI_ASSERT(collector != nullptr);
  uint64_t total_duration = 0;

  const ClKernelInfoMap& kernel_info_map = collector->GetKernelInfoMap();
  if (kernel_info_map.size() != 0) {
    for (auto& value : kernel_info_map) {
      total_duration += value.second.total_time;
    }
  }

  return total_duration;
}

static void PrintDeviceTable(
    ClKernelCollector* collector, const char* device_type) {
  PTI_ASSERT(collector != nullptr);
  PTI_ASSERT(device_type != nullptr);

  uint64_t total_duration = CalculateTotalTime(collector);
  if (total_duration > 0) {
    std::cerr << std::endl;
    std::cerr << "== " << device_type << " Backend: ==" << std::endl;
    std::cerr << std::endl;

    const ClKernelInfoMap& kernel_info_map = collector->GetKernelInfoMap();
    PTI_ASSERT(kernel_info_map.size() > 0);
    ClKernelCollector::PrintKernelsTable(kernel_info_map);
  }
}

static void PrintResults() {
  if (cpu_collector == nullptr && gpu_collector == nullptr) {
    return;
  }

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::chrono::duration<uint64_t, std::nano> time = end - start;

  std::cerr << std::endl;
  std::cerr << "=== Device Timing Results: ===" << std::endl;
  std::cerr << std::endl;
  std::cerr << "Total Execution Time (ns): " << time.count() << std::endl;

  if (cpu_collector != nullptr) {
    std::cerr << "Total Device Time for CPU (ns): " <<
      CalculateTotalTime(cpu_collector) << std::endl;
  }
  if (gpu_collector != nullptr) {
    std::cerr << "Total Device Time for GPU (ns): " <<
      CalculateTotalTime(gpu_collector) << std::endl;
  }

  if (cpu_collector != nullptr) {
    PrintDeviceTable(cpu_collector, "CPU");
  }
  if (gpu_collector != nullptr) {
    PrintDeviceTable(gpu_collector, "GPU");
  }

  std::cerr << std::endl;
}

// Internal Tool Interface ////////////////////////////////////////////////////

void EnableProfiling() {
  cl_device_id cpu_device = utils::cl::GetIntelDevice(CL_DEVICE_TYPE_CPU);
  cl_device_id gpu_device = utils::cl::GetIntelDevice(CL_DEVICE_TYPE_GPU);
  if (cpu_device == nullptr && gpu_device == nullptr) {
    std::cerr << "[WARNING] Unable to find device for tracing" << std::endl;
    return;
  }

  if (gpu_device == nullptr) {
    std::cerr << "[WARNING] Unable to find GPU device for tracing" <<
      std::endl;
  }
  if (cpu_device == nullptr) {
    std::cerr << "[WARNING] Unable to find CPU device for tracing" <<
      std::endl;
  }

  if (cpu_device != nullptr) {
    cpu_collector = ClKernelCollector::Create(cpu_device);
  }
  if (gpu_device != nullptr) {
    gpu_collector = ClKernelCollector::Create(gpu_device);
  }

  start = std::chrono::steady_clock::now();
}

void DisableProfiling() {
  if (cpu_collector != nullptr) {
    cpu_collector->DisableTracing();
  }
  if (gpu_collector != nullptr) {
    gpu_collector->DisableTracing();
  }
  PrintResults();
  if (cpu_collector != nullptr) {
    delete cpu_collector;
  }
  if (gpu_collector != nullptr) {
    delete gpu_collector;
  }
}