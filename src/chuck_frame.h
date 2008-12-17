/*----------------------------------------------------------------------------
    ChucK Concurrent, On-the-fly Audio Programming Language
      Compiler and Virtual Machine

    Copyright (c) 2004 Ge Wang and Perry R. Cook.  All rights reserved.
      http://chuck.cs.princeton.edu/
      http://soundlab.cs.princeton.edu/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
    U.S.A.
-----------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
// file: chuck_frame.h
// desc: ...
//
// author: Andrew Appel (appel@cs.princeton.edu)
// modified: Ge Wang (gewang@cs.princeton.edu)
//           Perry R. Cook (prc@cs.princeton.edu)
// date: Autumn 2002
//-----------------------------------------------------------------------------
#ifndef __CHUCK_FRAME_H__
#define __CHUCK_FRAME_H__

#include "chuck_def.h"

#include <string>
#include <vector>


//-----------------------------------------------------------------------------
// name: struct Chuck_Local
// desc: ...
//-----------------------------------------------------------------------------
struct Chuck_Local
{
    // name
    std::string name;
    // the size of the local
    t_CKUINT size;
    // is ref
    t_CKBOOL is_ref;
    // the offset
    t_CKUINT offset;

    // constructor
    Chuck_Local()
    { size = 0; is_ref = FALSE; offset = 0; }
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Frame
// desc: ...
//-----------------------------------------------------------------------------
struct Chuck_Frame
{
    // yes
    std::string name;
    // the offset
    t_CKUINT curr_offset;
    // not sure
    t_CKUINT num_access;
    // offset stack
    std::vector<Chuck_Local *> stack;

public:
    Chuck_Frame();
    ~Chuck_Frame() { }

public:
    // push scope
    void push_scope();
    // add local
    Chuck_Local * alloc_local( t_CKUINT size, const std::string & name, t_CKBOOL is_ref );
    // pop scope
    void pop_scope( std::vector<Chuck_Local *> & out );
};




#endif
