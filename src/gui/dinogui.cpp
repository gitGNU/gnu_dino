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
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <sstream>
#include <utility>

#include <gtkmm/messagedialog.h>

#include "dinogui.hpp"
#include "plugindialog.hpp"
#include "plugininterfaceimplementation.hpp"
#include "song.hpp"


using namespace Dino;
using namespace Gdk;
using namespace Glib;
using namespace Gtk;
using namespace sigc;
using namespace std;


// needed for the about dialog
// XXX change this to GPLv3
#define GPL_TEXT \
"		    GNU GENERAL PUBLIC LICENSE\n" \
"		       Version 2, June 1991\n" \
"\n" \
" Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n" \
" 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA\n" \
" Everyone is permitted to copy and distribute verbatim copies\n" \
" of this license document, but changing it is not allowed.\n" \
"\n" \
"			    Preamble\n" \
"\n" \
"  The licenses for most software are designed to take away your\n" \
"freedom to share and change it.  By contrast, the GNU General Public\n" \
"License is intended to guarantee your freedom to share and change free\n" \
"software--to make sure the software is free for all its users.  This\n" \
"General Public License applies to most of the Free Software\n" \
"Foundation's software and to any other program whose authors commit to\n" \
"using it.  (Some other Free Software Foundation software is covered by\n" \
"the GNU Lesser General Public License instead.)  You can apply it to\n" \
"your programs, too.\n" \
"\n" \
"  When we speak of free software, we are referring to freedom, not\n" \
"price.  Our General Public Licenses are designed to make sure that you\n" \
"have the freedom to distribute copies of free software (and charge for\n" \
"this service if you wish), that you receive source code or can get it\n" \
"if you want it, that you can change the software or use pieces of it\n" \
"in new free programs; and that you know you can do these things.\n" \
"\n" \
"  To protect your rights, we need to make restrictions that forbid\n" \
"anyone to deny you these rights or to ask you to surrender the rights.\n" \
"These restrictions translate to certain responsibilities for you if you\n" \
"distribute copies of the software, or if you modify it.\n" \
"\n" \
"  For example, if you distribute copies of such a program, whether\n" \
"gratis or for a fee, you must give the recipients all the rights that\n" \
"you have.  You must make sure that they, too, receive or can get the\n" \
"source code.  And you must show them these terms so they know their\n" \
"rights.\n" \
"\n" \
"  We protect your rights with two steps: (1) copyright the software, and\n" \
"(2) offer you this license which gives you legal permission to copy,\n" \
"distribute and/or modify the software.\n" \
"\n" \
"  Also, for each author's protection and ours, we want to make certain\n" \
"that everyone understands that there is no warranty for this free\n" \
"software.  If the software is modified by someone else and passed on, we\n" \
"want its recipients to know that what they have is not the original, so\n" \
"that any problems introduced by others will not reflect on the original\n" \
"authors' reputations.\n" \
"\n" \
"  Finally, any free program is threatened constantly by software\n" \
"patents.  We wish to avoid the danger that redistributors of a free\n" \
"program will individually obtain patent licenses, in effect making the\n" \
"program proprietary.  To prevent this, we have made it clear that any\n" \
"patent must be licensed for everyone's free use or not licensed at all.\n" \
"\n" \
"  The precise terms and conditions for copying, distribution and\n" \
"modification follow.\n" \
"\n" \
"		    GNU GENERAL PUBLIC LICENSE\n" \
"   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n" \
"\n" \
"  0. This License applies to any program or other work which contains\n" \
"a notice placed by the copyright holder saying it may be distributed\n" \
"under the terms of this General Public License.  The \"Program\", below,\n" \
"refers to any such program or work, and a \"work based on the Program\"\n" \
"means either the Program or any derivative work under copyright law:\n" \
"that is to say, a work containing the Program or a portion of it,\n" \
"either verbatim or with modifications and/or translated into another\n" \
"language.  (Hereinafter, translation is included without limitation in\n" \
"the term \"modification\".)  Each licensee is addressed as \"you\".\n" \
"\n" \
"Activities other than copying, distribution and modification are not\n" \
"covered by this License; they are outside its scope.  The act of\n" \
"running the Program is not restricted, and the output from the Program\n" \
"is covered only if its contents constitute a work based on the\n" \
"Program (independent of having been made by running the Program).\n" \
"Whether that is true depends on what the Program does.\n" \
"\n" \
"  1. You may copy and distribute verbatim copies of the Program's\n" \
"source code as you receive it, in any medium, provided that you\n" \
"conspicuously and appropriately publish on each copy an appropriate\n" \
"copyright notice and disclaimer of warranty; keep intact all the\n" \
"notices that refer to this License and to the absence of any warranty;\n" \
"and give any other recipients of the Program a copy of this License\n" \
"along with the Program.\n" \
"\n" \
"You may charge a fee for the physical act of transferring a copy, and\n" \
"you may at your option offer warranty protection in exchange for a fee.\n" \
"\n" \
"  2. You may modify your copy or copies of the Program or any portion\n" \
"of it, thus forming a work based on the Program, and copy and\n" \
"distribute such modifications or work under the terms of Section 1\n" \
"above, provided that you also meet all of these conditions:\n" \
"\n" \
"    a) You must cause the modified files to carry prominent notices\n" \
"    stating that you changed the files and the date of any change.\n" \
"\n" \
"    b) You must cause any work that you distribute or publish, that in\n" \
"    whole or in part contains or is derived from the Program or any\n" \
"    part thereof, to be licensed as a whole at no charge to all third\n" \
"    parties under the terms of this License.\n" \
"\n" \
"    c) If the modified program normally reads commands interactively\n" \
"    when run, you must cause it, when started running for such\n" \
"    interactive use in the most ordinary way, to print or display an\n" \
"    announcement including an appropriate copyright notice and a\n" \
"    notice that there is no warranty (or else, saying that you provide\n" \
"    a warranty) and that users may redistribute the program under\n" \
"    these conditions, and telling the user how to view a copy of this\n" \
"    License.  (Exception: if the Program itself is interactive but\n" \
"    does not normally print such an announcement, your work based on\n" \
"    the Program is not required to print an announcement.)\n" \
"\n" \
"These requirements apply to the modified work as a whole.  If\n" \
"identifiable sections of that work are not derived from the Program,\n" \
"and can be reasonably considered independent and separate works in\n" \
"themselves, then this License, and its terms, do not apply to those\n" \
"sections when you distribute them as separate works.  But when you\n" \
"distribute the same sections as part of a whole which is a work based\n" \
"on the Program, the distribution of the whole must be on the terms of\n" \
"this License, whose permissions for other licensees extend to the\n" \
"entire whole, and thus to each and every part regardless of who wrote it.\n" \
"\n" \
"Thus, it is not the intent of this section to claim rights or contest\n" \
"your rights to work written entirely by you; rather, the intent is to\n" \
"exercise the right to control the distribution of derivative or\n" \
"collective works based on the Program.\n" \
"\n" \
"In addition, mere aggregation of another work not based on the Program\n" \
"with the Program (or with a work based on the Program) on a volume of\n" \
"a storage or distribution medium does not bring the other work under\n" \
"the scope of this License.\n" \
"\n" \
"  3. You may copy and distribute the Program (or a work based on it,\n" \
"under Section 2) in object code or executable form under the terms of\n" \
"Sections 1 and 2 above provided that you also do one of the following:\n" \
"\n" \
"    a) Accompany it with the complete corresponding machine-readable\n" \
"    source code, which must be distributed under the terms of Sections\n" \
"    1 and 2 above on a medium customarily used for software interchange; or,\n" \
"\n" \
"    b) Accompany it with a written offer, valid for at least three\n" \
"    years, to give any third party, for a charge no more than your\n" \
"    cost of physically performing source distribution, a complete\n" \
"    machine-readable copy of the corresponding source code, to be\n" \
"    distributed under the terms of Sections 1 and 2 above on a medium\n" \
"    customarily used for software interchange; or,\n" \
"\n" \
"    c) Accompany it with the information you received as to the offer\n" \
"    to distribute corresponding source code.  (This alternative is\n" \
"    allowed only for noncommercial distribution and only if you\n" \
"    received the program in object code or executable form with such\n" \
"    an offer, in accord with Subsection b above.)\n" \
"\n" \
"The source code for a work means the preferred form of the work for\n" \
"making modifications to it.  For an executable work, complete source\n" \
"code means all the source code for all modules it contains, plus any\n" \
"associated interface definition files, plus the scripts used to\n" \
"control compilation and installation of the executable.  However, as a\n" \
"special exception, the source code distributed need not include\n" \
"anything that is normally distributed (in either source or binary\n" \
"form) with the major components (compiler, kernel, and so on) of the\n" \
"operating system on which the executable runs, unless that component\n" \
"itself accompanies the executable.\n" \
"\n" \
"If distribution of executable or object code is made by offering\n" \
"access to copy from a designated place, then offering equivalent\n" \
"access to copy the source code from the same place counts as\n" \
"distribution of the source code, even though third parties are not\n" \
"compelled to copy the source along with the object code.\n" \
"\n" \
"  4. You may not copy, modify, sublicense, or distribute the Program\n" \
"except as expressly provided under this License.  Any attempt\n" \
"otherwise to copy, modify, sublicense or distribute the Program is\n" \
"void, and will automatically terminate your rights under this License.\n" \
"However, parties who have received copies, or rights, from you under\n" \
"this License will not have their licenses terminated so long as such\n" \
"parties remain in full compliance.\n" \
"\n" \
"  5. You are not required to accept this License, since you have not\n" \
"signed it.  However, nothing else grants you permission to modify or\n" \
"distribute the Program or its derivative works.  These actions are\n" \
"prohibited by law if you do not accept this License.  Therefore, by\n" \
"modifying or distributing the Program (or any work based on the\n" \
"Program), you indicate your acceptance of this License to do so, and\n" \
"all its terms and conditions for copying, distributing or modifying\n" \
"the Program or works based on it.\n" \
"\n" \
"  6. Each time you redistribute the Program (or any work based on the\n" \
"Program), the recipient automatically receives a license from the\n" \
"original licensor to copy, distribute or modify the Program subject to\n" \
"these terms and conditions.  You may not impose any further\n" \
"restrictions on the recipients' exercise of the rights granted herein.\n" \
"You are not responsible for enforcing compliance by third parties to\n" \
"this License.\n" \
"\n" \
"  7. If, as a consequence of a court judgment or allegation of patent\n" \
"infringement or for any other reason (not limited to patent issues),\n" \
"conditions are imposed on you (whether by court order, agreement or\n" \
"otherwise) that contradict the conditions of this License, they do not\n" \
"excuse you from the conditions of this License.  If you cannot\n" \
"distribute so as to satisfy simultaneously your obligations under this\n" \
"License and any other pertinent obligations, then as a consequence you\n" \
"may not distribute the Program at all.  For example, if a patent\n" \
"license would not permit royalty-free redistribution of the Program by\n" \
"all those who receive copies directly or indirectly through you, then\n" \
"the only way you could satisfy both it and this License would be to\n" \
"refrain entirely from distribution of the Program.\n" \
"\n" \
"If any portion of this section is held invalid or unenforceable under\n" \
"any particular circumstance, the balance of the section is intended to\n" \
"apply and the section as a whole is intended to apply in other\n" \
"circumstances.\n" \
"\n" \
"It is not the purpose of this section to induce you to infringe any\n" \
"patents or other property right claims or to contest validity of any\n" \
"such claims; this section has the sole purpose of protecting the\n" \
"integrity of the free software distribution system, which is\n" \
"implemented by public license practices.  Many people have made\n" \
"generous contributions to the wide range of software distributed\n" \
"through that system in reliance on consistent application of that\n" \
"system; it is up to the author/donor to decide if he or she is willing\n" \
"to distribute software through any other system and a licensee cannot\n" \
"impose that choice.\n" \
"\n" \
"This section is intended to make thoroughly clear what is believed to\n" \
"be a consequence of the rest of this License.\n" \
"\n" \
"  8. If the distribution and/or use of the Program is restricted in\n" \
"certain countries either by patents or by copyrighted interfaces, the\n" \
"original copyright holder who places the Program under this License\n" \
"may add an explicit geographical distribution limitation excluding\n" \
"those countries, so that distribution is permitted only in or among\n" \
"countries not thus excluded.  In such case, this License incorporates\n" \
"the limitation as if written in the body of this License.\n" \
"\n" \
"  9. The Free Software Foundation may publish revised and/or new versions\n" \
"of the General Public License from time to time.  Such new versions will\n" \
"be similar in spirit to the present version, but may differ in detail to\n" \
"address new problems or concerns.\n" \
"\n" \
"Each version is given a distinguishing version number.  If the Program\n" \
"specifies a version number of this License which applies to it and \"any\n" \
"later version\", you have the option of following the terms and conditions\n" \
"either of that version or of any later version published by the Free\n" \
"Software Foundation.  If the Program does not specify a version number of\n" \
"this License, you may choose any version ever published by the Free Software\n" \
"Foundation.\n" \
"\n" \
"  10. If you wish to incorporate parts of the Program into other free\n" \
"programs whose distribution conditions are different, write to the author\n" \
"to ask for permission.  For software which is copyrighted by the Free\n" \
"Software Foundation, write to the Free Software Foundation; we sometimes\n" \
"make exceptions for this.  Our decision will be guided by the two goals\n" \
"of preserving the free status of all derivatives of our free software and\n" \
"of promoting the sharing and reuse of software generally.\n" \
"\n" \
"			    NO WARRANTY\n" \
"\n" \
"  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n" \
"FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n" \
"OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n" \
"PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n" \
"OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n" \
"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n" \
"TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n" \
"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n" \
"REPAIR OR CORRECTION.\n" \
"\n" \
"  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n" \
"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n" \
"REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n" \
"INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n" \
"OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n" \
"TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n" \
"YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n" \
"PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n" \
"POSSIBILITY OF SUCH DAMAGES.\n" \
"\n" \
"		     END OF TERMS AND CONDITIONS\n" \
"\n" \
"	    How to Apply These Terms to Your New Programs\n" \
"\n" \
"  If you develop a new program, and you want it to be of the greatest\n" \
"possible use to the public, the best way to achieve this is to make it\n" \
"free software which everyone can redistribute and change under these terms.\n" \
"\n" \
"  To do so, attach the following notices to the program.  It is safest\n" \
"to attach them to the start of each source file to most effectively\n" \
"convey the exclusion of warranty; and each file should have at least\n" \
"the \"copyright\" line and a pointer to where the full notice is found.\n" \
"\n" \
"    <one line to give the program's name and a brief idea of what it does.>\n" \
"    Copyright (C) <year>  <name of author>\n" \
"\n" \
"    This program is free software; you can redistribute it and/or modify\n" \
"    it under the terms of the GNU General Public License as published by\n" \
"    the Free Software Foundation; either version 3 of the License, or\n" \
"    (at your option) any later version.\n" \
"\n" \
"    This program is distributed in the hope that it will be useful,\n" \
"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
"    GNU General Public License for more details.\n" \
"\n" \
"    You should have received a copy of the GNU General Public License\n" \
"    along with this program; if not, write to the Free Software\n" \
"    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA\n" \
"\n" \
"\n" \
"Also add information on how to contact you by electronic and paper mail.\n" \
"\n" \
"If the program is interactive, make it output a short notice like this\n" \
"when it starts in an interactive mode:\n" \
"\n" \
"    Gnomovision version 69, Copyright (C) year name of author\n" \
"    Gnomovision comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n" \
"    This is free software, and you are welcome to redistribute it\n" \
"    under certain conditions; type `show c' for details.\n" \
"\n" \
"The hypothetical commands `show w' and `show c' should show the appropriate\n" \
"parts of the General Public License.  Of course, the commands you use may\n" \
"be called something other than `show w' and `show c'; they could even be\n" \
"mouse-clicks or menu items--whatever suits your program.\n" \
"\n" \
"You should also get your employer (if you work as a programmer) or your\n" \
"school, if any, to sign a \"copyright disclaimer\" for the program, if\n" \
"necessary.  Here is a sample; alter the names:\n" \
"\n" \
"  Yoyodyne, Inc., hereby disclaims all copyright interest in the program\n" \
"  `Gnomovision' (which makes passes at compilers) written by James Hacker.\n" \
"\n" \
"  <signature of Ty Coon>, 1 April 1989\n" \
"  Ty Coon, President of Vice\n" \
"\n" \
"This General Public License does not permit incorporating your program into\n" \
"proprietary programs.  If your program is a subroutine library, you may\n" \
"consider it more useful to permit linking proprietary applications with the\n" \
"library.  If this is what you want to do, use the GNU Lesser General\n" \
"Public License instead of this License.\n"


