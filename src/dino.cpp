#include <cassert>
#include <cmath>
#include <iostream>
#include <list>
#include <sstream>
#include <utility>

#include "evilscrolledwindow.hpp"
#include "song.hpp"
#include "dino.hpp"
#include "trackwidget.hpp"


Dino::Dino(int argc, char** argv, RefPtr<Xml> xml) : mSeq("Dino") {
  
  mSeq.setSong(mSong);
  
  // insert the pattern editor widgets into the GUI
  window = w<Gtk::Window>(xml, "mainWindow");
  w<HBox>(xml, "hbxTrackCombo")->pack_start(mCmbTrack);
  w<HBox>(xml, "hbxPatternCombo")->pack_start(mCmbPattern);
  Scrollbar* scbHorizontal = w<Scrollbar>(xml, "scbPatternEditor");
  Scrollbar* scbVertical = w<Scrollbar>(xml, "scbNoteEditor");
  EvilScrolledWindow* scwNoteEditor = manage(new EvilScrolledWindow);
  EvilScrolledWindow* scwCCEditor = manage(new EvilScrolledWindow(true,false));
  Box* boxNoteEditor = w<Box>(xml, "boxNoteEditor");
  Box* boxCCEditor = w<Box>(xml, "boxCCEditor");
  boxNoteEditor->pack_start(*scwNoteEditor);
  boxCCEditor->pack_start(*scwCCEditor);
  scwNoteEditor->add(pe);
  scwCCEditor->add(cce);
  scbHorizontal->set_adjustment(*scwNoteEditor->get_hadjustment());
  scwCCEditor->set_hadjustment(*scwNoteEditor->get_hadjustment());
  scbVertical->set_adjustment(*scwNoteEditor->get_vadjustment());
  sbCCNumber = w<SpinButton>(xml, "sbCCNumber");
  lbCCDescription = w<Label>(xml, "lbCCDescription");
  sbCCNumber->signal_value_changed().
    connect(sigc::mem_fun(this, &Dino::slotCCNumberChanged));
  sbCCNumber->set_value(1);
  sbCCNumber->set_numeric(true);
  sbCCEditorSize = w<SpinButton>(xml, "sbCCEditorSize");
  sbCCEditorSize->signal_value_changed().
    connect(sigc::mem_fun(this, &Dino::slotCCEditorSizeChanged));
  sbCCEditorSize->set_editable(false);
  
  // insert the arrangement editor widgets into the GUI
  VBox* boxArrangementEditor = w<VBox>(xml, "boxArrangementEditor");
  scbHorizontal = w<Scrollbar>(xml, "scbHArrangementEditor");
  scbVertical = w<Scrollbar>(xml, "scbVArrangementEditor");
  EvilScrolledWindow* scwArrangementEditor = manage(new EvilScrolledWindow);
  scbHorizontal->set_adjustment(*scwArrangementEditor->get_hadjustment());
  scbVertical->set_adjustment(*scwArrangementEditor->get_vadjustment());
  boxArrangementEditor->pack_start(*scwArrangementEditor);
  mVbxTrackEditor = manage(new VBox(false, 2));
  scwArrangementEditor->add(*mVbxTrackEditor);
  updateTrackWidgets();
  updateTrackCombo();
  
  // get other widgets
  mAboutDialog = w<Dialog>(xml, "dlgAbout");
  
  // connect menu signals
  map<const char*, void (Dino::*)(void)> menuSlots;
  menuSlots["new1"] = &Dino::slotFileNew;
  menuSlots["open1"] = &Dino::slotFileOpen;
  menuSlots["save1"] = &Dino::slotFileSave;
  menuSlots["save_as1"] = &Dino::slotFileSaveAs;
  menuSlots["quit1"] = &Dino::slotFileQuit;
  menuSlots["cut1"] = &Dino::slotEditCut;
  menuSlots["copy1"] = &Dino::slotEditCopy;
  menuSlots["paste1"] = &Dino::slotEditPaste;
  menuSlots["delete1"] = &Dino::slotEditDelete;
  menuSlots["add_track1"] = &Dino::slotEditAddTrack;
  menuSlots["delete_track1"] = &Dino::slotEditDeleteTrack;
  menuSlots["add_pattern1"] = &Dino::slotEditAddPattern;
  menuSlots["delete_pattern1"] = &Dino::slotEditDeletePattern;
  menuSlots["about1"] = &Dino::slotHelpAboutDino;
  menuSlots["play1"] = &Dino::slotTransportPlay;
  menuSlots["stop1"] = &Dino::slotTransportStop;
  menuSlots["go_to_start1"] = &Dino::slotTransportGoToStart;
  map<const char*, void (Dino::*)(void)>::const_iterator iter;
  for (iter = menuSlots.begin(); iter != menuSlots.end(); ++iter) {
    MenuItem* mi = w<Gtk::MenuItem>(xml, iter->first);
    assert(mi);
    mi->signal_activate().connect(sigc::mem_fun(*this, iter->second));
  }
  
  
  window->show_all();
}


