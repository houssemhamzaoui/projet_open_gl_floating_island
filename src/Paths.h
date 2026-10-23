#pragma once

#include <filesystem>
#include <string>

std::filesystem::path resourcePath(const std::string& relativePath);

std::filesystem::path userDataPath(const std::string& fileName);

void showFatalError(const std::string& message);