DinoGUI::DinoGUI(int argc, char** argv) 
  : m_seq("Dino", m_song),
    m_proxy(m_song),
    m_dbus("org.nongnu.dino"),
    m_dbus_obj(0),
    m_plif(*this, m_song, m_seq, m_proxy, m_dbus.get_name()),
    m_plib(m_plif),
    m_valid(false) {
  
  if (!m_seq.is_valid()) {
    MessageDialog dlg("Could not initialise the sequencer!", 
                      false, MESSAGE_ERROR);
    dlg.run();
    return;
  }
  
  if (!init_lash(argc, argv, m_seq.get_jack_name())) {
    MessageDialog dlg("Could not initialise LASH!", 
                      false, MESSAGE_ERROR);
    dlg.run();
    return;
  }
  
  m_valid = true;
  
  m_dbus_obj = new DinoDBusObject(m_proxy, m_seq);
  m_dbus.register_object("/", m_dbus_obj);
  signal_timeout().
    connect(bind(mem_fun(m_dbus, &DBus::Connection::run), 0), 50);
  
  // initialise the main window
  m_window.set_title("Dino");
  Gtk::Window::set_default_icon_from_file(DATA_DIR "/head.png");
  VBox* vbox = manage(new VBox);
  m_window.add(*vbox);
  MenuBar* mbar = manage(new MenuBar);
  init_menus(*mbar);
  vbox->pack_start(*mbar, PACK_SHRINK);
  vbox->pack_start(m_nb);
  m_statusbar.set_has_resize_grip(false);
  vbox->pack_start(m_statusbar, PACK_SHRINK);
  m_nb.set_border_width(3);
  
  // initialise the "About" dialog
  m_about_dialog.set_icon_from_file(DATA_DIR "/head.png");
  m_about_dialog.set_name("Dino");
  m_about_dialog.set_version(VERSION);
  m_about_dialog.set_copyright("\u00A9 " CR_YEAR " Lars Luthman "
                               "<lars.luthman@gmail.com>");
  m_about_dialog.set_comments("A pattern based MIDI sequencer for GNU/Linux");
  m_about_dialog.set_license(GPL_TEXT);
  m_about_dialog.set_logo(Pixbuf::create_from_file(DATA_DIR "/midisaurus.png"));
  
  // initialise the "Plugins" dialog
  m_plug_dialog.set_icon_from_file(DATA_DIR "/head.png");
  m_plug_dialog.set_library(m_plib);
  
  m_nb.signal_switch_page().
    connect(sigc::hide<0>(mem_fun(*this, &DinoGUI::page_switched)));
  
  load_plugins(argc, argv);
  
  reset_gui();
  set_status("Welcome to Dino!");
}


