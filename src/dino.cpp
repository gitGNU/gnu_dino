#include <cassert>
#include <cmath>
#include <iostream>
#include <list>
#include <sstream>
#include <utility>

#include "dino.hpp"
#include "evilscrolledwindow.hpp"
#include "ruler.hpp"
#include "song.hpp"
#include "trackwidget.hpp"


using namespace sigc;


Dino::Dino(int argc, char** argv, RefPtr<Xml> xml) 
  : m_xml(xml), m_seq("Dino"), m_pattern_ruler_1(0, 1, 1, 20, 20) {

  m_seq.set_song(m_song);
  
  m_window = w<Gtk::Window>("main_window");
  m_about_dialog = w<Dialog>("dlg_about");
  
  init_pattern_editor();
  init_sequence_editor();
  init_info_editor();
  init_menus();
  
  m_window->show_all();
  
}


Gtk::Window* Dino::get_window() {
  return m_window;
}


void Dino::slot_file_new() {
  //m_song = Song();
}


void Dino::slot_file_open() {
  
}


void Dino::slot_file_save() {
  m_song.write_file("output.dino");
}


void Dino::slot_file_save_as() {
  m_song.write_file("output.dino");
}


void Dino::slot_file_quit() {
  Main::quit();
}


void Dino::slot_edit_cut() {

}


void Dino::slot_edit_copy() {

}


void Dino::slot_edit_paste() {

}


void Dino::slot_edit_delete() {
  
}


void Dino::slot_edit_add_track() {
  Mutex::Lock(m_song.get_big_lock());
  m_song.add_track();
}
 

void Dino::slot_edit_delete_track() {
  int trackID = m_cmb_track.get_active_id();
  if (trackID >= 0) {
    Mutex::Lock(m_song.get_big_lock());
    m_song.remove_track(trackID);
  }
}


void Dino::slot_edit_add_pattern() {
  int trackID = m_cmb_track.get_active_id();
  if (trackID >= 0) {
    int patternID = m_song.get_tracks().find(trackID)->second.add_pattern(8,4, 4);
    Pattern* pat = &m_song.get_tracks().find(trackID)->
      second.get_patterns().find(patternID)->second;
  }
}
 

void Dino::slot_edit_delete_pattern() {
  
}


void Dino::slot_transport_play() {
  m_seq.play();
}


void Dino::slot_transport_stop() {
  m_seq.stop();
}


void Dino::slot_transport_go_to_start() {
  m_seq.goto_beat(0);
}


void Dino::slot_help_about_dino() {
  assert(m_about_dialog != NULL);
  m_about_dialog->run();
}


void Dino::update_track_widgets() {
  m_vbx_track_editor->children().clear();
  for (map<int, Track>::iterator iter = m_song.get_tracks().begin();
       iter != m_song.get_tracks().end(); ++iter) {
    TrackWidget* tw = manage(new TrackWidget(&m_song));
    tw->set_track(&iter->second);
    m_vbx_track_editor->pack_start(*tw, PACK_SHRINK);
  }
  m_vbx_track_editor->show_all();
}


void Dino::update_track_combo(int activeTrack) {
  m_track_pattern_connection.block();
  int oldActive = m_cmb_track.get_active_id();
  m_cmb_track.clear();
  int newActive = activeTrack;
  if (m_song.get_tracks().size() > 0) {
    char tmp[10];
    for (map<int, Track>::iterator iter = m_song.get_tracks().begin();
	 iter != m_song.get_tracks().end(); ++iter) {
      sprintf(tmp, "%03d", iter->first);
      m_cmb_track.append_text(tmp, iter->first);
      if (newActive == -1 || (activeTrack == -1 && iter->first <= oldActive))
	newActive = iter->first;
    }
  }
  else {
    m_cmb_track.append_text("No tracks");
  }
  m_cmb_track.set_active_id(newActive);
  m_track_pattern_connection.unblock();
  if (oldActive == -1 || newActive != oldActive)
    active_track_changed();
}


