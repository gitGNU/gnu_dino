#ifndef NOTE_HPP
#define NOTE_HPP


namespace Dino {
  
  
  class NoteEvent;
  class Pattern;
  
  /** A note. */
  class Note {
  public:
    
    /** Returns the length of the note in pattern steps. */
    unsigned int get_length() const;
    /** Returns the MIDI key of the note. */
    unsigned char get_key() const;
    /** Returns the MIDI velocity of the note. */
    unsigned char get_velocity() const;
    /** Returns the pattern step that the note starts on. */
    unsigned int get_step() const;
    
    /** Returns the note on event. */
    NoteEvent* get_note_on();
    /** Returns the note off event. */
    NoteEvent* get_note_off();
    
  protected:
    
    friend class Pattern;
    
    /** This is protected so only friends can create Note objects. */
    Note(NoteEvent* on, NoteEvent* off);
    
    NoteEvent* m_note_on;
    NoteEvent* m_note_off;
    
  };
  

}

#endif