Gtk::Window& DinoGUI::get_window() {
  return m_window;
}


int DinoGUI::add_page(const std::string& label, GUIPage& page) {
  int result = m_nb.append_page(page, label);
  m_nb.set_tab_reorderable(page, true);
  m_window.show_all();
  return result;
}


void DinoGUI::remove_page(GUIPage& page) {
  m_nb.remove_page(page);
}


void DinoGUI::remove_page(int pagenum) {
  m_nb.remove_page(pagenum);
}


void DinoGUI::slot_file_open() {
  m_seq.stop();
  m_seq.go_to_beat(SongTime(0, 0));
  m_song.load_file("output.dino");
  reset_gui();
  m_seq.reset_ports();
}


void DinoGUI::slot_file_save() {
  m_song.write_file("output.dino");
}


void DinoGUI::slot_file_save_as() {
  m_song.write_file("output.dino");
}


void DinoGUI::slot_file_clear_all() {
  m_seq.stop();
  m_seq.go_to_beat(SongTime(0, 0));
  m_song.clear();
  m_song.set_length(SongTime(32, 0));
  reset_gui();
}


void DinoGUI::slot_file_quit() {
  m_seq.stop();
  Main::quit();
}


void DinoGUI::slot_edit_undo() {
  assert(m_proxy.can_undo());
  m_proxy.undo();
}