void Dino::update_pattern_combo(int activePattern) {
  int newActive = activePattern;
  m_cmb_pattern.clear();
  int trackID = m_cmb_track.get_active_id();
  if (trackID >= 0) {
    const Track& trk(m_song.get_tracks().find(trackID)->second);
    m_cmb_pattern.clear();
    map<int, Pattern>::const_iterator iter;
    char tmp[10];
    for (iter = trk.get_patterns().begin(); 
	 iter != trk.get_patterns().end(); ++iter) {
      sprintf(tmp, "%03d", iter->first);
      m_cmb_pattern.append_text(tmp, iter->first);
      if (newActive == -1)
	newActive = iter->first;
    }
  }
  
  if (newActive == -1)
    m_cmb_pattern.append_text("No patterns");
  m_cmb_pattern.set_active_id(newActive);
  update_editor_widgets();
}


void Dino::update_editor_widgets() {
  int trackID = m_cmb_track.get_active_id();
  int patternID = m_cmb_pattern.get_active_id();
  Pattern* pat = NULL;
  if (trackID != -1 && patternID != -1) {
    Track& trk = m_song.get_tracks().find(trackID)->second;
    pat = &(trk.get_patterns().find(patternID)->second);
  }
  m_pe.set_pattern(pat);
  m_cce.set_pattern(pat);
  if (pat) {
    m_pattern_ruler_1.setLength(pat->get_length());
    m_pattern_ruler_1.setSubdivisions(pat->get_steps());
    m_pattern_ruler_1.setDivisionSize(8 * pat->get_steps());
  }
  else {
    m_pattern_ruler_1.setLength(0);
  }
}


void Dino::slot_cc_number_changed() {
  m_lb_cc_description->set_text(cc_descriptions[m_sb_cc_number->get_value_as_int()]);
  m_cce.set_cc_number(m_sb_cc_number->get_value_as_int());
}


void Dino::slot_cc_editor_size_changed() {
  m_cce.set_height(m_sb_cc_editor_size->get_value_as_int());
}


void Dino::active_track_changed() {
  update_pattern_combo();
  m_pattern_added_connection.disconnect();
  m_pattern_removed_connection.disconnect();
  int active_track = m_cmb_track.get_active_id();
  if (active_track == -1)
    return;
  Track& t(m_song.get_tracks().find(active_track)->second);
  slot<void, int> update_slot = mem_fun(*this, &Dino::update_pattern_combo);
  m_pattern_added_connection = t.signal_pattern_added.connect(update_slot);
  m_pattern_removed_connection = t.signal_pattern_removed.connect(update_slot);
}


void Dino::init_pattern_editor() {
  
  // get all the widgets from the glade file
  Box* boxPatternRuler1 = w<Box>("box_pattern_ruler_1");
  HBox* hbx_track_combo = w<HBox>("hbx_track_combo");
  HBox* hbx_pattern_combo = w<HBox>("hbx_pattern_combo");
  Scrollbar* scbHorizontal = w<Scrollbar>("scb_pattern_editor");
  Scrollbar* scbVertical = w<Scrollbar>("scb_note_editor");
  Box* boxNoteEditor = w<Box>("box_note_editor");
  Box* boxCCEditor = w<Box>("box_cc_editor");
  m_sb_cc_number = w<SpinButton>("sb_cc_number");
  m_lb_cc_description = w<Label>("lb_cc_description");
  m_sb_cc_editor_size = w<SpinButton>("sb_cc_editor_size");
  
  // add and connect the combo boxes
  hbx_pattern_combo->pack_start(m_cmb_pattern);
  hbx_track_combo->pack_start(m_cmb_track);
  m_track_pattern_connection = m_cmb_track.signal_changed().
    connect(mem_fun(*this, &Dino::active_track_changed));
  m_pattern_editor_connection = m_cmb_pattern.signal_changed().
    connect(mem_fun(*this, &Dino::update_editor_widgets));

  // add the ruler
  EvilScrolledWindow* scwPatternRuler1 = 
    manage(new EvilScrolledWindow(true, false));
  boxPatternRuler1->pack_start(*scwPatternRuler1);
  scwPatternRuler1->add(m_pattern_ruler_1);
  
  // add the note editor
  EvilScrolledWindow* scwNoteEditor = manage(new EvilScrolledWindow);
  boxNoteEditor->pack_start(*scwNoteEditor);
  scwNoteEditor->add(m_pe);
  
  // add the CC editor
  EvilScrolledWindow* scwCCEditor = manage(new EvilScrolledWindow(true,false));
  boxCCEditor->pack_start(*scwCCEditor);
  scwCCEditor->add(m_cce);
  
  // synchronise scrolling
  scwPatternRuler1->set_hadjustment(scwNoteEditor->get_hadjustment());
  scbHorizontal->set_adjustment(*scwNoteEditor->get_hadjustment());
  scbVertical->set_adjustment(*scwNoteEditor->get_vadjustment());
  scwCCEditor->set_hadjustment(*scwNoteEditor->get_hadjustment());

  // connect and setup the CC controls
  m_sb_cc_number->signal_value_changed().
    connect(sigc::mem_fun(this, &Dino::slot_cc_number_changed));
  m_sb_cc_number->set_value(1);
  m_sb_cc_number->set_numeric(true);
  m_sb_cc_editor_size->signal_value_changed().
    connect(sigc::mem_fun(this, &Dino::slot_cc_editor_size_changed));
  m_sb_cc_editor_size->set_editable(false);
  
  // connect external signals
  slot<void, int> update_combos =mem_fun(*this, &Dino::update_track_combo);
  m_song.signal_track_added.connect(update_combos);
  m_song.signal_track_removed.connect(hide(bind(update_combos, -1)));
  
  update_track_combo();
}


