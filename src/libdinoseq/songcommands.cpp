/****************************************************************************
   Dino - A simple pattern based MIDI sequencer
   
   Copyright (C) 2006-2007  Lars Luthman <lars.luthman@gmail.com>
   
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

#include <limits>

#include "controller_numbers.hpp"
#include "deleter.hpp"
#include "interpolatedevent.hpp"
#include "keyinfo.hpp"
#include "note.hpp"
#include "noteselection.hpp"
#include "pattern.hpp"
#include "song.hpp"
#include "songcommands.hpp"
#include "track.hpp"


// The classes in this namespace are only used internally
namespace {
  
  
  class _SetSongLength : public Dino::Command {
  public:
    
    _SetSongLength(Dino::Song& song, int length)
      : Dino::Command("Change song length [INTERNAL]"),
	m_song(song),
	m_length(length),
	m_oldlength() {

    }
    
    bool do_command() {
      m_oldlength = m_song.get_length().get_beat();
      m_song.set_length(Dino::SongTime(m_length, 0));
      return true;
    }
    
    bool undo_command() {
      m_song.set_length(Dino::SongTime(m_oldlength, 0));
      return true;
    }
    
  protected:
    
    Dino::Song& m_song;
    int m_length;
    int m_oldlength;
    
  };


  class _RemovePattern : public Dino::Command {
  public:

    _RemovePattern(Dino::Song& song, int track, int pattern)
      : Dino::Command("Remove pattern"),
	m_song(song),
	m_track(track),
	m_pattern(pattern),
	m_patptr(0) {
      
    }
    
    
    ~_RemovePattern() {
      if (m_patptr)
	Dino::Deleter::queue(m_patptr);
    }
    
    
    bool do_command() {
      Dino::Song::TrackIterator titer = m_song.tracks_find(m_track);
      if (titer == m_song.tracks_end())
	return false;
      Dino::Track::PatternIterator piter = titer->pat_find(m_pattern);
      if (piter == titer->pat_end())
	return false;
      m_patptr = titer->disown_pattern(piter->get_id());
      return (m_patptr != 0);
    }
    
    
    bool undo_command() {
      if (!m_patptr)
	return false;
      Dino::Song::TrackIterator titer = m_song.tracks_find(m_track);
      if (titer == m_song.tracks_end())
	return false;
      Dino::Track::PatternIterator piter = titer->add_pattern(m_patptr);
      if (piter == titer->pat_end())
	return false;
      m_patptr = 0;
      return true;
    }

  protected:

    Dino::Song& m_song;
    int m_track;
    int m_pattern;
    Dino::Pattern* m_patptr;

  };


}


namespace Dino {
  
  
  SetSongLength::SetSongLength(Song& song, int length)
    : CompoundCommand("Change song length"),
      m_song(song),
      m_length(length) {
    
  }
  
  
  bool SetSongLength::do_command() {
    
    clear();
    
    if (m_length < m_song.get_length().get_beat()) {
      
      // remove or change the loop
      if (m_song.get_loop_start().get_beat() > m_length)
	append(new SetLoopStart(m_song, -1));
      if (m_song.get_loop_end().get_beat() > m_length)
	append(new SetLoopEnd(m_song, m_length));
      
      // remove all tempochanges after the new length
      Song::TempoIterator tmpiter = m_song.tempo_find(m_length);
      for ( ; tmpiter != m_song.tempo_end(); ++tmpiter) {
	if (tmpiter->get_beat() >= m_length)
	  append(new RemoveTempoChange(m_song, tmpiter->get_beat()));
      }
      
      // remove or resize all sequence entries that extend beyond the
      // new length
      Song::TrackIterator titer = m_song.tracks_begin();
      for ( ; titer != m_song.tracks_end(); ++titer) {
	Track::SequenceIterator siter = titer->seq_begin();
	for ( ; siter != titer->seq_end(); ++siter) {
	  if (siter->get_start().get_beat() >= m_length)
	    append(new RemoveSequenceEntry(m_song, titer->get_id(), 
					   siter->get_start()));
	  else if ((siter->get_start() + siter->get_length()).get_beat() > 
		   m_length)
	    append(new SetSequenceEntryLength(m_song, titer->get_id(),
					      siter->get_start(),
					      SongTime(m_length, 0) - 
					      siter->get_start()));
	}
      }

    }
    
    append(new _SetSongLength(m_song, m_length));
    
    CompoundCommand::do_command();
  }
  

  SetLoopStart::SetLoopStart(Song& song, int beat) 
    : Command("Set loop start"),
      m_song(song),
      m_beat(beat),
      m_oldbeat(-1) {

  }
  
  
  bool SetLoopStart::do_command() {
    m_oldbeat = m_song.get_loop_start().get_beat();
    if (m_beat == m_oldbeat || m_beat > m_song.get_length().get_beat())
      return false;
    m_song.set_loop_start(m_beat);
    return true;
  }
  
  
  bool SetLoopStart::undo_command() {
    if (m_oldbeat > m_song.get_length().get_beat())
      return false;
    m_song.set_loop_start(m_oldbeat);
    return true;
  }

  
  SetLoopEnd::SetLoopEnd(Song& song, int beat) 
    : Command("Set loop start"),
      m_song(song),
      m_beat(beat),
      m_oldbeat(-1) {

  }
  
  
  bool SetLoopEnd::do_command() {
    m_oldbeat = m_song.get_loop_end().get_beat();
    if (m_beat == m_oldbeat || m_beat > m_song.get_length().get_beat())
      return false;
    m_song.set_loop_end(m_beat);
    return true;
  }
  
  
  bool SetLoopEnd::undo_command() {
    if (m_oldbeat > m_song.get_length().get_beat())
      return false;
    m_song.set_loop_end(m_oldbeat);
    return true;
  }

  
  AddTrack::AddTrack(Song& song, const std::string& name, 
		     Song::TrackIterator* iter)
    : Command("Add track"),
      m_song(song),
      m_name(name),
      m_id(-1),
      m_iter_store(iter) {

  }
  
  
  bool AddTrack::do_command() {
    Song::TrackIterator titer = m_song.add_track(m_name);
    if (m_iter_store) {
      *m_iter_store = titer;
      m_iter_store = 0;
    }
    if (titer == m_song.tracks_end())
      return false;
    m_id = titer->get_id();
    return true;
  }
  
  
  bool AddTrack::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_id);
    if (titer == m_song.tracks_end())
      return false;
    return m_song.remove_track(titer);
  }
  

  RemoveTrack::RemoveTrack(Song& song, int id)
    : Command("Remove track"),
      m_song(song),
      m_id(id),
      m_trk(0) {

  }
  
  
  RemoveTrack::~RemoveTrack() {
    if (m_trk)
      Deleter::queue(m_trk);
  }
  
  
  bool RemoveTrack::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_id);
    if (titer == m_song.tracks_end())
      return false;
    m_trk = m_song.disown_track(titer);
    if (!m_trk)
      return false;
    return true;
  }


  bool RemoveTrack::undo_command() {
    Song::TrackIterator titer = m_song.add_track(m_trk);
    m_trk = 0;
    return (titer != m_song.tracks_end());
  }
  
  
  AddTempoChange::AddTempoChange(Song& song, int beat, double bpm,
				 Song::TempoIterator* iter)
    : Command("Set tempo change"),
      m_song(song),
      m_beat(beat),
      m_bpm(bpm),
      m_oldbpm(-1),
      m_iter_store(iter) {

  }
  
  
  bool AddTempoChange::do_command() {
    if (m_beat < 0 || m_beat >= m_song.get_length().get_beat()) {
      if (m_iter_store) {
	*m_iter_store = m_song.tempo_end();
	m_iter_store = 0;
      }
      return false;
    }
    m_oldbpm = -1;
    Song::TempoIterator iter = m_song.tempo_find(m_beat);
    if (iter->get_beat() == m_beat) {
      if (iter->get_bpm() == m_bpm) {
	if (m_iter_store) {
	  *m_iter_store = m_song.tempo_end();
	  m_iter_store = 0;
	}
	return false;
      }
      m_oldbpm = iter->get_bpm();
    }
    iter = m_song.add_tempo_change(m_beat, m_bpm);
    if (m_iter_store) {
      *m_iter_store = iter;
      m_iter_store = 0;
    }
    return (iter != m_song.tempo_end());
  }
  
  
  bool AddTempoChange::undo_command() {
    if (m_oldbpm == -1) {
      Song::TempoIterator iter = m_song.tempo_find(m_beat);
      if (iter == m_song.tempo_end())
	return false;
      m_song.remove_tempo_change(iter);
    }
    else
      m_song.add_tempo_change(m_beat, m_oldbpm);
    return true;
  }
  

  RemoveTempoChange::RemoveTempoChange(Song& song, unsigned long beat)
    : Command("Remove tempo change"),
      m_song(song),
      m_beat(beat),
      m_bpm(-1) {

  }
  
  
  bool RemoveTempoChange::do_command() {
    Song::TempoIterator iter = m_song.tempo_find(m_beat);
    if (iter == m_song.tempo_end() || iter->get_beat() != m_beat)
      return false;
    m_bpm = iter->get_bpm();
    m_song.remove_tempo_change(iter);
    return true;
  }
  
  
  bool RemoveTempoChange::undo_command() {
    m_song.add_tempo_change(m_beat, m_bpm);
    return true;
  }


  SetTrackName::SetTrackName(Song& song, int track, const std::string& name)
    : Command("Set track name"),
      m_song(song),
      m_track(track),
      m_name(name) {
    
  }
  
  
  bool SetTrackName::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    m_oldname = titer->get_name();
    if (m_oldname == m_name)
      return false;
    titer->set_name(m_name);
    return true;
  }

  
  bool SetTrackName::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    titer->set_name(m_oldname);
    return true;
  }
  

  AddPattern::AddPattern(Song& song, int track, const std::string& name, 
			 int length, int steps, Track::PatternIterator* iter)
    : Command("Add pattern"),
      m_song(song),
      m_track(track),
      m_name(name),
      m_length(length),
      m_steps(steps),
      m_iter_store(iter),
      m_id(-1) {
    
  }
  
  
  bool AddPattern::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end()) {
      if (m_iter_store) {
	*m_iter_store = Track::PatternIterator();
	m_iter_store = 0;
      }
      return false;
    }
    Track::PatternIterator iter = titer->add_pattern(m_name, m_length, m_steps);
    if (m_iter_store) {
      *m_iter_store = iter;
      m_iter_store = 0;
    }
    if (iter != titer->pat_end()) {
      m_id = iter->get_id();
      return true;
    }
    return false;
  }
  
  
  bool AddPattern::undo_command() {
    if (m_id == -1)
      return false;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_id);
    if (piter == titer->pat_end())
      return false;
    titer->remove_pattern(m_id);
    return true;
  }


  DuplicatePattern::DuplicatePattern(Song& song, int track, int pattern,
				     Track::PatternIterator* iter)
    : Command("Duplicate pattern"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_iter_store(iter),
      m_id(-1) {
    
  }
  
  
  bool DuplicatePattern::do_command() {
    // XXX this can be written with less code
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end()) {
      if (m_iter_store) {
	*m_iter_store = Track::PatternIterator();
	m_iter_store = 0;
      }
      return false;
    }
    Track::PatternIterator src = titer->pat_find(m_pattern);
    if (src == titer->pat_end()) {
      if (m_iter_store) {
	*m_iter_store = Track::PatternIterator();
	m_iter_store = 0;
      }
      return false;
    }
    Track::PatternIterator iter = titer->duplicate_pattern(src);
    if (m_iter_store) {
      *m_iter_store = iter;
      m_iter_store = 0;
    }
    if (iter != titer->pat_end()) {
      m_id = iter->get_id();
      return true;
    }
    return false;
  }
  
  
  bool DuplicatePattern::undo_command() {
    if (m_id == -1)
      return false;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_id);
    if (piter == titer->pat_end())
      return false;
    titer->remove_pattern(m_id);
    return true;
  }


  RemovePattern::RemovePattern(Song& song, int track, int pattern)
    : CompoundCommand("Remove pattern"),
      m_song(song),
      m_track(track),
      m_pattern(pattern) {

  }
  
  
  bool RemovePattern::do_command() {
    
    clear();
    
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    
    // first remove all sequence entries for this pattern (in an undoable way)
    Track::SequenceIterator siter;
    for (siter = titer->seq_begin(); siter != titer->seq_end(); ++siter) {
      if (siter->get_pattern_id() == m_pattern)
	append(new RemoveSequenceEntry(m_song, m_track, siter->get_start()));
    }
    
    append(new _RemovePattern(m_song, m_track, m_pattern));
    
    return CompoundCommand::do_command();
  }
  

  AddSequenceEntry::AddSequenceEntry(Song& song, int track, 
				     const SongTime& beat, int pattern, 
				     const SongTime& length)
    : Command("Add sequence entry"),
      m_song(song),
      m_track(track),
      m_beat(beat),
      m_pattern(pattern),
      m_length(length) {

  }
  
  
  bool AddSequenceEntry::do_command() {
    if (m_beat.get_beat() < 0 || m_beat >= m_song.get_length())
      return false;
    if (m_beat + m_length > m_song.get_length())
      return false;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    if (m_length.get_beat() > piter->get_length())
      return false;
    // XXX can probably be optimised
    Track::SequenceIterator siter;
    for (siter = titer->seq_begin(); siter != titer->seq_end(); ++siter) {
      if (siter->get_start() < m_beat + m_length &&
	  siter->get_start() + siter->get_length() > m_beat) 
	return false;
    }
    return (titer->set_sequence_entry(m_beat, m_pattern, m_length) !=
	    titer->seq_end());
  }
  
  
  bool AddSequenceEntry::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::SequenceIterator siter = titer->seq_find(m_beat);
    if (siter == titer->seq_end())
      return false;
    titer->remove_sequence_entry(siter);
    return true;
  }

  
  RemoveSequenceEntry::RemoveSequenceEntry(Song& song, int track, 
					   const SongTime& beat)
    : Command("Remove sequence entry"),
      m_song(song),
      m_track(track),
      m_beat(beat),
      m_pattern(-1) {

  }
  
  
  bool RemoveSequenceEntry::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::SequenceIterator siter = titer->seq_find(m_beat);
    if (siter == titer->seq_end())
      return false;
    m_beat = siter->get_start();
    m_pattern = siter->get_pattern_id();
    m_length = siter->get_length();
    titer->remove_sequence_entry(siter);
    return true;
  }
  
  
  bool RemoveSequenceEntry::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    titer->set_sequence_entry(m_beat, m_pattern, m_length);
    return true;
  }


  SetSequenceEntryLength::SetSequenceEntryLength(Song& song, int track, 
						 const SongTime& beat, 
						 const SongTime& length)
    : Command("Change sequence entry length"),
      m_song(song),
      m_track(track),
      m_beat(beat),
      m_length(length) {

  }
  
  
  bool SetSequenceEntryLength::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::SequenceIterator siter = titer->seq_find(m_beat);
    if (siter == titer->seq_end())
      return false;
    m_beat = siter->get_start();
    m_old_length = siter->get_length();
    if (m_old_length == m_length)
      return false;
    titer->set_seq_entry_length(siter, m_length);
    return true;
  }

  
  bool SetSequenceEntryLength::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::SequenceIterator siter = titer->seq_find(m_beat);
    if (siter == titer->seq_end())
      return false;
    titer->set_seq_entry_length(siter, m_old_length);
    return true;
  }


  SetTrackMidiChannel::SetTrackMidiChannel(Song& song, int track, int channel)
    : Command("Change track MIDI channel"),
      m_song(song),
      m_track(track),
      m_channel(channel),
      m_old_channel(-1) {

  }

  
  bool SetTrackMidiChannel::do_command() {
    if (m_channel < 0 || m_channel >= 16)
      return false;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    m_old_channel = titer->get_channel();
    titer->set_channel(m_channel);
    return true;
  }
  
  
  bool SetTrackMidiChannel::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    titer->set_channel(m_old_channel);
    return true;
  }


  AddController::AddController(Song& song, int track, long number, 
			       const std::string& name, int default_v, 
			       int min, int max, bool global)
    : Command("Add controller"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_name(name),
      m_default(default_v),
      m_min(min),
      m_max(max),
      m_global(global) {

  }
  
  
  bool AddController::do_command() {
    if (!is_cc(m_number) && !is_pbend(m_number))
      return false;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    return titer->add_controller(m_number, m_name, m_default, 
				 m_min, m_max, m_global);
  }
  

  bool AddController::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    return titer->remove_controller(m_number);
  }


  RemoveController::RemoveController(Song& song, int track, long number) 
    : Command("Remove controller"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_info(0) {

  }
  
  
  RemoveController::~RemoveController() {
    if (m_info)
      Deleter::queue(m_info);
    std::map<int, Curve*>::iterator iter;
    for (iter = m_curves.begin(); iter != m_curves.end(); ++iter)
      Deleter::queue(iter->second);
  }
  
  
  bool RemoveController::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    m_info = titer->disown_controller(m_number, m_curves);
    if (!m_info)
      return false;
  }
  

  bool RemoveController::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    bool result = titer->add_controller(m_info, m_curves);
    m_curves.clear();
    return result;
  }


  SetControllerName::SetControllerName(Song& song, int track, long number, 
				       const std::string& name)
    : Command("Set controller name"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_name(name) {

  }

  
  bool SetControllerName::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	m_old_name = ctrls[i]->get_name();
	titer->set_controller_name(m_number, m_name);
	return true;
      }
    }
    return false;
  }
  
  
  bool SetControllerName::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	titer->set_controller_name(m_number, m_old_name);
	return true;
      }
    }
    return false;
  }


  SetControllerMin::SetControllerMin(Song& song, int track, 
				     long number, int min) 
    : Command("Set controller minimum"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_min(min) {

  }
  
  
  bool SetControllerMin::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	m_oldmin = ctrls[i]->get_min();
	titer->set_controller_min(m_number, m_min);
	return true;
      }
    }
    return false;
  }
  
  
  bool SetControllerMin::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	titer->set_controller_min(m_number, m_oldmin);
	return true;
      }
    }
    return false;
  }
  

  SetControllerMax::SetControllerMax(Song& song, int track, 
				     long number, int max) 
    : Command("Set controller maximum"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_max(max) {

  }
  
  
  bool SetControllerMax::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	m_oldmax = ctrls[i]->get_max();
	titer->set_controller_max(m_number, m_max);
	return true;
      }
    }
    return false;
  }
  
  
  bool SetControllerMax::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	titer->set_controller_max(m_number, m_oldmax);
	return true;
      }
    }
    return false;
  }
  

  SetControllerDefault::SetControllerDefault(Song& song, int track, 
				     long number, int _default) 
    : Command("Set controller default"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_default(_default) {

  }
  
  
  bool SetControllerDefault::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	m_olddefault = ctrls[i]->get_default();
	titer->set_controller_default(m_number, m_default);
	return true;
      }
    }
    return false;
  }
  
  
  bool SetControllerDefault::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	titer->set_controller_default(m_number, m_olddefault);
	return true;
      }
    }
    return false;
  }
  

  SetControllerNumber::SetControllerNumber(Song& song, int track, 
					   long number, long newnumber) 
    : Command("Set controller global"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_newnumber(newnumber) {

  }
  
  
  bool SetControllerNumber::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_newnumber)
	return false;
    }
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	titer->set_controller_number(m_number, m_newnumber);
	return true;
      }
    }
    return false;
  }
  
  
  bool SetControllerNumber::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number)
	return false;
    }
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_newnumber) {
	titer->set_controller_number(m_newnumber, m_number);
	return true;
      }
    }
    return false;
  }
  

  SetControllerGlobal::SetControllerGlobal(Song& song, int track, 
					   long number, bool global) 
    : Command("Set controller global"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_global(global) {

  }
  
  
  bool SetControllerGlobal::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	m_oldglobal = ctrls[i]->get_global();
	titer->set_controller_global(m_number, m_global);
	return true;
      }
    }
    return false;
  }
  
  
  bool SetControllerGlobal::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    const std::vector<ControllerInfo*>& ctrls = titer->get_controllers();
    unsigned i;
    for (i = 0; i < ctrls.size(); ++i) {
      if (ctrls[i]->get_number() == m_number) {
	titer->set_controller_global(m_number, m_oldglobal);
	return true;
      }
    }
    return false;
  }


  AddKey::AddKey(Song& song, int track, unsigned char number, 
		 const string& name) 
    : Command("Add named key"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_name(name) {

  }
  
  
  bool AddKey::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    return titer->add_key(m_number, m_name);
  }
  
  
  bool AddKey::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    return titer->remove_key(m_number);
  }


  RemoveKey::RemoveKey(Song& song, int track, unsigned char number)
    : Command("Remove named key"),
      m_song(song),
      m_track(track),
      m_number(number) {

  }
  
  
  bool RemoveKey::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    size_t index = titer->find_key(m_number);
    if (index < 128)
      m_name = titer->get_keys()[index]->get_name();
    return titer->remove_key(m_number);
  }
  
  
  bool RemoveKey::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    return titer->add_key(m_number, m_name);
  }


  SetKeyName::SetKeyName(Song& song, int track, unsigned char number,
			 const std::string& name)
    : Command("Change key name"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_name(name) {

  }
  
  
  bool SetKeyName::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    size_t index = titer->find_key(m_number);
    if (index < 128) {
      m_oldname = titer->get_keys()[index]->get_name();
      titer->get_keys()[index]->set_name(m_name);
      return true;
    }
    return false;
  }
  
  
  bool SetKeyName::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    size_t index = titer->find_key(m_number);
    titer->get_keys()[index]->set_name(m_oldname);
    return true;
  }


  SetKeyNumber::SetKeyNumber(Song& song, int track, unsigned char old_number,
			     unsigned char new_number)
    : Command("Change key name"),
      m_song(song),
      m_track(track),
      m_oldnumber(old_number),
      m_newnumber(new_number) {

  }
  
  
  bool SetKeyNumber::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    return titer->set_key_number(m_oldnumber, m_newnumber);
  }
  
  
  bool SetKeyNumber::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    return titer->set_key_number(m_newnumber, m_oldnumber);
  }


  SetTrackMode::SetTrackMode(Song& song, int track, Track::Mode mode)
    : Command("Change track mode"),
      m_song(song),
      m_track(track),
      m_mode(mode) {

  }
  
  
  bool SetTrackMode::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    m_oldmode = titer->get_mode();
    titer->set_mode(m_mode);
    return true;
  }
  
  
  bool SetTrackMode::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    titer->set_mode(m_oldmode);
    return true;
  }


  SetPatternName::SetPatternName(Song& song, int track, 
				 int pattern, const std::string& name)
    : Command("Set pattern name"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_name(name) {

  }
  
  
  bool SetPatternName::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    m_oldname = piter->get_name();
    piter->set_name(m_name);
    return true;
  }
  
  
  bool SetPatternName::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    piter->set_name(m_oldname);
    return true;
  }


  SetPatternLength::SetPatternLength(Song& song, int track, int pattern, 
				     unsigned int beats)
    : CompoundCommand("Set pattern length"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_beats(beats) {

  }
  
  
  bool SetPatternLength::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    m_oldbeats = piter->get_length();
    
    if (m_beats < m_oldbeats) {
      Pattern::NoteIterator niter;
      unsigned n = m_beats * piter->get_steps();
      for (niter = piter->notes_begin(); niter != piter->notes_end(); ++niter) {
	if (niter->get_step() >= n) {
	  append(new DeleteNote(m_song, m_track, m_pattern, 
				niter->get_step(), niter->get_key()));
	}
	else if (niter->get_step() + niter->get_length() > n) {
	  append(new SetNoteSize(m_song, m_track, m_pattern, niter->get_step(),
				 niter->get_key(), n - niter->get_step()));
	}
      }
    }
    
    if (!CompoundCommand::do_command())
      return false;
    
    piter->set_length(m_beats);
    return true;
  }

  
  bool SetPatternLength::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    piter->set_length(m_oldbeats);
    return CompoundCommand::undo_command();
  }
  

  SetPatternSteps::SetPatternSteps(Song& song, int track, int pattern, 
				   unsigned int steps)
    : Command("Set pattern steps"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_steps(steps) {

  }
  
  
  bool SetPatternSteps::do_command() {
    if (m_steps == 0)
      return false;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    m_oldsteps = piter->get_steps();
    NoteSelection sel(&*piter);
    m_step = numeric_limits<unsigned int>::max();
    m_key = 0;
    Pattern::NoteIterator niter;
    for (niter = piter->notes_begin(); niter != piter->notes_end(); ++niter) {
      if (niter->get_step() < m_step)
	m_step = niter->get_step();
      if (niter->get_key() > m_key)
	m_key = niter->get_key();
      sel.add_note(niter);
    }
    m_notes = NoteCollection(sel);
    piter->set_steps(m_steps);
    return true;
  }
  
  
  bool SetPatternSteps::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    // XXX could maybe use a Pattern::clear_notes() here
    while (piter->notes_begin() != piter->notes_end())
      piter->delete_note(piter->notes_begin());
    piter->set_steps(m_oldsteps);
    if (m_notes.begin() != m_notes.end())
      piter->add_notes(m_notes, m_step, m_key);
    return true;
  }


  AddNote::AddNote(Song& song, int track, int pattern, 
		   unsigned int step, int key, int velocity, int length)
    : Command("Add note"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_step(step),
      m_key(key),
      m_velocity(velocity),
      m_length(length) {

  }
  
  
  bool AddNote::do_command() {
    if (m_key >= 128 || m_key < 0)
      return false;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    unsigned int n = piter->get_length() * piter->get_steps();
    if (m_step >= n || m_step + m_length > n)
      return false;
    Pattern::NoteIterator niter = piter->add_note(m_step, m_key, 
						  m_velocity, m_length);
    return (niter != piter->notes_end());
  }
  
  
  bool AddNote::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::NoteIterator niter = piter->find_note(m_step, m_key);
    if (niter == piter->notes_end())
      return false;
    piter->delete_note(niter);
    return true;
  }
  

  AddNotes::AddNotes(Song& song, int track, int pattern, 
		     const NoteCollection& notes, int step, 
		     int key, NoteSelection* selection)
    : Command("Add notes"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_notes(notes),
      m_step(step),
      m_key(key),
      m_selection(selection) {

  }
  
  
  bool AddNotes::do_command() {
    if (m_key >= 128 || m_key < 0)
      return false;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    unsigned int n = piter->get_length() * piter->get_steps();
    if (m_step >= n)
      return false;
    return piter->add_notes(m_notes, m_step, m_key, m_selection);
  }
  
  
  bool AddNotes::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    NoteCollection::ConstIterator iter;
    for (iter = m_notes.begin(); iter != m_notes.end(); ++iter) {
      Pattern::NoteIterator niter = piter->find_note(m_step + iter->start, 
						     iter->key + m_key);
      if (niter != piter->notes_end())
	piter->delete_note(niter);
    }
    return true;
  }


  SetNoteVelocity::SetNoteVelocity(Song& song, int track, int pattern, 
				   int step, int key, int velocity)
    : Command("Set note velocity"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_step(step),
      m_key(key),
      m_velocity(velocity) {

  }
  
  
  bool SetNoteVelocity::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::NoteIterator niter = piter->find_note(m_step, m_key);
    if (niter == piter->notes_end())
      return false;
    m_oldvelocity = niter->get_velocity();
    piter->set_velocity(niter, m_velocity);
    return true;
  }
  
  
  bool SetNoteVelocity::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::NoteIterator niter = piter->find_note(m_step, m_key);
    if (niter == piter->notes_end())
      return false;
    piter->set_velocity(niter, m_oldvelocity);
    return true;
  }


  SetNoteSize::SetNoteSize(Song& song, int track, int pattern, 
				   int step, int key, int size)
    : Command("Resize note"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_step(step),
      m_key(key),
      m_size(size) {

  }
  
  
  bool SetNoteSize::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::NoteIterator niter = piter->find_note(m_step, m_key);
    if (niter == piter->notes_end())
      return false;
    m_oldsize = niter->get_length();
    piter->resize_note(niter, m_size);
    return true;
  }
  
  
  bool SetNoteSize::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::NoteIterator niter = piter->find_note(m_step, m_key);
    if (niter == piter->notes_end())
      return false;
    piter->resize_note(niter, m_oldsize);
    return true;
  }


  DeleteNote::DeleteNote(Song& song, int track, int pattern, int step, int key)
    : Command("Delete note"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_step(step),
      m_key(key) {

  }
  
  
  bool DeleteNote::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::NoteIterator niter = piter->find_note(m_step, m_key);
    if (niter == piter->notes_end())
      return false;
    m_step = niter->get_step();
    m_velocity = niter->get_velocity();
    m_length = niter->get_length();
    piter->delete_note(niter);
    return true;
  }
  
  
  bool DeleteNote::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    piter->add_note(m_step, m_key, m_velocity, m_length);
    return true;
  }

  
  AddPatternCurvePoint::AddPatternCurvePoint(Song& song, int track, 
					     int pattern, long number,
					     unsigned int step, int value)
    : Command("Add curve point"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_number(number),
      m_step(step),
      m_value(value),
      m_wasold(false) {

  }
  
  
  bool AddPatternCurvePoint::do_command() {
    cerr<<__PRETTY_FUNCTION__<<endl;
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    cerr<<__PRETTY_FUNCTION__<<" 1"<<endl;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    cerr<<__PRETTY_FUNCTION__<<" 2"<<endl;
    Pattern::CurveIterator citer = piter->curves_find(m_number);
    if (citer == piter->curves_end())
      return false;
    cerr<<__PRETTY_FUNCTION__<<" 3"<<endl;
    if (m_step > citer->get_size())
      return false;
    cerr<<__PRETTY_FUNCTION__<<" 4"<<endl;
    if (m_value < citer->get_info().get_min())
      return false;
    cerr<<__PRETTY_FUNCTION__<<" 5"<<endl;
    if (m_value > citer->get_info().get_max())
      return false;
    cerr<<__PRETTY_FUNCTION__<<" 6"<<endl;
    const InterpolatedEvent* e = citer->get_event(m_step);
    if (e) {
      if (m_step == citer->get_size()) {
	m_oldvalue = e->get_end();
	m_wasold = true;
      }
      else if (m_step == e->get_step()) {
	m_oldvalue = e->get_start();
	m_wasold = true;
      }
    }
    piter->add_curve_point(citer, m_step, m_value);
    return true;
  }
  
  
  bool AddPatternCurvePoint::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::CurveIterator citer = piter->curves_find(m_number);
    if (citer == piter->curves_end())
      return false;
    if (m_wasold)
      piter->add_curve_point(citer, m_step, m_oldvalue);
    else
      piter->remove_curve_point(citer, m_step);
  }


  AddTrackCurvePoint::AddTrackCurvePoint(Song& song, int track, long number,
					 unsigned int step, int value)
    : Command("Add curve point"),
      m_song(song),
      m_track(track),
      m_number(number),
      m_step(step),
      m_value(value),
      m_wasold(false) {

  }
  
  
  bool AddTrackCurvePoint::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::CurveIterator citer = titer->curves_find(m_number);
    if (citer == titer->curves_end())
      return false;
    if (m_step > m_song.get_length().get_beat())
      return false;
    if (m_value < citer->get_info().get_min())
      return false;
    if (m_value > citer->get_info().get_max())
      return false;
    const InterpolatedEvent* e = citer->get_event(m_step);
    if (e) {
      if (m_step == citer->get_size()) {
	m_oldvalue = e->get_end();
	m_wasold = true;
      }
      else if (m_step == e->get_step()) {
	m_oldvalue = e->get_start();
	m_wasold = true;
      }
    }
    citer->add_point(m_step, m_value);
    return true;
  }
  
  
  bool AddTrackCurvePoint::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::CurveIterator citer = titer->curves_find(m_number);
    if (citer == titer->curves_end())
      return false;
    if (m_wasold)
      citer->add_point(m_step, m_oldvalue);
    else
      citer->remove_point(m_step);
  }


  RemovePatternCurvePoint::RemovePatternCurvePoint(Song& song, int track, 
						   int pattern, long number,
						   unsigned int step)
    : Command("Remove curve point"),
      m_song(song),
      m_track(track),
      m_pattern(pattern),
      m_number(number),
      m_step(step) {

  }
  
  
  bool RemovePatternCurvePoint::do_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::CurveIterator citer = piter->curves_find(m_number);
    if (citer == piter->curves_end())
      return false;
    if (m_step >= citer->get_size())
      return false;
    const InterpolatedEvent* e = citer->get_event(m_step);
    if (!e || m_step != e->get_step())
      return false;
    
    m_oldvalue = e->get_start();
    piter->remove_curve_point(citer, m_step);
    return true;
  }
  
  
  bool RemovePatternCurvePoint::undo_command() {
    Song::TrackIterator titer = m_song.tracks_find(m_track);
    if (titer == m_song.tracks_end())
      return false;
    Track::PatternIterator piter = titer->pat_find(m_pattern);
    if (piter == titer->pat_end())
      return false;
    Pattern::CurveIterator citer = piter->curves_find(m_number);
    if (citer == piter->curves_end())
      return false;
    piter->add_curve_point(citer, m_step, m_oldvalue);
  }


}