void DinoGUI::slot_edit_cut() {
  GUIPage* page = 
    dynamic_cast<GUIPage*>(m_nb.get_nth_page(m_nb.get_current_page()));
  assert(page);
  page->cut_selection();
}


void DinoGUI::slot_edit_copy() {
  GUIPage* page = 
    dynamic_cast<GUIPage*>(m_nb.get_nth_page(m_nb.get_current_page()));
  assert(page);
  page->copy_selection();
}


void DinoGUI::slot_edit_paste() {
  GUIPage* page = 
    dynamic_cast<GUIPage*>(m_nb.get_nth_page(m_nb.get_current_page()));
  assert(page);
  page->paste();
}


void DinoGUI::slot_edit_delete() {
  GUIPage* page = 
    dynamic_cast<GUIPage*>(m_nb.get_nth_page(m_nb.get_current_page()));
  assert(page);
  page->delete_selection();
}


void DinoGUI::slot_edit_select_all() {
  GUIPage* page = 
    dynamic_cast<GUIPage*>(m_nb.get_nth_page(m_nb.get_current_page()));
  assert(page);
  page->select_all();
}


void DinoGUI::slot_transport_play() {
  m_seq.play();
}


void DinoGUI::slot_transport_stop() {
  m_seq.stop();
}


void DinoGUI::slot_transport_go_to_start() {
  m_seq.go_to_beat(SongTime(0, 0));
}


