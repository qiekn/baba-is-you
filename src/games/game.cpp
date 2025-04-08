// Copyright (c) 2020-2023 Chris Ohk

#include "games/game.h"
#include "enums/game_enums.h"

Game::Game(std::string_view filename) {
  map_.Load(filename);
  ParseRules();
  game_state_ = GameState::PLAYING;
}

void Game::Reset() {
  map_.Reset();
  ParseRules();
  game_state_ = GameState::PLAYING;
}

Map &Game::GetMap() { return map_; }

const Map &Game::GetMap() const { return map_; }

RuleManager &Game::GetRuleManager() { return rule_manager_; }

GameState Game::GetGameState() const { return game_state_; }

ObjectType Game::GetPlayerIcon() const { return plaer_icon_; }

void Game::MovePlayer(Direction dir) {
  auto positions = GetMap().GetPositions(plaer_icon_);
  for (auto &[x, y] : positions) {
    if (CanMove(x, y, dir)) {
      ProcessMove(x, y, dir, plaer_icon_);
    }
  }
  ParseRules();
  CheckPlayState();
}

void Game::ParseRules() {
  rule_manager_.Clear();

  const std::size_t width = map_.GetWidth();
  const std::size_t height = map_.GetHeight();

  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      map_.At(x, y).isRule = false;
    }
  }

  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      ParseRule(x, y, RuleDirection::HORIZONTAL);
      ParseRule(x, y, RuleDirection::VERTICAL);
    }
  }

  plaer_icon_ = rule_manager_.FindPlayer();
}

void Game::ParseRule(std::size_t x, std::size_t y, RuleDirection direction) {
  const std::size_t width = map_.GetWidth();
  const std::size_t height = map_.GetHeight();

  if (direction == RuleDirection::HORIZONTAL) {
    if (x + 2 >= width) {
      return;
    }

    if (map_.At(x, y).HasNounType() && map_.At(x + 1, y).HasVerbType() &&
        (map_.At(x + 2, y).HasNounType() ||
         map_.At(x + 2, y).HasPropertyType())) {
      rule_manager_.Add({map_.At(x, y), map_.At(x + 1, y), map_.At(x + 2, y)});

      map_.At(x, y).isRule = true;
      map_.At(x + 1, y).isRule = true;
      map_.At(x + 2, y).isRule = true;
    }
  } else if (direction == RuleDirection::VERTICAL) {
    if (y + 2 >= height) {
      return;
    }

    if (map_.At(x, y).HasNounType() && map_.At(x, y + 1).HasVerbType() &&
        (map_.At(x, y + 2).HasNounType() ||
         map_.At(x, y + 2).HasPropertyType())) {
      rule_manager_.Add({map_.At(x, y), map_.At(x, y + 1), map_.At(x, y + 2)});

      map_.At(x, y).isRule = true;
      map_.At(x, y + 1).isRule = true;
      map_.At(x, y + 2).isRule = true;
    }
  }
}

bool Game::CanMove(std::size_t x, std::size_t y, Direction dir) {
  int _x = static_cast<int>(x);
  int _y = static_cast<int>(y);

  const auto width = static_cast<int>(map_.GetWidth());
  const auto height = static_cast<int>(map_.GetHeight());

  int dx = 0, dy = 0;
  if (dir == Direction::UP) {
    dy = -1;
  } else if (dir == Direction::DOWN) {
    dy = 1;
  } else if (dir == Direction::LEFT) {
    dx = -1;
  } else if (dir == Direction::RIGHT) {
    dx = 1;
  }

  _x += dx;
  _y += dy;

  // Check boundary
  if (_x < 0 || _x >= width || _y < 0 || _y >= height) {
    return false;
  }

  const std::vector<ObjectType> types = map_.At(_x, _y).GetTypes();

  // Check the icon has property 'STOP'.
  if (rule_manager_.HasProperty(types, ObjectType::STOP)) {
    return false;
  }

  if (rule_manager_.HasProperty(types, ObjectType::PUSH) ||
      map_.At(_x, _y).HasTextType()) {
    if (!CanMove(_x, _y, dir)) {
      return false;
    }
  }

  return true;
}

void Game::ProcessMove(std::size_t x, std::size_t y, Direction dir,
                       ObjectType type) {
  int _x = static_cast<int>(x);
  int _y = static_cast<int>(y);

  int dx = 0, dy = 0;
  if (dir == Direction::UP) {
    dy = -1;
  } else if (dir == Direction::DOWN) {
    dy = 1;
  } else if (dir == Direction::LEFT) {
    dx = -1;
  } else if (dir == Direction::RIGHT) {
    dx = 1;
  }

  _x += dx;
  _y += dy;

  const std::vector<ObjectType> types = map_.At(_x, _y).GetTypes();

  if (rule_manager_.HasProperty(types, ObjectType::PUSH)) {
    auto rules = rule_manager_.GetRules(ObjectType::PUSH);

    for (auto &rule : rules) {
      const ObjectType nounType = std::get<0>(rule.objects).GetTypes()[0];
      ProcessMove(_x, _y, dir, ConvertTextToIcon(nounType));
    }
  } else if (rule_manager_.HasProperty(types, ObjectType::SINK) ||
             rule_manager_.HasProperty(types, ObjectType::DEFEAT)) {
    map_.RemoveObject(x, y, type);
    return;
  } else if (map_.At(_x, _y).HasTextType()) {
    ProcessMove(_x, _y, dir, types[0]);
  }

  map_.AddObject(_x, _y, type);
  map_.RemoveObject(x, y, type);
}

void Game::CheckPlayState() {
  const auto youRules = rule_manager_.GetRules(ObjectType::YOU);
  if (youRules.empty()) {
    game_state_ = GameState::LOST;
    return;
  }

  auto positions = map_.GetPositions(plaer_icon_);
  if (positions.empty()) {
    game_state_ = GameState::LOST;
    return;
  }

  auto winRules = rule_manager_.GetRules(ObjectType::WIN);
  for (auto &pos : positions) {
    for (auto &rule : winRules) {
      const ObjectType type = std::get<0>(rule.objects).GetTypes()[0];

      if (map_.At(pos.first, pos.second).HasType(ConvertTextToIcon(type))) {
        game_state_ = GameState::WON;
      }
    }
  }
}