void Dino::init_sequence_editor() {
  // get the widgets
  VBox* boxArrangementEditor = w<VBox>("box_arrangement_editor");
  Scrollbar* scbHorizontal = w<Scrollbar>("scbh_arrangement_editor");
  Scrollbar* scbVertical = w<Scrollbar>("scbv_arrangement_editor");
  Box* boxSequenceRuler = w<Box>("box_sequence_ruler");
  
  // add the ruler
  EvilScrolledWindow* scwSequenceRuler = 
    manage(new EvilScrolledWindow(true, false));
  boxSequenceRuler->pack_start(*scwSequenceRuler);
  scwSequenceRuler->add(*manage(new ::Ruler(m_song.get_length(),
					    1, 4, 20, 20)));

  // add the box for the trackwidgets
  EvilScrolledWindow* scwArrangementEditor = manage(new EvilScrolledWindow);
  boxArrangementEditor->pack_start(*scwArrangementEditor);
  m_vbx_track_editor = manage(new VBox(false, 2));
  scwArrangementEditor->add(*m_vbx_track_editor);
  
  // synchronise scrolling
  scwSequenceRuler->set_hadjustment(scwArrangementEditor->get_hadjustment());
  scbHorizontal->set_adjustment(*scwArrangementEditor->get_hadjustment());
  scbVertical->set_adjustment(*scwArrangementEditor->get_vadjustment());
  
  // connect external signals
  slot<void, int> update_track_view = 
    hide(mem_fun(*this, &Dino::update_track_widgets));
  m_song.signal_track_added.connect(update_track_view);
  m_song.signal_track_removed.connect(update_track_view);
  m_song.signal_length_changed.connect(update_track_view);
  
  update_track_widgets();
}


void Dino::init_menus() {
  // connect menu signals
  map<const char*, void (Dino::*)(void)> menuSlots;
  menuSlots["new1"] = &Dino::slot_file_new;
  menuSlots["open1"] = &Dino::slot_file_open;
  menuSlots["save1"] = &Dino::slot_file_save;
  menuSlots["save_as1"] = &Dino::slot_file_save_as;
  menuSlots["quit1"] = &Dino::slot_file_quit;
  menuSlots["cut1"] = &Dino::slot_edit_cut;
  menuSlots["copy1"] = &Dino::slot_edit_copy;
  menuSlots["paste1"] = &Dino::slot_edit_paste;
  menuSlots["delete1"] = &Dino::slot_edit_delete;
  menuSlots["add_track1"] = &Dino::slot_edit_add_track;
  menuSlots["delete_track1"] = &Dino::slot_edit_delete_track;
  menuSlots["add_pattern1"] = &Dino::slot_edit_add_pattern;
  menuSlots["delete_pattern1"] = &Dino::slot_edit_delete_pattern;
  menuSlots["about1"] = &Dino::slot_help_about_dino;
  menuSlots["play1"] = &Dino::slot_transport_play;
  menuSlots["stop1"] = &Dino::slot_transport_stop;
  menuSlots["go_to_start1"] = &Dino::slot_transport_go_to_start;
  map<const char*, void (Dino::*)(void)>::const_iterator iter;
  for (iter = menuSlots.begin(); iter != menuSlots.end(); ++iter) {
    MenuItem* mi = w<Gtk::MenuItem>(iter->first);
    assert(mi);
    mi->signal_activate().connect(sigc::mem_fun(*this, iter->second));
  }
}