void DinoGUI::slot_help_about_dino() {
  m_about_dialog.run();
  m_about_dialog.hide();
}


void DinoGUI::reset_gui() {
  std::list<Widget*> pages = m_nb.get_children();
  std::list<Widget*>::iterator iter;
  for (iter = pages.begin(); iter != pages.end(); ++iter) {
    GUIPage* page = dynamic_cast<GUIPage*>(*iter);
    assert(page);
    page->reset_gui();
  }
}


void DinoGUI::init_menus(MenuBar& mbar) {

  MenuItem* file_item = manage(new MenuItem("_File", true));
  Menu* file_menu = manage(new Menu);
  file_item->set_submenu(*file_menu);
  mbar.append(*file_item);
  create_menu_item(*file_menu, "Clear all", "file_clear_all",
                   Stock::CLEAR, &DinoGUI::slot_file_clear_all);
  file_menu->append(*manage(new SeparatorMenuItem));
  create_menu_item(*file_menu, Stock::QUIT, "file_quit", 
                   &DinoGUI::slot_file_quit);
  
  MenuItem* edit_item = manage(new MenuItem("_Edit", true));
  Menu* edit_menu = manage(new Menu);
  edit_item->set_submenu(*edit_menu);
  mbar.append(*edit_item);
  create_menu_item(*edit_menu, Stock::UNDO, "edit_undo",
		   &DinoGUI::slot_edit_undo);
  m_proxy.signal_stack_changed().
    connect(compose(mem_fun(*this, &DinoGUI::update_undo),
		    mem_fun(m_proxy, &CommandProxy::get_next_undo_name)));
  update_undo(m_proxy.get_next_undo_name());
  edit_menu->append(*manage(new SeparatorMenuItem));
  create_menu_item(*edit_menu, Stock::CUT, "edit_cut", 
                   &DinoGUI::slot_edit_cut);
  create_menu_item(*edit_menu, Stock::COPY, "edit_copy", 
                   &DinoGUI::slot_edit_copy);
  create_menu_item(*edit_menu, Stock::PASTE, "edit_paste", 
                   &DinoGUI::slot_edit_paste);
  create_menu_item(*edit_menu, Stock::DELETE, "edit_delete", 
                   &DinoGUI::slot_edit_delete);
  edit_menu->append(*manage(new SeparatorMenuItem));
  create_menu_item(*edit_menu, "Select all", "edit_select_all", 
                   &DinoGUI::slot_edit_select_all);
  
  MenuItem* transport_item = manage(new MenuItem("_Transport", true));
  Menu* transport_menu = manage(new Menu);
  transport_item->set_submenu(*transport_menu);
  mbar.append(*transport_item);
  create_menu_item(*transport_menu, Stock::MEDIA_PLAY, "transport_play", 
                   &DinoGUI::slot_transport_play);
  create_menu_item(*transport_menu, "_Stop", "transport_stop", 
                   Stock::MEDIA_PAUSE, &DinoGUI::slot_transport_stop);
  create_menu_item(*transport_menu, "_Go to start", "transport_go_to_start", 
                   Stock::MEDIA_PREVIOUS, &DinoGUI::slot_transport_go_to_start);
  
  MenuItem* plugins_item = manage(new MenuItem("_Plugins", true));
  Menu* plugins_menu = manage(new Menu);
  plugins_item->set_submenu(*plugins_menu);
  mbar.append(*plugins_item);
  create_menu_item(*plugins_menu, "_Manage plugins", "plugins_manage_plugins", 
                   Stock::DISCONNECT, &DinoGUI::slot_plugins_manage);

  MenuItem* help_item = manage(new MenuItem("_Help", true));
  Menu* help_menu = manage(new Menu);
  help_item->set_submenu(*help_menu);
  mbar.append(*help_item);
  create_menu_item(*help_menu, "_About Dino", "help_about", 
                   Stock::ABOUT, &DinoGUI::slot_help_about_dino);
}


