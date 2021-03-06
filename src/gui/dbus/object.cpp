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

#include <cstring>
#include <iostream>

#include "argument.hpp"
#include "object.hpp"


using namespace std;


namespace DBus {
  

  Object::Object() {

  }
  
  
  Object::~Object() {
    
  }
  
    
  void Object::add_method(const std::string& interface, 
			  const std::string& method,
			  const std::string& typesig,
			  Method handler) {
    m_ifs[interface][method][typesig] = handler;
  }
  
  
  void Object::add_signal(const std::string& interface, 
			  const std::string& signal,
			  const std::string& typesig) {
    
  }
  
  
  const Object::InterfaceMap& Object::get_interfaces() const {
    return m_ifs;
  }


  DBusHandlerResult Object::message_function(DBusConnection* conn, 
					     DBusMessage* msg) {
    const char* iface = dbus_message_get_interface(msg);
    const char* member = dbus_message_get_member(msg);
    const char* typesig = dbus_message_get_signature(msg);
    
    if (!member || !typesig) {
      DBusMessage* reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED,
						  "No method name given");
      dbus_connection_send(conn, reply, 0);
      return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    map<string, map<string, Method> >::iterator miter;
    
    // if there is an interface, look for the method in there
    if (iface) {
      InterfaceMap::iterator ifiter = m_ifs.find(iface);
      if (ifiter == m_ifs.end()) {
	DBusMessage* reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED,
						    "No such interface");
	dbus_connection_send(conn, reply, 0);
	return DBUS_HANDLER_RESULT_HANDLED;
      }
      miter = ifiter->second.find(member);
      if (miter == ifiter->second.end()) {
	DBusMessage* reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED,
						    "No such method");
	dbus_connection_send(conn, reply, 0); 
	return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
    
    // if not, look in all interfaces
    else {
      InterfaceMap::iterator ifiter;
      for (ifiter = m_ifs.begin(); ifiter != m_ifs.end(); ++ifiter) {
	miter = ifiter->second.find(member);
	if (miter != ifiter->second.end())
	  break;
      }
      if (ifiter == m_ifs.end()) {
	DBusMessage* reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED,
						    "No such method");
	dbus_connection_send(conn, reply, 0); 
	return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
    
    // find the correct overloaded method
    map<string, Method>::iterator tsiter = miter->second.find(typesig);
    if (tsiter == miter->second.end()) {
      DBusMessage* reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED,
						  "Incorrect type signature");
      dbus_connection_send(conn, reply, 0);
      return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    // through all the checks - get the arguments
    int argc = strlen(typesig);
    Argument* argv = new Argument[argc];
    DBusMessageIter aiter;
    dbus_message_iter_init(msg, &aiter);
    for (int i = 0; i < argc; ++i, dbus_message_iter_next(&aiter)) {
      dbus_uint64_t value;
      dbus_message_iter_get_basic(&aiter, &value);
      int type = dbus_message_iter_get_arg_type(&aiter);
      if (type == DBUS_TYPE_INT32)
	argv[i] = Argument(*reinterpret_cast<int*>(&value));
      else if (type == DBUS_TYPE_DOUBLE)
	argv[i] = Argument(*reinterpret_cast<double*>(&value));
      else if (type == DBUS_TYPE_STRING)
	argv[i] = Argument(*reinterpret_cast<const char**>(&value));
    }
    
    // call the method!
    if (tsiter->second(argc, argv)) {
      DBusMessage* reply = dbus_message_new_method_return(msg);
      dbus_connection_send(conn, reply, 0);
    }
    else {
      DBusMessage* reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED,
						  "Method handler failed");
      dbus_connection_send(conn, reply, 0);
    }
    delete [] argv;
    
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  
}