int foo() {
  static int c = 1024;
  cerr<<"foo: "<<c<<endl;
  return ++c;
}

void bar(int i) {
  cerr<<i<<endl;
}


void Dino::init_info_editor() {
  m_ent_title = w<Entry>("ent_title");
  m_ent_author = w<Entry>("ent_author");
  m_text_info = w<TextView>("text_info");
  
  m_song.signal_title_changed.connect(mem_fun(m_ent_title, &Entry::set_text));
  m_song.signal_author_changed.connect(mem_fun(m_ent_author,&Entry::set_text));
  slot<void> set_title = compose(mem_fun(m_song, &Song::set_title),
				 mem_fun(m_ent_title, &Entry::get_text));
  m_ent_title->signal_changed().connect(set_title);
  slot<void> set_author = compose(mem_fun(m_song, &Song::set_author),
				  mem_fun(m_ent_author, &Entry::get_text));
  m_ent_author->signal_changed().connect(set_author);
}


char* Dino::cc_descriptions[] = { "Bank select MSB",
				  "Modulation", 
				  "Breath controller",
				  "Undefined",
				  "Foot controller",
				  "Portamento time",
				  "Data entry MSB",
				  "Channel volume",
				  "Balance", 
				  "Undefined",
				  "Pan",
				  "Expression",
				  "Effect control 1",
				  "Effect control 2",
				  "Undefined",
				  "Undefined",
				  "General purpose controller 1",
				  "General purpose controller 2",
				  "General purpose controller 3",
				  "General purpose controller 4",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Bank select LSB",
				  "Modulation LSB",
				  "Breath controller LSB",
				  "Undefined 03 LSB",
				  "Foot controller LSB",
				  "Portamento time LSB",
				  "Data entry LSB",
				  "Channel volume LSB",
				  "Balance LSB",
				  "Undefined 09 LSB",
				  "Pan LSB",
				  "Expression LSB",
				  "Effect control 1 LSB",
				  "Effect control 2 LSB",
				  "Undefined 14 LSB",
				  "Undefined 15 LSB",
				  "General purpose controller 1 LSB",
				  "General purpose controller 2 LSB",
				  "General purpose controller 3 LSB",
				  "General purpose controller 4 LSB",
				  "Undefined 20 LSB",
				  "Undefined 21 LSB",
				  "Undefined 22 LSB",
				  "Undefined 23 LSB",
				  "Undefined 24 LSB",
				  "Undefined 25 LSB",
				  "Undefined 26 LSB",
				  "Undefined 27 LSB",
				  "Undefined 28 LSB",
				  "Undefined 29 LSB",
				  "Undefined 30 LSB",
				  "Undefined 31 LSB",
				  "Damper pedal on/off",
				  "Portamento on/off",
				  "Sustenuto on/off",
				  "Soft pedal on/off",
				  "Legato on/off",
				  "Hold 2 on/off",
				  "Sound controller 1 - Variation",
				  "Sound controller 2 - Timbre",
				  "Sound controller 3 - Release time",
				  "Sound controller 4 - Attack time",
				  "Sound controller 5 - Brightness",
				  "Sound controller 6 - Decay time",
				  "Sound controller 7 - Vibrato rate",
				  "Sound controller 8 - Vibrato depth",
				  "Sound controller 9 - Vibrato delay",
				  "Sound controller 10",
				  "General purpose controller 5",
				  "General purpose controller 6",
				  "General purpose controller 7",
				  "General purpose controller 8",
				  "Portamento control",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Effect depth 1 - Reverb",
				  "Effect depth 2 - Tremolo",
				  "Effect depth 3 - Chorus",
				  "Effect depth 4 - Celeste",
				  "Effect depth 5 - Phaser",
				  "Data increment",
				  "Data decrement",
				  "NRPN LSB",
				  "NRPN MSB",
				  "RPN LSB",
				  "RPN MSB",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "Undefined",
				  "CMM All sound off",
				  "CMM Reset all controllers",
				  "CMM Local control on/off",
				  "CMM All notes off",
				  "CMM Omni mode off",
				  "CMM Omni mode on",
				  "CMM Poly mode on/off",
				  "CMM Poly mode on",
				  NULL };

				  
