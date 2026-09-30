#pragma once
#include <vector>
#include <string>

std::vector<int> buildLPSArray(const std::string& pattern);
int kmpSearch(const std::string& text, const std::string& pattern);