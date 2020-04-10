#pragma once

#include <types.h>

#include <window/geometry.h>
#include <window/color.h>

#include <string>

namespace brdrive {

class IWindow {
public:
  IWindow();
  virtual ~IWindow();

  auto title() const -> const std::string&;
  auto title(const std::string& title) -> IWindow&;

  auto geometry() const -> const Geometry&;
  auto geometry(const Geometry& geometry) -> IWindow&;

  auto background() const -> const Color&;
  auto background(const Color& bg) -> IWindow&;

  virtual auto create() -> IWindow& = 0;
  virtual auto show() -> IWindow& = 0;

  virtual auto destroy() -> IWindow& = 0;

  virtual auto drawString(const std::string& str, const Geometry& geom, const Color& color,
      const std::string& font = "") -> IWindow& = 0;

protected:
  std::string title_;
  Geometry geometry_;
  Color background_;
};

}
