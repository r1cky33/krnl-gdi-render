#pragma once

#include "NT.hpp"
#include "Signature Scan.hpp"
#include "Render.hpp"
#include <ntdddisk.h>
#include <string>

#define DebugPrint(fmt, ...) DbgPrint("[GDIHook]: " fmt, ##__VA_ARGS__)

struct RectangleRequest {
	int posX, posY, posX1, posY1;
};