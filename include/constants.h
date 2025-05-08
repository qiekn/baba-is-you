#pragma once

#include <filesystem>

const int KFps = 60;

const bool kAnimated = false;
const bool kMaxUndoHistry = 99;

const int kCellSize = 24;
const int kScale = 2;
const int kRows = 18;
const int kCols = 22;
const int kCellsPerRow = kCols;
const int kCellsPerColumn = kRows;

const int kScreenBorder = 1 * kCellSize * kScale;
const int kScreenWidth = kCellSize * kCols * kScale + kScreenBorder;
const int kScreenHeight = kCellSize * kRows * kScale + kScreenBorder;

const std::filesystem::path kAssets = ASSETS;
const std::filesystem::path kLevels = LEVELS;
