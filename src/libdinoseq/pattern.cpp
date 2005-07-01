#include <algorithm>
#include <cassert>
#include <iostream>

#include "deleter.hpp"
#include "pattern.hpp"


Pattern::Pattern() : m_name("Untitled"), m_dirty(false) {
  
}


Pattern::Pattern(const string& name, int length, int steps, int cc_steps) 
  : m_name(name), m_length(length), m_steps(steps), 
    m_cc_steps(cc_steps), m_dirty(false) {
  assert(m_steps > 0);
  assert(m_cc_steps > 0);
  assert(m_cc_steps % m_steps == 0);
  m_min_step = m_length * m_steps;
  m_max_step = -1;
  m_min_note = 128;
  m_max_note = -1;
  for (int i = 0; i < m_length * m_steps; ++i)
    m_notes.push_back(NULL);
}


const string& Pattern::get_name() const {
  return m_name;
}


void Pattern::set_name(const string& name) {
  if (name != m_name) {
    m_name = name;
    signal_name_changed(m_name);
  }
}


/** This function tries to add a new note with the given parameters. If 
    there is another note starting at the same step and with the same value,
    the old note will be deleted. If there is a note that is playing at that
    step with the same value, the old note will be shortened so it ends just
    before @c step. This function will also update the @c previous and 
    @c next pointers in the Note, so the doubly linked list will stay
    consistent with the note map. */
void Pattern::add_note(int step, int value, int noteLength) {
  assert(step >= 0);
  assert(step < m_length * m_steps);
  assert(value >= 0);
  assert(value < 128);
  
  NoteEvent* note_on;
  NoteEvent* note_off;
  
  
  // if a note with this value is playing at this step, stop it
  NoteEvent* playing_note;
  for (int i = step - 1; i >= 0; --i) {
    if (find_note_event(i, value, false, playing_note))
      break;
    if (find_note_event(i, value, true, playing_note)) {
      playing_note->length = step - i;
      note_off = new NoteEvent(false, step - 1, value, 0, 0, playing_note);
      add_note_event(step - 1, note_off);
      NoteEvent* tmp = playing_note->assoc;
      playing_note->assoc = note_off;
      if (tmp)
	delete_note_event(tmp->step, tmp);
      break;
    }
  }

  // if a note with this value is starting at this step, delete it 
  // (unless the length is the same as for the new note)
  if (find_note_event(step, value, true, playing_note)) {
    if (playing_note->length == noteLength)
      return;
    delete_note_event(step, playing_note);
    for (int j = step; j < m_length * m_steps; ++j) {
      if (find_note_event(j, value, false, playing_note)) {
	delete_note_event(j, playing_note);
	break;
      }
    }
  }
  
  // make sure that the new note fits
  int newLength = noteLength;
  for (int i = step + 1; i < step + noteLength; ++i) {
    if (find_note_event(i, value, true, playing_note)) {
      newLength = i - step;
      break;
    }
  }
  
  // add the note events
  note_off = new NoteEvent(false, step + newLength, value, 0, 0);
  add_note_event(step + newLength - 1, note_off);
  note_on = new NoteEvent(true, step, value, 0, newLength, note_off);
  note_off->assoc = note_on;
  add_note_event(step, note_on);
  
  signal_note_added(step, value, newLength);
}


/** This function will delete the note with the given value playing at the
    given step, if there is one. It will also update the @c previous and 
    @c next pointers so the doubly linked list will stay consistent with the
    note map. It will return the step that the deleted note started on,
    or -1 if no note was deleted. */
int Pattern::delete_note(int step, int value) {
  NoteEvent* event;
  
  // look at the current step
  if (find_note_event(step, value, true, event)) {
    delete_note_event(step, event);
    for (int i = step; i < m_length * m_steps; ++i) {
      if (find_note_event(i, value, false, event)) {
	delete_note_event(i, event);
	signal_note_removed(step, value);
	return step;
      }
    }
  }
  
  // look at earlier steps
  for (int i = step - 1; i >= 0; --i) {
    if (find_note_event(i, value, false, event))
      return -1;
    if (find_note_event(i, value, true, event)) {
      delete_note_event(i, event);
      for (int j = step; j < m_length * m_steps; ++j) {
	if (find_note_event(j, value, false, event)) {
	  delete_note_event(j, event);
	  break;
	}
      }
      signal_note_removed(i, value);
      return i;
    }
  }
    
  return -1;
}


void Pattern::add_cc(int ccNumber, int step, int value) {
  assert(ccNumber >= 0 && ccNumber < 128);
  assert(step >= 0 && step < m_length * m_cc_steps);
  assert(value >= 0 && value < 128);
  bool isUpdate = false;
  if (m_control_changes[ccNumber].changes.find(step) != 
      m_control_changes[ccNumber].changes.end())
    isUpdate = true;
  m_control_changes[ccNumber].changes[step] = new CCEvent(ccNumber, value);
  if (isUpdate)
    signal_cc_changed(ccNumber, step, value);
  else
    signal_cc_added(ccNumber, step, value);
}