MenuItem* DinoGUI::create_menu_item(Gtk::Menu& menu, const std::string& label,
                                    const std::string& name, 
                                    const Gtk::StockID& id,
                                    void (DinoGUI::*mslot)(void)) {
  using namespace Menu_Helpers;
  Gtk::Image* img = manage(new Gtk::Image(id, ICON_SIZE_MENU));
  ImageMenuElem elem(label, *img, mem_fun(*this, mslot));
  m_menuitems[name] = elem.get_child().operator->();
  menu.items().push_back(elem);
  return elem.get_child().operator->();
}


MenuItem* DinoGUI::create_menu_item(Gtk::Menu& menu, const std::string& label,
                                    const std::string& name,
                                    void (DinoGUI::*mslot)(void)) {
  using namespace Menu_Helpers;
  MenuElem elem(label, mem_fun(*this, mslot));
  m_menuitems[name] = elem.get_child().operator->();
  menu.items().push_back(elem);
  return elem.get_child().operator->();
}


MenuItem* DinoGUI::create_menu_item(Gtk::Menu& menu, const StockID& id,
                                    const std::string& name,
                                    void (DinoGUI::*mslot)(void)) {
  using namespace Menu_Helpers;
  StockMenuElem elem(id, mem_fun(*this, mslot));
  m_menuitems[name] = elem.get_child().operator->();
  menu.items().push_back(elem);
  return elem.get_child().operator->();
}


