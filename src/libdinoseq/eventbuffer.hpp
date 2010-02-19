/*****************************************************************************
    libdinoseq - a library for MIDI sequencing
    Copyright (C) 2009  Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef EVENTBUFFER_HPP
#define EVENTBUFFER_HPP


namespace Dino {

  
  class SongTime;
  
  
  /** An abstract base class for MIDI event buffers.
      All non-abstract derived classes must implement write_event().
      
      @ingroup sequencing */
  class EventBuffer {
  public:
    
    /** This function is called by Sequencable::sequence() to write events
	to the buffer. */
    virtual bool write_event(SongTime const& st, 
			     size_t bytes, unsigned char const* data) = 0;
    
  };


}


#endif