int Pattern::delete_cc(int ccNumber, int step) {
  assert(ccNumber >= 0 && ccNumber < 128);
  assert(step >= 0 && step < m_length * m_cc_steps);
  CCData& data(m_control_changes[ccNumber]);
  map<int, CCEvent*>::iterator iter = data.changes.find(step);
  if (iter != data.changes.end()) {
    delete iter->second;
    data.changes.erase(iter);
    signal_cc_removed(ccNumber, step);
    return step;
  }
  return -1;
}


const vector<Pattern::NoteEvent*>& Pattern::get_notes() const {
  return m_notes;
}


Pattern::CCData& Pattern::get_cc(int ccNumber) {
  assert(ccNumber >= 0 && ccNumber < 128);
  return m_control_changes[ccNumber];
}


int Pattern::get_steps() const {
  return m_steps;
}


int Pattern::get_cc_steps() const {
  return m_cc_steps;
}


int Pattern::get_length() const {
  return m_length;
}
  

void Pattern::get_dirty_rect(int* minStep, int* minNote, 
			     int* maxStep, int* maxNote) {
  if (minStep)
    *minStep = this->m_min_step;
  if (minNote)
    *minNote = this->m_min_note;
  if (maxStep)
    *maxStep = this->m_max_step;
  if (maxNote)
    *maxNote = this->m_max_note;
  this->m_min_step = m_length * m_steps;
  this->m_max_step = -1;
  this->m_min_note = 128;
  this->m_max_note = -1;
}


bool Pattern::is_dirty() const {
  return m_dirty;
}


void Pattern::make_clean() const {
  m_dirty = false;
}


bool Pattern::fill_xml_node(Element* elt) const {
  char tmp_txt[10];
  elt->set_attribute("name", m_name);
  sprintf(tmp_txt, "%d", get_length());
  elt->set_attribute("length", tmp_txt);
  sprintf(tmp_txt, "%d", get_steps());
  elt->set_attribute("steps", tmp_txt);
  sprintf(tmp_txt, "%d", get_cc_steps());
  elt->set_attribute("ccsteps", tmp_txt);
  for (unsigned int i = 0; i < m_notes.size(); ++i) {
    NoteEvent* ne = m_notes[i];
    if (ne && ne->note_on) {
      Element* note_elt = elt->add_child("note");
      sprintf(tmp_txt, "%d", ne->step);
      note_elt->set_attribute("step", tmp_txt);
      sprintf(tmp_txt, "%d", ne->value);
      note_elt->set_attribute("value", tmp_txt);
      sprintf(tmp_txt, "%d", ne->length);
      note_elt->set_attribute("length", tmp_txt);
    }
  }
  return true;
}


bool Pattern::parse_xml_node(const Element* elt) {
  m_name = "";
  Node::NodeList nodes = elt->get_children("name");
  if (nodes.begin() != nodes.end()) {
    const Element* name_elt = dynamic_cast<const Element*>(*nodes.begin());
    if (name_elt) {
      m_name = name_elt->get_child_text()->get_content();
      signal_name_changed(m_name);
    }
  }

  nodes = elt->get_children("note");
  Node::NodeList::const_iterator iter;
  for (iter = nodes.begin(); iter != nodes.end(); ++iter) {
    const Element* note_elt = dynamic_cast<const Element*>(*iter);
    if (!note_elt)
      continue;
    int step, value, length;
    sscanf(note_elt->get_attribute("step")->get_value().c_str(), "%d", &step);
    sscanf(note_elt->get_attribute("value")->get_value().c_str(), 
	   "%d", &value);
    sscanf(note_elt->get_attribute("length")->get_value().c_str(), 
	   "%d", &length);
    add_note(step, value, length);
  }
  return true;
}


Pattern::NoteEvent* Pattern::get_events(unsigned int& beat, unsigned int& tick, 
					unsigned int before_beat, 
					unsigned int before_tick,
					unsigned int ticks_per_beat) const {
  return NULL;
}


bool Pattern::find_note_event(int step, int value, bool note_on, 
			      NoteEvent*& event) {
  event = m_notes[step];
  while (event) {
    if (event->value == value && event->note_on == note_on)
      return true;
    event = event->next;
  }
  return false;
}


void Pattern::delete_note_event(int step, NoteEvent* event) {
  if (!event->previous)
    m_notes[step] = event->next;
  else
    event->previous->next = event->next;
  queue_deletion(event);
}
  

void Pattern::add_note_event(int step, NoteEvent* event) {
  // note on events are added at the beginning of the list
  if (event->note_on) {
    event->previous = NULL;
    event->next = m_notes[step];
    m_notes[step] = event;
  }
  
  // note off events are added at the end of the list
  else {
    event->next = NULL;
    if (!m_notes[step]) {
      event->previous = NULL;
      m_notes[step] = event;
    }
    else {
      NoteEvent* previous = m_notes[step];
      while (previous->next)
	previous = previous->next;
      event->previous = previous;
      previous->next = event;
    }
  }
}