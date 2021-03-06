/****************************************************************************
   Dino - A simple pattern based MIDI sequencer
   
   Copyright (C) 2006  Lars Luthman <lars.luthman@gmail.com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, 
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
****************************************************************************/

#include <cassert>
#include <iostream>
#include <sstream>

#include "commandproxy.hpp"
#include "controllerinfo.hpp"
#include "curve.hpp"
#include "curveeditor.hpp"
#include "controller_numbers.hpp"
#include "interpolatedevent.hpp"


using namespace Glib;
using namespace Gtk;
using namespace Gdk;
using namespace std;
using namespace Dino;
using namespace sigc;

  
CurveEditor::CurveEditor(Dino::CommandProxy& proxy) 
  : m_bg_colour1("#FFFFFF"),
    m_bg_colour2("#EAEAFF"),
    m_grid_colour("#9C9C9C"),
    m_edge_colour("#000000"),
    m_fg_colour("#008000"),
    m_step_width(8),
    m_alternation(4),
    m_curve(0),
    m_proxy(proxy),
    m_drag_step(-1) {

  RefPtr<Colormap> cmap = Colormap::get_system();
  cmap->alloc_color(m_bg_colour1);
  cmap->alloc_color(m_bg_colour2);
  cmap->alloc_color(m_grid_colour);
  cmap->alloc_color(m_edge_colour);
  cmap->alloc_color(m_fg_colour);

  add_events(BUTTON_PRESS_MASK | BUTTON_RELEASE_MASK | BUTTON_MOTION_MASK);
  set_size_request(-1, 68);
}


void CurveEditor::set_curve(int track, int pattern, const Dino::Curve* curve) {
  m_track = track;
  m_pattern = pattern;
  m_curve = curve;
  if (m_curve) {
    set_size_request(m_curve->get_size() * m_step_width, 68);
    slot<void> draw = mem_fun(*this, &CurveEditor::queue_draw);
    m_curve->signal_point_added().
      connect(sigc::hide(sigc::hide(draw)));
    m_curve->signal_point_changed().
      connect(sigc::hide(sigc::hide(draw)));
    m_curve->signal_point_removed().connect(sigc::hide(draw));
    // XXX need some signal from the curve when the size changes
    //m_pat->signal_length_changed().connect(sigc::hide(draw));
    //m_pat->signal_steps_changed().connect(sigc::hide(draw));
  }
  queue_draw();
}


void CurveEditor::set_step_width(int width) {
  assert(width > 0);
  m_step_width = width;
  if (m_curve) {
    set_size_request(m_curve->get_size() * m_step_width, 68);
    queue_draw();
  }
}


void CurveEditor::set_alternation(int k) {
  assert(k > 0);
  m_alternation = k;
  queue_draw();
}


bool CurveEditor::on_button_press_event(GdkEventButton* event) {
  if (!m_curve)
    return false;
  
  // add a CC event
  if (event->button == 1 || event->button == 2) {
    int step = xpix2step(int(event->x));
    if (step <= int(m_curve->get_size())) {
      int value = ypix2value(int(event->y));
      int min = m_curve->get_info().get_min();
      int max = m_curve->get_info().get_max();
      value = (value < min ? min : value);
      value = (value > max ? max : value);
      m_proxy.add_curve_point(m_track, m_pattern, 
			      m_curve->get_info().get_number(), 
			      SongTime(step, 0), value);
      //m_curve->add_point(step, value);
      stringstream oss;
      oss<<"New value: "<<value;
      m_signal_status(ref(oss.str()));
      if (event->button == 2)
	m_drag_step = step;
      else
	m_drag_step = -1;
    }
  }
  
  // remove a CC event
  else if (event->button == 3) {
    int step;
    if ((step = xpix2step(int(event->x))) < int(m_curve->get_size())) {
      m_proxy.remove_curve_point(m_track, m_pattern,
				 m_curve->get_info().get_number(), 
				 SongTime(step, 0));
      //m_curve->remove_point(step);
    }
  }

  return true;
}


bool CurveEditor::on_button_release_event(GdkEventButton* event) {
  return false;
}


