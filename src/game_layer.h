#pragma once

#include <filesystem>
#include <variant>
#include <vector>

#include <entt/entt.hpp>

#include "ids.h"
#include "layer.h"
#include "level.h"
#include "rule_engine.h"
#include "sprite_sheet.h"
#include "undo_stack.h"

class GameLayer : public Layer {
 public:
  GameLayer();

  void OnAttach() override;
  void OnDetach() override;
  void OnUpdate(float dt) override;
  void OnRender() override;
  void OnImGuiRender() override;

 private:
  // Level / registry
  void LoadLevelFromPath(const std::filesystem::path& path);
  void SaveLevelToPath(const std::filesystem::path& path);
  void BuildRegistryFromLevel();
  Level ExtractLevelFromRegistry() const;
  void ClearRegistry();
  void SpawnObject(ObjectId id, int x, int y);
  void SpawnText(TextId id, int x, int y);

  // Turn step
  void Step(Direction dir);
  void RecomputeRules();
  bool TryMove(entt::entity who, Direction dir);
  bool CanEnter(int x, int y, int dx, int dy);
  void PushChain(int x, int y, int dx, int dy);
  void RunWinDefeat();

  // Rendering
  void DrawEntities();

  // Panels
  void DrawScenePanel();
  void DrawRulesPanel();
  void DrawEditorPanel();

  // Editor
  void HandleEditorMouse();
  void PlaceBrushAt(int col, int row);
  void EraseAt(int col, int row);

  static std::pair<int, int> Delta(Direction dir);
  static const char* PrettyName(ObjectId id);
  static const char* PrettyName(TextId id);

  entt::registry registry_;
  SpriteSheet sprites_;
  Level level_;
  std::vector<Rule> rules_;
  UndoStack undo_;
  float anim_timer_ = 0.0f;
  int current_frame_ = 1;
  bool won_ = false;

  // Editor state
  bool edit_mode_ = false;
  std::variant<ObjectId, TextId> brush_ = ObjectId::Baba;

  // Save-as buffer
  char save_name_[64] = "custom.json";
};
