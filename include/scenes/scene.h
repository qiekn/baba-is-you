class Scene {
public:
  int Id();
  virtual void Draw() = 0;
  virtual void Update() = 0;

  virtual ~Scene() = default;

private:
  int id_;
};
