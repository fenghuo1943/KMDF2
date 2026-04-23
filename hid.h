#pragma once
#include <ntddk.h>
#include <hidport.h>
#define REPORT_DESC_SIZE 52   

extern const UCHAR G_ReportDescriptor[];
extern const HID_DESCRIPTOR G_HidDescriptor;