bool DinoGUI::init_lash(int argc, char** argv, const std::string& jack_name) {
  dbg(1, "Initialising LASH client");
  m_lash_client = lash_init(lash_extract_args(&argc, &argv), "dino", 
                            LASH_Config_File, LASH_PROTOCOL(2, 0));
  
  if (m_lash_client) {
    lash_event_t* event = lash_event_new_with_type(LASH_Client_Name);
    lash_event_set_string(event, "Dino");
    lash_send_event(m_lash_client, event);      
    lash_jack_client_name(m_lash_client, jack_name.c_str());
    signal_timeout().
      connect(mem_fun(*this, &DinoGUI::slot_check_ladcca_events), 500);
  }
  else
    dbg(0, "Could not initialise LASH!");
  return (m_lash_client != 0);
}


bool DinoGUI::slot_check_ladcca_events() {
  lash_event_t* event;
  while ((event = lash_get_event(m_lash_client))) {
    
    // save
    if (lash_event_get_type(event) == LASH_Save_File) {
      set_status("Received LASH Save command");
      if (m_song.write_file(string(lash_event_get_string(event)) + "/song")) {
        lash_send_event(m_lash_client, 
                        lash_event_new_with_type(LASH_Save_File));
      }
    }
    
    // restore
    else if (lash_event_get_type(event) == LASH_Restore_File) {
      set_status("Received LASH Restore command");
      if (m_song.load_file(string(lash_event_get_string(event)) + "/song")) {
        reset_gui();
        lash_send_event(m_lash_client,
                        lash_event_new_with_type(LASH_Restore_File));
      }
    }
    
    // quit
    else if (lash_event_get_type(event) == LASH_Quit) {
      Main::instance()->quit();
    }
    
    lash_event_destroy(event);
  }
  return true;
}


