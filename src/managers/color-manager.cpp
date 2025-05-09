#include "managers/color-manager.h"
#include <raylib.h>
#include <fstream>
#include <string>
#include "games/object.h"
#include "games/utils.h"
#include "imgui.h"
#include "json.h"

ColorManager::ColorManager() { InitDefaultColors(); }

ColorManager::~ColorManager() {}

Color ColorManager::GetColor(ObjectType type) const {
  auto it = color_map_.find(type);
  if (it != color_map_.end()) {
    return it->second;
  }
  // if not found, return default color
  auto default_it = color_map_.find(ObjectType::DEFAULT);
  if (default_it != color_map_.end()) {
    return default_it->second;
  }
  // fallback to white if nothing is defined
  return Haxc("#ffffff");
}

void ColorManager::SetColor(ObjectType type, const Color& color) {
  color_map_[type] = color;
}

void ColorManager::DrawColorEditor() {
  ImGui::Begin("Color Editor");

  if (ImGui::Button("Save Colors")) SaveToFile("colors.json");

  ImGui::SameLine();
  if (ImGui::Button("Load Colors")) LoadFromFile("colors.json");

  ImGui::Separator();

  {  // Category buttons for collapsing/expanding groups
    auto ToggleCategory = [&](ObjectType category) {
      category_option_[ObjectType::TEXT_BEGIN] = false;
      category_option_[ObjectType::NOUN_BEGIN] = false;
      category_option_[ObjectType::VERB_BEGIN] = false;
      category_option_[ObjectType::PROPERTY_BEGIN] = false;
      category_option_[ObjectType::ICON_BEGIN] = false;

      category_option_[category] = true;
    };

    if (ImGui::Button("Nouns")) ToggleCategory(ObjectType::NOUN_BEGIN);

    ImGui::SameLine();

    if (ImGui::Button("Verbs")) ToggleCategory(ObjectType::VERB_BEGIN);

    ImGui::SameLine();

    if (ImGui::Button("Properties")) ToggleCategory(ObjectType::PROPERTY_BEGIN);

    ImGui::SameLine();

    if (ImGui::Button("Icons")) ToggleCategory(ObjectType::ICON_BEGIN);
  }

  ImGui::Separator();

  // Default color
  {
    float col[4] = {color_map_[ObjectType::DEFAULT].r / 255.0f,
                    color_map_[ObjectType::DEFAULT].g / 255.0f,
                    color_map_[ObjectType::DEFAULT].b / 255.0f,
                    color_map_[ObjectType::DEFAULT].a / 255.0f};

    if (ImGui::ColorEdit4("DEFAULT", col)) {
      color_map_[ObjectType::DEFAULT] =
          Color{(unsigned char)(col[0] * 255), (unsigned char)(col[1] * 255),
                (unsigned char)(col[2] * 255), (unsigned char)(col[3] * 255)};
    }
  }

  ImGui::Separator();

  // For each color in the map
  for (auto& [type, color] : color_map_) {
    // Skip DEFAULT as it's handled separately
    if (type == ObjectType::DEFAULT) continue;

    // Skip category markers (BEGIN/END)
    if (type == ObjectType::TEXT_BEGIN || type == ObjectType::TEXT_END ||
        type == ObjectType::NOUN_BEGIN || type == ObjectType::NOUN_END ||
        type == ObjectType::VERB_BEGIN || type == ObjectType::VERB_END ||
        type == ObjectType::PROPERTY_BEGIN ||
        type == ObjectType::PROPERTY_END || type == ObjectType::ICON_BEGIN ||
        type == ObjectType::ICON_END) {
      continue;
    }

    std::string type_name = EnumToFullString(type).data();

    // Check if this editor should be shown
    // clang-format off
    if (IsNoun(type) && !category_option_[ObjectType::NOUN_BEGIN]) continue;
    if (IsVerb(type) && !category_option_[ObjectType::VERB_BEGIN]) continue;
    if (IsProperty(type) && !category_option_[ObjectType::PROPERTY_BEGIN]) continue;
    if (IsIcon(type) && !category_option_[ObjectType::ICON_BEGIN]) continue;

    float col[4] = {
      color.r / 255.0f,
      color.g / 255.0f,
      color.b / 255.0f,
      color.a / 255.0f
    };

    if (ImGui::ColorEdit4(type_name.c_str(), col, ImGuiColorEditFlags_NoInputs)) {
      color = Color{
          (unsigned char)(col[0] * 255),
          (unsigned char)(col[1] * 255),
          (unsigned char)(col[2] * 255),
          (unsigned char)(col[3] * 255)
      };
    }
    // clang-format on
  }
  ImGui::End();
}

bool ColorManager::SaveToFile(const std::string& filename) {
  nlohmann::json json_colors;

  for (const auto& [type, color] : color_map_) {
    int id = static_cast<int>(type);
    json_colors[id] = {
        {"r", color.r}, {"g", color.g}, {"b", color.b}, {"a", color.a}};
  }

  std::ofstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  file << json_colors.dump(4);
  return true;
}

bool ColorManager::LoadFromFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  nlohmann::json json_colors;
  try {
    file >> json_colors;

    for (auto& [type_id, color_json] : json_colors.items()) {
      color_map_[static_cast<ObjectType>(std::stoi(type_id))] =
          Color{(unsigned char)color_json["r"].get<int>(),
                (unsigned char)color_json["g"].get<int>(),
                (unsigned char)color_json["b"].get<int>(),
                (unsigned char)color_json["a"].get<int>()};
    }
    return true;
  } catch (const std::exception& e) {
    return false;
  }
}

void ColorManager::InitDefaultColors() {
  // default color for all types
  color_map_[ObjectType::DEFAULT] = Haxc("#ffffff");

  // set category colors
  color_map_[ObjectType::TEXT_BEGIN] = GRAY;
  color_map_[ObjectType::NOUN_BEGIN] = RED;
  color_map_[ObjectType::VERB_BEGIN] = GREEN;
  color_map_[ObjectType::PROPERTY_BEGIN] = BLUE;
  color_map_[ObjectType::ICON_BEGIN] = YELLOW;

#define X(a) color_map_[ObjectType::NOUN_##a] = Haxc("#ffffff");
#include "noun.def"
#undef X

#define X(a) color_map_[ObjectType::VERB_##a] = Haxc("#ffffff");
#include "verb.def"
#undef X

#define X(a) color_map_[ObjectType::PROP_##a] = Haxc("#ffffff");
#include "properties.def"
#undef X

#define X(a) color_map_[ObjectType::ICON_##a] = Haxc("#ffffff");
#include "noun.def"
#undef X

  // initialize edit flags
  for (auto& [type, _] : color_map_) {
    category_option_[type] = false;
  }
}
