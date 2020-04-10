#include <window/window.h>

namespace brdrive {

IWindow::IWindow() :
  geometry_({ 0, 0, 400, 400 }), background_(Color::transparent())
{
}

IWindow::~IWindow()

{
}

auto IWindow::geometry() const -> const Geometry&
{
  return geometry_;
}

auto IWindow::geometry(const Geometry& geom) -> IWindow&
{
  geometry_ = geom;
  return *this;
}

auto IWindow::title() const -> const std::string&
{
  return title_;
}

auto IWindow::title(const std::string& t) -> IWindow&
{
  title_ = t;
  return *this;
}

auto IWindow::background() const -> const Color&
{
  return background_;
}

auto IWindow::background(const Color& bg) -> IWindow&
{
  background_ = bg;
  return *this;
}

}