bool CurveEditor::on_motion_notify_event(GdkEventMotion* event) {
  if (!m_curve)
    return false;
  
  // add a CC event
  if (event->state & GDK_BUTTON1_MASK || event->state & GDK_BUTTON2_MASK) {
    int step = m_drag_step;
    if (step == -1)
      step = xpix2step(int(event->x));
    if (step <= int(m_curve->get_size())) {
      int value = ypix2value(int(event->y));
      int min = m_curve->get_info().get_min();
      int max = m_curve->get_info().get_max();
      value = (value < min ? min : value);
      value = (value > max ? max : value);
      m_proxy.add_curve_point(m_track, m_pattern, 
			      m_curve->get_info().get_number(), 
			      SongTime(step, 0), value);
      stringstream oss;
      oss<<"New value: "<<value;
      m_signal_status(ref(oss.str()));
    }
  }
  
  // remove a CC event
  else if (event->state & GDK_BUTTON3_MASK) {
    int step;
    if ((step = xpix2step(int(event->x))) < 
	int(m_curve->get_size())) {
      m_proxy.remove_curve_point(m_track, m_pattern,
				 m_curve->get_info().get_number(), 
				 SongTime(step, 0));
    }
  }

  return true;
}


void CurveEditor::on_realize() {
  DrawingArea::on_realize();
  RefPtr<Gdk::Window> win = get_window();
  m_gc = GC::create(win);
  win->clear();
}


bool CurveEditor::on_expose_event(GdkEventExpose* event) {
  
  if (!m_curve)
    return true;
  
  RefPtr<Gdk::Window> win = get_window();
  
  unsigned steps = m_curve->get_size();
  unsigned spb = m_alternation;
  
  for (unsigned i = 0; i < steps; i += spb) {
    if ((i / spb) % 2 == 0)
      m_gc->set_foreground(m_bg_colour1);
    else
      m_gc->set_foreground(m_bg_colour2);
    win->draw_rectangle(m_gc, true, i * m_step_width, 0, 
			spb * m_step_width, get_height());
  }
  
  m_gc->set_foreground(m_grid_colour);
  for (unsigned i = 0; i < steps; ++i) {
    win->draw_line(m_gc, (i + 1) * m_step_width, 0, 
		   (i + 1) * m_step_width, get_height());
  }
  
  for (unsigned i = 0; i < steps; ++i) {
    const InterpolatedEvent* event = 
      m_curve->get_event(i);
    if (event && event->get_step() == i) {
      m_gc->set_foreground(m_edge_colour);
      win->draw_line(m_gc, m_step_width * i, value2ypix(event->get_start()),
		     m_step_width * (i + event->get_length()),
		     value2ypix(event->get_end()));
      m_gc->set_foreground(m_fg_colour);
      win->draw_rectangle(m_gc, true, m_step_width * i - 2, 
			  value2ypix(event->get_start()) - 2, 5, 5);
      m_gc->set_foreground(m_edge_colour);
      win->draw_rectangle(m_gc, false, m_step_width * i - 2, 
			  value2ypix(event->get_start()) - 2, 5, 5);
    }
    if (event && i == steps - 1) {
      m_gc->set_foreground(m_fg_colour);
      win->draw_rectangle(m_gc, true, m_step_width * steps - 2, 
			  value2ypix(event->get_end()) - 2, 5, 5);
      m_gc->set_foreground(m_edge_colour);
      win->draw_rectangle(m_gc, false, m_step_width * steps - 2, 
			  value2ypix(event->get_end()) - 2, 5, 5);
    }
      
  }
  
  return true;
}


int CurveEditor::value2ypix(int value) {
  if (m_curve) {
    int max = m_curve->get_info().get_max();
    int min = m_curve->get_info().get_min();
    return get_height() - int(get_height() * (value - min) / double(max - min));
  }
  return 0;
}


int CurveEditor::ypix2value(int y) { 
  if (m_curve) {
    int max = m_curve->get_info().get_max();
    int min = m_curve->get_info().get_min();
    return min + int((get_height() - y) * (max - min) / double(get_height()));
  }
  return 0;
}


int CurveEditor::step2xpix(int value) {
  return value * m_step_width;
}


int CurveEditor::xpix2step(int value) {
  return value / m_step_width;
}


sigc::signal<void, const std::string&>& CurveEditor::signal_status() {
  return m_signal_status;
}