Gtk::Window* Dino::getWindow() {
  return window;
}


void Dino::slotFileNew() {
  mSong = Song();
}


void Dino::slotFileOpen() {
  
}


void Dino::slotFileSave() {
  mSong.writeFile("output.dino");
}


void Dino::slotFileSaveAs() {
  mSong.writeFile("output.dino");
}


void Dino::slotFileQuit() {
  Main::quit();
}


void Dino::slotEditCut() {

}


void Dino::slotEditCopy() {

}


void Dino::slotEditPaste() {

}


void Dino::slotEditDelete() {
  
}


void Dino::slotEditAddTrack() {
  mSong.addTrack();
  updateTrackWidgets();
  updateTrackCombo();
}
 

void Dino::slotEditDeleteTrack() {
  
}


void Dino::slotEditAddPattern() {
  istringstream iss(mCmbTrack.get_active_text());
  int trackID;
  iss>>trackID;
  int patternID = mSong.getTracks().find(trackID)->second.addPattern(8, 4, 4);
  Pattern* pat = &mSong.getTracks().find(trackID)->
    second.getPatterns().find(patternID)->second; 
  pe.setPattern(pat);
  cce.setPattern(pat);
}
 

void Dino::slotEditDeletePattern() {
  
}


void Dino::slotTransportPlay() {
  mSeq.play();
}


void Dino::slotTransportStop() {
  mSeq.stop();
}


void Dino::slotTransportGoToStart() {
  mSeq.gotoBeat(0);
}


void Dino::slotHelpAboutDino() {
  assert(mAboutDialog != NULL);
  mAboutDialog->run();
}


void Dino::updateTrackWidgets() {
  mVbxTrackEditor->children().clear();
  for (map<int, Track>::iterator iter = mSong.getTracks().begin();
       iter != mSong.getTracks().end(); ++iter) {
    TrackWidget* tw = manage(new TrackWidget(&mSong));
    tw->setTrack(&iter->second);
    mVbxTrackEditor->pack_start(*tw, PACK_SHRINK);
  }
  mVbxTrackEditor->show_all();
}


void Dino::updateTrackCombo() {
  //mCmbTrack.clear();
  char tmp[10];
  for (map<int, Track>::iterator iter = mSong.getTracks().begin();
       iter != mSong.getTracks().end(); ++iter) {
    sprintf(tmp, "%03d", iter->first);
    mCmbTrack.append_text(tmp);
  }
}


void Dino::updatePatternCombo() {

}


void Dino::slotCCNumberChanged() {
  lbCCDescription->set_text(mCCDescriptions[sbCCNumber->get_value_as_int()]);
  cce.setCCNumber(sbCCNumber->get_value_as_int());
}


void Dino::slotCCEditorSizeChanged() {
  cce.setHeight(sbCCEditorSize->get_value_as_int());
}


char* Dino::mCCDescriptions[] = { "Bank select MSB",
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

				  