void DinoGUI::page_switched(guint index) {
  GUIPage* page = dynamic_cast<GUIPage*>(m_nb.get_nth_page(index));
  if (!page)
    return;
  bool clipboard_active = page->get_flags() & GUIPage::PageSupportsClipboard;
  m_menuitems["edit_cut"]->set_sensitive(clipboard_active);
  m_menuitems["edit_copy"]->set_sensitive(clipboard_active);
  m_menuitems["edit_paste"]->set_sensitive(clipboard_active);
  m_menuitems["edit_delete"]->set_sensitive(clipboard_active);
  m_menuitems["edit_select_all"]->set_sensitive(clipboard_active);
}


void DinoGUI::slot_plugins_manage() {
  m_plug_dialog.run();
  m_plug_dialog.hide();
}


void DinoGUI::load_plugins(int argc, char** argv) {
  PluginLibrary::iterator iter;
  iter = m_plib.find("Core actions");
  if (iter != m_plib.end())
    m_plib.load_plugin(iter);
  iter = m_plib.find("Arrangement editor");
  if (iter != m_plib.end())
    m_plib.load_plugin(iter);
  iter = m_plib.find("Pattern editor");
  if (iter != m_plib.end())
    m_plib.load_plugin(iter);
  iter = m_plib.find("Info editor");
  if (iter != m_plib.end())
    m_plib.load_plugin(iter);
}


bool DinoGUI::is_valid() const {
  return m_valid;
}


unsigned DinoGUI::set_status(const std::string& str, int timeout) {
  unsigned message_id = m_statusbar.push(str);
  if (timeout) {
    signal_timeout().
      connect(bind_return(bind(bind(mem_fun(m_statusbar, 
                                            &Statusbar::remove_message), 
                               0), message_id), false), timeout);
  }
  return message_id;
}


void DinoGUI::update_undo(const std::string& next_undo_name) {
  cerr<<"next_undo_name = "<<next_undo_name<<endl;
  m_menuitems["edit_undo"]->set_sensitive(next_undo_name.size() > 0);
}

